
"""
This file implements the ECCLPFormer class and QuantLPFormer to
decode a low density parity check (LDPC) error correcting code or
quantize via the dual of an LDPC code using a linear programming
relaxation.  The basic idea is that you instantiate an ECCLPFormer or
QuantLPFormer and give it the code parameters.  Then you tell it to
form an LP and either solve it or print it out.

Before using this file you need to download and install the freely
available GNU Linear Programming Kit (GLPK).  If you install the
executable glpsol from GLPK in a weird place that is not in the path
seen by python, set the default for the LPSolver variable in the
__init__ method of the LPFormer base class to point to the glpsol
executable.

Some examples of how to use the ECCLPFormer and QuantLPFormer are shown
below.  While this file can stand alone (provided you have GLPK), it
is most useful when used with the other pycodes routines developed
by Emin Martinian and available from http://freshmeat.net/projects/pycodes.

If you have these other files available you can have python test the
examples provided below by using the command 'python FormLP.py'.
These tests can range from less than a minute to a 3GHz Linux system
to around 5 minutes on an older G3 Mac running OS 10.2.

######################################################################
#                                                                    #
# Start error correction examples                                    #
#                                                                    #
######################################################################


# The following is a simple example of how to use the ECCLPFormer class
# for a parity check matrix representing the code shown below.
#
# y0  y1     y2  y3
#  =  =      =   =
#  \  |\    /|  /
#   \ | \  / | /
#    \|  \/  |/
#     +   +  +
#
# The only two codewords for this code are 0000 and 1111.
# First we generat the ECCLPFormer class, then we use it solve for
# the optimal y0,y1,y2,y3 given the received data [1,0,1,1].
# This received data corresponds to sending 1111 and getting an
# error on the second bit.  The LP decoder correcterly decoes
# to the answer y0,y1,y2,y3 = 1,1,1,1.
#
>>> from FormLP import *
>>> r = ECCLPFormer(4,1,[[0,1],[1,2],[2,3]])
>>> r.FormLP([1,0,1,1])
>>> (v,s,o) = r.SolveLP()
>>> print v
[1.0, 1.0, 1.0, 1.0]

# Next we do LP decoding for a medium size Gallager code assuming
# that the all zeros codeword was transmitted.  Feldman, Karger,
# and Wainwright argue that analyzing things assuming the all-0
# codeword was sent over a binary symmetric channel is valid provided
# the LP satisfies certain conditions (see their 2003 CISS paper
# for more details).  IMPORTANT: the all-0 assumption works for
# analyzing things sent over a BSC but *NOT* over an erasure channel.
# The following test takes about a minute to run on a Mac G3.
>>> N = 1200
>>> K = 600
>>> numErrors = 90 # error rate of 7.5%
>>> from FormLP import *
>>> from CodeMaker import *
>>> from random import *
>>> regL = make_H_gallager(N,3,6)
>>> origSource = [0]*N
>>> recSource = list(origSource)
>>> i = 0
>>> while (i < numErrors):
... 	index = randrange(N)
... 	if (0 == recSource[index]):
... 	    recSource[randrange(N)] = 1
... 	    i = i+1
... 
>>> r = ECCLPFormer(N,K,regL)
>>> r.FormLP(recSource)
>>> (v,s,o) = r.SolveLP()
>>> errors = map(lambda x,y: int(x) != int(y), origSource,v)
>>> print 'num errors = ',  errors.count(1)
num errors =  0

######################################################################
#                                                                    #
# Start quantization examples                                        #
#                                                                    #
######################################################################


# The following is a simple example of how to use the QuantLPFormer class
# for a generator matrix representing the code shown below.
#
# y0  y1     y2  y3
#  +  +      +   +
#  \  |\    /|  /
#   \ | \  / | /
#    \|  \/  |/
#    x0  x1  x2
#
# First we generat the QuantLPFormer class, then we use it solve for
# the optimal x0,x1,x2 when y0,y1,y2,y3=[1,0,0,1].  The answer turns 
# out to be x0,x1,x2 = 1,1,1.
#
>>> from FormLP import *
>>> r = QuantLPFormer(4,3,[[0],[0,1],[1,2],[2]])
>>> r.FormLP([1,0,0,1])
>>> (v,s,o) = r.SolveLP()
>>> print v
[1.0, 1.0, 1.0]

# In the following example we take the dual of a (7,4) Hamming code using the
# built in function TransposeCodeMatrix and then use that as the generator
# matrix for quantization.  In this example we quantize the
# sequence 0,*,*,*,*,*,1 where the *'s represent don't cares which can
# be reconstructed to either 0 or 1.
# 
>>> r = QuantLPFormer(7,3,TransposeCodeMatrix(7,4,[[0,1,2,4],[1,2,3,5],[2,3,4,6]]))
>>> r.FormLP([0,.5,.5,.5,.5,.5,1])
>>> (v,s,o) = r.SolveLP()
>>> print v
[0.0, 0.0, 1.0]


#
# Iteratively solve an quantization LP for a medium size code.  
# This does not seem to work all that well, but none of the other LP
# relaxations does much better at quantization either.
#
>>> N = 300
>>> K = 150
>>> numErase = 180
>>> numIter = 1000
>>> from FormLP import *
>>> from CodeMaker import *
>>> from random import *
>>> regL = make_H_gallager(N,3,6)
>>> source = map(lambda x: round(random()),range(N))
>>> for i in range(numErase):
... 	source[randrange(N)] = 0.5
... 
>>> r = QuantLPFormer(N,K,TransposeCodeMatrix(N,K,regL))
>>> r.FormLP(source)
>>> (v,s,o) = r.IterSolveLP(numIter,verbose=0)
>>> from encoders import *
>>> recon = EncodeFromLinkArray(map(lambda x: int(x),v),N,regL)
>>> diffs = map(lambda x,y: x != 0.5 and x != y, source,recon)
>>> print 'num flips = ',  diffs.count(1)
num flips =  25
>>> if (25 != diffs.count(1)):
... 	print 'failure may be due to diffs w/glpsol on different platforms'
>>>

# The following example illustrates what can go wrong with the LP
# relaxation in doing quantization.  Choosing v = [1.0,1.0,0.0] would
# reconstruct the source perfectly in all the unerased positions
# (places where the source is not 0.5).  But the LP relaxation produces
# the vector [1.0/3.0, 1.0/3.0, 1.0/3.0].  First, this 'solution' is
# not even binary, and second even rounding the bits would not give the
# right answer.

>>> from FormLP import *
>>> from CodeMaker import *
>>> from random import *
>>> hammingCode = [[0,1,2,4],[1,2,3,5],[2,3,4,6]]
>>> source = [0.5, 0.0, 0.0, 0.5, 1.0, 0.5, 0.5]
>>> r = QuantLPFormer(7,3,TransposeCodeMatrix(7,4,hammingCode))
>>> r.FormErasureLP(source)
>>> (v,s,o) = r.SolveLP()
>>> from encoders import *
>>> recon = EncodeFromLinkArray(map(lambda x: int(x),v),7,hammingCode)
>>> diffs = map(lambda x,y: x != 0.5 and x != y, source,recon)
>>> print 'num flips = ', diffs.count(1)
num flips =  1

"""

