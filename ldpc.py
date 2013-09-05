#!/usr/bin/env python
# vim: set fileencoding=UTF-8 filetype=python :
r"""
LDPC error correction module

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
import numpy as np

from common import SqError

from pycodes.pyLDPC import LDPCCode
from pycodes.utils.channels import *


### classes
class Callable:
    def __init__(self, anycallable):
        self.__call__ = anycallable 


class Ldpc():
    """
    """
    def __init__(self, error_rate, pcm_file):
        """
        """
        # init code
        self._load_code(error_rate, pcm_file) 
        self._code = LDPCCode(self._n, self._k, self._e, self._H)


    def decode(self, c, err_prob, max_iter=50):
        """
        """
        n, k, e, H = self._n, self._k, self._e, self._H
        code = self._code
        p = n-k
        if len(c) != self._n: raise SqError("Code word length error!")

        # prepare belief vector
        ev = BSCLLR(c.tolist(), err_prob)
        parity_beliefs = 1e6-1 # parity is error free
        ev[:p] = np.ones(p) * parity_beliefs
        code.setevidence(ev, alg='SumProductBP')

        # decode
        for num_rounds in xrange(max_iter):
            code.decode()
            if code.badchecks() == 0: break

        # prepare return vector
        bad_checks = code.badchecks()
        beliefs = np.array(code.getbeliefs())
        result = (beliefs > 0.5).view('u1')
        par_diff =  result[:p] - c[:p]
        
        if not (par_diff == 0).all(): 
            raise SqError("Parity changed during decoding.")
        
        return result, num_rounds+1, bad_checks


    def encode(self, d):
        """
        """
        n, k, e, H = self._n, self._k, self._e, self._H
        parity = np.zeros(n-k, 'u1')
        for i, row in zip(xrange(len(H)), H): 
            parity[i] = d[np.array(row[1:], 'u1')].sum() % 2
        return parity


    def get_code_para(self):
        return self._n, self._k, self._code_err


    # private
    def _load_code(self, error_rate, fname):
        """ 
        " load code table for error_rate
        """
        n, k, e, H = self._load_pcm(fname)
        self._n = n
        self._k = k
        self._e = e
        self._H = H
        self._code_err = error_rate


    def _load_pcm(fname):
        """
        """
        # read file, first line is header
        fcode = open(fname, 'rb')
        line = fcode.readline()
        n, p, e = np.fromstring(line, 'i', sep=' ')
        k = n - p

        # second is degree of variable nodes
        line = fcode.readline()
        vn_deg = np.fromstring(line, 'i', sep=' ')

        # third is degree of check nodes
        line = fcode.readline()
        cn_deg = np.fromstring(line, 'i', sep=' ')

        # fourth is edge dst var node address from check node
        line = fcode.readline()
        edges = np.fromstring(line, 'i', sep=' ')

        # sanity checking of redundant information
        if n != vn_deg.size: raise SqError("PCM inconsistent")
        if p != cn_deg.size: raise SqError("PCM inconsistent")
        if e != edges.size: raise SqError("PCM inconsistent")
        if e != vn_deg.sum(): raise SqError("PCM inconsistent")
        if e != cn_deg.sum(): raise SqError("PCM inconsistent")

        # lut for finding varnodes corresponding to edges
        vn_lut = np.zeros(e, 'i')
        idx = 0
        for i, deg in zip(range(n), vn_deg):
            vn_lut[idx:idx+deg] = i
            idx += deg
        if vn_lut.size != e: raise SqError("PCM inconsistent")

        # generate parity check matrix
        H = [] # must be list for LDPCCode
        idx = 0
        for deg in cn_deg:
            nx_adj = vn_lut[edges[idx:idx+deg]]
            H.append(nx_adj.tolist())
            idx += deg
        if len(H) != p: raise SqError("H inconsistent")
        if reduce(lambda x,y: x+y, map(len,H)) != e: raise SqError("PCM inconsistent")

        fcode.close()
        return n, k, e, H 
    _load_pcm = Callable(_load_pcm)

    def _manual_setev(self, c, err_prob):
        
        if len(c) != self._n: raise SqError("Code word length error!")
        
        p = self._n - self._k
        ev = BSCLLR(c.tolist(), err_prob)
        parity_beliefs = 1e6-1 # parity is error free
        ev[:p] = np.ones(p) * parity_beliefs
        self._code.setevidence(ev, alg='SumProductBP')


    def _manual_decode(self):
        self._code.decode()


    def _manual_get_beliefs(self):
        return np.array(self._code.getbeliefs())


