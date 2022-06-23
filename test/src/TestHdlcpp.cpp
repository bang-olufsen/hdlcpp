#include <catch.hpp>
#include "turtle/catch.hpp"

#define protected public
#include "Hdlcpp.hpp"

class HdlcppFixture {
public:
    HdlcppFixture()
    {
        hdlcpp = std::make_shared<Hdlcpp::Hdlcpp>(
            [this](std::span<uint8_t> buffer) { return transportRead(buffer); },
            [this](const std::span<const uint8_t> buffer) { return transportWrite(buffer); },
            bufferSize,
            1 // Use a 1 ms timeout to speed up tests
            );

        // For testing only execute a single iteration instead of blocking
        hdlcpp->stopped = true;
    }

    size_t transportRead(std::span<uint8_t> buffer)
    {
        std::memcpy(buffer.data(), readBuffer.data(), readBuffer.size());
        return readBuffer.size();
    }

    size_t transportWrite(const std::span<const uint8_t> buffer)
    {
        writeBuffer.assign(buffer.begin(), buffer.end());
        return writeBuffer.size();
    }

    const uint16_t bufferSize = 64;
    const uint8_t frameAck[6] = { 0x7e, 0xff, 0x41, 0x0a, 0xa3, 0x7e };
    const uint8_t frameNack[6] = { 0x7e, 0xff, 0x29, 0x44, 0x4c, 0x7e };
    const uint8_t frameData[7] = { 0x7e, 0xff, 0x12, 0x55, 0x36, 0xa3, 0x7e };
    const uint8_t frameDataInvalid[7] = { 0x7e, 0xff, 0x12, 0x33, 0x67, 0xf8, 0x7e };
    const uint8_t frameDataDoubleFlagSequence[9] = { 0x7e, 0x7e, 0xff, 0x12, 0x55, 0x36, 0xa3, 0x7e, 0x7e };

    std::shared_ptr<Hdlcpp::Hdlcpp> hdlcpp;
    std::vector<uint8_t> readBuffer;
    std::vector<uint8_t> writeBuffer;
    uint8_t dataBuffer[10];
};

