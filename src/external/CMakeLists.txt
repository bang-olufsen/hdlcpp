if (BUILD_TESTING)
    set(PYBIND11_CPP_STANDARD -std=c++17)
    add_subdirectory(pybind11)
endif()
