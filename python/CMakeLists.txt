set(MODULE_NAME phdlcpp)

pybind11_add_module(${MODULE_NAME} Phdlcpp.cpp)
install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION /usr/lib)
