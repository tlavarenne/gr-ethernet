#ifndef INCLUDED_ETHERNET_SLICER3_H
#define INCLUDED_ETHERNET_SLICER3_H

#include <gnuradio/ethernet/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace ethernet {

class ETHERNET_API slicer3 : virtual public gr::sync_block
{
public:
    typedef std::shared_ptr<slicer3> sptr;
    
    static sptr make(float threshold = 0.33f);
    
    virtual void set_threshold(float threshold) = 0;
    virtual float threshold() const = 0;
};

} // namespace ethernet
} // namespace gr

#endif
