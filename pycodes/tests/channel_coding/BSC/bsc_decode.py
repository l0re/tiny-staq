
"""
This file creates an LDPC code, generates a random
binary vector, puts it through a binary symmetric channel and tries to
decode.
"""

import pycodes, pycodes.pyLDPC
from pycodes.utils import channels, encoders, CodeMaker

######################################################################
#
# You can set the following parameters as desired.
#
######################################################################

col_w = 3
row_w = 6
N=1200
numIter = 20
noiseDensity = .058

######################################################################
#
# You probably should not need to change anything below this point.
#
######################################################################

K=N*col_w/row_w
E=(N-K)*row_w
L=CodeMaker.make_H_gallager(N,col_w,row_w)

code = pycodes.pyLDPC.LDPCCode(N,K,E,L)

c = [0]*N
ev = channels.BSCOutput(c,noiseDensity)
code.setevidence(ev,alg='default')

for i in range(numIter):
    code.decode()

beliefs = code.getbeliefs()

bitDiffs = reduce(lambda x,y: x+y,map(lambda z: z > 0.5,beliefs))

print `bitDiffs` + ' bits wrong after ' + `numIter` + ' iterations.'



ev = channels.BSCOutput(c,noiseDensity)
code.setevidence(ev,alg='default')

for i in range(numIter):
    code.decode()

beliefs = code.getbeliefs()

bitDiffs = reduce(lambda x,y: x+y,map(lambda z: z > 0.5,beliefs))

print `bitDiffs` + ' bits wrong after ' + `numIter` + ' iterations.'
