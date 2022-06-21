if (BUILD_TESTING)
    set(-DCMAKE_CXX_STANDARD=20)
    add_subdirectory(pybind11)
endif()
