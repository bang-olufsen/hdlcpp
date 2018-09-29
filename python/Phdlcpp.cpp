#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include "../include/Hdlcpp.hpp"

using Pybind11Read = std::function<char(int)>;
using Pybind11Write = std::function<int(pybind11::bytes)>;

PYBIND11_MODULE(phdlcpp, m)
{
    pybind11::class_<Hdlcpp::Hdlcpp>(m, "Hdlcpp")
        .def(pybind11::init([](Pybind11Read read, Pybind11Write write, unsigned short bufferSize,
            unsigned short writeTimeout, unsigned char writeRetries) {
            return std::unique_ptr<Hdlcpp::Hdlcpp>(new Hdlcpp::Hdlcpp(
                [read](unsigned char *data, unsigned short length) { //!
                    // Read a single byte as the python serial read will wait for all bytes to be present
                    length = 1;
                    data[0] = static_cast<unsigned char>(read(length));

                    return length;
                },
                [write](const unsigned char *data, unsigned short length) {
                    return write(pybind11::bytes((char *)data, length));
                },
                bufferSize, writeTimeout, writeRetries));
        }))
        .def("read", [](Hdlcpp::Hdlcpp *hdlcpp, unsigned short length) {
            int bytes;
            unsigned char data[length];

            if ((bytes = hdlcpp->read(data, length)) < 0)
                bytes = 0;

            return pybind11::bytes((char *)data, bytes);
        })
        .def("write", [](Hdlcpp::Hdlcpp *hdlcpp, char *data, unsigned short length) {
            return hdlcpp->write((unsigned char *)data, length);
        });
}
