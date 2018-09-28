#include <pybind11/pybind11.h>
#include <pybind11/functional.h>
#include "../include/Yahdlc.hpp"

using Pybind11Read = std::function<char(int)>;
using Pybind11Write = std::function<int(pybind11::bytes)>;

PYBIND11_MODULE(pyahdlc, m)
{
    pybind11::class_<Yahdlc::Yahdlc>(m, "Yahdlc")
        .def(pybind11::init([](Pybind11Read read, Pybind11Write write, unsigned short bufferSize,
            unsigned short timeout, unsigned char retries) {
            return std::unique_ptr<Yahdlc::Yahdlc>(new Yahdlc::Yahdlc(
                [read](unsigned char *data, unsigned short length) { //!
                    // Read a single byte as the python serial read will wait for all bytes to be present
                    length = 1;
                    data[0] = static_cast<unsigned char>(read(length));

                    return length;
                },
                [write](const unsigned char *data, unsigned short length) {
                    return write(pybind11::bytes((char *)data, length));
                },
                bufferSize, timeout, retries));
        }))
        .def("read", [](Yahdlc::Yahdlc *yahdlc, unsigned short length) {
            int bytes;
            unsigned char data[length];

            if ((bytes = yahdlc->read(data, length)) < 0)
                bytes = 0;

            return pybind11::bytes((char *)data, bytes);
        })
        .def("write", [](Yahdlc::Yahdlc *yahdlc, char *data, unsigned short length) {
            return yahdlc->write((unsigned char *)data, length);
        });
}
