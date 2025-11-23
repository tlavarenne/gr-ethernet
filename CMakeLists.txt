cmake_minimum_required(VERSION 3.8)
project(gr-ethernet CXX C)

set(CMAKE_CXX_STANDARD 17)

find_package(Gnuradio "3.10" REQUIRED COMPONENTS blocks)
find_package(Boost REQUIRED COMPONENTS system)

include(GNUInstallDirs)
set(GR_INCLUDE_DIR ${CMAKE_INSTALL_INCLUDEDIR})
set(GR_LIBRARY_DIR ${CMAKE_INSTALL_LIBDIR})
set(GRC_BLOCKS_DIR ${CMAKE_INSTALL_DATAROOTDIR}/gnuradio/grc/blocks)

add_subdirectory(include/gnuradio/ethernet)
add_subdirectory(lib)
add_subdirectory(grc)
add_subdirectory(python/ethernet)

message(STATUS "Building gr-ethernet for GNU Radio ${Gnuradio_VERSION}")
