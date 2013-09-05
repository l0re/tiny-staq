
import random
from pycodes.utils import CodeMaker
from pycodes.utils.channel_coding_perf import *
from pycodes.utils.channels import BSCOutput

seed = random.getstate()
random.setstate(seed)
print 'using seed',seed

col_w = 3
row_w = 6
N = 1002
K = N*col_w/row_w
E = (N-K)*row_w
L = CodeMaker.make_H_gallager(N,col_w,row_w)

noiseDensity = [0.08, 0.06, 0.04]
numTrials = [250]*len(noiseDensity)
maxIter = 20

tester = Tester([CodeParams(N,K,E,L,'small test','bit flips',noiseDensity,
                            BSCOutput,numTrials,maxIter,algorithm='default')])
tester.ReportStats(title='Small BSC test for code of length %i' % N,
                   reportFile='/tmp/small_BSC_test_results.py',
                   imgFile='/tmp/small_BSC_test_results.eps')

