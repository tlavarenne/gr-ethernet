# Trouver pybind11
find_package(pybind11 REQUIRED)

# Créer le module Python
pybind11_add_module(ethernet_python 
    python_bindings.cc
    slicer3_python.cc
    mlt3_to_scrambled_python.cc
    fastethernet_descrambler_python.cc
    ethernet_10baset_decoder_python.cc
    fastethernet_frame_decoder_python.cc
)

target_link_libraries(ethernet_python PUBLIC
    gnuradio-ethernet
)

target_include_directories(ethernet_python PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

install(TARGETS ethernet_python
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/python3.13/site-packages/gnuradio/ethernet
    COMPONENT pythonapi
)
