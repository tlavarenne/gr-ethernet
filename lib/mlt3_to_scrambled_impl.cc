#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mlt3_to_scrambled_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace ethernet {

mlt3_to_scrambled::sptr mlt3_to_scrambled::make()
{
    return gnuradio::make_block_sptr<mlt3_to_scrambled_impl>();
}

mlt3_to_scrambled_impl::mlt3_to_scrambled_impl()
    : gr::sync_block("mlt3_to_scrambled",
                     gr::io_signature::make(1, 1, sizeof(float)),
                     gr::io_signature::make(1, 1, sizeof(uint8_t))),
      d_prev(0.0f)
{
}

mlt3_to_scrambled_impl::~mlt3_to_scrambled_impl() {}

int mlt3_to_scrambled_impl::work(int noutput_items,
                                  gr_vector_const_void_star& input_items,
                                  gr_vector_void_star& output_items)
{
    const float* in = (const float*)input_items[0];
    uint8_t* out = (uint8_t*)output_items[0];
    
    float prev = d_prev;
    
    for (int i = 0; i < noutput_items; i++) {
        float current = in[i];
        out[i] = (current != prev) ? 1 : 0;
        prev = current;
    }
    
    d_prev = prev;
    return noutput_items;
}

} // namespace ethernet
} // namespace gr
