include(CTest)

set(CMAKE_ASM_COMPILER /usr/bin/gcc)
set(CMAKE_C_COMPILER /usr/bin/gcc)
set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_OBJCOPY /usr/bin/objcopy)

set(COMMON_FLAGS
    -Wall
    -Wextra
    -Werror
    -coverage
    -O0
)

set(COMMON_C_FLAGS
    ${COMMON_FLAGS}
    -std=gnu99
)

set(COMMON_CXX_FLAGS
    ${COMMON_FLAGS}
    -std=c++17
)

string(REPLACE ";" " " CMAKE_COMMON_C_FLAGS "${COMMON_C_FLAGS}")
string(REPLACE ";" " " CMAKE_COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS}")

set(CMAKE_C_FLAGS "${CMAKE_COMMON_C_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_COMMON_CXX_FLAGS}" CACHE STRING "" FORCE)
