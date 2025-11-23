#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

void bind_slicer3(py::module& m);
void bind_mlt3_to_scrambled(py::module& m);
void bind_fastethernet_descrambler(py::module& m);
void bind_ethernet_10baset_decoder(py::module& m);
void bind_fastethernet_frame_decoder(py::module& m);

PYBIND11_MODULE(ethernet_python, m)
{
    m.doc() = "Ethernet blocks for GNU Radio";
    
    bind_slicer3(m);
    bind_mlt3_to_scrambled(m);
    bind_fastethernet_descrambler(m);
    bind_ethernet_10baset_decoder(m);
    bind_fastethernet_frame_decoder(m);
}
