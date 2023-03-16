#include "../include/Hdlcpp.hpp"
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <memory>

using Pybind11Read = std::function<char(int)>;
using Pybind11Write = std::function<int(pybind11::bytes)>;

PYBIND11_MODULE(phdlcpp, m)
{
    pybind11::class_<Hdlcpp::Hdlcpp>(m, "Hdlcpp")
        .def(pybind11::init([](Pybind11Read read, Pybind11Write write, size_t bufferSize,
                                uint16_t writeTimeout, uint8_t writeRetries) {
            std::vector<Hdlcpp::value_type> readBuffer(bufferSize), writeBuffer(bufferSize);
            return std::make_unique<Hdlcpp::Hdlcpp>(
                [read](Hdlcpp::Container buffer) {
                    // Read a single byte as the python serial read will wait for all bytes to be present
                    uint8_t length = 1;
                    buffer[0] = static_cast<uint8_t>(read(length));

                    return length;
                },
                [write](Hdlcpp::ConstContainer buffer) {
                    return write(pybind11::bytes(reinterpret_cast<const char*>(buffer.data()), buffer.size()));
                },
                readBuffer,
                writeBuffer,
                writeTimeout, writeRetries);
        }))
        .def(
            "read", [](Hdlcpp::Hdlcpp* hdlcpp, uint16_t length) {
                int bytes;
                uint8_t data[length];

                if ((bytes = hdlcpp->read({ data, length })) < 0)
                    bytes = 0;

                return pybind11::bytes(reinterpret_cast<char*>(data), bytes);
            },
            pybind11::call_guard<pybind11::gil_scoped_release>())
        .def(
            "write", [](Hdlcpp::Hdlcpp* hdlcpp, char* data, uint16_t length) {
                return hdlcpp->write({ reinterpret_cast<uint8_t*>(data), length });
            },
            pybind11::call_guard<pybind11::gil_scoped_release>())
        .def(
            "close", [](Hdlcpp::Hdlcpp* hdlcpp) {
                hdlcpp->close();
            },
            pybind11::call_guard<pybind11::gil_scoped_release>());
}
