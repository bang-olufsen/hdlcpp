#include <catch.hpp>
#include "turtle/catch.hpp"

#define private public
#include "Hdlcpp.hpp"

class HdlcppFixture {
public:
    HdlcppFixture()
    {
        hdlcpp = std::make_shared<Hdlcpp::Hdlcpp>(
            [this](uint8_t *data, uint16_t length) { return transportRead(data, length); },
            [this](const uint8_t *data, uint16_t length) { return transportWrite(data, length); },
            bufferSize,
            1 // Use a 1 ms timeout to speed up tests
            );

        // For testing only execute a single iteration instead of blocking
        hdlcpp->stopped = true;
    }

    int transportRead(uint8_t *data, uint16_t length)
    {
        (void)length;
        std::memcpy(data, readBuffer.data(), readBuffer.size());
        return readBuffer.size();
    }

    int transportWrite(const uint8_t *data, uint16_t length)
    {
        writeBuffer.assign((char *)data, (char *)data + length);
        return length;
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
    uint8_t buffer[10];
};

TEST_CASE_METHOD(HdlcppFixture, "hdlcpp test", "[single-file]")
{
    SECTION("Test write with invalid input")
    {
        CHECK(hdlcpp->write(buffer, 0) == -EINVAL);
        CHECK(hdlcpp->write(nullptr, sizeof(buffer)) == -EINVAL);
    }

    SECTION("Test write with valid 1 byte data input")
    {
        hdlcpp->write(&frameData[3], 1);
        CHECK(std::memcmp(frameData, writeBuffer.data(), sizeof(frameData)) == 0);
    }

    SECTION("Test write/read with FlagSequence as data input")
    {
        hdlcpp->write(&hdlcpp->FlagSequence, 1);
        // Size should be 1 byte more compared to a 1 byte data frame due to escape of the value
        CHECK(writeBuffer.size() == (sizeof(frameData) + 1));

        readBuffer = writeBuffer;

        CHECK(hdlcpp->read(buffer, sizeof(buffer)) == 1);
        CHECK(buffer[0] == hdlcpp->FlagSequence);
    }

    SECTION("Test write/read with ControlEscape as data input")
    {
        hdlcpp->write(&hdlcpp->ControlEscape, 1);
        // Size should be 1 byte more compared to a 1 byte data frame due to escape of the value
        CHECK(writeBuffer.size() == (sizeof(frameData) + 1));

        readBuffer = writeBuffer;

        CHECK(hdlcpp->read(buffer, sizeof(buffer)) == 1);
        CHECK(buffer[0] == hdlcpp->ControlEscape);
    }

    SECTION("Test read with invalid input")
    {
        CHECK(hdlcpp->read(buffer, 0) == -EINVAL);
        CHECK(hdlcpp->read(nullptr, sizeof(buffer)) == -EINVAL);
        // This should fail as the buffer size is configured to less
        CHECK(hdlcpp->read(buffer, 256) == -EINVAL);

        readBuffer.assign(frameDataInvalid, frameDataInvalid + sizeof(frameDataInvalid));
        CHECK(hdlcpp->read(buffer, sizeof(buffer)) == -EIO);
        CHECK(std::memcmp(frameNack, writeBuffer.data(), sizeof(frameNack)) == 0);
    }

    SECTION("Test read of two valid 1 byte data frames")
    {
        readBuffer.assign(frameData, frameData + sizeof(frameData));
        CHECK(hdlcpp->read(buffer, sizeof(buffer)) == 1);
        CHECK(buffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);

        CHECK(hdlcpp->read(buffer, sizeof(buffer)) == 1);
        CHECK(buffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);
    }

    SECTION("Test read of valid 1 byte data frame in two chunks")
    {
        // Add the first 3 bytes to be read
        readBuffer.assign(frameData, frameData + 3);
        CHECK(hdlcpp->read(buffer, sizeof(buffer)) == -ENOMSG);
        CHECK(writeBuffer.empty());

        // Now add the remaining bytes to complete the data frame
        readBuffer.assign(frameData + 3, frameData + sizeof(frameData));
        CHECK(hdlcpp->read(buffer, sizeof(buffer)) == 1);
        CHECK(buffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);
    }

    SECTION("Test read of valid 1 byte data frame with double flag sequence")
    {
        readBuffer.assign(frameDataDoubleFlagSequence, frameDataDoubleFlagSequence + sizeof(frameDataDoubleFlagSequence));
        CHECK(hdlcpp->read(buffer, sizeof(buffer)) == 1);
        CHECK(buffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);
    }

    SECTION("Test read of two partial data frames")
    {
        readBuffer.assign(frameData, frameData + sizeof(frameData)); // One complete frame
        readBuffer.insert(readBuffer.end(), frameData, frameData + 3); // A partial frame
        CHECK(hdlcpp->read(buffer, sizeof(buffer)) == 1);
        CHECK(buffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);

        readBuffer.assign(frameData + 3, frameData + sizeof(frameData));
        CHECK(hdlcpp->read(buffer, sizeof(buffer)) == 1);
        CHECK(buffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);
    }

    SECTION("Test read of two complete data frames")
    {
        readBuffer.assign(frameData, frameData + sizeof(frameData));
        readBuffer.insert(readBuffer.end(), frameData, frameData + sizeof(frameData));

        // Read first frame
        CHECK(hdlcpp->read(buffer, sizeof(buffer)) == 1);
        CHECK(buffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);
        int expectedSequenceNumber = (hdlcpp->readSequenceNumber + 1) % 8;

        readBuffer.clear();
        
        REQUIRE(hdlcpp->readSequenceNumber == expectedSequenceNumber);
        // This must read the second frame without reading from transport layer
        CHECK(hdlcpp->read(buffer, sizeof(buffer)) == 1);
        CHECK(buffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);
    }

    SECTION("Test sequence number must increment - skip messages with wrong sequence number - resend ACK")
    {
        readBuffer.assign(frameData, frameData + sizeof(frameData));

        // Read first frame
        CHECK(hdlcpp->read(buffer, sizeof(buffer)) == 1);
        CHECK(buffer[0] == frameData[3]);
        CHECK(std::memcmp(frameAck, writeBuffer.data(), sizeof(frameAck)) == 0);
        int expectedSequenceNumber = hdlcpp->readSequenceNumber;

        readBuffer.clear();

        for(int i = 0; i < 8; i++){
            if(i == expectedSequenceNumber)
                continue;

            //TODO: generate me!
            REQUIRE(hdlcpp->readSequenceNumber != expectedSequenceNumber);
            // Make sure you don't get any frame
            CHECK(hdlcpp->read(buffer, sizeof(buffer)) != 1);
        }
    }

    SECTION("Test read of ack frame")
    {
        hdlcpp->writeSequenceNumber = 1;
        readBuffer.assign(frameAck, frameAck + sizeof(frameAck));
        CHECK(hdlcpp->read(buffer, sizeof(buffer)) == 0);
        CHECK(hdlcpp->writeResult == Hdlcpp::Hdlcpp::FrameAck);
    }

    SECTION("Test assume message as sent on receiving correct ack sequence number")
    {
        //TODO: add test
    }

    SECTION("Test don't ack current message until correct sequence number ack is received")
    {
        //TODO: add test
    }

    SECTION("Test read of nack frame")
    {
        hdlcpp->writeSequenceNumber = 1;
        readBuffer.assign(frameNack, frameNack + sizeof(frameNack));
        CHECK(hdlcpp->read(buffer, sizeof(buffer)) == 0);
        CHECK(hdlcpp->writeResult == Hdlcpp::Hdlcpp::FrameNack);
    }

    SECTION("Test resending message on nack frame")
    {
        //TODO: add test
    }

    SECTION("Test don't resend message on nack if sequence counter doesn't match")
    {
        //TODO: add test
    }

    SECTION("Test encode/decode functions with 1 byte data")
    {
        std::vector<uint8_t> data;
        uint16_t discardBytes = 0;
        uint8_t dataValue = 0x55, encodeSequenceNumber = 3, decodeSequenceNumber = 0;
        Hdlcpp::Hdlcpp::Frame encodeFrame = Hdlcpp::Hdlcpp::FrameData, decodeFrame = Hdlcpp::Hdlcpp::FrameNack;

        CHECK(hdlcpp->encode(encodeFrame, encodeSequenceNumber, &dataValue, sizeof(dataValue), data) > 0);
        CHECK(hdlcpp->decode(decodeFrame, decodeSequenceNumber, data, buffer, sizeof(buffer), discardBytes) > 0);
        CHECK(encodeFrame == decodeFrame);
        CHECK(encodeSequenceNumber == decodeSequenceNumber);
        CHECK(buffer[0] == dataValue);
    }

    SECTION("Test close function")
    {
        hdlcpp->close();
        CHECK(hdlcpp->stopped);
    }

    SECTION("Test sync sending sequence counter")
    {
        //TODO: In case when one of sides get restarted it has to resync sequence counter
        //      Idea is to on first sending package wait for NACK and use sequence counter
        //      from NACK and resend first package
    }

    SECTION("Test sync receiving sequence counter")
    {
        //TODO: In case when one of sides get restarted it has to resync sequence counter
        //      Idea is to on first receiving package store current sequence number and use
        //      it for validating next packages
    }
}
