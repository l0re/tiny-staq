
"""
This file creates a the dual of an LDPC code, generates a random
binary vector with some fraction of erased samples and tries to
find a codeword in the code which matches the unerased samples exactly.

If a matching codeword is found a message to that effect is printed.
If no matching codeword can be found an exception is raised and the
error is described.  Usually this happens if the fraction of erased
samples is too low.
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
N=12000
fractionErased = .60

######################################################################
#
# You probably should not need to change anything below this point.
#
######################################################################

K=N*col_w/row_w
E=(N-K)*row_w
L=CodeMaker.make_H_gallager(N,col_w,row_w)

code = pycodes.pyLDPC.DualLDPCCode(N,K,E,L)
numPositionsToErase=int(round(fractionErased*N))

c = channels.GetRandomBinaryCodeword(N)
channels.EraseExactlyE(c,numPositionsToErase)
evidence = channels.GetLLRsForErasureChannel(c)

code.setevidence(evidence,alg='BECQuant')

result = code.decode(verbose=0)
if (result!=None):
    raise Exception, ('Quantization failed due to result ' + `result` + '.')

beliefs = code.getbeliefs()[0:(N-K)] # these are beliefs for hidden vars
result = encoders.EncodeFromLinkArray(map(lambda x: x<0,beliefs),N,L)
numDiffs = reduce(lambda a,b: a+b,
                  map(lambda a,b: (b!=None and int(a) != b),result,c)) 
if (0 != numDiffs):
    raise Exception, ('Decoded result differed from source in ' + `numDiffs` +
                      ' unerased positions.\n')

print ('Succesfully quantized length ' + `N` + ' source with ' +
       `numPositionsToErase` + ' erasures.')
