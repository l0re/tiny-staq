

"""
This module contains various regression tests designed to make sure
channel decoding functions are working correctly.
"""

import pycodes
from pycodes.utils import channels, CodeMaker, testing
from math import exp

(N,K) = (1200,600)
L = CodeMaker.make_H_gallager(N,3,6) 
E = (N-K)*6 # number of edges in the code graph

testing.RequireSuccessfulDecoding(
    'Small BEC test decoded with SumProductBP',
    N,K,E,L,'erasures',0.35,channels.BECOutput,
    numTrials=50,maxIter=20,algorithm='SumProductBP',requiredSuccessRate=0.8)

testing.RequireSuccessfulDecoding(
    'Small BEC test decoded with LRSumProductBP',
    N,K,E,L,'erasures',0.35,
    lambda x,y: map(lambda z: exp(min(z,10)),channels.BECOutput(x,y)),
    numTrials=50,maxIter=20,algorithm='LRSumProductBP',
    requiredSuccessRate=0.8)
