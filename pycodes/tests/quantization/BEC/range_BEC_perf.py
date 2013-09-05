
"""
The following script does BEC quantization using regular and
irregular codes of various sizes.  
"""

import random
import pycodes
from pycodes.utils import CodeMaker
from pycodes.utils.bec_quant_perf import *

seed = '(unknown)'
seed = (1, (12597, 22473, 295), None)
random.setstate(seed)
seed = random.getstate()
print 'Using seed', seed

N=[120,1200, 12000, 120000]

col_w = [3]*len(N)
row_w = [6]*len(N)

leftDegrees = {2:0.33241, 3:.24632, 4:.11014, 6:0.31112}
rightDegrees = {6:.76611, 7:.23389}

# the following degree distribution is from Luby, Mitzenmacher et al. ISIT '98
# it doesn't seem to work...
#leftDegrees = {3:.44506, 5:.26704, 9:0.14835, 17:.07854, 33:0.04046,65:0.02055}
#rightDegrees = {7:.35282,8:.29548,19:.10225, 20:0.18321,84:0.04179,85:0.02445}

K=map(lambda n,c,r: n*c/r,N,col_w,row_w)

L_r=map(lambda n,c,r: CodeMaker.make_H_gallager(n,c,r),N,col_w,row_w)
E_r=map(lambda n,k,r: (n-k)*r,N,K,row_w)
L_i=map(lambda n,k: CodeMaker.MakeIrregularLDPCCodeFromLambdaRho
        (n,k,leftDegrees,rightDegrees),N,K)

def CountEdges(linkArray):
    return reduce(lambda x,y: x+y, map(len,linkArray))
E_i=map(CountEdges,L_i)

numSteps = [10]*len(N)

numToErase = map(lambda n,k,ns:
                 range(round((n-k)*1.025),round(k*1.175),round(.15*k/ns)),
                 N,K,numSteps)
numTimes = map(lambda x: [250]*len(x),numToErase)

cpList = (map(lambda n,k,e,l,ne,nt:
              CodeParams(n,k,e,l,'N='+`n`+' regular code',ne,nt),
              N,K,E_r,L_r,numToErase,numTimes) +
          map(lambda n,k,e,l,ne,nt:
              CodeParams(n,k,e,l,'N='+`n`+' irregular code',ne,nt),
              N,K,E_i,L_i,numToErase,numTimes))

tester = BECTester(cpList)

tester.ReportStats(title='Test results w/seed %s: \n' % (`seed`),
                   reportFile='/tmp/range_test_results.py',
                   imgFile='/tmp/range_test_results.eps')

