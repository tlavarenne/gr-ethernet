#ifndef INCLUDED_ETHERNET_SLICER3_IMPL_H
#define INCLUDED_ETHERNET_SLICER3_IMPL_H

#include <gnuradio/ethernet/slicer3.h>

namespace gr {
namespace ethernet {

class slicer3_impl : public slicer3
{
private:
    float d_threshold;

public:
    slicer3_impl(float threshold);
    ~slicer3_impl();
    
    void set_threshold(float threshold) override;
    float threshold() const override;
    
    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) override;
};

} // namespace ethernet
} // namespace gr

#endif
