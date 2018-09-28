#!/bin/bash
set -e

mkdir -p .build-external; pushd .build-external
cmake ../external; make -j "$(nproc)"
popd

mkdir -p .build-x86; pushd .build-x86
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/gcc.cmake ..; make -j "$(nproc)"; ctest --verbose
popd
