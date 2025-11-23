#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/ethernet/ethernet_10baset_decoder.h>

void bind_ethernet_10baset_decoder(py::module& m)
{
    using ethernet_10baset_decoder = ::gr::ethernet::ethernet_10baset_decoder;

    py::class_<ethernet_10baset_decoder, gr::sync_block, gr::block, gr::basic_block,
               std::shared_ptr<ethernet_10baset_decoder>>(m, "ethernet_10baset_decoder", py::dynamic_attr())
        .def(py::init(&ethernet_10baset_decoder::make),
             py::arg("tag_name") = "packet",
             "Creates an Ethernet 10BASE-T decoder");
}
