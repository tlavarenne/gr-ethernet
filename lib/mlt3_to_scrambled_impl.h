#ifndef INCLUDED_ETHERNET_MLT3_TO_SCRAMBLED_IMPL_H
#define INCLUDED_ETHERNET_MLT3_TO_SCRAMBLED_IMPL_H

#include <gnuradio/ethernet/mlt3_to_scrambled.h>

namespace gr {
namespace ethernet {

class mlt3_to_scrambled_impl : public mlt3_to_scrambled
{
private:
    float d_prev;

public:
    mlt3_to_scrambled_impl();
    ~mlt3_to_scrambled_impl();
    
    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) override;
};

} // namespace ethernet
} // namespace gr

#endif
