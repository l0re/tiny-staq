#!/usr/bin/env python
# vim: set fileencoding=UTF-8 filetype=python :
r"""
Error correction module with LDPC

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
from ldpc import Ldpc

import numpy as np

### globals
n = get_name()
code_db = code_db_1


### classes
class SqEcLdpc(SqModule):
    """
    """
    def __init__(self, sock, in_q, out_q, master=True):
        super(SqEcLdpc, self).__init__(sock, in_q, out_q, master) 

        # setup codes
        self._code_db_dir = code_db_dir
        self._code_db = code_db
       
        # init private vars
        self._code = []
        self._code_err = []
        for code in self._code_db:
            c = Ldpc(code[0], code_db_dir+code[1])
            self._code.append(c)
            self._code_err.append(code[0])
        self._err_bl = [0.06] * 10


    def run_master(self):
        while True:
            key_id, meta, key = self.read_key()
            if key_id < 0: break
           
            # state machine for code selection
            code_idx, ev_err = self.get_code_idx()

            # generate parities, Ldpc requires unpacked keys
            parity = self._code[code_idx].encode(np.unpackbits(np.fromstring(key, 'u1')))

            # send to bob and wait for error rate
            t = time()
            self.sendall(struct.pack('<III', key_id, code_idx, int(ev_err*1e6)) + parity.tostring())
            data = self.recvall()
            peer_key_id, error_rate = struct.unpack('<II', data)
            error_rate /= 1e6
            self.debug('Peer needed {} s to decode block.', time()-t)
            self.debug('Received error rate from Bob', error_rate)

            # update stats for code selection
            self.update_stats(error_rate)

            # enqueue key if corrected
            if error_rate < 1:
                meta['error_rate'] = error_rate
                meta['disclosed_bits'] = parity.size
                self.write_key(key_id, meta, key)


    def run_slave(self):
        while True:
            key_id, meta, key = self.read_key()
            if key_id < 0: break
            
            # received msg
            data = self.recvall()
            
            # extract data from packet
            peer_key_id, code_idx, err_mean = struct.unpack('<III', data[:12])
            err_mean /= 1e6
            if peer_key_id != key_id: raise SqError("EC: ERROR key mismatch")
            parity = np.fromstring(data[12:], 'u1')

            # decode
            self.debug("Selected code {}. Mean error is {}".format(code_idx, err_mean))
            error_estimate = err_mean
            cw = np.append(parity, np.unpackbits(np.fromstring(key, 'u1')))
            result, num_rounds, bad_checks = self._code[code_idx].decode(cw, error_estimate)
            debug("Codeword found after {} rounds.".format(num_rounds))
            if bad_checks == 0:
                error_rate = float((cw-result).sum())/(cw.size-parity.size)
                error_rate = int(error_rate*1e6) / 1e6 # truncate float
            else:
                error_rate = 1
            key_corr = np.packbits(result[parity.size:]).tostring()
            
            # send rate to alice
            self.sendall(struct.pack('<II', key_id, int(error_rate*1e6)))
            
            # enqueue key if corrected
            if error_rate < 1:
                meta['error_rate'] = error_rate
                meta['disclosed_bits'] = parity.size
                self.write_key(key_id, meta, key_corr)


    def get_code_idx(self):
            err_mean = float(reduce(lambda x, y: x+y, self._err_bl))/len(self._err_bl) 
            for code_idx in xrange(len(self._code_err)):
                if self._code_err[code_idx] > err_mean: break
            self.debug("Code {} for {}. Mean is {}.".format(code_idx, self._code_err[code_idx], err_mean))
            return code_idx, err_mean


    def update_stats(self, error_rate):
            # store error rate backlog
            self._err_bl.insert(0, error_rate)
            self._err_bl.pop()

