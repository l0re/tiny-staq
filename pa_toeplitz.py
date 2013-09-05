#!/usr/bin/env python
# vim: set fileencoding=UTF-8 filetype=python :
r"""
Privacy amplification module based on Toeplitz matrix

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

# local
from common import *
from ll import *
from staqmath import *

# numpy
import numpy as np
import scipy.signal as scsi

### globals 
n = get_name()
KEY_MIN_SIZE_BYTE = 1024


### classes
class SqPaToeplitz(SqModule):
    """
    """
    def __init__(self, sock, in_q, out_q, master=True):
        super(SqPaToeplitz, self).__init__(sock, in_q, out_q, master) 


    def run_master(self):
        key_buf_id = []
        key_buf_err = 0
        key_buf_dis = 0
        key_buf_meta = []
        key_buf = ''
        while True:
            key_id, meta, key = self.read_key()
            if key_id < 0: break

            # accumulate in buffer
            key_buf_id.append(key_id)
            key_buf_err += meta['error_rate']
            key_buf_dis += meta['disclosed_bits']
            key_buf_meta.append(meta)
            key_buf += key
            if len(key_buf) < KEY_MIN_SIZE_BYTE: continue

            key_id = key_buf_id[0]
            key_error = key_buf_err / len(key_buf_id)
            key_disclosed = key_buf_dis
            key = key_buf
            
            key_buf_id = []
            key_buf_err = 0
            key_buf_dis = 0
            key_buf = ''

            # calc final tag size, byte aligned
            tagsize = int((len(key) * 8.0 - key_disclosed) * bin_entropy(key_error)  / 8)
                                                                                    
            # generate toeplitz matrix                                              
            toeplitz_np = np.array(np.random.randint(0, 2, (len(key)+tagsize)*8-1),  dtype=np.uint8)
                                                                                    
            # send to bob and wait for hashtag                                      
            self.sendall(struct.pack('<I', key_id) + toeplitz_np.tostring())       

            # apply hash
            key_np = np.unpackbits(np.frombuffer(key, 'u1'))
            final_key_np = hash_toeplitz(key_np, toeplitz_np)
            final_key = np.packbits(final_key_np)
            self.debug("Final key = ", final_key, len(final_key))

            # enqueue key if correct
            self.write_key(key_id, meta, final_key_np.tostring())
           

    def run_slave(self):
        key_buf_id = []
        key_buf_err = 0
        key_buf_dis = 0
        key_buf = ''
        while True:
            key_id, meta, key = self.read_key()
            if key_id < 0: break
            
            # accumulate in buffer
            key_buf_id.append(key_id)
            key_buf_err += meta['error_rate']
            key_buf_dis += meta['disclosed_bits']
            key_buf += key
            if len(key_buf) < KEY_MIN_SIZE_BYTE: continue

            key_id = key_buf_id[0]
            key_error = key_buf_err / len(key_buf_id)
            key_disclosed = key_buf_dis
            key = key_buf
            
            key_buf_id = []
            key_buf_err = 0
            key_buf_dis = 0
            key_buf = ''
            
            # received msg
            data = self.recvall()
           
            # extract data from packet
            peer_key_id = struct.unpack('<I', data[:4])[0]
            toeplitz_np = np.frombuffer(data[4:], dtype=np.uint8)
            if peer_key_id != key_id: 
                debug("ERROR key mismatch", key_id, peer_key_id)
                raise SqError("ERROR key mismatch")
            
            # apply hash
            key_np = np.unpackbits(np.frombuffer(key, 'u1'))
            final_key_np = hash_toeplitz(key_np, toeplitz_np)
            final_key = np.packbits(final_key_np)
            self.debug("Final key = ", final_key, len(final_key))
            
            # enqueue key
            self.write_key(key_id, meta, final_key_np.tostring())
        
