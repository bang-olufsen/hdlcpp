project(external)

include(ExternalProject)
cmake_minimum_required(VERSION 3.1)

ExternalProject_Add(
    turtle
    GIT_REPOSITORY https://github.com/mat007/turtle
    GIT_TAG e92f52821da312c5e30b447c8046893e576aefca
    SOURCE_DIR ../src/external/turtle
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Add(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2
    GIT_TAG v2.4.0
    SOURCE_DIR ../src/external/Catch2
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Add(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11
    GIT_TAG v2.2.4
    SOURCE_DIR ../src/external/pybind11
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
)
