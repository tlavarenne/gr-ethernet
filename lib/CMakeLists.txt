add_library(gnuradio-ethernet SHARED
    slicer3_impl.cc
    mlt3_to_scrambled_impl.cc
    mlt3_to_scrambled_impl.cc
    mlt3_to_scrambled_impl.cc
    fastethernet_descrambler_impl.cc
    ethernet_10baset_decoder_impl.cc
    fastethernet_frame_decoder_impl.cc
)

target_link_libraries(gnuradio-ethernet PUBLIC
    gnuradio::gnuradio-runtime
    gnuradio::gnuradio-blocks
    Boost::boost
)

target_include_directories(gnuradio-ethernet
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

install(TARGETS gnuradio-ethernet
    LIBRARY DESTINATION ${GR_LIBRARY_DIR}
)
