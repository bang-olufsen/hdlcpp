project(hdlcpp)
cmake_minimum_required(VERSION 3.1)

add_library(${PROJECT_NAME} src/main.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC include)

if (BUILD_TESTING)
    if (BUILD_EXTERNAL)
        add_subdirectory(src)
    endif()
    add_subdirectory(test)
    add_subdirectory(python)
endif()
