include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage(
    NAME Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    VERSION 2.4.0
    SOURCE_DIR ../src/external/Catch2
    DOWNLOAD_ONLY True
)

CPMAddPackage(
    NAME turtle
    GIT_REPOSITORY https://github.com/mat007/turtle
    GIT_TAG 0dd0dfa15fbc2774213523fb3a65bc1acb2857b3
    SOURCE_DIR ../src/external/turtle
    DOWNLOAD_ONLY True
)

CPMAddPackage(
    NAME pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11
    VERSION 2.9.2
    SOURCE_DIR ../src/external/pybind11
    DOWNLOAD_ONLY True
)