TEST_CASE_METHOD(HdlcppFixture, "hdlcpp test", "[single-file]")
{
    SECTION("Test write with invalid input")
    {
        CHECK(hdlcpp->write({dataBuffer, 0}) == -EINVAL);
        CHECK(hdlcpp->write({}) == -EINVAL);
    }

    SECTION("Test write with valid 1 byte data input")
    {
        hdlcpp->write({&frameData[3], 1});
        CHECK(std::memcmp(frameData, writeBuffer.data(), sizeof(frameData)) == 0);
    }

    SECTION("Test write/read with FlagSequence as data input")
    {
        hdlcpp->write({&hdlcpp->FlagSequence, 1});
        // Size should be 1 byte more compared to a 1 byte data frame due to escape of the value
        CHECK(writeBuffer.size() == (sizeof(frameData) + 1));

        readBuffer = writeBuffer;

        CHECK(hdlcpp->read(dataBuffer) == 1);
        CHECK(dataBuffer[0] == hdlcpp->FlagSequence);
    }

    SECTION("Test write/read with ControlEscape as data input")
    {
        hdlcpp->write({&hdlcpp->ControlEscape, 1});
        // Size should be 1 byte more compared to a 1 byte data frame due to escape of the value
        CHECK(writeBuffer.size() == (sizeof(frameData) + 1));

        readBuffer = writeBuffer;

        CHECK(hdlcpp->read(dataBuffer) == 1);
        CHECK(dataBuffer[0] == hdlcpp->ControlEscape);
    }

    SECTION("Test read with invalid input")
    {
        CHECK(hdlcpp->read({dataBuffer, 0}) == -EINVAL);
        CHECK(hdlcpp->read({}) == -EINVAL);
        // This should fail as the buffer size is configured to less
        CHECK(hdlcpp->read({dataBuffer, 256}) == -EINVAL);

        readBuffer.assign(frameDataInvalid, frameDataInvalid + sizeof(frameDataInvalid));
        CHECK(hdlcpp->read(dataBuffer) == -EIO);
        CHECK(std::memcmp(frameNack, writeBuffer.data(), sizeof(frameNack)) == 0);
    }

    SECTION("Test read of two valid 1 byte data frames")
    {
        readBuffer.assign(frameData, frameData + sizeof(frameData));
        CHECK(hdlcpp->read(dataBuffer) == 1);
        CHECK(dataBuffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);

        CHECK(hdlcpp->read(dataBuffer) == 1);
        CHECK(dataBuffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);
    }

    SECTION("Test read of valid 1 byte data frame in two chunks")
    {
        // Add the first 3 bytes to be read
        readBuffer.assign(frameData, frameData + 3);
        CHECK(hdlcpp->read(dataBuffer) == -ENOMSG);
        CHECK(writeBuffer.empty());

        // Now add the remaining bytes to complete the data frame
        readBuffer.assign(frameData + 3, frameData + sizeof(frameData));
        CHECK(hdlcpp->read(dataBuffer) == 1);
        CHECK(dataBuffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);
    }

    SECTION("Test read of valid 1 byte data frame with double flag sequence")
    {
        readBuffer.assign(frameDataDoubleFlagSequence, frameDataDoubleFlagSequence + sizeof(frameDataDoubleFlagSequence));
        CHECK(hdlcpp->read(dataBuffer) == 1);
        CHECK(dataBuffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);
    }

    SECTION("Test read of two partial data frames")
    {
        readBuffer.assign(frameData, frameData + sizeof(frameData)); // One complete frame
        readBuffer.insert(readBuffer.end(), frameData, frameData + 3); // A partial frame
        CHECK(hdlcpp->read(dataBuffer) == 1);
        CHECK(dataBuffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);

        readBuffer.assign(frameData + 3, frameData + sizeof(frameData));
        CHECK(hdlcpp->read(dataBuffer) == 1);
        CHECK(dataBuffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);
    }

    SECTION("Test read of two complete data frames")
    {
        readBuffer.assign(frameData, frameData + sizeof(frameData));
        readBuffer.insert(readBuffer.end(), frameData, frameData + sizeof(frameData));

        // Read first frame
        CHECK(hdlcpp->read(dataBuffer) == 1);
        CHECK(dataBuffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);

        readBuffer.clear();

        // This must read the second frame without reading from transport layer
        CHECK(hdlcpp->read(dataBuffer) == 1);
        CHECK(dataBuffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);
    }

    SECTION("Test read of ack frame")
    {
        hdlcpp->writeSequenceNumber = 1;
        readBuffer.assign(frameAck, frameAck + sizeof(frameAck));
        CHECK(hdlcpp->read(dataBuffer) == 0);
        CHECK(hdlcpp->writeResult == Hdlcpp::Hdlcpp::FrameAck);
    }

    SECTION("Test read of nack frame")
    {
        hdlcpp->writeSequenceNumber = 1;
        readBuffer.assign(frameNack, frameNack + sizeof(frameNack));
        CHECK(hdlcpp->read(dataBuffer) == 0);
        CHECK(hdlcpp->writeResult == Hdlcpp::Hdlcpp::FrameNack);
    }

    SECTION("Test encode/decode functions with 1 byte data")
    {
        std::vector<uint8_t> data;
        uint16_t discardBytes = 0;
        uint8_t dataValue = 0x55, encodeSequenceNumber = 3, decodeSequenceNumber = 0;
        Hdlcpp::Hdlcpp::Frame encodeFrame = Hdlcpp::Hdlcpp::FrameData, decodeFrame = Hdlcpp::Hdlcpp::FrameNack;

        CHECK(hdlcpp->encode(encodeFrame, encodeSequenceNumber, {&dataValue, sizeof(dataValue)}, data) > 0);
        CHECK(hdlcpp->decode(decodeFrame, decodeSequenceNumber, data, dataBuffer, discardBytes) > 0);
        CHECK(encodeFrame == decodeFrame);
        CHECK(encodeSequenceNumber == decodeSequenceNumber);
        CHECK(dataBuffer[0] == dataValue);
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
        CHECK(hdlcpp->read(dataBuffer) == 2);
        readBuffer.assign(frame2, frame2 + sizeof(frame2));
        CHECK(hdlcpp->read(dataBuffer) == -EIO);
        readBuffer.assign(frame3, frame3 + sizeof(frame3));
        CHECK(hdlcpp->read(dataBuffer) == 9);
        readBuffer.assign(frame4, frame4 + sizeof(frame4));
        CHECK(hdlcpp->read(dataBuffer) == -ENOMSG);
        readBuffer.assign(frame5, frame5 + sizeof(frame5));
        CHECK(hdlcpp->read(dataBuffer) == -ENOMSG);
        readBuffer.assign(frame6, frame6 + sizeof(frame6));
        CHECK(hdlcpp->read(dataBuffer) == 9);
        readBuffer.assign(frame7, frame7 + sizeof(frame7));
        CHECK(hdlcpp->read(dataBuffer) == 0);
        // Check that the initial frame can still be decoded successfully
        readBuffer.assign(frame1, frame1 + sizeof(frame1));
        CHECK(hdlcpp->read(dataBuffer) == 2);
    }
}
