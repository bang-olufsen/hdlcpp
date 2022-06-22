#!/bin/bash
set -e

mkdir -p .build-x86; pushd .build-x86
cmake --no-warn-unused-cli -DBUILD_EXTERNAL=1 -DCMAKE_TOOLCHAIN_FILE=cmake/gcc.cmake ..;
make -j "$(nproc)"
sudo make -j "$(nproc)" install

popd
