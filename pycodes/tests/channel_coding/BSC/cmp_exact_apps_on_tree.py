
"""
This regression test decodes various tree codes via brute force
and makes sure the results correspond to the output of the
sum-product algorithm.
"""

from pycodes.utils.testing import *

p = 0.15

TestLDPCCodeOnBSC('(7,6) Single Parity Check Code',7,6,1,[range(7)],p,1)

TestLDPCCodeOnBSC('(5,3) Double Parity Check Code',5,3,2,[[0,1,2],[2,3,4]],
                  p,2)

TestLDPCCodeOnBSC('(7,4) Triple Parity Check Code',7,4,3,
                  [[0,1,2],[2,3,4],[4,5,6]],p,3)


TestLDPCCodeOnBSC('(7,6) Single Parity Check Code',7,6,1,[range(7)],p,1,
                  verbose=0,evType='r',alg='LRSumProductBP')

TestLDPCCodeOnBSC('(5,3) Double Parity Check Code',5,3,2,[[0,1,2],[2,3,4]],
                  p,2,verbose=0,evType='r',alg='LRSumProductBP')

TestLDPCCodeOnBSC('(7,4) Triple Parity Check Code',7,4,3,
                  [[0,1,2],[2,3,4],[4,5,6]],p,3,
                  verbose=0,evType='r',alg='LRSumProductBP')
