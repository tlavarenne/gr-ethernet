#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ethernet_10baset_decoder_impl.h"
#include <gnuradio/io_signature.h>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace gr {
namespace ethernet {

ethernet_10baset_decoder::sptr ethernet_10baset_decoder::make(const std::string& tag_name)
{
    return gnuradio::make_block_sptr<ethernet_10baset_decoder_impl>(tag_name);
}

ethernet_10baset_decoder_impl::ethernet_10baset_decoder_impl(const std::string& tag_name)
    : gr::sync_block("ethernet_10baset_decoder",
                     gr::io_signature::make(1, 1, sizeof(uint8_t)),
                     gr::io_signature::make(0, 0, 0)),
      d_state("IDLE"),
      d_header_bytes(128),
      d_header_samples(128 * 8 * 2)
{
    d_tag_key = pmt::intern(tag_name);
    d_out_port = pmt::intern("decoded");
    message_port_register_out(d_out_port);
    
    std::cout << "[10BASE-T Decoder] Initialized" << std::endl;
}

ethernet_10baset_decoder_impl::~ethernet_10baset_decoder_impl() {}

std::string ethernet_10baset_decoder_impl::decode_manchester(const std::vector<uint8_t>& samples)
{
    std::string bits;
    for (size_t i = 0; i < samples.size() - 1; i += 2) {
        uint8_t a = samples[i];
        uint8_t b = samples[i + 1];
        if (a == 0 && b == 1) {
            bits += "1";
        } else if (a == 1 && b == 0) {
            bits += "0";
        }
    }
    return bits;
}

std::vector<uint8_t> ethernet_10baset_decoder_impl::extract_bytes(
    const std::string& bits, int start_bit, int length_bytes)
{
    std::vector<uint8_t> result;
    for (int i = 0; i < length_bytes; i++) {
        int bit_pos = start_bit + i * 8;
        if (bit_pos + 8 > (int)bits.length()) {
            return std::vector<uint8_t>();
        }
        std::string byte_bin = bits.substr(bit_pos, 8);
        std::reverse(byte_bin.begin(), byte_bin.end());
        try {
            uint8_t v = std::stoi(byte_bin, nullptr, 2);
            result.push_back(v);
        } catch (...) {
            return std::vector<uint8_t>();
        }
    }
    return result;
}

std::string ethernet_10baset_decoder_impl::fmt_mac(const std::vector<uint8_t>& bytes)
{
    std::ostringstream oss;
    for (size_t i = 0; i < bytes.size(); i++) {
        if (i > 0) oss << ":";
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)bytes[i];
    }
    return oss.str();
}

std::string ethernet_10baset_decoder_impl::fmt_ipv4(const std::vector<uint8_t>& bytes)
{
    std::ostringstream oss;
    for (size_t i = 0; i < bytes.size(); i++) {
        if (i > 0) oss << ".";
        oss << (int)bytes[i];
    }
    return oss.str();
}

std::string ethernet_10baset_decoder_impl::fmt_ipv6(const std::vector<uint8_t>& bytes)
{
    std::ostringstream oss;
    for (size_t i = 0; i < bytes.size(); i += 2) {
        if (i > 0) oss << ":";
        oss << std::hex << std::setw(4) << std::setfill('0') 
            << ((bytes[i] << 8) | bytes[i + 1]);
    }
    return oss.str();
}

std::string ethernet_10baset_decoder_impl::ethertype_name(int val)
{
    if (val < 0x0600) return "Length (" + std::to_string(val) + ")";
    switch (val) {
        case 0x0800: return "IPv4";
        case 0x0806: return "ARP";
        case 0x86DD: return "IPv6";
        case 0x8100: return "802.1Q VLAN";
        case 0x8847: return "MPLS unicast";
        case 0x8848: return "MPLS multicast";
        default: {
            std::ostringstream oss;
            oss << "0x" << std::hex << std::setw(4) << std::setfill('0') << val;
            return oss.str();
        }
    }
}

std::string ethernet_10baset_decoder_impl::l4_name(int proto)
{
    switch (proto) {
        case 1: return "ICMP";
        case 6: return "TCP";
        case 17: return "UDP";
        default: return "Proto " + std::to_string(proto);
    }
}

std::string ethernet_10baset_decoder_impl::tcp_flags_str(uint8_t flags)
{
    std::vector<std::string> flag_names;
    if (flags & 0x02) flag_names.push_back("SYN");
    if (flags & 0x10) flag_names.push_back("ACK");
    if (flags & 0x01) flag_names.push_back("FIN");
    if (flags & 0x04) flag_names.push_back("RST");
    if (flags & 0x08) flag_names.push_back("PSH");
    if (flags & 0x20) flag_names.push_back("URG");
    
    std::string result;
    for (size_t i = 0; i < flag_names.size(); i++) {
        if (i > 0) result += " ";
        result += flag_names[i];
    }
    return result;
}

