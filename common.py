#!/usr/bin/env python
# vim: set fileencoding=UTF-8 filetype=python :
r"""
Common stuff used by both peers

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

import os
import sys
import socket
import struct
from time import time
from ll import *

import Queue as Q
from multiprocessing import Process, Queue

from numpy import log2

SLAVE_HOST = 'localhost'
SLAVE_PORT = 17019
QUEUE_SIZE = 1000

RK_ERROR_ALICE = 0.00
RK_ERROR_BOB = 0.01

max_msg_len = 1024


### codes
code_db_dir = 'ldpc_codes/'
sys.path.insert(0, os.path.abspath(code_db_dir))
print(sys.path)
import peg_8k_db
code_db_1 = peg_8k_db.code_db

EC_DATA_SIZE = peg_8k_db.code_n # FIXME: currently only ldpc block size supported
IN_BLOCKSIZE = EC_DATA_SIZE # FIXME: set to ldpc code size
NOT_CORRECTED = 0xFFFFFFFF

### classes
class TimeMeas():
    """
    """
    def __init__(self):
        self._t_start = time()
        self._t_lap = []
    def res(self):
        self.__init__()
    def tot(self):
        return time() - self._t_start
        

class SqModule(Process):
    """
    """
    def __init__(self, sock, in_q, out_q, master=True):
        super(SqModule, self).__init__()
        self._debug_level = 0
        self._sock = sock
        self._in_q = in_q
        self._out_q = out_q
        self._master = master
        self._first_module = False
        if self._master: 
            self._run = self.run_master
            self._side = 'Master'
        else: 
            self._run = self.run_slave
            self._side = 'Slave'

    # run entry point
    def run(self):
        try: self._run()
        except socket.error:
            self.debug("ERROR: Socket error.")
            self._sock.close()
            if self._first_module == True: 
                self.write_key(-1, {}, b'Shutdown!')

    # main worker
    def run_master(self):
        while True:
            key_id, meta, key = self.read_key()
            self.write_key(key_id, meta, key)

    def run_slave(self):
        while True:
            key_id, meta, key = self.read_key()
            self.write_key(key_id, meta, key)

    # helper methods
    def set_first_module(self):
        self._first_module = True

    def set_debug_level(self, level):
        self._debug_level = level

    def read_key(self):
        key_id, meta, key = self._in_q.get()
        self.add_timestamp(meta)
        self.debug('Read key:', key_id, meta)
        if key_id < 0:
            self.debug('Shutting down, no more key material.')
            self.write_key(key_id, {}, key)
        return key_id, meta, key

    def write_key(self, key_id, meta, key):
        self.add_timestamp(meta)
        if self._out_q: self._out_q.put([key_id, meta, key])

    def add_timestamp(self, meta):
        if meta.has_key('timestamp'): meta['timestamp'].append(time())
        else: meta['timestamp'] = [time()]

    def sendall(self, msg):
        self._sock.sendall(struct.pack('<I', len(msg))+msg)

    def recvall(self):
        data = self._sock.recv(4)
        if len(data) != 4: raise socket.error
        msg_len = struct.unpack('<I', data[:4])[0]
        data = self._sock.recv(msg_len)
        if len(data) != msg_len: raise socket.error
        return data

    def debug(self, *args):
        if self._debug_level > 0: 
            print('Debug: {0}: {1}:'.format(self._name, self._side), *args)
    
    def dbglvl(self, lvl, *args):
        if self._debug_level >= lvl: 
            print('Debug: {0}: {1}:'.format(self._name, self._side), *args)


class SqError(Exception):
    """
    """
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

### functions
def main_loop(q, s, m, f, block_limit):
    # start main loop
    key_id = 0
    x = []
    while True:
        
        # read raw key from stream
        rk = f.read(IN_BLOCKSIZE)
        if len(rk) != IN_BLOCKSIZE: break

        # put block into queue
        sent = False
        shutdown = False
        while not sent:
            try: q[0].put([key_id, {'timestamp':[time()]}, rk], timeout=0.5)
            except: debug("Input queue is full. Timeout!")
            else: sent = True

            # check if all processes are running
            for mod in m:
                if not mod.is_alive(): shutdown = True
            if shutdown: break

        # check for shutdown requrest
        if shutdown: 
            debug("Found terminated process. Exiting.")
            break

        # inc counter 
        key_id += 1

        # check if num blocks is reached
        if block_limit and key_id >= block_limit: 
            s[1].close() # trigger Bob shutdown
            debug("Block limit is reached. Exiting.")
            break
            

def fadeout(q, s, m):
    debug('No more keys to read. Rundown chain.')
    try: q[0].put([-1, {}, b'Shutdown!'], False)
    except: pass
    
    debug("Wait for processes.")
    for mod in m: 
        if mod.is_alive(): 
            debug("Waiting for {0} to shutdown.".format(mod))
            mod.join()
        debug("Process {0} is down.".format(mod))

    debug('Closing sockets.') 
    for sock in s: 
        try: sock.shutdown(socket.SHUT_RDWR)
        except: pass
        sock.close()
        debug("Socket {0} closed.".format(sock))

    debug('Shudown queues.') 
    for queue in q:
        try: 
            while True: msg = queue.get(True, 0.2)
        except Q.Empty: pass


# generate file name
def rk_fname(error_rate): 
    return 'keys/rk_{0:.3f}'.format(error_rate)


def bin_entropy(p):
    return -p * log2(p) - (1-p) * log2(1-p)


def ensure_dir(f):
    d = os.path.dirname(f)
    if not os.path.isdir(d):
        os.makedirs(d)

