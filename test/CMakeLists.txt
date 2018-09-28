set(MODULE_NAME test-hdlcpp)

add_executable(${MODULE_NAME} src/TestHdlcpp.cpp)
target_link_libraries(${MODULE_NAME} catch turtle hdlcpp)

add_test(${MODULE_NAME} ${MODULE_NAME})
