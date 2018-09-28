set(MODULE_NAME test-yahdlc)

add_executable(${MODULE_NAME} src/TestYahdlc.cpp)
target_link_libraries(${MODULE_NAME} catch turtle yahdlc)

add_test(${MODULE_NAME} ${MODULE_NAME})
