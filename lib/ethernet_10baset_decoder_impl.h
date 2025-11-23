#ifndef INCLUDED_ETHERNET_ETHERNET_10BASET_DECODER_IMPL_H
#define INCLUDED_ETHERNET_ETHERNET_10BASET_DECODER_IMPL_H

#include <gnuradio/ethernet/ethernet_10baset_decoder.h>
#include <pmt/pmt.h>
#include <vector>
#include <string>

namespace gr {
namespace ethernet {

class ethernet_10baset_decoder_impl : public ethernet_10baset_decoder
{
private:
    pmt::pmt_t d_tag_key;
    pmt::pmt_t d_out_port;
    
    std::string d_state;
    std::vector<uint8_t> d_buffer;
    
    int d_header_bytes;
    int d_header_samples;
    
    std::string decode_manchester(const std::vector<uint8_t>& samples);
    std::vector<uint8_t> extract_bytes(const std::string& bits, int start_bit, int length_bytes);
    std::string fmt_mac(const std::vector<uint8_t>& bytes);
    std::string fmt_ipv4(const std::vector<uint8_t>& bytes);
    std::string fmt_ipv6(const std::vector<uint8_t>& bytes);
    std::string ethertype_name(int val);
    std::string l4_name(int proto);
    std::string tcp_flags_str(uint8_t flags);
    std::string payload_preview(const std::vector<uint8_t>& payload_bytes, int max_bytes);
    void process_header();

public:
    ethernet_10baset_decoder_impl(const std::string& tag_name);
    ~ethernet_10baset_decoder_impl();
    
    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) override;
};

} // namespace ethernet
} // namespace gr

#endif
