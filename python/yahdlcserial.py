#!/usr/bin/python3
import serial
from yahdlc import Yahdlc

class YahdlcSerial(Yahdlc):
    def __init__(self, port, baudrate=115200, bufferSize=256, timeout=2000, retries=1):
        self.serial = serial.Serial(port, baudrate)
        super().__init__(self._transportRead, self._transportWrite, bufferSize, timeout, retries)

    def _transportRead(self, length):
        return self.serial.read(length)

    def _transportWrite(self, data):
        return self.serial.write(data)
