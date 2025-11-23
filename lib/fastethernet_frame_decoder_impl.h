#ifndef INCLUDED_ETHERNET_FASTETHERNET_FRAME_DECODER_IMPL_H
#define INCLUDED_ETHERNET_FASTETHERNET_FRAME_DECODER_IMPL_H

#include <gnuradio/ethernet/fastethernet_frame_decoder.h>
#include <pmt/pmt.h>
#include <deque>
#include <string>
#include <map>

namespace gr {
namespace ethernet {

class fastethernet_frame_decoder_impl : public fastethernet_frame_decoder
{
private:
    pmt::pmt_t d_out_port;
    
    std::string d_idle;
    std::string d_marqueur_debut;
    std::string d_marqueur_fin;
    std::map<std::string, std::string> d_table_5b4b;
    
    std::deque<char> d_tampon;
    std::string d_trame_courante;
    bool d_dans_une_trame;
    int d_compteur_timeout;
    int d_MAX_BITS_SANS_FIN;
    
    int d_compteur_trames;
    int d_compteur_erreurs;
    
    bool nettoyer_idle();
    std::string decode_5b_4b(const std::string& bits_5b);
    std::string echange_paquet(const std::string& bits);
    std::string binaire_vers_hexa(const std::string& bits);
    std::string tcp_flags_str(uint8_t flags);
    std::string payload_preview(const std::string& hex_data, int offset, int max_bytes);
    void send_frame_message(const std::string& hex_trame, int numero);
    void afficher_trame(const std::string& hex_trame, int numero);
    bool traiter_trame(const std::string& trame_bits);

public:
    fastethernet_frame_decoder_impl();
    ~fastethernet_frame_decoder_impl();
    
    int work(int noutput_items,
             gr_vector_const_void_star& input_items,
             gr_vector_void_star& output_items) override;
};

} // namespace ethernet
} // namespace gr

#endif
