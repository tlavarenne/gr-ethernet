#ifndef INCLUDED_ETHERNET_FASTETHERNET_DESCRAMBLER_IMPL_H
#define INCLUDED_ETHERNET_FASTETHERNET_DESCRAMBLER_IMPL_H

#include <gnuradio/ethernet/fastethernet_descrambler.h>
#include <vector>
#include <deque>

namespace gr {
namespace ethernet {

class fastethernet_descrambler_impl : public fastethernet_descrambler
{
private:
    int d_search_window;
    int d_idle_run;
    int d_max_idle_no_idle;
    int d_max_in_frame_no_idle;
    bool d_print_debug;
    
    bool d_synced;
    std::vector<int> d_lfsr;
    
    std::deque<int> d_search_buffer;
    std::deque<int> d_idle_check_buffer;
    std::deque<int> d_recent_bits;
    
    bool d_in_frame;
    int d_bits_since_sfd;
    int d_total_processed;
    int d_debug_count;
    int d_resync_count;
    int d_check_interval;
    
    int descramble_bit(int bit_in);
    void descramble_chunk(const std::vector<int>& bits, 
                         const std::vector<int>& initial_state,
                         std::vector<int>& out,
                         std::vector<int>& final_state);
    bool has_run_of_ones(const std::vector<int>& bits, int run_len);
    bool detect_frame_start();
    bool check_sync_health();
    bool search_initial_state();

public:
    fastethernet_descrambler_impl(int search_window,
                                  int idle_run,
                                  int max_idle_no_idle,
                                  int max_in_frame_no_idle,
                                  bool print_debug);
    ~fastethernet_descrambler_impl();
    
    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) override;
};

} // namespace ethernet
} // namespace gr

#endif
