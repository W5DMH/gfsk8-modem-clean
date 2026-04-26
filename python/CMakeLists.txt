# python/CMakeLists.txt - build the gfsk8 Python module

find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(gfsk8_pymod gfsk8_python.cpp)

set_target_properties(gfsk8_pymod PROPERTIES
    OUTPUT_NAME "gfsk8"
)

target_link_libraries(gfsk8_pymod PRIVATE gfsk8modem)

target_include_directories(gfsk8_pymod PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_features(gfsk8_pymod PRIVATE cxx_std_17)
