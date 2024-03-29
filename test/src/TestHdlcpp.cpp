#include "turtle/catch.hpp"
#include <catch.hpp>

#define protected public
#include "Hdlcpp.hpp"

class HdlcppFixture {
    static constexpr uint16_t bufferSize = 64;

public:
    HdlcppFixture()
        : hdlcpp_writeBuffer(bufferSize)
    {
        hdlcpp = std::make_shared<Hdlcpp::Hdlcpp>(
            [this](Hdlcpp::Container buffer) { return transportRead(buffer); },
            [this](Hdlcpp::ConstContainer buffer) { return transportWrite(buffer); },
            hdlcpp_readBuffer,
            hdlcpp_writeBuffer,
            1 // Use a 1 ms timeout to speed up tests
        );

        // For testing only execute a single iteration instead of blocking
        hdlcpp->stopped = true;
    }

    size_t transportRead(Hdlcpp::Container buffer)
    {
        std::memcpy(buffer.data(), readBuffer.data(), readBuffer.size());
        return readBuffer.size();
    }

    size_t transportWrite(Hdlcpp::ConstContainer buffer)
    {
        writeBuffer.assign(buffer.begin(), buffer.end());
        return writeBuffer.size();
    }

    const uint8_t frameAck[6] = { 0x7e, 0xff, 0x41, 0x0a, 0xa3, 0x7e };
    const uint8_t frameNack[6] = { 0x7e, 0xff, 0x29, 0x44, 0x4c, 0x7e };
    const uint8_t frameData[7] = { 0x7e, 0xff, 0x12, 0x55, 0x36, 0xa3, 0x7e };
    const uint8_t frameDataInvalid[7] = { 0x7e, 0xff, 0x12, 0x33, 0x67, 0xf8, 0x7e };
    const uint8_t frameDataDoubleFlagSequence[9] = { 0x7e, 0x7e, 0xff, 0x12, 0x55, 0x36, 0xa3, 0x7e, 0x7e };

    std::shared_ptr<Hdlcpp::Hdlcpp> hdlcpp;
    Hdlcpp::StaticBuffer<Hdlcpp::Calculate<bufferSize>::WithOverhead> hdlcpp_readBuffer {};
    std::vector<Hdlcpp::value_type> hdlcpp_writeBuffer {};
    std::vector<uint8_t> readBuffer;
    std::vector<uint8_t> writeBuffer;
    uint8_t dataBuffer[10];
};

