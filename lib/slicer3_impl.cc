#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "slicer3_impl.h"
#include <gnuradio/io_signature.h>

namespace gr {
namespace ethernet {

slicer3::sptr slicer3::make(float threshold)
{
    return gnuradio::make_block_sptr<slicer3_impl>(threshold);
}

slicer3_impl::slicer3_impl(float threshold)
    : gr::sync_block("slicer3",
                     gr::io_signature::make(1, 1, sizeof(float)),
                     gr::io_signature::make(1, 1, sizeof(float))),
      d_threshold(threshold)
{
}

slicer3_impl::~slicer3_impl() {}

void slicer3_impl::set_threshold(float threshold)
{
    d_threshold = threshold;
}

float slicer3_impl::threshold() const
{
    return d_threshold;
}

int slicer3_impl::work(int noutput_items,
                       gr_vector_const_void_star& input_items,
                       gr_vector_void_star& output_items)
{
    const float* in = (const float*)input_items[0];
    float* out = (float*)output_items[0];
    
    for (int i = 0; i < noutput_items; i++) {
        if (in[i] > d_threshold) {
            out[i] = 1.0f;
        } else if (in[i] < -d_threshold) {
            out[i] = -1.0f;
        } else {
            out[i] = 0.0f;
        }
    }
    
    return noutput_items;
}

} // namespace ethernet
} // namespace gr
