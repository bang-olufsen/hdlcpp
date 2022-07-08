set(MODULE_NAME phdlcpp)

pybind11_add_module(${MODULE_NAME} Phdlcpp.cpp)
target_compile_definitions(${MODULE_NAME} PRIVATE -DPYTHON_HDLCPP_BUFFER_SIZE=${PYTHON_HDLCPP_BUFFER_SIZE})
install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION /usr/lib)
