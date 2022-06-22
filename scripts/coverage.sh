#!/bin/bash
set -e

pushd .build-x86 1>/dev/null

rm -f python/Phdlcpp.cpp.gcno
lcov -q -c -i -d . -o base.info
lcov -q -c -d . -o test.info 2>/dev/null
lcov -q -a base.info -a test.info > total.info
lcov -q -r total.info "*usr/include/*" "*CMakeFiles*" "*/test/*" "*Catch2*" "*turtle*" \
"*pybind11*" "*python*" -o coverage.xml
genhtml -o coverage coverage.xml
echo "Coverage report can be found in $(pwd)/coverage"

popd 1>/dev/null
