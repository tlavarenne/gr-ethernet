#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "fastethernet_descrambler_impl.h"
#include <gnuradio/io_signature.h>
#include <iostream>

namespace gr {
namespace ethernet {

fastethernet_descrambler::sptr 
fastethernet_descrambler::make(int search_window,
                                int idle_run,
                                int max_idle_no_idle,
                                int max_in_frame_no_idle,
                                bool print_debug)
{
    return gnuradio::make_block_sptr<fastethernet_descrambler_impl>(
        search_window, idle_run, max_idle_no_idle, max_in_frame_no_idle, print_debug);
}

fastethernet_descrambler_impl::fastethernet_descrambler_impl(
    int search_window,
    int idle_run,
    int max_idle_no_idle,
    int max_in_frame_no_idle,
    bool print_debug)
    : gr::sync_block("fastethernet_descrambler",
                     gr::io_signature::make(1, 1, sizeof(uint8_t)),
                     gr::io_signature::make(1, 1, sizeof(uint8_t))),
      d_search_window(search_window),
      d_idle_run(idle_run),
      d_max_idle_no_idle(max_idle_no_idle),
      d_max_in_frame_no_idle(max_in_frame_no_idle),
      d_print_debug(print_debug),
      d_synced(false),
      d_lfsr(11, 0),
      d_in_frame(false),
      d_bits_since_sfd(0),
      d_total_processed(0),
      d_debug_count(0),
      d_resync_count(0),
      d_check_interval(50)
{
}

fastethernet_descrambler_impl::~fastethernet_descrambler_impl() {}

int fastethernet_descrambler_impl::descramble_bit(int bit_in)
{
    int bi = bit_in & 1;
    int ldd = d_lfsr[8] ^ d_lfsr[10];
    int bout = bi ^ ldd;
    
    d_lfsr.insert(d_lfsr.begin(), ldd);
    d_lfsr.pop_back();
    
    return bout;
}

void fastethernet_descrambler_impl::descramble_chunk(
    const std::vector<int>& bits,
    const std::vector<int>& initial_state,
    std::vector<int>& out,
    std::vector<int>& final_state)
{
    std::vector<int> lfsr = initial_state;
    out.clear();
    
    for (size_t i = 0; i < bits.size(); i++) {
        int bi = bits[i] & 1;
        int ldd = lfsr[8] ^ lfsr[10];
        int bout = bi ^ ldd;
        out.push_back(bout);
        
        lfsr.insert(lfsr.begin(), ldd);
        lfsr.pop_back();
    }
    
    final_state = lfsr;
}

bool fastethernet_descrambler_impl::has_run_of_ones(const std::vector<int>& bits, int run_len)
{
    int count = 0;
    for (size_t i = 0; i < bits.size(); i++) {
        if (bits[i] == 1) {
            count++;
            if (count >= run_len) return true;
        } else {
            count = 0;
        }
    }
    return false;
}

bool fastethernet_descrambler_impl::detect_frame_start()
{
    if (d_recent_bits.size() < 30) return false;
    
    std::vector<int> last_30(d_recent_bits.end() - 30, d_recent_bits.end());
    
    int idle_count = 0;
    for (int i = 0; i < 20; i++) {
        if (last_30[i] == 1) idle_count++;
    }
    
    if (idle_count >= 18) {
        int non_idle_count = 0;
        for (int i = 20; i < 30; i++) {
            if (last_30[i] == 0) non_idle_count++;
        }
        if (non_idle_count >= 3) return true;
    }
    
    return false;
}

bool fastethernet_descrambler_impl::check_sync_health()
{
    int max_check = d_in_frame ? d_max_in_frame_no_idle : d_max_idle_no_idle;
    
    if ((int)d_idle_check_buffer.size() < max_check) return true;
    
    std::vector<int> recent_bits(d_idle_check_buffer.end() - max_check, 
                                  d_idle_check_buffer.end());
    
    if (has_run_of_ones(recent_bits, d_idle_run)) {
        d_in_frame = false;
        d_bits_since_sfd = 0;
        return true;
    }
    
    if (!d_in_frame && detect_frame_start()) {
        d_in_frame = true;
        d_bits_since_sfd = 0;
        if (d_print_debug) {
            std::cout << "[AutoReSync] Frame start detected at position " 
                      << d_total_processed << std::endl;
        }
        return true;
    }
    
    if (d_in_frame) {
        d_bits_since_sfd++;
        if (d_bits_since_sfd > d_max_in_frame_no_idle) {
            if (d_print_debug) {
                std::cout << "[AutoReSync] Frame too long (" << d_bits_since_sfd 
                          << " bits) - lost sync" << std::endl;
            }
            return false;
        }
        return true;
    }
    
    if (d_print_debug) {
        std::cout << "\n============================================================" << std::endl;
        std::cout << "[AutoReSync] SYNC LOST!" << std::endl;
        std::cout << "[AutoReSync] No IDLE in last " << max_check << " bits" << std::endl;
        std::cout << "[AutoReSync] Position: " << d_total_processed << std::endl;
        std::cout << "============================================================\n" << std::endl;
    }
    
    return false;
}

bool fastethernet_descrambler_impl::search_initial_state()
{
    if ((int)d_search_buffer.size() < d_search_window) return false;
    
    std::vector<int> window(d_search_buffer.end() - d_search_window, 
                            d_search_buffer.end());
    
    for (int s = 0; s < 2048; s++) {
        std::vector<int> state_bits(11);
        for (int i = 0; i < 11; i++) {
            state_bits[i] = (s >> (10 - i)) & 1;
        }
        
        std::vector<int> descrambled;
        std::vector<int> final_state;
        descramble_chunk(window, state_bits, descrambled, final_state);
        
        if (has_run_of_ones(descrambled, d_idle_run)) {
            d_lfsr = final_state;
            d_synced = true;
            d_idle_check_buffer.clear();
            d_recent_bits.clear();
            d_in_frame = false;
            d_bits_since_sfd = 0;
            
            if (d_print_debug) {
                std::string resync_msg = (d_resync_count > 0) ? 
                    " (RE-SYNC #" + std::to_string(d_resync_count) + ")" : "";
                    
                std::cout << "\n============================================================" << std::endl;
                std::cout << "[AutoReSync] State found" << resync_msg << std::endl;
                std::cout << "[AutoReSync] Initial state: " << s << std::endl;
                std::cout << "[AutoReSync] Position: " << d_total_processed << " bits" << std::endl;
                std::cout << "============================================================\n" << std::endl;
            }
            
            return true;
        }
    }
    
    return false;
}

int fastethernet_descrambler_impl::work(int noutput_items,
                                         gr_vector_const_void_star& input_items,
                                         gr_vector_void_star& output_items)
{
    const uint8_t* in = (const uint8_t*)input_items[0];
    uint8_t* out = (uint8_t*)output_items[0];
    
    if (!d_synced) {
        for (int i = 0; i < noutput_items; i++) {
            int bit = in[i] & 1;
            d_search_buffer.push_back(bit);
            d_total_processed++;
            
            if ((int)d_search_buffer.size() > d_search_window + 100) {
                d_search_buffer.pop_front();
            }
            
            out[i] = in[i];
        }
        
        if ((int)d_search_buffer.size() >= d_search_window) {
            if (search_initial_state()) {
                d_resync_count++;
            }
        }
        
        return noutput_items;
    } else {
        for (int i = 0; i < noutput_items; i++) {
            int descrambled_bit = descramble_bit(in[i]);
            out[i] = descrambled_bit;
            d_total_processed++;
            
            d_idle_check_buffer.push_back(descrambled_bit);
            d_recent_bits.push_back(descrambled_bit);
            
            if ((int)d_idle_check_buffer.size() > d_max_in_frame_no_idle + 500) {
                d_idle_check_buffer.pop_front();
            }
            if ((int)d_recent_bits.size() > 50) {
                d_recent_bits.pop_front();
            }
        }
        
        if (d_total_processed % d_check_interval == 0) {
            if (!check_sync_health()) {
                d_synced = false;
                d_lfsr.assign(11, 0);
                d_search_buffer.clear();
                d_idle_check_buffer.clear();
                d_recent_bits.clear();
                d_in_frame = false;
                d_resync_count++;
            }
        }
        
        return noutput_items;
    }
}

} // namespace ethernet
} // namespace gr
