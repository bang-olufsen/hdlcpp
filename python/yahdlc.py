#!/usr/bin/python3
import pyahdlc

class Yahdlc:
    def __init__(self, transportRead, transportWrite, buffersize=256, timeout=2000, retries=1):
        self.yahdlc = pyahdlc.Yahdlc(transportRead, transportWrite, buffersize, timeout, retries)

    def read(self, length):
        return self.yahdlc.read(length)

    def write(self, data, length):
        return self.yahdlc.write(data, length)
