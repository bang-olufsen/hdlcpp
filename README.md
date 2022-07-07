# hdlcpp
![Github Build](https://github.com/bang-olufsen/hdlcpp/workflows/build/badge.svg) [![codecov](https://codecov.io/gh/bang-olufsen/hdlcpp/branch/master/graph/badge.svg)](https://codecov.io/gh/bang-olufsen/hdlcpp) [![License](https://img.shields.io/badge/license-MIT_License-blue.svg?style=flat)](LICENSE)

Hdlcpp is a header-only C++11 framing protocol optimized for embedded communication. It uses the [HDLC](https://en.wikipedia.org/wiki/High-Level_Data_Link_Control) asynchronous framing format for handling data integrity and retransmissions. Hdlcpp is the successor of the [yahdlc](https://github.com/bang-olufsen/yahdlc) implementation written in C.

## Usage

Hdlcpp requires that a transport read and write function is supplied as e.g. a [lambda expression](https://en.cppreference.com/w/cpp/language/lambda) for the actual data transport. Hereby Hdlcpp can easily be integrated e.g. with different UART implementations. It required a read and write buffer. The buffer type must to compatible with [std::span](https://en.cppreference.com/w/cpp/container/span), basically any contiguous sequence containers. The buffers can be size independent for encoding/decoding data, write timeout and number of write retries are also configurable when constructing the instance.

```cpp
hdlcpp = std::make_shared<Hdlcpp::Hdlcpp>(
    [this](std::span<uint8_t> buffer) { return serial->read(buffer); },
    [this](const std::span<const uint8_t> buffer) { return serial->write(buffer); },
    readBuffer, writeBuffer, writeTimeout, writeRetries);
```

In the case where the underlying transport layer does not support `std::span`, the pointer to the first element and the size can be extracted from the span like so.
```cpp
hdlcpp = std::make_shared<Hdlcpp::Hdlcpp>(
    [this](std::span<uint8_t> buffer) { return serial->read(buffer.data(), buffer.size()); },
    [this](const std::span<const uint8_t> buffer) { return serial->write(buffer.data(), buffer.size()); },
    readBuffer, writeBuffer, writeTimeout, writeRetries);
```

To read and write data using Hdlcpp the read and write functions are used. These could again e.g. be used as lambdas expressions to a protocol implementation (layered architecture). The protocol could e.g. be [nanopb](https://github.com/nanopb/nanopb).

```cpp
protocol = std::make_shared<Protocol>(
    [this](std::span<uint8_t> buffer) { return hdlcpp->read(buffer); },
    [this](const std::span<const uint8_t> buffer) { return hdlcpp->write(buffer); });
```

## Python binding

A python binding made using [pybind11](https://github.com/pybind/pybind11) can be found under the [python](https://github.com/bang-olufsen/hdlcpp/tree/master/python) folder which can be used e.g. for automated testing.

## HDLC implementation

The supported HDLC frames are limited to DATA (I-frame with Poll bit), ACK (S-frame Receive Ready with Final bit) and NACK (S-frame Reject with Final bit). All DATA frames are acknowledged or negative acknowledged. The Address and Control fields uses the 8-bit format which means that the highest sequence number is 7. The FCS field is 16-bit.

Acknowledge of frame | Negative acknowledge of frame | Acknowledge of frame lost
--- | --- | ---
![](https://bang-olufsen.gravizo.com/svg?%3B%0A%40startuml%3B%0Ahide%20footbox%3B%0AA%20-%3E%20B:%20DATA%20[sequence%20number%20=%201]%3B%0AB%20-%3E%20A:%20DATA%20[sequence%20number%20=%204]%3B%0AB%20-%3E%20A:%20ACK%20[sequence%20number%20=%202]%3B%0AA%20-%3E%20B:%20ACK%20[sequence%20number%20=%205]%3B%0A%40enduml) | ![](https://bang-olufsen.gravizo.com/svg?%3B%0A%40startuml%3B%0Ahide%20footbox%3B%0AA%20-%3E%20B:%20DATA%20[sequence%20number%20=%201]%3B%0AB%20-%3E%20A:%20NACK%20[sequence%20number%20=%201]%3B%0AA%20-%3E%20B:%20DATA%20[sequence%20number%20=%201]%3B%0A%40enduml) | ![](https://bang-olufsen.gravizo.com/svg?%3B%0A%40startuml%3B%0Ahide%20footbox%3B%0AA%20-%3E%20B:%20DATA%20[sequence%20number%20=%201]%3B%0AB%20-%3Ex%20A:%20ACK%20[sequence%20number%20=%202]%3B%0A...%20Timeout%20...%3B%0AA%20-%3E%20B:%20DATA%20[sequence%20number%20=%201]%3B%0A%40enduml)

## Build and run unit tests

The build setup is configured to be run from the given Docker Container. Tested with a PC running 64 bit Ubuntu. WSL will also work.

* [Clone](https://docs.github.com/en/get-started/getting-started-with-git/about-remote-repositories) this repository.
* Install Docker: `sudo apt install docker.io`. For WSL follow [this guide](https://docs.microsoft.com/en-us/windows/wsl/tutorials/wsl-containers).
* Run `./build.sh -h` to see build options.

NOTE: If using remote-containers for ie. VSCode you can open the folder in the container automatically (see `.devcontainer`).
