/* -*- c++ -*- */

#define LIBLTE_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "liblte_swig_doc.i"

%{
#include "liblte/pbch_ue.h"
%}


%include "liblte/pbch_ue.h"
GR_SWIG_BLOCK_MAGIC2(liblte, pbch_ue);
