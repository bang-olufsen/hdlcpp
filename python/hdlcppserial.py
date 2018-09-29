#!/usr/bin/python3
from serial import Serial
from hdlcpp import Hdlcpp

class HdlcppSerial(Hdlcpp):
    def __init__(self, port, baudrate=115200, bufferSize=256, writeTimeout=1000, writeRetries=1):
        self.serial = Serial(port, baudrate)
        super().__init__(self._transportRead, self._transportWrite, bufferSize, writeTimeout, writeRetries)

    def _transportRead(self, length):
        return self.serial.read(length)

    def _transportWrite(self, data):
        return self.serial.write(data)
