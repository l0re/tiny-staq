
"""
This script tests the BSC quantizer on a small code.
"""

import random
from pycodes.utils.CodeMaker import *
from pycodes.utils.bsc_quant_perf import *

seed = '(unknown)'
seed = (1, (12597, 22473, 295), None)
seed = random.getstate()
random.setstate(seed)
print 'Using seed', seed

numTimes = 200
numIter = 1000
N=3000
K=N/2

#leftDegrees = {2:0.33241, 3:.24632, 4:.11014, 6:0.31112}
#rightDegrees = {6:.76611, 7:.23389}
#L = MakeIrregularLDPCCodeFromLambdaRho(N,K,leftDegrees,rightDegrees)

L = make_H_gallager(N,3,6)

E=reduce(lambda x,y: x+y, map(len,L))

tester = BSCTester([CodeParams(N,K,E,L,'small test',numTimes,numIter)],
                   verbose=0)
tester.ReportStats(title='BSC quantization test results for length %i code:\n'
                   % N,reportFile='small_BSC_test.py',
                   imgFile = 'small_BSC_test.eps')