TEST_CASE_METHOD(HdlcppFixture, "hdlcpp test", "[single-file]")
{
    SECTION("Test write with invalid input")
    {
        CHECK(hdlcpp->write(Hdlcpp::AddressBroadcast, { dataBuffer, 0 }) == -EINVAL);
        CHECK(hdlcpp->write(Hdlcpp::AddressBroadcast, {}) == -EINVAL);
    }

    SECTION("Test write with valid 1 byte data input")
    {
        hdlcpp->write(Hdlcpp::AddressBroadcast, { &frameData[3], 1 });
        CHECK(std::memcmp(frameData, writeBuffer.data(), sizeof(frameData)) == 0);
    }

    SECTION("Test write/read with FlagSequence as data input")
    {
        hdlcpp->write(Hdlcpp::AddressBroadcast, { &hdlcpp->FlagSequence, 1 });
        // Size should be 1 byte more compared to a 1 byte data frame due to escape of the value
        CHECK(writeBuffer.size() == (sizeof(frameData) + 1));

        readBuffer = writeBuffer;

        CHECK(hdlcpp->read(dataBuffer).size == 1);
        CHECK(dataBuffer[0] == hdlcpp->FlagSequence);
    }

    SECTION("Test write/read with ControlEscape as data input")
    {
        hdlcpp->write(Hdlcpp::AddressBroadcast, { &hdlcpp->ControlEscape, 1 });
        // Size should be 1 byte more compared to a 1 byte data frame due to escape of the value
        CHECK(writeBuffer.size() == (sizeof(frameData) + 1));

        readBuffer = writeBuffer;

        CHECK(hdlcpp->read(dataBuffer).size == 1);
        CHECK(dataBuffer[0] == hdlcpp->ControlEscape);
    }

    SECTION("Test read with invalid input")
    {
        CHECK(hdlcpp->read({ dataBuffer, 0 }).size == -EINVAL);
        CHECK(hdlcpp->read({}).size == -EINVAL);
        // This should fail as the buffer size is configured to less
        CHECK(hdlcpp->read({ dataBuffer, 256 }).size == -EINVAL);

        readBuffer.assign(frameDataInvalid, frameDataInvalid + sizeof(frameDataInvalid));
        CHECK(hdlcpp->read(dataBuffer).size == -EIO);
        CHECK(std::memcmp(frameNack, writeBuffer.data(), sizeof(frameNack)) == 0);
    }

    SECTION("Test read of two valid 1 byte data frames")
    {
        readBuffer.assign(frameData, frameData + sizeof(frameData));
        CHECK(hdlcpp->read(dataBuffer).size == 1);
        CHECK(dataBuffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);

        CHECK(hdlcpp->read(dataBuffer).size == 1);
        CHECK(dataBuffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);
    }

    SECTION("Test read of valid 1 byte data frame in two chunks")
    {
        // Add the first 3 bytes to be read
        readBuffer.assign(frameData, frameData + 3);
        CHECK(hdlcpp->read(dataBuffer).size == -ENOMSG);
        CHECK(writeBuffer.empty());

        // Now add the remaining bytes to complete the data frame
        readBuffer.assign(frameData + 3, frameData + sizeof(frameData));
        CHECK(hdlcpp->read(dataBuffer).size == 1);
        CHECK(dataBuffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);
    }

    SECTION("Test read of valid 1 byte data frame with double flag sequence")
    {
        readBuffer.assign(frameDataDoubleFlagSequence, frameDataDoubleFlagSequence + sizeof(frameDataDoubleFlagSequence));
        CHECK(hdlcpp->read(dataBuffer).size == 1);
        CHECK(dataBuffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);
    }

    SECTION("Test read of two partial data frames")
    {
        readBuffer.assign(frameData, frameData + sizeof(frameData)); // One complete frame
        readBuffer.insert(readBuffer.end(), frameData, frameData + 3); // A partial frame
        CHECK(hdlcpp->read(dataBuffer).size == 1);
        CHECK(dataBuffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);

        readBuffer.assign(frameData + 3, frameData + sizeof(frameData));
        CHECK(hdlcpp->read(dataBuffer).size == 1);
        CHECK(dataBuffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);
    }

    SECTION("Test read of two complete data frames")
    {
        readBuffer.assign(frameData, frameData + sizeof(frameData));
        readBuffer.insert(readBuffer.end(), frameData, frameData + sizeof(frameData));

        // Read first frame
        CHECK(hdlcpp->read(dataBuffer).size == 1);
        CHECK(dataBuffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);

        readBuffer.clear();

        // This must read the second frame without reading from transport layer
        CHECK(hdlcpp->read(dataBuffer).size == 1);
        CHECK(dataBuffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);
    }

    SECTION("Test read of ack frame")
    {
        hdlcpp->writeSequenceNumber = 1;
        readBuffer.assign(frameAck, frameAck + sizeof(frameAck));
        CHECK(hdlcpp->read(dataBuffer).size == 0);
        CHECK(hdlcpp->writeResult == Hdlcpp::Hdlcpp::FrameAck);
    }

    SECTION("Test read of nack frame")
    {
        hdlcpp->writeSequenceNumber = 1;
        readBuffer.assign(frameNack, frameNack + sizeof(frameNack));
        CHECK(hdlcpp->read(dataBuffer).size == 0);
        CHECK(hdlcpp->writeResult == Hdlcpp::Hdlcpp::FrameNack);
    }

    SECTION("Test encode/decode functions with 1 byte data and varying addresses")
    {
        const uint8_t encodedAddress { GENERATE(range<uint8_t>(0x0, 0xff)) };
        if (encodedAddress == Hdlcpp::Hdlcpp::ControlEscape || encodedAddress == Hdlcpp::Hdlcpp::FlagSequence)
            return; // FIXME: Not supported!

        uint8_t decodedAddress { 0 };

        std::array<uint8_t, 256> data {};
        uint16_t discardBytes = 0;
        uint8_t dataValue = 0x55, encodeSequenceNumber = 3, decodeSequenceNumber = 0;
        Hdlcpp::Hdlcpp::Frame encodeFrame = Hdlcpp::Hdlcpp::FrameData, decodeFrame = Hdlcpp::Hdlcpp::FrameNack;

        CHECK(hdlcpp->encode(encodedAddress, encodeFrame, encodeSequenceNumber, { &dataValue, sizeof(dataValue) }, { data }) > 0);
        CHECK(hdlcpp->decode(decodedAddress, decodeFrame, decodeSequenceNumber, data, dataBuffer, discardBytes) > 0);

        CHECK(decodedAddress == encodedAddress);
        CHECK(encodeFrame == decodeFrame);
        CHECK(encodeSequenceNumber == decodeSequenceNumber);
        CHECK(dataBuffer[0] == dataValue);
    }

    SECTION("Test encode buffer too small")
    {
        // Slowly increasing buffer size to traverse down Hdlcpp::encode function.
        Hdlcpp::Hdlcpp::Frame encodeFrame = Hdlcpp::Hdlcpp::FrameData;
        uint8_t dataValue = 0x55, encodeSequenceNumber = 3;

        {
            std::array<uint8_t, 0> buffer {};
            CHECK(hdlcpp->encode(Hdlcpp::AddressBroadcast, encodeFrame, encodeSequenceNumber, {}, { buffer }) == -EINVAL);
        }

        {
            std::array<uint8_t, 1> buffer {};
            CHECK(hdlcpp->encode(Hdlcpp::AddressBroadcast, encodeFrame, encodeSequenceNumber, {}, { buffer }) == -EINVAL);
        }

        {
            std::array<uint8_t, 2> buffer {};
            CHECK(hdlcpp->encode(Hdlcpp::AddressBroadcast, encodeFrame, encodeSequenceNumber, {}, { buffer }) == -EINVAL);
        }

        {
            std::array<uint8_t, 3> buffer {};
            CHECK(hdlcpp->encode(Hdlcpp::AddressBroadcast, encodeFrame, encodeSequenceNumber, {}, { buffer }) == -EINVAL);
        }

        {
            std::array<uint8_t, 3> buffer {};
            CHECK(hdlcpp->encode(Hdlcpp::AddressBroadcast, encodeFrame, encodeSequenceNumber, { &dataValue, sizeof(dataValue) }, { buffer }) == -EINVAL);
        }

        {
            std::array<uint8_t, 4> buffer {};
            CHECK(hdlcpp->encode(Hdlcpp::AddressBroadcast, encodeFrame, encodeSequenceNumber, { &dataValue, sizeof(dataValue) }, { buffer }) == -EINVAL);
        }

        {
            std::array<uint8_t, 6> buffer {};
            CHECK(hdlcpp->encode(Hdlcpp::AddressBroadcast, encodeFrame, encodeSequenceNumber, { &dataValue, sizeof(dataValue) }, { buffer }) == -EINVAL);
        }
    }

    SECTION("Test escape in a too small buffer")
    {
        std::array<uint8_t, 0> buffer {};
        Hdlcpp::Hdlcpp::span<uint8_t> span(buffer);
        const auto value { GENERATE(0x7e, 0x7d) };
        CHECK(hdlcpp->escape(value, span) == -EINVAL);
    }

    SECTION("Test close function")
    {
        hdlcpp->close();
        CHECK(hdlcpp->stopped);
    }

    SECTION("Test lockup scenario")
    {
        // Frames captured from lockup scenario
        const uint8_t frame1[] = { 0x7e, 0xff, 0x12, 0x12, 0x00, 0x00, 0xaf, 0x7e };
        const uint8_t frame2[] = { 0x7e, 0xff, 0x14, 0x4a, 0x07, 0x0a, 0x7e, 0xff, 0x14 };
        const uint8_t frame3[] = { 0x4a, 0x07, 0x0a, 0x01, 0x00, 0x10, 0x01, 0x20, 0x64, 0xca, 0x51, 0x7e };
        const uint8_t frame4[] = { 0x7e, 0xff, 0x14 };
        const uint8_t frame5[] = { 0x4a, 0x07, 0x0a, 0x01, 0x00, 0x10, 0x01, 0x20, 0x64, 0xca };
        const uint8_t frame6[] = { 0x51, 0x7e };
        const uint8_t frame7[] = { 0x7e, 0xff, 0x21, 0x0c, 0xc0, 0x7e };

        readBuffer.assign(frame1, frame1 + sizeof(frame1));
        CHECK(hdlcpp->read(dataBuffer).size == 2);
        readBuffer.assign(frame2, frame2 + sizeof(frame2));
        CHECK(hdlcpp->read(dataBuffer).size == -EIO);
        readBuffer.assign(frame3, frame3 + sizeof(frame3));
        CHECK(hdlcpp->read(dataBuffer).size == 9);
        readBuffer.assign(frame4, frame4 + sizeof(frame4));
        CHECK(hdlcpp->read(dataBuffer).size == -ENOMSG);
        readBuffer.assign(frame5, frame5 + sizeof(frame5));
        CHECK(hdlcpp->read(dataBuffer).size == -ENOMSG);
        readBuffer.assign(frame6, frame6 + sizeof(frame6));
        CHECK(hdlcpp->read(dataBuffer).size == 9);
        readBuffer.assign(frame7, frame7 + sizeof(frame7));
        CHECK(hdlcpp->read(dataBuffer).size == 0);
        // Check that the initial frame can still be decoded successfully
        readBuffer.assign(frame1, frame1 + sizeof(frame1));
        CHECK(hdlcpp->read(dataBuffer).size == 2);
    }

    SECTION("Test push_back on full buffer")
    {
        std::array<uint8_t, 1> buffer {};

        Hdlcpp::Hdlcpp::span<uint8_t> span(buffer);

        CHECK(span.push_back(1));
        CHECK_FALSE(span.push_back(2));
        CHECK(buffer.back() == 1);
    }
}
