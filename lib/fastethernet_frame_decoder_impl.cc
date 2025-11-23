#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fastethernet_frame_decoder_impl.h"
#include <gnuradio/io_signature.h>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace gr {
namespace ethernet {

fastethernet_frame_decoder::sptr fastethernet_frame_decoder::make()
{
    return gnuradio::make_block_sptr<fastethernet_frame_decoder_impl>();
}

fastethernet_frame_decoder_impl::fastethernet_frame_decoder_impl()
    : gr::sync_block("fastethernet_frame_decoder",
                     gr::io_signature::make(1, 1, sizeof(uint8_t)),
                     gr::io_signature::make(0, 0, 0)),
      d_idle("11111111111111111111"),
      d_marqueur_debut("111111100010001"),
      d_marqueur_fin("011010011111111"),
      d_dans_une_trame(false),
      d_compteur_timeout(0),
      d_MAX_BITS_SANS_FIN(30000),
      d_compteur_trames(0),
      d_compteur_erreurs(0)
{
    d_out_port = pmt::intern("decoded");
    message_port_register_out(d_out_port);
    
    d_table_5b4b["11110"] = "0000"; d_table_5b4b["01001"] = "0001";
    d_table_5b4b["10100"] = "0010"; d_table_5b4b["10101"] = "0011";
    d_table_5b4b["01010"] = "0100"; d_table_5b4b["01011"] = "0101";
    d_table_5b4b["01110"] = "0110"; d_table_5b4b["01111"] = "0111";
    d_table_5b4b["10010"] = "1000"; d_table_5b4b["10011"] = "1001";
    d_table_5b4b["10110"] = "1010"; d_table_5b4b["10111"] = "1011";
    d_table_5b4b["11010"] = "1100"; d_table_5b4b["11011"] = "1101";
    d_table_5b4b["11100"] = "1110"; d_table_5b4b["11101"] = "1111";
    d_table_5b4b["11000"] = "J";    d_table_5b4b["10001"] = "K";
    d_table_5b4b["01101"] = "T";    d_table_5b4b["00111"] = "R";
    
    d_tampon = std::deque<char>(200);
    
    std::cout << "[Frame Decoder] Initialise" << std::endl;
}

fastethernet_frame_decoder_impl::~fastethernet_frame_decoder_impl() {}

bool fastethernet_frame_decoder_impl::nettoyer_idle()
{
    std::string chaine(d_tampon.begin(), d_tampon.end());
    size_t idx = chaine.find(d_idle);
    if (idx != std::string::npos) {
        std::string reste = chaine.substr(idx + d_idle.length());
        d_tampon.clear();
        std::string prefix = "1111111111";
        for (char c : prefix) d_tampon.push_back(c);
        for (char c : reste) d_tampon.push_back(c);
        return true;
    }
    return false;
}

std::string fastethernet_frame_decoder_impl::decode_5b_4b(const std::string& bits_5b)
{
    std::string result;
    for (size_t i = 0; i + 5 <= bits_5b.length(); i += 5) {
        std::string code = bits_5b.substr(i, 5);
        auto it = d_table_5b4b.find(code);
        if (it != d_table_5b4b.end()) {
            std::string decoded = it->second;
            if (decoded != "J" && decoded != "K" && decoded != "T" && decoded != "R") {
                result += decoded;
            }
        }
    }
    return result.length() >= 8 ? result : "";
}

std::string fastethernet_frame_decoder_impl::echange_paquet(const std::string& bits)
{
    std::string result;
    for (size_t i = 0; i + 8 <= bits.length(); i += 8) {
        result += bits.substr(i + 4, 4) + bits.substr(i, 4);
    }
    return result;
}

std::string fastethernet_frame_decoder_impl::binaire_vers_hexa(const std::string& bits)
{
    std::ostringstream oss;
    for (size_t i = 0; i + 8 <= bits.length(); i += 8) {
        std::string octet = bits.substr(i, 8);
        bool valid = true;
        for (char c : octet) {
            if (c != '0' && c != '1') {
                valid = false;
                break;
            }
        }
        if (!valid) return "";
        
        int byte_val = std::stoi(octet, nullptr, 2);
        oss << std::hex << std::setw(2) << std::setfill('0') << byte_val;
    }
    return oss.str();
}

std::string fastethernet_frame_decoder_impl::tcp_flags_str(uint8_t flags)
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

std::string fastethernet_frame_decoder_impl::payload_preview(
    const std::string& hex_data, int offset, int max_bytes)
{
    try {
        if (offset >= (int)hex_data.length()) return "";
        
        std::string payload_hex = hex_data.substr(offset);
        if (payload_hex.length() < 2) return "";
        
        size_t preview_len = std::min((size_t)(max_bytes * 2), payload_hex.length());
        std::string preview = payload_hex.substr(0, preview_len);
        
        std::ostringstream oss;
        for (size_t i = 0; i < preview.length(); i += 2) {
            if (i > 0) oss << " ";
            oss << preview.substr(i, 2);
        }
        
        int payload_len = payload_hex.length() / 2;
        if (payload_len > max_bytes) {
            oss << " ... (" << payload_len << " octets total)";
        }
        
        return oss.str();
    } catch (...) {
        return "";
    }
}

void fastethernet_frame_decoder_impl::send_frame_message(const std::string& hex_trame, int numero)
{
    if (hex_trame.length() < 42) return;
    
    int frame_length = hex_trame.length() / 2;
    
    std::string dest_mac = hex_trame.substr(14, 12);
    std::string src_mac = hex_trame.substr(26, 12);
    std::string ethertype_hex = hex_trame.substr(38, 4);
    std::string data = hex_trame.substr(42);
    
    std::string dest_mac_fmt, src_mac_fmt;
    for (int i = 0; i < 12; i += 2) {
        if (i > 0) dest_mac_fmt += ":";
        dest_mac_fmt += dest_mac.substr(i, 2);
    }
    for (int i = 0; i < 12; i += 2) {
        if (i > 0) src_mac_fmt += ":";
        src_mac_fmt += src_mac.substr(i, 2);
    }
    
    int ethertype = 0;
    try {
        ethertype = std::stoi(ethertype_hex, nullptr, 16);
    } catch (...) {}
    
    std::string ethertype_name;
    switch (ethertype) {
        case 0x0800: ethertype_name = "IPv4"; break;
        case 0x0806: ethertype_name = "ARP"; break;
        case 0x86DD: ethertype_name = "IPv6"; break;
        case 0x8100: ethertype_name = "VLAN"; break;
        default: ethertype_name = "Unknown";
    }
    
    pmt::pmt_t d = pmt::make_dict();
    d = pmt::dict_add(d, pmt::intern("frame_num"), pmt::from_long(numero));
    d = pmt::dict_add(d, pmt::intern("length"), pmt::from_long(frame_length));
    d = pmt::dict_add(d, pmt::intern("frame_length"), pmt::from_long(frame_length));
    d = pmt::dict_add(d, pmt::intern("mac_dst"), pmt::intern(dest_mac_fmt));
    d = pmt::dict_add(d, pmt::intern("mac_src"), pmt::intern(src_mac_fmt));
    d = pmt::dict_add(d, pmt::intern("ethertype_outer"), pmt::from_long(ethertype));
    d = pmt::dict_add(d, pmt::intern("ethertype_outer_name"), pmt::intern(ethertype_name));
    d = pmt::dict_add(d, pmt::intern("ethertype"), pmt::from_long(ethertype));
    d = pmt::dict_add(d, pmt::intern("ethertype_name"), pmt::intern(ethertype_name));
    
    d = pmt::dict_add(d, pmt::intern("has_vlan"), pmt::from_bool(false));
    d = pmt::dict_add(d, pmt::intern("vlan_id"), pmt::from_long(-1));
    d = pmt::dict_add(d, pmt::intern("vlan_pcp"), pmt::from_long(0));
    d = pmt::dict_add(d, pmt::intern("vlan_dei"), pmt::from_long(0));
    
    int ip_version = 0;
    std::string ip_src = "";
    std::string ip_dst = "";
    int ip_ttl = -1;
    int l4_proto = -1;
    std::string l4_name = "";
    int src_port = -1;
    int dst_port = -1;
    std::string tcp_flags = "";
    int icmp_type = -1;
    int icmp_code = -1;
    std::string info = "";
    std::string payload_str = "";
    
    if (ethertype == 0x0800 && data.length() >= 40) {
        try {
            ip_version = 4;
            
            std::string ttl_hex = data.substr(16, 2);
            ip_ttl = std::stoi(ttl_hex, nullptr, 16);
            
            std::string proto_byte = data.substr(18, 2);
            int proto_val = std::stoi(proto_byte, nullptr, 16);
            std::string proto_name;
            switch (proto_val) {
                case 0x06: proto_name = "TCP"; break;
                case 0x11: proto_name = "UDP"; break;
                case 0x01: proto_name = "ICMP"; break;
                default: proto_name = "Proto " + std::to_string(proto_val);
            }
            l4_proto = proto_val;
            l4_name = proto_name;
            
            std::ostringstream oss_src, oss_dst;
            for (int i = 24; i < 32; i += 2) {
                if (i > 24) oss_src << ".";
                oss_src << std::stoi(data.substr(i, 2), nullptr, 16);
            }
            for (int i = 32; i < 40; i += 2) {
                if (i > 32) oss_dst << ".";
                oss_dst << std::stoi(data.substr(i, 2), nullptr, 16);
            }
            ip_src = oss_src.str();
            ip_dst = oss_dst.str();
            
            if ((proto_name == "TCP" || proto_name == "UDP") && data.length() >= 48) {
                src_port = std::stoi(data.substr(40, 4), nullptr, 16);
                dst_port = std::stoi(data.substr(44, 4), nullptr, 16);
                
                if (proto_name == "TCP" && data.length() >= 66) {
                    std::string flags_hex = data.substr(66, 2);
                    int flags_val = std::stoi(flags_hex, nullptr, 16);
                    tcp_flags = tcp_flags_str(flags_val);
                }
                
                info = ip_src + ":" + std::to_string(src_port) + " -> " + 
                       ip_dst + ":" + std::to_string(dst_port) + " (" + proto_name + ")";
                
                int payload_offset = 40 + 40;
                payload_str = payload_preview(data, payload_offset, 64);
                
            } else if (proto_name == "ICMP" && data.length() >= 42) {
                icmp_type = std::stoi(data.substr(40, 2), nullptr, 16);
                icmp_code = data.length() >= 44 ? std::stoi(data.substr(42, 2), nullptr, 16) : 0;
                info = "ICMP type " + std::to_string(icmp_type) + ", code " + 
                       std::to_string(icmp_code) + " " + ip_src + " -> " + ip_dst;
            } else {
                info = proto_name + " " + ip_src + " -> " + ip_dst;
            }
        } catch (...) {}
    }
    
    d = pmt::dict_add(d, pmt::intern("ip_version"), pmt::from_long(ip_version));
    d = pmt::dict_add(d, pmt::intern("ip_src"), pmt::intern(ip_src));
    d = pmt::dict_add(d, pmt::intern("ip_dst"), pmt::intern(ip_dst));
    d = pmt::dict_add(d, pmt::intern("ip_ttl"), pmt::from_long(ip_ttl));
    d = pmt::dict_add(d, pmt::intern("l4_proto"), pmt::from_long(l4_proto));
    d = pmt::dict_add(d, pmt::intern("l4_name"), pmt::intern(l4_name));
    d = pmt::dict_add(d, pmt::intern("src_port"), pmt::from_long(src_port));
    d = pmt::dict_add(d, pmt::intern("dst_port"), pmt::from_long(dst_port));
    d = pmt::dict_add(d, pmt::intern("tcp_flags"), pmt::intern(tcp_flags));
    d = pmt::dict_add(d, pmt::intern("icmp_type"), pmt::from_long(icmp_type));
    d = pmt::dict_add(d, pmt::intern("icmp_code"), pmt::from_long(icmp_code));
    d = pmt::dict_add(d, pmt::intern("payload_preview"), pmt::intern(payload_str));
    d = pmt::dict_add(d, pmt::intern("info"), pmt::intern(info));
    
    message_port_pub(d_out_port, d);
}

void fastethernet_frame_decoder_impl::afficher_trame(const std::string& hex_trame, int numero)
{
    if (hex_trame.length() < 42) return;
    
    std::string dest_mac = hex_trame.substr(14, 12);
    std::string src_mac = hex_trame.substr(26, 12);
    std::string ethertype = hex_trame.substr(38, 4);
    std::string data = hex_trame.substr(42);
    
    std::string dest_mac_fmt, src_mac_fmt;
    for (int i = 0; i < 12; i += 2) {
        if (i > 0) dest_mac_fmt += ":";
        dest_mac_fmt += dest_mac.substr(i, 2);
    }
    for (int i = 0; i < 12; i += 2) {
        if (i > 0) src_mac_fmt += ":";
        src_mac_fmt += src_mac.substr(i, 2);
    }
    
    std::string ethertype_name;
    if (ethertype == "0800") ethertype_name = "IPv4";
    else if (ethertype == "0806") ethertype_name = "ARP";
    else if (ethertype == "86dd") ethertype_name = "IPv6";
    else if (ethertype == "8100") ethertype_name = "VLAN";
    else ethertype_name = "Unknown";
    
    std::cout << "\n======================================================================" << std::endl;
    std::cout << "Trame #" << numero << " - " << (hex_trame.length() / 2) << " octets" << std::endl;
    std::cout << "======================================================================" << std::endl;
    std::cout << "DEST MAC:   " << dest_mac_fmt << std::endl;
    std::cout << "SRC MAC:    " << src_mac_fmt << std::endl;
    std::cout << "EtherType:  " << ethertype << " (" << ethertype_name << ")" << std::endl;
    
    if (ethertype == "0800" && data.length() >= 40) {
        try {
            int ttl = std::stoi(data.substr(16, 2), nullptr, 16);
            std::string proto_byte = data.substr(18, 2);
            std::string proto_name;
            if (proto_byte == "06") proto_name = "TCP";
            else if (proto_byte == "11") proto_name = "UDP";
            else if (proto_byte == "01") proto_name = "ICMP";
            else proto_name = proto_byte;
            
            std::ostringstream oss_src, oss_dst;
            for (int i = 24; i < 32; i += 2) {
                if (i > 24) oss_src << ".";
                oss_src << std::stoi(data.substr(i, 2), nullptr, 16);
            }
            for (int i = 32; i < 40; i += 2) {
                if (i > 32) oss_dst << ".";
                oss_dst << std::stoi(data.substr(i, 2), nullptr, 16);
            }
            
            std::cout << "Protocol:   " << proto_name << std::endl;
            std::cout << "IP Source:  " << oss_src.str() << std::endl;
            std::cout << "IP Dest:    " << oss_dst.str() << std::endl;
            std::cout << "TTL:        " << ttl << std::endl;
            
            if ((proto_name == "TCP" || proto_name == "UDP") && data.length() >= 48) {
                int sport = std::stoi(data.substr(40, 4), nullptr, 16);
                int dport = std::stoi(data.substr(44, 4), nullptr, 16);
                std::cout << "Ports:      " << sport << " -> " << dport << std::endl;
                
                if (proto_name == "TCP" && data.length() >= 66) {
                    int flags = std::stoi(data.substr(66, 2), nullptr, 16);
                    std::string flags_str = tcp_flags_str(flags);
                    if (!flags_str.empty()) {
                        std::cout << "TCP Flags:  " << flags_str << std::endl;
                    }
                }
            }
        } catch (...) {}
    }
    
    std::cout << "======================================================================" << std::endl;
}

bool fastethernet_frame_decoder_impl::traiter_trame(const std::string& trame_bits)
{
    try {
        size_t idx_debut = trame_bits.find(d_marqueur_debut);
        size_t idx_fin = trame_bits.find(d_marqueur_fin, idx_debut);
        
        if (idx_debut == std::string::npos || idx_fin == std::string::npos) return false;
        
        std::string trame_5b = trame_bits.substr(idx_debut + d_marqueur_debut.length(), 
                                                  idx_fin - idx_debut - d_marqueur_debut.length());
        if (trame_5b.length() < 5) return false;
        
        std::string trame_4b = decode_5b_4b(trame_5b);
        if (trame_4b.empty()) return false;
        
        std::string trame_echangee = echange_paquet(trame_4b);
        std::string hex_trame = binaire_vers_hexa(trame_echangee);
        
        if (hex_trame.empty() || hex_trame.length() < 42) return false;
        
        d_compteur_trames++;
        
        afficher_trame(hex_trame, d_compteur_trames);
        send_frame_message(hex_trame, d_compteur_trames);
        
        return true;
        
    } catch (...) {
        return false;
    }
}

int fastethernet_frame_decoder_impl::work(int noutput_items,
                                           gr_vector_const_void_star& input_items,
                                           gr_vector_void_star& output_items)
{
    const uint8_t* bits_descrambles = (const uint8_t*)input_items[0];
    
    for (int i = 0; i < noutput_items; i++) {
        char bit_str = (bits_descrambles[i] & 1) ? '1' : '0';
        
        if (d_tampon.size() >= 200) d_tampon.pop_front();
        d_tampon.push_back(bit_str);
        
        nettoyer_idle();
        std::string chaine_courante(d_tampon.begin(), d_tampon.end());
        
        if (!d_dans_une_trame) {
            std::string pattern_debut = d_marqueur_debut + "01011010110101101011";
            
            if (chaine_courante.find(pattern_debut) != std::string::npos) {
                d_dans_une_trame = true;
                d_compteur_timeout = 0;
                size_t pos = chaine_courante.find(d_marqueur_debut);
                d_trame_courante = chaine_courante.substr(pos);
                d_tampon.clear();
                for (char c : d_trame_courante) d_tampon.push_back(c);
            }
            
        } else {
            d_trame_courante += bit_str;
            d_compteur_timeout++;
            
            if (d_trame_courante.find(d_marqueur_fin) != std::string::npos) {
                size_t pos = d_trame_courante.find(d_marqueur_fin);
                std::string contenu = d_trame_courante.substr(0, pos);
                std::string trame_complete = contenu + d_marqueur_fin;
                std::string reste = d_trame_courante.substr(pos + d_marqueur_fin.length());
                
                if (!traiter_trame(trame_complete)) {
                    d_compteur_erreurs++;
                }
                
                d_tampon.clear();
                for (char c : reste) d_tampon.push_back(c);
                d_trame_courante.clear();
                d_dans_une_trame = false;
                d_compteur_timeout = 0;
                
            } else if (d_compteur_timeout >= d_MAX_BITS_SANS_FIN) {
                d_tampon.clear();
                d_trame_courante.clear();
                d_dans_une_trame = false;
                d_compteur_timeout = 0;
            }
        }
    }
    
    return noutput_items;
}

} // namespace ethernet
} // namespace gr
