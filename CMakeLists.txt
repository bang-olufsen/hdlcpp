project(hdlcpp)

include(ExternalProject)
cmake_minimum_required(VERSION 3.6)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE include)

if (BUILD_TESTING)
    add_subdirectory(src)
    add_subdirectory(test)
    add_subdirectory(python)
endif()
