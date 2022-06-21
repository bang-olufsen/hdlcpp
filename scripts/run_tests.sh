#!/bin/bash
set -e

pushd .build-x86
ctest -j "$(nproc)" --output-on-failure --timeout 5 "$@"
popd
