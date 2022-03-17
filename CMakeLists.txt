project(hdlcpp)
cmake_minimum_required(VERSION 3.1)

add_library(${PROJECT_NAME} INTERFACE)

include(GNUInstallDirs)

target_include_directories(${PROJECT_NAME}
    INTERFACE ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME})

install(FILES include/Hdlcpp.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hdlcpp)

install(EXPORT ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    NAMESPACE hdlcpp:: FILE ${PROJECT_NAME}-config.cmake
    EXPORT_LINK_INTERFACE_LIBRARIES
)

if (BUILD_HDLCPP_MAIN)
    include_directories(include)
    add_library(${PROJECT_NAME}-main src/main.cpp)
    target_include_directories(${PROJECT_NAME}-main PUBLIC include)
endif()

if (BUILD_TESTING)
    include_directories(include)
    if (BUILD_EXTERNAL)
        add_subdirectory(src)
    endif()
    add_subdirectory(test)
    add_subdirectory(python)
endif()