import string, tempfile, commands, re

def q(a,b,c):
    if a:
        return b
    else:
        return c

def CountOnes(numBitsToCheck,z):
    mask = 1
    count = 0
    for i in range(numBitsToCheck):
        if (mask & z):
            count = count + 1
        mask = mask << 1
    return count 

def OddParity(numBitsToCheck,z):
    return CountOnes(numBitsToCheck,z) % 2

def EvenParity(numBitsToCheck,z):
    return not OddParity(numBitsToCheck,z)

def TransposeCodeMatrix(N,K,codeMatrix):
    """
    Take a parity check code matrix with N columns and N-K rows and
    transposes it.

    The input codeMatrix is a list of N-K lists representing the rows
    of the parity check matrix.  Specifically, codeMatrix[i][j]=t
    means that the jth one in the ith row appears in column t.

    This function returns a list of N lists representing the transpose.
    So result[i][j]=t means that the jth one in the ith column appears
    in row t.

    This function is useful for taking a code produced by the file
    CodeMaker.py and putting it into a form suitable for the
    QuantLPFormer class.
    """
    assert N-K == len(codeMatrix)

    result = range(N)
    for i in range(N):    
        result[i] = []
    for row in range(N-K):
        for col in codeMatrix[row]:
            result[col].append(row)
    return result

