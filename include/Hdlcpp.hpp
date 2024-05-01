// The MIT License (MIT)

// Copyright (c) 2020 Bang & Olufsen a/s

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <array>
#include <atomic>
#include <cerrno>
#include <functional>
#include <mutex>
#include <span>
#include <thread>

namespace Hdlcpp {

template <typename T>
class Buffer {
    using Container = std::span<T>;

public:
    Buffer(Container buffer)
        : m_buffer(buffer)
    {
    }

    [[nodiscard]] bool empty() const
    {
        return std::span<typename Container::value_type>(m_head, m_tail).empty();
    }

    constexpr size_t capacity()
    {
        return m_buffer.size();
    }

    typename Container::iterator begin()
    {
        return m_head;
    }

    typename Container::iterator end()
    {
        return m_tail;
    }

    Container dataSpan()
    {
        return { m_head, m_tail };
    }

    Container unusedSpan()
    {
        return { m_tail, m_buffer.end() };
    }

    void appendToTail(size_t tail)
    {
        m_tail += tail;
    }

    constexpr typename Container::iterator erase(typename Container::iterator first, typename Container::iterator last)
    {
        if (last < m_tail) {
            Container toBeMoved(last, m_tail);
            for (const auto& byte : toBeMoved) {
                *first++ = byte;
            }
        }
        m_tail = first;
        return m_tail;
    }

    void clear()
    {
        m_head = m_buffer.begin();
        m_tail = m_head;
    }

private:
    Container m_buffer {};
    typename Container::iterator m_head { m_buffer.begin() };
    typename Container::iterator m_tail { m_buffer.begin() };
};

using Container = std::span<uint8_t>;
using ConstContainer = const std::span<const Container::element_type>;
using value_type = Container::value_type;

template <size_t Capacity>
using StaticBuffer = std::array<Container::value_type, Capacity>;

template <size_t Capacity>
struct Calculate {
    // For details see: https://en.wikipedia.org/wiki/High-Level_Data_Link_Control#Structure
    static constexpr size_t WithOverhead { Capacity * 2 + 8 };
};

using TransportRead = std::function<int(Container buffer)>;
using TransportWrite = std::function<int(ConstContainer buffer)>;

using TransportAddress = uint8_t;
static constexpr TransportAddress AddressBroadcast { 0xff };

struct ReadResponse {
    const int size;
    const TransportAddress address;
};

//! @param Capacity The buffer size to be allocated for encoding/decoding frames
class Hdlcpp {
public:
    //! @brief Constructs the Hdlcpp instance
    //! @param read A std::function for reading from the transport layer (e.g. UART)
    //! @param write A std::function for writing to the transport layer (e.g. UART)
    //! @param writeTimeout The write timeout in milliseconds to wait for an ack/nack
    //! @param writeRetries The number of write retries in case of timeout
    Hdlcpp(TransportRead read, TransportWrite write, Container readBuffer, Container writeBuffer, uint16_t writeTimeout = 100, uint8_t writeRetries = 1)
        : transportRead(std::move(read))
        , transportWrite(std::move(write))
        , readBuffer(readBuffer)
        , writeBuffer(writeBuffer)
        , readFrame(FrameNack)
        , writeTimeout(writeTimeout)
        , writeRetries(writeRetries)
    {
    }

    //! @brief Destructs the Hdlcpp instance
    virtual ~Hdlcpp() = default;

