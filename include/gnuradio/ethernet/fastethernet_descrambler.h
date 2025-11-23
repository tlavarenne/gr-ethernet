/* -*- c++ -*- */
/*
 * Copyright 2025 Thomas Lavarenne.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_ETHERNET_FASTETHERNET_DESCRAMBLER_H
#define INCLUDED_ETHERNET_FASTETHERNET_DESCRAMBLER_H

#include <gnuradio/ethernet/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace ethernet {

/*!
 * \brief <+description of block+>
 * \ingroup ethernet
 *
 */
class ETHERNET_API fastethernet_descrambler : virtual public gr::sync_block {
public:
  typedef std::shared_ptr<fastethernet_descrambler> sptr;

  /*!
   * \brief Return a shared_ptr to a new instance of
   * ethernet::fastethernet_descrambler.
   *
   * To avoid accidental use of raw pointers,
   * ethernet::fastethernet_descrambler's constructor is in a private
   * implementation class. ethernet::fastethernet_descrambler::make is the
   * public interface for creating new instances.
   */
  static sptr make(int search_window = 50, int idle_run = 40,
                   int max_idle_no_idle = 100, int max_in_frame_no_idle = 20000,
                   bool print_debug = false);
};

} // namespace ethernet
} // namespace gr

#endif /* INCLUDED_ETHERNET_FASTETHERNET_DESCRAMBLER_H */