class LPFormer:
    """
    The LPFormer class is a base class for the ECCLPFormer and
    QuantLPFormer classes.  You should use one of those and *not*
    use LPFormer class directly.
    """
    
    def __init__(self,N,K,codeMatrix,
                 LPSolver = 'glpsol',
                 LPSolveOpts = ' --nomip '):
        """
        N:          Number of outputs.
        K:          Number of inputs.
        codeMatrix: A representation of the generator matrix for the code
                    as a list of lists.  Specifically, codeMatrix[i] should
                    be a list of the variables which are summed modulo 2
                    to produce the ith output.
        LPSolver:   Path to the executable for the GNU Linear Programming Kit.
        LPSolveOpts:Options to GLPK.
        """
        self.N = N
        self.K = K
        self.codeMatrix = codeMatrix
        self.LPSolver = LPSolver
        self.LPSolveOpts = LPSolveOpts

        self.Reset()

    def Reset(self):
        self.output = []
        self.channelData = []


    def SolveLP(self):
        """
        Solve the linear program developed by calling the FormLP method
        (i.e., make sure you call FormLP before calling this).

        The return value is a 3-tuple (v,s,o) where v represents the
        (possibly fractional) values for the K variables, s represents
        the status returned by the LPSolver, and o is the text output
        of the LPSolver.
        """
        
        resultFile = tempfile.mktemp()
        modelFile = tempfile.mktemp()
        fd = open(modelFile,'w')
        fd.write(self.GetLP())
        fd.close()
        (s,o) = commands.getstatusoutput(self.LPSolver + self.LPSolveOpts +
                                         ' --model ' + modelFile  +
                                         ' -o ' + resultFile)
        if (s):
            print 'Output from calling LPSolver:\n', o
        varVals = self.ExtractVarValuesFromLPSolution(resultFile)
        return (varVals,s,o)

    def IterSolveLP(self,numIter,verbose=1):
        """
        numIter:  Maximum number of iterations to perform.

        Solve the LP then find the most certain variable and freeze it
        to the nearest value, solve the LP, freeze another var, etc.

        Returns the same 3-tuple as SolveLP().
        """

        channelData = self.channelData
        extraConstraints = []
        varsToCheck = {}
        for i in range(self.K):
            varsToCheck[i] = 1
        for i in range(numIter):
            if (verbose):
                print 'Doing iter ', i
            self.Reset()
            self.FormLP(channelData,string.join(extraConstraints))
            (v,s,o) = self.SolveLP()
            extraConstraints.append(self.FixCertainVars(varsToCheck,v))
            (varIndex,numAmbigousVars) = self.FindSureVarToForce(varsToCheck,v)
            if (-1 != varIndex):
                del varsToCheck[varIndex]
            if (numAmbigousVars == 0):
                if (verbose):
                    print 'Terminating since no ambiguous vars left.'
                return (v,s,o)
            else:
                valueToForce = round(v[varIndex])
                if (verbose):
                    print 'Found ', numAmbigousVars, ' ambiguous vars.'
                    print 'Forcing var ',varIndex,' to be ',valueToForce
                    print ' (old value was ', v[varIndex], ' )'
                extraConstraints.append('s.t. forceCon' + `i` + ': ' +
                                        ' relaxedVars[' + `varIndex` + '] = '
                                        + `valueToForce` + ';\n')
        return (v,s,o)

    def IterSolveLPDecimateAmbiguous(self,numIter,verbose=1):
        """
        numIter:  Maximum number of iterations to perform.

        Solve the LP then find an ambiguous variable and freeze it
        to the nearest value, solve the LP, freeze another var, etc.

        Returns the same 3-tuple as SolveLP().
        """
        channelData = self.channelData
        extraConstraints = []
        for i in range(numIter):
            if (verbose):
                print 'Doing iter ', i
            self.Reset()
            self.FormLP(channelData,string.join(extraConstraints))
            (v,s,o) = self.SolveLP()
            (varIndex,numAmbigousVars) = self.FindAmbiguousVarToForce(v)
            if (0.0 == v[varIndex] or
                1.0 == v[varIndex]):
                if (verbose):
                    print 'Terminating since no ambiguous vars left.'
                return (v,s,o)
            else:
                valueToForce = round(v[varIndex])
                if (verbose):
                    print 'Found ', numAmbigousVars, ' ambiguous vars.'
                    print 'Forcing var ', varIndex, ' to be ', valueToForce
                    print ' (old value was ', v[varIndex], ' )'
                extraConstraints.append('s.t. forceCon' + `i` + ': ' +
                                        ' relaxedVars[' + `varIndex` + '] = '
                                        + `valueToForce` + ';\n')
        return (v,s,o)


    def GetLP(self):
        "Print the linear program formed by the FormLP method."
        return string.join(self.output,'\n')

