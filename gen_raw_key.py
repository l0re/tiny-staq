#!/usr/bin/env python
# vim: set fileencoding=UTF-8 filetype=python :
r"""
Key statistics calculation

AUTHORS:

- Thomas Loruenser (2013): initial version

"""
###############################################################################
# Copyright 2013, Thomas Loruenser <thomas.loruenser@ait.ac.at>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
###############################################################################

from __future__ import print_function

import sys
import os
import numpy as np

from common import rk_fname
from common import ensure_dir

### globals
size_byte = 2**20
fpcoff = 1000

### functions
# generate raw key and store to file
def gen_key_file(error_rate, get_fname):
    # gen raw key
    rk = np.zeros(size_byte*8, 'u1')

    # introduce error
    rk += (np.random.randint(0, fpcoff, size_byte*8) < error_rate*fpcoff).view('u1')
    print("Generated error rate:", float(rk.sum())/rk.size, rk)

    # save to file
    frk = open(rk_fname(error_rate), 'wb')
    frk.write(np.packbits(rk).tostring())
    frk.close()


### main
if __name__ == '__main__':

    # ensure keys dir exist
    ensure_dir(rk_fname(0.0))

    # print some info
    print("File size=", size_byte)
    
    # gen key files
    for fperror in range(0, 70, 5):
        gen_key_file(float(fperror)/fpcoff, rk_fname)

