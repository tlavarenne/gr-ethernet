#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/ethernet/mlt3_to_scrambled.h>

void bind_mlt3_to_scrambled(py::module& m)
{
    using mlt3_to_scrambled = ::gr::ethernet::mlt3_to_scrambled;

    py::class_<mlt3_to_scrambled, gr::sync_block, gr::block, gr::basic_block,
               std::shared_ptr<mlt3_to_scrambled>>(m, "mlt3_to_scrambled", py::dynamic_attr())
        .def(py::init(&mlt3_to_scrambled::make),
             "Creates an MLT-3 to scrambled bits converter");
}
