/* -*- c++ -*- */
/*
 * Copyright 2025 Thomas Lavarenne.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_ETHERNET_MLT3_TO_SCRAMBLED_H
#define INCLUDED_ETHERNET_MLT3_TO_SCRAMBLED_H

#include <gnuradio/ethernet/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
namespace ethernet {

/*!
 * \brief <+description of block+>
 * \ingroup ethernet
 *
 */
class ETHERNET_API mlt3_to_scrambled : virtual public gr::sync_block {
public:
  typedef std::shared_ptr<mlt3_to_scrambled> sptr;

  /*!
   * \brief Return a shared_ptr to a new instance of
   * ethernet::mlt3_to_scrambled.
   *
   * To avoid accidental use of raw pointers, ethernet::mlt3_to_scrambled's
   * constructor is in a private implementation
   * class. ethernet::mlt3_to_scrambled::make is the public interface for
   * creating new instances.
   */
  static sptr make();
};

} // namespace ethernet
} // namespace gr

#endif /* INCLUDED_ETHERNET_MLT3_TO_SCRAMBLED_H */
