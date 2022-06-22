#!/bin/bash
set -e

mkdir -p .build; pushd .build
cmake --no-warn-unused-cli -DBUILD_HDLCPP_MAIN=1 -DBUILD_TESTING=0 -DCMAKE_TOOLCHAIN_FILE=cmake/gcc.cmake ..
make

popd