#
# The following methods are *not* supposed to be called directly by
# the user.  They are only meant to be called by other methods.
#

    def ExtractVarValuesFromLPSolution(self,resultFile):
        """
        Read the file resultFile containing the output of an LPSolver
        run and extract the values for the relaxedVars.

        This function returns a list for the relaxedVars.
        """
        FIND_VAR = 1
        SKIP_B = 2
        GRAB_VALUE = 3
        mode = FIND_VAR

        varValues = []
        varNum = 0
        varRE = re.compile('relaxedVars\\[([0-9]+)\\]')

        fd = open(resultFile,'r')
        for word in fd.read().split():
            if (FIND_VAR == mode):
                m = varRE.match(word)
                if (m):
                    assert int(m.groups(0)[0]) == varNum
                    mode = SKIP_B
            elif (SKIP_B == mode):
                mode = GRAB_VALUE
            elif (GRAB_VALUE == mode):
                mode = FIND_VAR
                varValues.append(float(word))
                varNum += 1
        return varValues

    def FixCertainVars(self,varsToCheck,partialSolution):
        """
        varsToCheck: A hash table where the keys are variables which
                     have not yet been frozen to be 0 or 1.  Vars which
                     are frozen will be removed from this hash.
        partialSolution: Values for each of the relaxedVars.

        This function goes through all the vars in partialSolution and
        freezes all vars which are 0 or 1.  Specifically, it returns
        a string which when added to the LP will give constraints to
        freeze the desired vars.
        """
        result = []
        for i in varsToCheck.keys():
            if (partialSolution[i] == 0.0 or partialSolution[i] == 1.0):
                del varsToCheck[i]
                result.append('s.t. forceCon__' + `i` + ': ' +
                              ' relaxedVars[' + `i` + '] = '
                              + `partialSolution[i]` + ';\n')
        return string.join(result)
    
    def FindSureVarToForce(self,varsToCheck,partialSolution):
        """
        varsToCheck: A hash table where the keys are variables which
                     have not yet been frozen to be 0 or 1.  Vars which
                     are frozen will be removed from this hash.
        partialSolution: Values for each of the relaxedVars.

        Goes through all varsToCheck in partialSolution and finds the
        most certain var (the one closest to being 0 or 1).  The
        resulting var is removed from varsToCheck and returned in
        a tuple along with the number of vars which are not 0 or 1.
        """
        
        mostCertainIndex = -1
        mostCertainValue = -1
        numAmbigousVars = 0
        for i in varsToCheck.keys():
            certainty = abs(0.5 - partialSolution[i])
            if (certainty > mostCertainValue):
                mostCertainValue = certainty
                mostCertainIndex = i
            if (certainty < 0.5):
                numAmbigousVars = numAmbigousVars + 1
        return (mostCertainIndex,numAmbigousVars)

    def FindAmbiguousVarToForce(self,partialSolution):
        """
        partialSolution: Values for each of the relaxedVars.

        Finds the most ambiguous var (the one closest to being 0.5).
        The index of this var is returned in a tuple along with the
        number of vars which are not either 0 or 1.
        """
        leastCertainIndex = -1
        leastCertainValue = 2.0
        numAmbigousVars = 0
        for i in range(len(partialSolution)):
            certainty = abs(0.5 - partialSolution[i])
            if (certainty <= leastCertainValue):
                leastCertainValue = certainty
                leastCertainIndex = i
            if (certainty < 0.5):
                numAmbigousVars = numAmbigousVars + 1
        return (leastCertainIndex,numAmbigousVars)

                       
    def FormLPSourceData(self,channelData):
        self.channelData = channelData
        self.output.append('param : channel :=\n')
        for i in range(len(channelData)):
            self.output.append('  ' + `i`  + ' ' + `1-2*channelData[i]`)
        self.output.append(';\n')

    def FormLPDataSection(self):
        self.output.append('\ndata;\n')


