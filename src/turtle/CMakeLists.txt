set(MODULE_NAME turtle)

if(BUILD_TESTING)
    add_library(${MODULE_NAME} INTERFACE)
    target_include_directories(${MODULE_NAME} INTERFACE ${PROJECT_SOURCE_DIR}/src/external/turtle/include)
    target_compile_definitions(${MODULE_NAME} INTERFACE "MOCK_NO_AUTO_PTR")
endif()
