set(MODULE_NAME test-hdlcpp)

include_directories(../include)
add_executable(${MODULE_NAME} src/TestHdlcpp.cpp)

add_test(${MODULE_NAME} ${MODULE_NAME})
