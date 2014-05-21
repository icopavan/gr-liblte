#!/usr/bin/env python
# -*- coding: utf-8 -*-
# 
# Copyright 2014 <+YOU OR YOUR COMPANY+>.
# 
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this software; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
# 

import os
from gnuradio import gr, gr_unittest
from gnuradio import blocks
import liblte_swig as liblte

class qa_pbch_ue (gr_unittest.TestCase):

    def setUp (self):
        self.tb = gr.top_block ()

    def tearDown (self):
        self.tb = None

    def test_001_t (self):
        src = blocks.file_source(gr.sizeof_gr_complex*1,"out.bin")
        thr = blocks.throttle(gr.sizeof_gr_complex*1, 100000)
        dst = liblte.pbch_ue()
        self.tb.connect(src, thr)
        self.tb.connect(thr, dst)
        self.tb.run()

if __name__ == '__main__':
    print 'Blocked waiting for GDB attach (pid = %d)' % (os.getpid(),)
    raw_input ('Press Enter to continue: ')
    gr_unittest.run(qa_pbch_ue, "qa_pbch_ue.xml")
