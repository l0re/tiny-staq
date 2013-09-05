#!/usr/bin/env python
# vim: set fileencoding=UTF-8 filetype=python :
r"""
Toeplitz multiplication based on fft

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

import numpy as np
import scipy.signal as scsi


### functions
def hash_toeplitz(key_vec, toeplitz_vec):
    return np.array(scsi.fftconvolve(toeplitz_vec, key_vec, mode='valid')%2, 'u1')

