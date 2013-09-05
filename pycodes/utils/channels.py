"""

     Copyright 2003 Mitsubishi Electric Research Laboratories All Rights
     Reserved.  Permission to use, copy and modify this software and its
     documentation without fee for educational, research and non-profit
     purposes, is hereby granted, provided that the above copyright
     notice and the following three paragraphs appear in all copies.      

     To request permission to incorporate this software into commercial
     products contact:  Vice President of Marketing and Business         
     Development;  Mitsubishi Electric Research Laboratories (MERL), 201
     Broadway, Cambridge, MA   02139 or <license@merl.com>.        
                                                                               
     IN NO EVENT SHALL MERL BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, 
     SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST        
     PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS 
     DOCUMENTATION, EVEN IF MERL HAS BEEN ADVISED OF THE POSSIBILITY OF
     SUCH DAMAGES.

     MERL SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
     FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN
     ``AS IS'' BASIS, AND MERL HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE,
     SUPPORT, UPDATES, ENHANCEMENTS OR MODIFICATIONS.
"""

"""
The channels package contains various utilities to simulate channels.
Type print channels.__dict__.keys() for a list of provided functions.
"""

import random
from math import *

def BSCLLR(c,p):
    """
    c: A list of ones and zeros representing a codeword received over a BSC.
    p: Flip probability of the BSC.
    Returns log-likelihood ratios for c.
    """

    N = len(c)
    evidence = [0]*N
    for i in range(N):
        if (c[i]):
            evidence[i] = log(p/(1-p))
        else:
            evidence[i] = log((1-p)/p)
    return evidence


def BSC(c,p):
    """
    BSC(c,p):
    c: A list of ones and zeros representing a codeword to transmit.
    p: Cross-over probability on a binary symmetric channel.
    Flips bits of c with probability p and returns the log-likelihood
    ratios for the channel evidence correspond to the result.

    NOTE: c is modified.  Use BSCOutput if you don't want c changed.
    """

    N = len(c)
    for i in range(0,N):
        if (random.random() < p):
            c[i] = 1^c[i]
            
    evidence = BSCLLR(c,p)
    return evidence

def BEC(c,e):
    """
    BSC(c,p):
    c: A list of ones and zeros representing a codeword to transmit.
    e: Erasure probability on a binary erasure channel.
    Erases bits of c with probability e and returns the log-likelihood
    ratios for the channel evidence correspond to the result.

    NOTE: c is modified.  Use BECOutput if you don't want c changed.
    """

    N = len(c)
    for i in range(0,N):
        if (random.random() < e):
            c[i] = None

    return map(lambda x: 10000*MapZeroOneNoneToLLR(x),c)


def BSCOutput(c,p):
    """
    BSCOutput(c,p):

    c: A list of ones and zeros representing a codeword to transmit.
    p: Cross-over probability on a binary symmetric channel.
    Flips bits of c with probability p and returns the log-likelihood
    ratios for the channel evidence correspond to the result.

    NOTE: c is not modified.  See BSC if you want c to be modified.
    """

    i = list(c)
    return BSC(i,p)

def BECOutput(c,e):
    """
    BSCOutput(c,p):

    c: A list of ones and zeros representing a codeword to transmit.
    e: Erasure probability on a binary erasure channel.

    Erases bits of c with probability e and returns the log-likelihood
    ratios for the channel evidence correspond to the result.

    NOTE: c is not modified, if you want it modified see BEC.
    """

    i = list(c)
    return BEC(i,e)


def FlipExactlyF(c,f):
    "Flip exactly f bits of c."

    N = len(c)
    flips = [-1]*f
    for i in range(f):
        r = -1
        while(flips.count(r)):
            r = random.randrange(0,N)
        flips[i] = r
        c[r] = c[r]^1

def EraseExactlyE(c,e):
    "Erase exactly e bits of c by setting them to None."
    N = len(c)
    for i in range(e):
        r = random.randrange(0,N)
        while(c[r] == None):
            r = random.randrange(0,N)
        c[r] = None

def MapZeroOneNoneToLLR(x):
    if (None == x):
        return 0
    elif (1 == x):
        return -1
    elif (0 == x):
        return 1
    else:
        raise Exception, 'Unexpected input in MapZeroOneNoneToLLR.'

def GetRandomBinaryCodeword(N):
    return map(lambda x: random.randrange(2), range(N))

def GetLLRsForErasureChannel(c):
    ev = map(MapZeroOneNoneToLLR,c)
    return ev
    