class QuantLPFormer(LPFormer):
    """
    This class forms a linear programming relaxation to do quantization.
    The main methods intended to be called by the user are listed below,
    see the accompanying documentation for this class or the base class
    LPFormer for details.

    __init__
    FormLP
    FormErasureLP
    SolveLP
    IterSolveLP
    IterSolveLPDecimateAmbiguous
    GetLP
    """


    def FormLP(self,channelData,extraVarData=''):
        """
        channelData:   A description of the source values or channelData.
                       1's and 0's are represented as 1's and 0's while
                       erasures or don't cares are represented with 0.5.
        extraVarData:  A string representing extra stuff to put in the
                       var section of the linear program.

        This function forms the LP required to quantize the channelData.
        After this function is called you can call one of the solve
        methods.

        Note that if you are doing erasure quantization you can either
        use this method or the FormErasureLP method but not both.
        """
        self.FormLPPreamble()
        self.FormLPVarSection()
        self.output.append(extraVarData)        
        self.FormLPDataSection()
        self.FormLPSourceData(channelData)        


    def FormErasureLP(self,channelData,extraVarData=''):
        """
        channelData:   A description of the source values or channelData.
                       1's and 0's are represented as 1's and 0's while
                       erasures or don't cares are represented with 0.5.
        extraVarData:  A string representing extra stuff to put in the
                       var section of the linear program.

        This function forms the LP required to quantize the
        channelData.  using the erasure metric.  That is it attempts
        to exactly match all 0's and 1's and ignores 0.5's.  After
        this function is called you can call one of the solve methods.

        Note that if you are doing erasure quantization you can either
        use this method or the FormLP method but not both, but you
        *must* be doing erasure quantization to use this method.

        The main difference in this method is that it encodes the
        problem purely in the constraints of the linear program and
        does not have any objective function.
        """        
        self.FormLPPreamble()
        self.FormConstraintsConnectingChecksAndVars()
        self.output.append(extraVarData)
        self.FormErasureConstraints(channelData)        
        self.FormLPDataSection()

    def MakeConstraintsConnectingCheckAndVars(self,checkNum,
                                              varIndexes,checkOn):
        """
        checkNum:   The index of the check to work on.
        varIndexes: List of indexes of the vars connected to check checkNum.
        checkOn:    Whether to make a constraint for the check being on
                    (in which case checkOn=1) or off (checkOn=0).

        This function adds constraints to the linear program requiring
        that the modulo-2 sum of the relaxedVars named by varIndexes
        equals checkOn for relaxedChecks[checkNum].

        ############################################################
        #
        # The following describes the formula for obtaining the 
        # constraints enforcing that all things connected to a
        # parity checck sum to 0 modulo 2.

        As Martin Wainwright explained to me there are two ways to
        represent these constraints using 'configuration variables'
        as discussed in the paper

        J. Feldman, D. Karger and M. Wainwright
        Using linear programming to decode LDPC codes.
        Conference on Information Sciences and Systems, Baltimore, March 2003.

        While this form is conceptually simpler, the LP relaxation can
        be more compactly written by projecting out the configuration
        variables to get an LP as in example 1 of the paper

        J. Feldman, D. Karger and M. J. Wainwright,
        LP Decoding  (Invited Paper)
        Allerton Conference on Communication, Control, and Computing
        October 1--3, 2003; Urbana-Champaign, IL

        The rule for making the compact LP relaxation is as follows.
        Let x = x0, x1, ..., xC be the C variables involved in a parity
        check constraint.  let S be a subset of these C variables which
        are set to 1.  For example S = {0,2} means x0=x2=1 while the
        other xi are 0.  For each such subset with odd cardinality, the
        corresponding constraint in the LP is

           sum_{i in S} xi - sum_{i not in S} xi <= |S| - 1

        We obtain the required constraints for the LP by summing doing
        this for all possible subsets, S, with odd cardinality.
        
        For example, if x0,x1,x2 are connected to a check, then the
        required subsets of odd cardinality are
        S = {0,1,2}, S' = {0}, S'' = {1}, S'' = {2}
        yielding the constraints

        x0 + x1 + x2 <= 2
        x0 - x1 - x2 <= 0        
        -x0 + x1 - x2 <= 0
        -x0 - x1 + x2 <= 0                        
        
        """
        assert checkOn == 0 or checkOn == 1
        result = []
        varsForCheck = len(varIndexes)

        for configuration in range(1 << varsForCheck):
            if ( not ((CountOnes(varsForCheck,configuration)+checkOn) % 2)):
                continue
            constraintList = []                
            constraintList.append(
                's.t. c' + `checkNum` + 'c' + `checkOn` + '_' +
                `configuration` + ': ' + q(checkOn,'','-') + 
                'relaxedChecks[' + `checkNum` + '] ')
            for i in range(varsForCheck):
                if (configuration & (1 << i)):
                    constraintList.append('+')
                else:
                    constraintList.append('-')
                constraintList.append('relaxedVars[' + `varIndexes[i]` + ']')
            constraintList.append('<= ' +
                                  `(q(checkOn,0,-1) +
                                    CountOnes(varsForCheck,configuration))`)
            constraintList.append(';\n')
            result.append(string.join(constraintList))
        return string.join(result)

    def FormLPPreamble(self):

        self.output.append('set Vars := {0..' + `self.K-1` + '};\n')
        self.output.append('set Checks := {0..' + `self.N-1` + '};\n')
        self.output.append("""

var relaxedChecks{c in Checks};
var relaxedVars{v in Vars};

s.t. checksUB{c in Checks}: relaxedChecks[c] >= 0;
s.t. checksLB{c in Checks}: relaxedChecks[c] <= 1;
s.t. varsUB{v in Vars}: relaxedVars[v] <= 1;
s.t. varsLB{v in Vars}: relaxedVars[v] >= 0;

        """)
        

    def FormConstraintsConnectingChecksAndVars(self):

        for checkNum in range(len(self.codeMatrix)):
            self.output.append(self.MakeConstraintsConnectingCheckAndVars(
                checkNum,self.codeMatrix[checkNum],1))
            self.output.append(self.MakeConstraintsConnectingCheckAndVars(
                checkNum,self.codeMatrix[checkNum],0))

    def FormLPVarSection(self):
        """
        Form an linear program relaxation suitable for decoding the
        code codeMatrix.
        """

        self.FormConstraintsConnectingChecksAndVars()

        self.output.append(
            'param channel{c in Checks};\n\n' + 
            'minimize flipCost: sum{c in Checks} relaxedChecks[c]*channel[c];\n' +
            '/* a cost of +1 is incurred if we make check i a 1 when */\n' +
            '/* the channel evidence for it is 0 and a cost of -1 is */\n' +
            '/* incurred if we make check i a 0 when the channel */\n' +
            '/* evidence for it is 0.  Thus we try to match the channel */\n' +
            '/* evidence as well as possible. */\n')

    def FormErasureConstraints(self,channelData):
        """
        channelData: A description of the source values or channelData.
                     1's and 0's are represented as 1's and 0's while
                     erasures or don't cares are represented with 0.5.
        """
        for i in range(len(channelData)):
            if (0.5 != channelData[i]):
                self.output.append('s.t. e_' + `i` + '_constraint: ' +
                                   'relaxedChecks[' + `i` + ']=' +
                                   `channelData[i]` + ';\n')
        
