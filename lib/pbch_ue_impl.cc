/* -*- c++ -*- */
/* 
 * Copyright 2014 <+YOU OR YOUR COMPANY+>.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "pbch_ue_impl.h"
#include "pbch_ue_wrapper.h"

namespace gr {
  namespace liblte {

    pbch_ue::sptr
    pbch_ue::make()
    {
      return gnuradio::get_initial_sptr
        (new pbch_ue_impl());
    }

    /*
     * The private constructor
     */
    pbch_ue_impl::pbch_ue_impl()
      : gr::block("pbch_ue",
              gr::io_signature::make(1, 1, sizeof(gr_complex)),
              gr::io_signature::make(0, 0, 0))
    {
      wrapper = new pbch_ue_wrapper();
    }

    /*
     * Our virtual destructor.
     */
    pbch_ue_impl::~pbch_ue_impl()
    {
      delete wrapper;
    }

    int
    pbch_ue_impl::general_work (int noutput_items,
                       gr_vector_int &ninput_items,
                       gr_vector_const_void_star &input_items,
                       gr_vector_void_star &output_items)
    {
        const gr_complex *in = (const gr_complex *) input_items[0];

        // Do <+signal processing+>
        wrapper->process((cf_t*)in, ninput_items[0]);

        // Tell runtime system how many output items we produced.
        return noutput_items;
    }

  } /* namespace liblte */
} /* namespace gr */

