set(MODULE_NAME catch)

if(BUILD_TESTING)
    add_library(${MODULE_NAME} src/catch.cpp)
    target_include_directories(${MODULE_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/src/external/Catch2/single_include/catch2)
endif()