class ECCLPFormer(LPFormer):
    """
    This class forms a linear programming relaxation to do error correction.
    The main methods intended to be called by the user are listed below,
    see the accompanying documentation for this class or the base class
    LPFormer for details.

    __init__
    FormLP
    FormErasureLP
    SolveLP
    IterSolveLP
    IterSolveLPDecimateAmbiguous
    GetLP
    """
    
    def FormLP(self,channelData,extraVarData=''):
        """
        channelData:   A description of the channel output values.
                       1's and 0's are represented as 1's and 0's while
                       erasures or don't cares are represented with 0.5.
        extraVarData:  A string representing extra stuff to put in the
                       var section of the linear program.

        This function forms the LP required to decode the channelData.
        After this function is called you can call one of the solve
        methods.

        Note that if you are doing erasure decoding you can either
        use this method or the FormErasureLP method but not both.
        """
        self.FormLPPreamble()
        self.FormLPVarSection()
        self.output.append(extraVarData)        
        self.FormLPDataSection()
        self.FormLPSourceData(channelData)        


    def FormErasureLP(self,channelData,extraVarData=''):
        """
        channelData:   A description of the recieved data from the channel
                       with 1's and 0's represented as 1's and 0's while
                       erasures or don't cares are represented with 0.5.
        extraVarData:  A string representing extra stuff to put in the
                       var section of the linear program.

        This function forms the LP required to decode the
        channelData using the erasure metric.  That is it attempts
        to exactly match all 0's and 1's and ignores 0.5's.  After
        this function is called you can call one of the solve methods.

        Note that if you are doing erasure decoding you can either
        use this method or the FormLP method but not both, but you
        *must* be doing erasure decoding to use this method.

        The main difference in this method is that it encodes the
        problem purely in the constraints of the linear program and
        does not have any objective function.
        """        
        self.FormLPPreamble()
        self.FormConstraintsConnectingChecksAndVars()
        self.output.append(extraVarData)
        self.FormErasureConstraints(channelData)        
        self.FormLPDataSection()

    def MakeConstraintsEnforcingParity(self,checkNum,varIndexes):
        """
        checkNum:   The index of the check to work on.
        varIndexes: List of indexes of the vars connected to check checkNum.

        This function adds constraints to the linear program requiring
        that the modulo-2 sum of the relaxedVars named by varIndexes
        equals 0.

        See the comment for MakeConstraintsEnforcingParity to see
        where the logic for this function comes from.
        """
        result = []
        varsForCheck = len(varIndexes)

        for configuration in range(1 << varsForCheck):
            if ( not (CountOnes(varsForCheck,configuration) % 2)):
                continue # only do stuff for subsets with odd number of 1's
            constraintList = []                
            constraintList.append(
                's.t. c' + `checkNum` + 'c' + '_' +
                `configuration` + ': ')
            for i in range(varsForCheck):
                if (configuration & (1 << i)):
                    constraintList.append('+')
                else:
                    constraintList.append('-')
                constraintList.append('relaxedVars[' + `varIndexes[i]` + ']')
            constraintList.append('<= ' +
                                  `CountOnes(varsForCheck,configuration)-1`)
            constraintList.append(';\n')
            result.append(string.join(constraintList))
        return string.join(result)

    def FormLPPreamble(self):

        self.output.append('set Vars := {0..' + `self.N-1` + '};\n')
        self.output.append("""

var relaxedVars{v in Vars};

s.t. varsUB{v in Vars}: relaxedVars[v] <= 1;
s.t. varsLB{v in Vars}: relaxedVars[v] >= 0;

        """)


    def FormLPVarSection(self):
        """
        Form an linear program relaxation suitable for decoding the
        code codeMatrix.
        """

        for checkNum in range(len(self.codeMatrix)):
            self.output.append(
                self.MakeConstraintsEnforcingParity(checkNum,
                                                    self.codeMatrix[checkNum]))
            

        self.output.append(
            'param channel{v in Vars};\n\n' + 
            'minimize flipCost: sum{v in Vars} relaxedVars[v]*channel[v];\n' +
            '/* a cost of +1 is incurred if we make var i a 1 when */\n' +
            '/* the channel evidence for it is 0 and a cost of -1 is */\n' +
            '/* incurred if we make var i a 0 when the channel */\n' +
            '/* evidence for it is 0.  Thus we try to match the channel */\n' +
            '/* evidence as well as possible. */\n')

    def FormErasureConstraints(self,channelData):
        """
        channelData: A description of the source values or channelData.
                     1's and 0's are represented as 1's and 0's while
                     erasures or don't cares are represented with 0.5.
        """
        for i in range(len(channelData)):
            if (0.5 != channelData[i]):
                self.output.append('s.t. e_' + `i` + '_constraint: ' +
                                   'relaxedVars[' + `i` + ']=' +
                                   `channelData[i]` + ';\n')

    

# The following code is used to make the doctest package
# check examples in docstrings.

def _test():
    import random
    random.setstate((1, (29245, 20096, 302), None)) # for consistent testing
    import doctest, FormLP
    return doctest.testmod(FormLP)

if __name__ == "__main__":
    _test()
    print 'Tests passed'
