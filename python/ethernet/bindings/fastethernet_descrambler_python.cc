#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/ethernet/fastethernet_descrambler.h>

void bind_fastethernet_descrambler(py::module& m)
{
    using fastethernet_descrambler = ::gr::ethernet::fastethernet_descrambler;

    py::class_<fastethernet_descrambler, gr::sync_block, gr::block, gr::basic_block,
               std::shared_ptr<fastethernet_descrambler>>(m, "fastethernet_descrambler", py::dynamic_attr())
        .def(py::init(&fastethernet_descrambler::make),
             py::arg("search_window") = 50,
             py::arg("idle_run") = 40,
             py::arg("max_idle_no_idle") = 100,
             py::arg("max_in_frame_no_idle") = 20000,
             py::arg("print_debug") = false,
             "Creates a Fast Ethernet descrambler with auto-resync");
}