std::string ethernet_10baset_decoder_impl::payload_preview(
    const std::vector<uint8_t>& payload_bytes, int max_bytes)
{
    if (payload_bytes.empty()) return "";
    
    std::ostringstream oss;
    size_t preview_len = std::min((size_t)max_bytes, payload_bytes.size());
    
    for (size_t i = 0; i < preview_len; i++) {
        if (i > 0) oss << " ";
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)payload_bytes[i];
    }
    
    if (payload_bytes.size() > (size_t)max_bytes) {
        oss << " ... (" << payload_bytes.size() << " octets total)";
    }
    
    return oss.str();
}

void ethernet_10baset_decoder_impl::process_header()
{
    if (d_buffer.size() < (size_t)(14 * 8 * 2)) return;
    
    std::vector<uint8_t> samples(d_buffer.begin(), 
                                  d_buffer.begin() + std::min((size_t)d_header_samples, d_buffer.size()));
    std::string bits = decode_manchester(samples);
    
    if (bits.length() < 112) return;
    
    int frame_length = bits.length() / 8;
    
    auto mac_dst_bytes = extract_bytes(bits, 0, 6);
    auto mac_src_bytes = extract_bytes(bits, 48, 6);
    auto type_bytes = extract_bytes(bits, 96, 2);
    
    if (mac_dst_bytes.empty() || mac_src_bytes.empty() || type_bytes.empty()) return;
    
    std::string mac_dst = fmt_mac(mac_dst_bytes);
    std::string mac_src = fmt_mac(mac_src_bytes);
    int ethertype = (type_bytes[0] << 8) | type_bytes[1];
    int ethertype_outer = ethertype;
    int ethertype_final = ethertype;
    std::string type_name_outer = ethertype_name(ethertype);
    
    // Affichage console
    static int frame_count = 0;
    frame_count++;
    
    std::cout << "\n======================================================================" << std::endl;
    std::cout << "Frame #" << frame_count << " - " << frame_length << " bytes" << std::endl;
    std::cout << "======================================================================" << std::endl;
    std::cout << "DEST MAC:   " << mac_dst << std::endl;
    std::cout << "SRC MAC:    " << mac_src << std::endl;
    std::cout << "EtherType:  0x" << std::hex << std::setw(4) << std::setfill('0') 
              << ethertype << " (" << type_name_outer << ")" << std::dec << std::endl;
    
    pmt::pmt_t d = pmt::make_dict();
    d = pmt::dict_add(d, pmt::intern("mac_dst"), pmt::intern(mac_dst));
    d = pmt::dict_add(d, pmt::intern("mac_src"), pmt::intern(mac_src));
    d = pmt::dict_add(d, pmt::intern("ethertype_outer"), pmt::from_long(ethertype_outer));
    d = pmt::dict_add(d, pmt::intern("ethertype_outer_name"), pmt::intern(type_name_outer));
    d = pmt::dict_add(d, pmt::intern("frame_length"), pmt::from_long(frame_length));
    
    bool has_vlan = false;
    int vlan_id = -1;
    int vlan_pcp = 0;
    int vlan_dei = 0;
    int ip_offset_bytes = 14;
    
    if (ethertype == 0x8100 && bits.length() >= (14 + 4) * 8) {
        has_vlan = true;
        auto tci_bytes = extract_bytes(bits, 112, 2);
        auto inner_type_bytes = extract_bytes(bits, 128, 2);
        if (!tci_bytes.empty() && !inner_type_bytes.empty()) {
            int tci = (tci_bytes[0] << 8) | tci_bytes[1];
            vlan_pcp = (tci >> 13) & 0x7;
            vlan_dei = (tci >> 12) & 0x1;
            vlan_id = tci & 0x0FFF;
            ethertype_final = (inner_type_bytes[0] << 8) | inner_type_bytes[1];
            ip_offset_bytes = 18;
            std::cout << "VLAN:       ID=" << vlan_id << " PCP=" << vlan_pcp << " DEI=" << vlan_dei << std::endl;
        }
    }
    
    d = pmt::dict_add(d, pmt::intern("has_vlan"), pmt::from_bool(has_vlan));
    d = pmt::dict_add(d, pmt::intern("vlan_id"), pmt::from_long(vlan_id));
    d = pmt::dict_add(d, pmt::intern("vlan_pcp"), pmt::from_long(vlan_pcp));
    d = pmt::dict_add(d, pmt::intern("vlan_dei"), pmt::from_long(vlan_dei));
    d = pmt::dict_add(d, pmt::intern("ethertype"), pmt::from_long(ethertype_final));
    d = pmt::dict_add(d, pmt::intern("ethertype_name"), pmt::intern(ethertype_name(ethertype_final)));
    
    int ip_version = 0;
    std::string ip_src = "";
    std::string ip_dst = "";
    int ip_ttl = -1;
    int l4_proto = -1;
    std::string l4_name_str = "";
    int src_port = -1;
    int dst_port = -1;
    std::string tcp_flags = "";
    int icmp_type = -1;
    int icmp_code = -1;
    std::string info = "";
    std::string payload_str = "";
    
    int ip_bit_off = ip_offset_bytes * 8;
    
    if (ethertype_final == 0x0800 && bits.length() >= (size_t)(ip_bit_off + 20 * 8)) {
        auto ip_header = extract_bytes(bits, ip_bit_off, 20);
        if (!ip_header.empty()) {
            uint8_t ver_ihl = ip_header[0];
            ip_version = ver_ihl >> 4;
            int ihl = ver_ihl & 0x0F;
            int ip_header_len = ihl * 4;
            ip_ttl = ip_header[8];
            int proto = ip_header[9];
            l4_proto = proto;
            l4_name_str = l4_name(proto);
            
            std::vector<uint8_t> src_bytes(ip_header.begin() + 12, ip_header.begin() + 16);
            std::vector<uint8_t> dst_bytes(ip_header.begin() + 16, ip_header.begin() + 20);
            ip_src = fmt_ipv4(src_bytes);
            ip_dst = fmt_ipv4(dst_bytes);
            
            std::cout << "Protocol:   " << l4_name_str << std::endl;
            std::cout << "IP Source:  " << ip_src << std::endl;
            std::cout << "IP Dest:    " << ip_dst << std::endl;
            std::cout << "TTL:        " << ip_ttl << std::endl;
            
            int l4_off_bytes = ip_offset_bytes + ip_header_len;
            int l4_off_bits = l4_off_bytes * 8;
            
            if ((proto == 6 || proto == 17) && bits.length() >= (size_t)(l4_off_bits + 4 * 8)) {
                auto ports = extract_bytes(bits, l4_off_bits, 4);
                if (!ports.empty()) {
                    src_port = (ports[0] << 8) | ports[1];
                    dst_port = (ports[2] << 8) | ports[3];
                    
                    std::cout << "Ports:      " << src_port << " -> " << dst_port << std::endl;
                    
                    if (proto == 6 && bits.length() >= (size_t)(l4_off_bits + 14 * 8)) {
                        auto tcp_hdr = extract_bytes(bits, l4_off_bits, 14);
                        if (!tcp_hdr.empty()) {
                            uint8_t flags_byte = tcp_hdr[13];
                            tcp_flags = tcp_flags_str(flags_byte);
                            if (!tcp_flags.empty()) {
                                std::cout << "TCP Flags:  " << tcp_flags << std::endl;
                            }
                        }
                    }
                    
                    info = ip_src + ":" + std::to_string(src_port) + " -> " + 
                           ip_dst + ":" + std::to_string(dst_port) + " (" + l4_name_str + ")";
                    
                    int payload_off_bytes = l4_off_bytes + 20;
                    int payload_off_bits = payload_off_bytes * 8;
                    if (bits.length() >= (size_t)(payload_off_bits + 8)) {
                        int remaining = frame_length - payload_off_bytes;
                        if (remaining > 0 && remaining < 1500) {
                            auto payload_data = extract_bytes(bits, payload_off_bits, std::min(remaining, 64));
                            if (!payload_data.empty()) {
                                payload_str = payload_preview(payload_data, 64);
                            }
                        }
                    }
                }
            } else if (proto == 1 && bits.length() >= (size_t)(l4_off_bits + 2 * 8)) {
                auto icmp_hdr = extract_bytes(bits, l4_off_bits, 2);
                if (!icmp_hdr.empty()) {
                    icmp_type = icmp_hdr[0];
                    icmp_code = icmp_hdr[1];
                    info = "ICMP type " + std::to_string(icmp_type) + ", code " + 
                           std::to_string(icmp_code) + " " + ip_src + " -> " + ip_dst;
                    std::cout << "ICMP:       Type=" << icmp_type << " Code=" << icmp_code << std::endl;
                }
            } else {
                info = "IPv4 " + l4_name_str + " " + ip_src + " -> " + ip_dst;
            }
        }
    } else if (ethertype_final == 0x86DD && bits.length() >= (size_t)(ip_bit_off + 40 * 8)) {
        auto ip6_header = extract_bytes(bits, ip_bit_off, 40);
        if (!ip6_header.empty()) {
            int ver = ip6_header[0] >> 4;
            ip_version = ver;
            int next_header = ip6_header[6];
            l4_proto = next_header;
            l4_name_str = l4_name(next_header);
            
            std::vector<uint8_t> src_bytes(ip6_header.begin() + 8, ip6_header.begin() + 24);
            std::vector<uint8_t> dst_bytes(ip6_header.begin() + 24, ip6_header.begin() + 40);
            ip_src = fmt_ipv6(src_bytes);
            ip_dst = fmt_ipv6(dst_bytes);
            
            std::cout << "Protocol:   " << l4_name_str << " (IPv6)" << std::endl;
            std::cout << "IP6 Source: " << ip_src << std::endl;
            std::cout << "IP6 Dest:   " << ip_dst << std::endl;
            
            int l4_off_bytes = ip_offset_bytes + 40;
            int l4_off_bits = l4_off_bytes * 8;
            
            if ((next_header == 6 || next_header == 17) && bits.length() >= (size_t)(l4_off_bits + 4 * 8)) {
                auto ports = extract_bytes(bits, l4_off_bits, 4);
                if (!ports.empty()) {
                    src_port = (ports[0] << 8) | ports[1];
                    dst_port = (ports[2] << 8) | ports[3];
                    info = ip_src + ":" + std::to_string(src_port) + " -> " + 
                           ip_dst + ":" + std::to_string(dst_port) + " (" + l4_name_str + ")";
                    std::cout << "Ports:      " << src_port << " -> " << dst_port << std::endl;
                }
            } else {
                info = "IPv6 " + l4_name_str + " " + ip_src + " -> " + ip_dst;
            }
        }
    } else if (ethertype_final == 0x0806) {
        info = "ARP";
        std::cout << "Protocol:   ARP" << std::endl;
    }
    
    std::cout << "======================================================================" << std::endl;
    
    d = pmt::dict_add(d, pmt::intern("ip_version"), pmt::from_long(ip_version));
    d = pmt::dict_add(d, pmt::intern("ip_src"), pmt::intern(ip_src));
    d = pmt::dict_add(d, pmt::intern("ip_dst"), pmt::intern(ip_dst));
    d = pmt::dict_add(d, pmt::intern("ip_ttl"), pmt::from_long(ip_ttl));
    d = pmt::dict_add(d, pmt::intern("l4_proto"), pmt::from_long(l4_proto));
    d = pmt::dict_add(d, pmt::intern("l4_name"), pmt::intern(l4_name_str));
    d = pmt::dict_add(d, pmt::intern("src_port"), pmt::from_long(src_port));
    d = pmt::dict_add(d, pmt::intern("dst_port"), pmt::from_long(dst_port));
    d = pmt::dict_add(d, pmt::intern("tcp_flags"), pmt::intern(tcp_flags));
    d = pmt::dict_add(d, pmt::intern("icmp_type"), pmt::from_long(icmp_type));
    d = pmt::dict_add(d, pmt::intern("icmp_code"), pmt::from_long(icmp_code));
    d = pmt::dict_add(d, pmt::intern("payload_preview"), pmt::intern(payload_str));
    d = pmt::dict_add(d, pmt::intern("info"), pmt::intern(info));
    
    message_port_pub(d_out_port, d);
}

