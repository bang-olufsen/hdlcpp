#!/usr/bin/python3
import phdlcpp

class Hdlcpp:
    def __init__(self, transportRead, transportWrite, bufferze = 256, writeTimeout=100, writeRetries=1):
        self.hdlcpp = phdlcpp.Hdlcpp(transportRead, transportWrite, bufferze, writeTimeout, writeRetries)

    def read(self, length):
        return self.hdlcpp.read(length)

    def write(self, data, length):
        return self.hdlcpp.write(data, length)

    def close(self):
        return self.hdlcpp.close()