#!/usr/bin/env python
# vim: set fileencoding=UTF-8 filetype=python :
r"""
Common tools for logging and error handling

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

_dbgLevel = 0


### classes

# TODO: make it an environment class
_name = None
def set_name(name): 
    global _name
    _name = name

def get_name(): return _name

def set_debug_lvl(lvl):
    global _dbgLevel
    _dbgLevel = lvl


### functions
def debug(*args):
    """
    """
    if _dbgLevel > 0: print('Debug: {0}:'.format(_name),  *args)

def dbglvl(lvl, *args):
    """
    """
    if lvl <= _dbgLevel: print('Debug {0}:'.format(lvl), _name, *args)

def error(msg='Unknown problem.'):
    """
    """
    print('Error: {0}'.format(msg))
    sys.exit(-1)

