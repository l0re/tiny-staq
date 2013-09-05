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
This file contains functions useful in writing various test scripts.
Of particular interest are the functions TestLDPCCodeOnBSC,
LDPCBruteForceApp, LDGWBruteForceApp.
"""

import sys

import pycodes, pycodes.pyLDPC
from pycodes.utils.channels import *
from pycodes.utils.converters import *
from pycodes.utils.encoders import EncodeFromLinkArray


def CountBits(c):
    "Count the number of bits in the 32-bit integer c."
    count = 0
    mask = 1
    for i in range(32):
        count = count + ((mask & c) != 0)
        mask = mask << 1
    return count

def parity(c):
    "Compute the parity of the 32-bit integer c."
    return CountBits(c) % 2

def binVecToInt(v):
    result = 0
    mask = 1
    for i in range(len(v)):
        result = result + v[i]*mask
        mask = mask << 1
    return result

def CountOnes(someList):
    return reduce(lambda x,y: x+y, someList)

def IsCodeword(possibleCW,H):
    for row in H:
        if ( CountOnes(map(lambda x,y: x*y,possibleCW,row)) % 2 ):
            return 0
    return 1

def IncrementList(someList):
    N = len(someList)
    i = 0
    while(i < N):
        if (someList[i]):
            someList[i] = 0 #carry the one
            i = i + 1
        else:
            someList[i] = 1
            return 1
    # one carried out the end so return false to say that we had overflow
    return 0

def LDPCBruteForceApp(H,p,c):
    """
    LDPCBruteForceApp(H,p,c):
    Compute the APPs for an arbitrary received codeword, c, for a code with
    parity check matrix H, on a BSC(p).  
    """

    N = len(H[0])
    K = N-len(H)
    probsOne = [0]*N
    probsZero = [0]*N
    possibleCW = [0]*N
    keepGoing = 1
    totalCodewords = 0

    while (keepGoing):
        if (IsCodeword(possibleCW,H)):
            totalCodewords += 1
            flips = CountOnes(map(lambda x,y: x^y,c,possibleCW))
            for i in range(N):
                if possibleCW[i]:
                    probsOne[i] += p**flips * (1-p)**(N-flips)
                else:
                    probsZero[i] += p**flips * (1-p)**(N-flips)
        keepGoing = IncrementList(possibleCW)

    return map(lambda o,z: o/(o+z), probsOne,probsZero)


def LDGWBruteForceApp(N,K,G,p,c):
    """
    LDGWBruteForceApp(N,K,G,p,c):
    Compute the APPs for an arbitrary received codeword, c, for a code with
    generator matrix G, on a BSC(p).  
    """

    probsOne = [0]*K
    probsZero = [0]*K
    infoBits = [0]*K
    keepGoing = 1

    while (keepGoing):
        possibleCW = EncodeFromLinkArray(infoBits,N,G)
        flips = CountOnes(map(lambda x,y: x^y,c,possibleCW))
        for i in range(K):
            if infoBits[i]:
                probsOne[i] += p**flips * (1-p)**(N-flips)
            else:
                probsZero[i] += p**flips * (1-p)**(N-flips)
        keepGoing = IncrementList(infoBits)

    return map(lambda o,z: o/(o+z), probsOne,probsZero)


def TestLDPCCodeOnBSC(title, N,K,M,linkArray,p,numIter,
                      verbose=0,evType='l',alg='default'):
    """
    TestLDPCCodeOnBSC(title, N,K,M,linkArray,p,numIter,
                      verbose=0,evType='l',alg='default'):
                      
    N:          Block length of test code.
    K:          Dimension of test code.
    M:          Number of constraints in test code (this may differ from
                K if the code description has redundant checks).
    linkArray:  A link array describing the code graph.
    p:          Flip probability on a binary symmetric channel.
    numIter:    Number of iterations to perform.
    verbose:    If true, diagnostic messages are printed.
    evType:     This can be either 'l' if the evidence should be
                set in log-likehood-ratio format or 'r' if it should
                be set in likelihood-ratio format.
    alg:        Algorithm to use for decoding.
    
    This function tests LDPCCode decoding by creating a code,
    transmitting the all zero codeword with probability p of getting a
    flip, and doing numIter iterations of the sum-product algorithm.

    If the resulting APPs are within 1e-5 of the true APPs,
    then a message saying that the test passed is printed.
    Otherwise, an Exception is raised.
    """

    if (verbose):
        import random
        seed = (1, (5879, 27432, 296), None)
        random.setstate(seed)
        print 'using seed', random.getstate()

    
    
    numEdges = CountEdgesInLinkArray(linkArray)
    code = pycodes.pyLDPC.LDPCCode(N,K,numEdges,linkArray,M)
    c = [0]*N
    evidence = BSC(c,p)
    if (verbose):
        print 'Decoding codeword', c
    if ('r' == evType):
        formattedEvidence = map(lambda x: exp(x), evidence)
    elif ('l' == evType):
        formattedEvidence = evidence
    else: 
        raise Exception, 'Illegal value for evType, must be l or r.'
    
    code.setevidence(formattedEvidence,alg=alg,verbose=verbose)
    for i in range(numIter):
        code.decode(verbose=verbose)

    trueAPP = LDPCBruteForceApp(LinkArrayToMatrix(N,linkArray),p,c)
    bpAPP = code.getbeliefs()
    maxDiff = reduce(max,map(lambda x,y: abs(x-y),trueAPP,bpAPP))

    if (maxDiff > 1e-5):
        msg = (title + ': iterative decoding does not match true APP.\n' 
               + 'True APPs =                ' + `trueAPP` + '\n'
               + 'APPs from sum-product BP = ' + `bpAPP` + '\n')
        raise Exception, msg
    else:
        print title + ' decoded with ' + alg + ': test passed.'


def RequireSuccessfulDecoding(name,N,K,E,L,noiseType,noiseLevel,
                              noiseFunc,numTrials=100,maxIter=20,
                              algorithm='default',requiredSuccessRate=0.95,
                              verbose=0):
    """
    RequireSuccesfulDecoding(name,N,K,E,L,noiseType,noiseLevel,
                             noiseFunc,numTrials,maxIter,
                             algorithm='default',requiredSuccessRate=0.95):
                      
        N,K,E,L:    The number of variables, dimension, number of edges,
                    and link array used to create an LDPCCode using the
                    pyLDPC.LDPCCode function.
        name:       Name for the test.
        noiseType:  Name for the noise type (e.g. bit flips, erasures, etc.).
        noiseLevel: A list parameters representing the noise level.
        noiseFunc:  A noise function which takes as input a list representing
                    the original codeword as the first argument and
                    noiseLevel as the second argument and returns a list
                    of representing the appropriate channel evidence.
        numTrials:  A list indicating how many trials to preform.
        maxIter:    Max number of iterations to preform.
        algorithm:  Optional name of the algorithm to use in decoding.
                    The default is 'default'.
        requiredSuccessRate: An optional argument specifying the success
                             rate required (defualt is 0.95).

        This function runs the given test and raises an exception
        of the required success rate is not met.
    """

    sys.stdout.write('Doing test "' + name + '": ')
    sys.stdout.flush()

    code = pycodes.pyLDPC.LDPCCode(N,K,E,L)
    c = [0]*N
    numSuccesses = 0

    for i in range(numTrials):
        ev = noiseFunc(c,noiseLevel)

        code.setevidence(ev,alg=algorithm,verbose=verbose)
        for i in range(maxIter):
            code.decode(verbose=verbose)

        dec = map(lambda x: x >= 0.5, code.getbeliefs(verbose=verbose))

        if (dec == c):
            numSuccesses = numSuccesses + 1

    sucRate = float(numSuccesses)/float(numTrials)
    if (sucRate < requiredSuccessRate):
        raise Exception, ('Test "' + name + '" failed.  Required ' +
                          `requiredSuccessRate` + ' successes but got only '
                          + `sucRate` + '.')
    else:
        sys.stdout.write(' test passed.\n')
        sys.stdout.flush()


def DualBruteForceNearestCodeword(N,K,L,w):
    """
    BruteForceNearestCodeword(N,K,L,w):
    
    Find nearest codeword to w in dual code described by the link array L.
    """
    assert N == len(w)

    minDiffs = N + 1
    for i in range(1<<K):
        infoBits = map(lambda x,y: ((1<<x) & y) != 0, range(K),[i]*K)
        cw = EncodeFromLinkArray(infoBits,N,L)
        diffs = reduce(lambda a,b: a+b, map(lambda x,y: x!=y, w, cw))
        if (diffs < minDiffs):
            minDiffs = diffs
            bestInfo = infoBits
            bestCW = cw
            
    return (minDiffs,bestInfo,bestCW)