int ethernet_10baset_decoder_impl::work(int noutput_items,
                                         gr_vector_const_void_star& input_items,
                                         gr_vector_void_star& output_items)
{
    const uint8_t* in = (const uint8_t*)input_items[0];
    
    std::vector<gr::tag_t> tags;
    get_tags_in_window(tags, 0, 0, noutput_items, d_tag_key);
    uint64_t nread = nitems_read(0);
    
    if (!tags.empty()) {
        gr::tag_t tag = tags[0];
        int local_offset = (int)(tag.offset - nread);
        d_state = "ACCUMULATING_HEADER";
        d_buffer.clear();
        
        if (local_offset >= 0 && local_offset < noutput_items) {
            for (int i = local_offset; i < noutput_items; i++) {
                d_buffer.push_back(in[i]);
            }
        }
    } else if (d_state == "ACCUMULATING_HEADER") {
        for (int i = 0; i < noutput_items; i++) {
            d_buffer.push_back(in[i]);
        }
    }
    
    if (d_state == "ACCUMULATING_HEADER" && (int)d_buffer.size() >= d_header_samples) {
        try {
            process_header();
        } catch (...) {
        }
        d_state = "IDLE";
        d_buffer.clear();
    }
    
    return noutput_items;
}

} // namespace ethernet
} // namespace gr
