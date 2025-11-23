#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/ethernet/fastethernet_frame_decoder.h>

void bind_fastethernet_frame_decoder(py::module& m)
{
    using fastethernet_frame_decoder = ::gr::ethernet::fastethernet_frame_decoder;

    py::class_<fastethernet_frame_decoder, gr::sync_block, gr::block, gr::basic_block,
               std::shared_ptr<fastethernet_frame_decoder>>(m, "fastethernet_frame_decoder", py::dynamic_attr())
        .def(py::init(&fastethernet_frame_decoder::make),
             "Creates a Fast Ethernet frame decoder (100BASE-TX)");
}
