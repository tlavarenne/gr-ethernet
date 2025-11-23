#ifndef INCLUDED_ETHERNET_ETHERNET_10BASET_DECODER_H
#define INCLUDED_ETHERNET_ETHERNET_10BASET_DECODER_H

#include <gnuradio/ethernet/api.h>
#include <gnuradio/sync_block.h>
#include <string>

namespace gr {
namespace ethernet {

class ETHERNET_API ethernet_10baset_decoder : virtual public gr::sync_block
{
public:
    typedef std::shared_ptr<ethernet_10baset_decoder> sptr;
    
    static sptr make(const std::string& tag_name = "packet");
};

} // namespace ethernet
} // namespace gr

#endif
