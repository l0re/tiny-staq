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

from common import *
from ll import *

from time import time
import numpy as np

### globals
n = get_name()


### classes
class SqKsStats(SqModule):
    """
    """
    def __init__(self, sock, in_q, out_q, master=True):
        super(SqKsStats, self).__init__(sock, in_q, None, master) 

    def run_master(self):
        tot_key_bits = 0
        tot_zero_bits = 0
        tot_one_bits = 0
        ts_final_last = 0
        while True:
            key_id, meta, key = self.read_key()
            if key_id < 0: break
            
            self.dbglvl(3, 'KS: Final key:', key_id, meta)

            # receive key for comparison
            data = self.recvall()
            peer_key_id = struct.unpack('<I', data[:4])[0]
            peer_key = data[4:]
            
            # compare keys
            if peer_key_id != key_id: 
                self.debug(n, "ERROR: key mismatch", key_id, peer_key_id)
                raise SqError("ERROR: key mismatch")
            self.debug("Comparison: Key ids are identical.", key_id)

            if peer_key != key:
                self.debug(n, "ERROR: key mismatch id=", key, peer_key)
                raise SqError("ERROR: key mismatch")
            self.debug("Comparison: Keys are identical! Hurra! len=", len(key))

            # generate stats
            key_np = np.unpackbits(np.frombuffer(key, 'u1'))
            zero_bits = (key_np == 0).sum()
            one_bits = (key_np == 1).sum()
            
            ts = np.array(meta['timestamp'])
            self.debug("Timing information:", np.diff(ts))
            key_rate = float(key_np.size)/(ts[-1]-ts_final_last)
            if ts_final_last: self.debug("Stats: Key rate: {:.0f}".format(key_rate))
            ts_final_last = ts[-1]

            self.debug("Stats: Key len in bits:", key_np.size)
            self.debug("Stats: Key:", key_np)
            self.debug("Stats: Key num zeros bits:", zero_bits)
            self.debug("Stats: Key num ones bits :", one_bits)

            # total stats
            tot_key_bits += key_np.size
            tot_zero_bits += zero_bits
            tot_one_bits += one_bits
            self.debug("Total stats: Key len in bits:", tot_key_bits)
            self.debug("Total stats: Key num zeros bits:", tot_zero_bits)
            self.debug("Total stats: Key num ones bits :", tot_one_bits)

            # some status info for non-debug mode
            if not self._debug_level:
                print("KeyId: {}, Len: {} bits, Rate: {:.3f} bps".format(key_id, key_np.size, key_rate))


    def run_slave(self):
        while True:
            key_id, meta, key = self.read_key()
            if key_id < 0: break
           
            # info
            self.debug('KS: Final key:', key_id, meta)

            # send key to master for comparison
            self.sendall(struct.pack('<I', key_id) + key)

