
"""
This test creates a dual of a small regular Gallager code,
generates random binary words, erases a fraction of the bits in the
word and tries to match the unerased positions with a codeword.
"""

import random
from pycodes.utils import CodeMaker
from pycodes.utils.bec_quant_perf import *

#random.setstate((1, (25685, 5259, 296), None))
seed = random.getstate()
print 'Using seed ', seed

col_w = 3
row_w = 6
N=3600
K=N*col_w/row_w
E=(N-K)*row_w
L=CodeMaker.make_H_gallager(N,col_w,row_w)

numSteps = 6
numToErase = range(round((N-K)*1.14),round(K*1.165),round(.025*K/numSteps))
numTimes = [200]*len(numToErase)

tester = BECTester([CodeParams(N,K,E,L,'small test',numToErase,numTimes)])

tester.ReportStats(title='Test results for small length %i code w/seed %s: \n'
                   % (N,`seed`), reportFile='/tmp/small_test_results.py',
                   imgFile='/tmp/small_test_results.eps')

