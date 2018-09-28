# hdlcpp

[![Build Status](https://travis-ci.org/bang-olufsen/hdlcpp.svg?branch=master)](https://travis-ci.org/bang-olufsen/hdlcpp) [![License](https://img.shields.io/badge/license-MIT_License-blue.svg?style=flat)](LICENSE)

Hdlcpp is a header-only C++17 framing protocol optimized for embedded communication. It is the successor of the previous [yahdlc](https://github.com/bang-olufsen/yahdlc) implementation written in C. It uses the HDLC asynchronous framing format. For more information see:

https://en.wikipedia.org/wiki/High-Level_Data_Link_Control

The supported frames are limited to DATA (I-frame with Poll bit), ACK (S-frame Receive Ready with Final bit) and NACK (S-frame Reject with Final bit). All DATA frames must be acknowledged or negative acknowledged using the defined ACK and NACK frames. The Address and Control fields uses the 8-bit format which means that the highest sequence number is 7. The FCS field is 16-bit.

Below are some examples on the usage:

```
Acknowledge of frames:
A ----> B   DATA [Seq No = 1]
A <---- B   DATA [Seq No = 4]
A <---- B    ACK [Seq No = 2]
A ----> B    ACK [Seq No = 5]

Negative acknowledge of frame:
A ----> B   DATA [Seq No = 1]
A <---- B   NACK [Seq No = 1]
A ----> B   DATA [Seq No = 1]

Acknowledge frame lost:
A ----> B   DATA [Seq No = 1]
A  X<-- B    ACK [Seq No = 2]
*Timeout*
A ----> B   DATA [Seq No = 1]
```
