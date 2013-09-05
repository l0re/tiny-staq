#!/usr/bin/env python
# vim: set fileencoding=UTF-8 filetype=python :
r"""
Confirmation module with Toeplitz matrix multiplication

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
from staqmath import *

from random import seed, randint

import numpy as np

### globals
n = get_name()

CO_TAGSIZE_BYTE = 8


### classes
class SqCoToeplitz(SqModule):
    """
    """
    def __init__(self, sock, in_q, out_q, master=True):
        super(SqCoToeplitz, self).__init__(sock, in_q, out_q, master) 


    def run_master(self):
        while True:
            key_id, meta, key = self.read_key()
            if key_id < 0: break
            
            # calc final tag size
            tagsize = CO_TAGSIZE_BYTE
            self.debug("CO: Tag size", tagsize)
            
            # generate toeplitz matrix
            toeplitz_np = np.array(np.random.randint(0, 2, (len(key)+tagsize)*8-1), 'u1')
            
            # send challenge to bob 
            self.sendall(struct.pack('<I', key_id) + toeplitz_np.tostring())

            # apply hash
            key_np = np.unpackbits(np.frombuffer(key, 'u1'))
            tag_np = hash_toeplitz(key_np, toeplitz_np)

            # receive tag
            data = self.recvall()
            peer_key_id = struct.unpack('<I', data[:4])[0]
            peertag_np = np.frombuffer(data[4:], 'u1')
                
            # compare tags
            if (tag_np == peertag_np).all(): confirmed = True
            else: confirmed = False
           
            # send if ok
            self.sendall(struct.pack('<IB', key_id, confirmed))

            # enqueue key if correct
            if not confirmed: 
                meta['error'] = 'Confirmation failed'
                self.debug("CO: ERROR: Confirmation failed", key_id, meta)
            else:
               meta['disclosed_bits'] += tagsize * 8
               self.write_key(key_id, meta, key)
    

    def run_slave(self):
        while True:
            key_id, meta, key = self.read_key()
            if key_id < 0: break
            
            # receive challenge 
            data = self.recvall()

            # extract data from packet
            peer_key_id = struct.unpack('<I', data[:4])[0]
            toeplitz_np = np.frombuffer(data[4:], 'u1')
            if peer_key_id != key_id: 
                self.debug("CO: ERROR: key mismatch", key_id, peer_key_id)
                raise SqError("CO: ERROR: key mismatch")
            
            # apply hash
            key_np = np.unpackbits(np.frombuffer(key, 'u1'))
            tag_np = hash_toeplitz(key_np, toeplitz_np)
            tagsize = tag_np.size / 8
            self.debug("CO: Got tag size", tagsize)
            
            # send tag and wait for ok
            self.sendall(struct.pack('<I', key_id) + tag_np.tostring())
           
            # wait for confimation
            data = self.recvall()
            peer_key_id, confirmed = struct.unpack('<IB', data)
            if peer_key_id != key_id: 
                self.debug(n, "CO: ERROR: key mismatch", key_id, peer_key_id)
                raise SqError("CO: ERROR: key mismatch")
            
            # enqueue key if corrected
            if not confirmed: 
                meta['error'] = 'Confirmation failed'
                self.debug("CO: ERROR: Confirmation failed", key_id, meta)
            else:
                meta['disclosed_bits'] += tagsize * 8
                self.write_key(key_id, meta, key)

