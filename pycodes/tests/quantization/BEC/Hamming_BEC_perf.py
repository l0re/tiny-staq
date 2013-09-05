
"""
This script tests the BEC quantizer on the dual of a Hamming code.
The features that make this test interesting are that the code is
small enough to visualize or decode with pencil and paper, yet still
contains cycles.  
"""

from pycodes.utils.bec_quant_perf import *

tester = BECTester([CodeParams(N=7,K=4,E=12,L=[[0,1,2,4],[1,2,3,5],[2,3,4,6]],
                              name='Hamming BEC Test',
                              numToErase=[0,1,2,3,4,5,6],
                              numTrials=[100]*7)])

tester.ReportStats(title='Dual of (7,4) Hamming Code Test',
                   reportFile='/tmp/Hamming_BEC_perf.txt',
                   imgFile='/tmp/Hamming_BEC_perf.eps')