    //! @brief Reads decoded data from the transport layer (blocks if TransportRead is blocking)
    //! @param data A pointer to an allocated buffer (should be bigger than max frame length)
    //! @param length The length of the allocated buffer
    //! @return The number of bytes received if positive or an error code from <cerrno>
    virtual ReadResponse read(Container buffer)
    {
        int result;
        TransportAddress address { AddressBroadcast };
        uint16_t discardBytes;

        if (!buffer.data() || buffer.empty() || (buffer.size() > readBuffer.capacity()))
            return { -EINVAL, address };

        do {
            bool doTransportRead { true };
            if (!readBuffer.empty()) {
                // Try to decode the readBuffer before potentially blocking in the transportRead
                result = decode(address, readFrame, readSequenceNumber, readBuffer.dataSpan(), buffer, discardBytes);
                if (result >= 0) {
                    doTransportRead = false;
                } else if (readBuffer.unusedSpan().size() == 0) {
                    // Drop the buffer in an attempt to recover from getting
                    // filled with an invalid message.
                    // FIXME: really start/stop codes should be tracked to
                    //        implement this in a more fail-safe way
                    readBuffer.clear();
                }
            }

            if (doTransportRead) {
                if ((result = transportRead(readBuffer.unusedSpan())) <= 0)
                    return { result, address };

                readBuffer.appendToTail(result);
                result = decode(address, readFrame, readSequenceNumber, readBuffer.dataSpan(), buffer, discardBytes);
            }

            if (discardBytes > 0) {
                readBuffer.erase(readBuffer.begin(), readBuffer.begin() + discardBytes);
            }

            if (result >= 0) {
                switch (readFrame) {
                case FrameData:
                    if (++readSequenceNumber > 7)
                        readSequenceNumber = 0;
                    writeFrame(address, FrameAck, readSequenceNumber, {});
                    return { result, address };
                case FrameAck:
                case FrameNack:
                    writeResult = readFrame;
                    break;
                }
            } else if ((result == -EIO) && (readFrame == FrameData)) {
                writeFrame(address, FrameNack, readSequenceNumber, {});
            }
        } while (!stopped);

        return { result, address };
    }

