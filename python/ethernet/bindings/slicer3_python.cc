#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/ethernet/slicer3.h>

void bind_slicer3(py::module& m)
{
    using slicer3 = ::gr::ethernet::slicer3;

    py::class_<slicer3, gr::sync_block, gr::block, gr::basic_block,
               std::shared_ptr<slicer3>>(m, "slicer3", py::dynamic_attr())
        .def(py::init(&slicer3::make),
             py::arg("threshold") = 0.33f,
             "Creates a 3-level slicer block")
        .def("set_threshold", &slicer3::set_threshold, py::arg("threshold"))
        .def("threshold", &slicer3::threshold);
}
