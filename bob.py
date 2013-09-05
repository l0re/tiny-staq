#!/usr/bin/env python
# vim: set fileencoding=UTF-8 filetype=python :
r"""
Bob

Responder peer for QKD stack.

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
import socket
import struct
import argparse

from time import sleep
from multiprocessing import Process, Queue

from common import *
from ll import *

import numpy as np

### globals
set_name('Bob')
n = get_name()


### load staq modules
from ec_ldpc import *
from co_toeplitz import *
from pa_toeplitz import *
from ks_stats import *


### main
def parseargs():
    """ Parse the commandline arguments
    """
    parser = argparse.ArgumentParser(description=n)
    parser.add_argument('-b', '--blocks', 
                        type=int, default=0, 
                        help='Number of blocks to read from file.')
    parser.add_argument('-ec', '--err-corr', 
                        type=int, default=1,
                        help='Select error correction.')
    parser.add_argument('-co', '--confirm',
                        type=int, default=1,
                        help='Select confirmation.')
    parser.add_argument('-pa', '--priv-amp',
                        type=int, default=1,
                        help='Select privacy amplification.')
    parser.add_argument('-v', '--verbose', nargs='?', 
                        type=int, default=0, const=1, 
                        help='Verbosity of debug messages.')
    return parser.parse_args()


if __name__ == '__main__':

    # init
    HOST = ''
    PORT = SLAVE_PORT
    args = parseargs()
    set_debug_lvl(args.verbose)
    debug("Arguments", args)
    block_limit = args.blocks
    
    # setup processing chain
    modules = []
    if args.err_corr == 0: modules.append(SqModule)
    else:  modules.append(SqEcLdpc)
    
    if args.confirm == 0: modules.append(SqModule)
    else: modules.append(SqCoToeplitz)
    
    if args.priv_amp == 0: modules.append(SqModule)
    else: modules.append(SqPaToeplitz)
    modules.append(SqKsStats)
    
    # accept connections
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # FIXME: why?
    srv.bind((HOST, PORT))
    srv.listen(5)
    s = [srv.accept()[0]] # cntrl connection
    for mod in modules: s.append(srv.accept()[0])
    srv.close()
   
    # queues and modules
    q = [Queue(QUEUE_SIZE)]
    m = []
    for i in xrange(len(modules)): 
        q.append(Queue(QUEUE_SIZE))
        m.append(modules[i](s[i+1], q[i], q[i+1], master=False))
        m[i].set_debug_level(args.verbose)
    m[0].set_first_module()

    # start processes
    for mod in m: mod.start()
    debug("Everybody is connected and running.")
    
    # open file
    f = open(rk_fname(RK_ERROR_BOB), "rb")
    
    # start main loop, TODO: change to exception exit
    main_loop(q, s, m, f, block_limit)

    # fade out
    fadeout(q, s, m)

    # exit
    f.close()
    sys.exit(0)

