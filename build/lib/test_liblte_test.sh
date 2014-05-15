#!/bin/sh
export GR_DONT_LOAD_PREFS=1
export srcdir=/home/suttonpd/code/checkout/github/suttonpd/gr-liblte/lib
export PATH=/home/suttonpd/code/checkout/github/suttonpd/gr-liblte/build/lib:$PATH
export LD_LIBRARY_PATH=/home/suttonpd/code/checkout/github/suttonpd/gr-liblte/build/lib:$LD_LIBRARY_PATH
export PYTHONPATH=$PYTHONPATH
test-liblte 
