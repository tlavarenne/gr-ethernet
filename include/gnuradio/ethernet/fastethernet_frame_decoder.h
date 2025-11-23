/* -*- c++ -*- */
/*
 * Copyright 2025 Thomas Lavarenne.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_ETHERNET_FASTETHERNET_FRAME_DECODER_H
#define INCLUDED_ETHERNET_FASTETHERNET_FRAME_DECODER_H

#include <gnuradio/ethernet/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace ethernet {

/*!
 * \brief <+description of block+>
 * \ingroup ethernet
 *
 */
class ETHERNET_API fastethernet_frame_decoder : virtual public gr::sync_block {
public:
  typedef std::shared_ptr<fastethernet_frame_decoder> sptr;

  /*!
   * \brief Return a shared_ptr to a new instance of
   * ethernet::fastethernet_frame_decoder.
   *
   * To avoid accidental use of raw pointers,
   * ethernet::fastethernet_frame_decoder's constructor is in a private
   * implementation class. ethernet::fastethernet_frame_decoder::make is the
   * public interface for creating new instances.
   */
  static sptr make();
};

} // namespace ethernet
} // namespace gr

#endif /* INCLUDED_ETHERNET_FASTETHERNET_FRAME_DECODER_H */