    //! @brief Writes data to be encoded and sent to the transport layer (thread safe)
    //! @param address Address of the receiver
    //! @param buffer Buffer storing the data to be sent
    //! @return The number of bytes sent if positive or an error code from <cerrno>
    virtual int write(TransportAddress address, ConstContainer buffer)
    {
        int result;

        if (!buffer.data() || buffer.empty())
            return -EINVAL;

        std::lock_guard<std::mutex> writeLock(writeMutex);

        // Sequence number is a 3-bit value
        if (++writeSequenceNumber > 7)
            writeSequenceNumber = 0;

        for (uint8_t tries = 0; tries <= writeRetries; tries++) {
            writeResult = -1;
            if ((result = writeFrame(address, FrameData, writeSequenceNumber, buffer)) <= 0)
                break;

            if (writeTimeout == 0)
                break;

            for (uint16_t i = 0; i < writeTimeout; i++) {
                result = writeResult;
                if (result >= 0) {
                    if (result == FrameNack)
                        break;

                    return buffer.size();
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            result = -ETIME;
        }

        return result;
    }

    //! @brief Closes the reading
    virtual void close()
    {
        stopped = true;
    }

protected:
    enum Frame {
        FrameData,
        FrameAck,
        FrameNack,
    };

    enum Control {
        ControlSFrameBit,
        ControlSendSeqNumberBit,
        ControlSFrameTypeBit,
        ControlSFrameTypeBit2,
        ControlPollBit,
        ControlReceiveSeqNumberBit,
    };

    enum ControlType {
        ControlTypeReceiveReady,
        ControlTypeReceiveNotReady,
        ControlTypeReject,
        ControlTypeSelectiveReject,
    };

    template <typename T>
    struct span {
        constexpr span(std::span<T> span)
            : m_span(span)
            , itr(span.begin())
        {
        }

        constexpr bool push_back(const T& value)
        {
            if (itr < m_span.end()) {
                *itr++ = value;
                return true;
            }
            return false;
        }

        constexpr size_t size()
        {
            return std::distance(m_span.begin(), itr);
        }

        std::span<T> m_span;
        typename std::span<T>::iterator itr;
    };

    int encode(TransportAddress address, Frame& frame, uint8_t& sequenceNumber, ConstContainer source, Hdlcpp::span<uint8_t> destination)
    {
        uint8_t value = 0;
        uint16_t i, fcs16Value = Fcs16InitValue;

        if (!destination.push_back(FlagSequence))
            return -EINVAL;

        fcs16Value = fcs16(fcs16Value, address);
        if (escape(address, destination) < 0)
            return -EINVAL;

        value = encodeControlByte(frame, sequenceNumber);
        fcs16Value = fcs16(fcs16Value, value);
        if (escape(value, destination) < 0)
            return -EINVAL;

        if (frame == FrameData) {
            if (!source.data() || source.empty())
                return -EINVAL;

            for (const auto& byte : source) {
                fcs16Value = fcs16(fcs16Value, byte);
                if (escape(byte, destination) < 0)
                    return -EINVAL;
            }
        }

        // Invert the FCS value accordingly to the specification
        fcs16Value ^= 0xFFFF;

        for (i = 0; i < sizeof(fcs16Value); i++) {
            value = ((fcs16Value >> (8 * i)) & 0xFF);
            if (escape(value, destination) < 0)
                return -EINVAL;
        }

        if (!destination.push_back(FlagSequence))
            return -EINVAL;

        return destination.size();
    }

    int decode(TransportAddress& address, Frame& frame, uint8_t& sequenceNumber, const Container source, Container destination, uint16_t& discardBytes) const
    {
        uint8_t value = 0;
        bool controlEscape = false;
        uint16_t i, fcs16Value = Fcs16InitValue, sourceSize = source.size();
        int result = -1, frameStartIndex = -1, frameStopIndex = -1, destinationIndex = 0;

        if (!destination.data() || destination.empty())
            return -EINVAL;

        for (i = 0; i < sourceSize; i++) {
            // First find the start flag sequence
            if (frameStartIndex < 0) {
                if (source[i] == FlagSequence) {
                    // Check if an additional flag sequence byte is present
                    if ((i < (sourceSize - 1)) && (source[i + 1] == FlagSequence)) {
                        // Just loop again to silently discard it (accordingly to HDLC)
                        continue;
                    }

                    frameStartIndex = i;
                }
            } else {
                if (source[i] == FlagSequence) {
                    // Check for end flag sequence
                    if (((i < (sourceSize - 1)) && (source[i + 1] == FlagSequence)) || ((frameStartIndex + 1) == i)) {
                        // Just loop again to silently discard it (accordingly to HDLC)
                        continue;
                    }

                    frameStopIndex = i;
                    break;
                } else if (source[i] == ControlEscape) { // FIXME: Addressing with "ControlEscape" will not work
                    controlEscape = true;
                } else {
                    if (controlEscape) {
                        controlEscape = false;
                        value = source[i] ^ 0x20;
                    } else {
                        value = source[i];
                    }

                    fcs16Value = fcs16(fcs16Value, value);
                    const int addressByteIndex = frameStartIndex + 1;
                    const int controlByteIndex = frameStartIndex + 2;

                    // Flag; Address; Control; Data ..
                    if (i == addressByteIndex) {
                        address = value;
                    } else if (i == controlByteIndex) {
                        decodeControlByte(value, frame, sequenceNumber);
                    } else if (i > controlByteIndex) {
                        destination[destinationIndex++] = value;
                    }
                }
            }
        }

        // Check for invalid frame (no start or end flag sequence)
        if ((frameStartIndex < 0) || (frameStopIndex < 0)) {
            discardBytes = 0;
            result = -ENOMSG;
        } else {
            // A frame is at least 4 bytes in size and has a valid FCS value
            if ((frameStopIndex >= (frameStartIndex + 4)) && (fcs16Value == Fcs16GoodValue)) {
                result = destinationIndex - sizeof(fcs16Value);
            } else {
                result = -EIO;
            }

            discardBytes = i;
        }

        return result;
    }

    int writeFrame(TransportAddress address, Frame frame, uint8_t sequenceNumber, ConstContainer data)
    {
        int result;

        if ((result = encode(address, frame, sequenceNumber, data, { writeBuffer })) < 0)
            return result;

        return transportWrite(std::span(writeBuffer).first(result));
    }

    int escape(uint8_t value, Hdlcpp::span<uint8_t>& destination) const
    {
        if ((value == FlagSequence) || (value == ControlEscape)) {
            if (!destination.push_back(ControlEscape))
                return -EINVAL;
            value ^= 0x20;
        }

        if (!destination.push_back(value))
            return -EINVAL;

        return 0;
    }

    static uint8_t encodeControlByte(Frame frame, uint8_t sequenceNumber)
    {
        uint8_t value = 0;

        // For details see: https://en.wikipedia.org/wiki/High-Level_Data_Link_Control
        switch (frame) {
        case FrameData:
            // Create the HDLC I-frame control byte with Poll bit set
            value |= (sequenceNumber << ControlSendSeqNumberBit);
            value |= (1 << ControlPollBit);
            break;
        case FrameAck:
            // Create the HDLC Receive Ready S-frame control byte with Poll bit cleared
            value |= (sequenceNumber << ControlReceiveSeqNumberBit);
            value |= (1 << ControlSFrameBit);
            break;
        case FrameNack:
            // Create the HDLC Receive Ready S-frame control byte with Poll bit cleared
            value |= (sequenceNumber << ControlReceiveSeqNumberBit);
            value |= (ControlTypeReject << ControlSFrameTypeBit);
            value |= (1 << ControlSFrameBit);
            break;
        }

        return value;
    }

    static void decodeControlByte(uint8_t value, Frame& frame, uint8_t& sequenceNumber)
    {
        // Check if the frame is a S-frame
        if ((value >> ControlSFrameBit) & 0x1) {
            // Check if S-frame type is a Receive Ready (ACK)
            if (((value >> ControlSFrameTypeBit) & 0x3) == ControlTypeReceiveReady) {
                frame = FrameAck;
            } else {
                // Assume it is an NACK since Receive Not Ready, Selective Reject and U-frames are not supported
                frame = FrameNack;
            }

            // Add the 3-bit receive sequence number from the S-frame
            sequenceNumber = (value >> ControlReceiveSeqNumberBit) & 0x7;
        } else {
            // It must be an I-frame so add the 3-bit send sequence number (receive sequence number is not used)
            frame = FrameData;
            sequenceNumber = (value >> ControlSendSeqNumberBit) & 0x7;
        }
    }

    static uint16_t fcs16(uint16_t fcs16Value, uint8_t value)
    {
        static constexpr uint16_t fcs16ValueTable[256] = { 0x0000, 0x1189, 0x2312, 0x329b,
            0x4624, 0x57ad, 0x6536, 0x74bf, 0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c,
            0xdbe5, 0xe97e, 0xf8f7, 0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c,
            0x75b7, 0x643e, 0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff,
            0xe876, 0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
            0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5, 0x3183,
            0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c, 0xbdcb, 0xac42,
            0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974, 0x4204, 0x538d, 0x6116,
            0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb, 0xce4c, 0xdfc5, 0xed5e, 0xfcd7,
            0x8868, 0x99e1, 0xab7a, 0xbaf3, 0x5285, 0x430c, 0x7197, 0x601e, 0x14a1,
            0x0528, 0x37b3, 0x263a, 0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960,
            0xbbfb, 0xaa72, 0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630,
            0x17b9, 0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
            0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738, 0xffcf,
            0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70, 0x8408, 0x9581,
            0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7, 0x0840, 0x19c9, 0x2b52,
            0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff, 0x9489, 0x8500, 0xb79b, 0xa612,
            0xd2ad, 0xc324, 0xf1bf, 0xe036, 0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5,
            0x4f6c, 0x7df7, 0x6c7e, 0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7,
            0xc03c, 0xd1b5, 0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74,
            0x5dfd, 0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
            0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c, 0xc60c,
            0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3, 0x4a44, 0x5bcd,
            0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb, 0xd68d, 0xc704, 0xf59f,
            0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232, 0x5ac5, 0x4b4c, 0x79d7, 0x685e,
            0x1ce1, 0x0d68, 0x3ff3, 0x2e7a, 0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a,
            0xb0a3, 0x8238, 0x93b1, 0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb,
            0x0e70, 0x1ff9, 0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9,
            0x8330, 0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78 };

        return (fcs16Value >> 8) ^ fcs16ValueTable[(fcs16Value ^ value) & 0xff];
    }

    static constexpr uint16_t Fcs16InitValue = 0xffff;
    static constexpr uint16_t Fcs16GoodValue = 0xf0b8;
    static constexpr uint8_t FlagSequence = 0x7e;
    static constexpr uint8_t ControlEscape = 0x7d;

    std::mutex writeMutex;
    TransportRead transportRead;
    TransportWrite transportWrite;
    Buffer<uint8_t> readBuffer;
    Container writeBuffer;
    Frame readFrame;
    uint16_t writeTimeout;
    uint8_t writeRetries;
    uint8_t readSequenceNumber { 0 };
    uint8_t writeSequenceNumber { 0 };
    std::atomic<int> writeResult { -1 };
    std::atomic<bool> stopped { false };
};

} // namespace Hdlcpp
