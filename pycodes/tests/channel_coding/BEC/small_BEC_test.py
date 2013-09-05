
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

erasureProb = [0.4, 0.375, 0.35, 0.325]
numTrials = [250]*len(erasureProb)
maxIter = 20

tester = Tester([CodeParams(N,K,E,L,'small test','erasures',erasureProb,
                            BECOutput,numTrials,maxIter,algorithm='default')])
tester.ReportStats(title='Small BEC test for code of length %i' % N,
                   reportFile='/tmp/small_BEC_test_results.py',
                   imgFile='/tmp/small_BEC_test_results.eps')

