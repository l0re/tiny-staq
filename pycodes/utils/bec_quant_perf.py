"""

     Copyright 2003 Mitsubishi Electric Research Laboratories All Rights
     Reserved.  Permission to use, copy and modify this software and its
     documentation without fee for educational, research and non-profit
     purposes, is hereby granted, provided that the above copyright
     notice and the following three paragraphs appear in all copies.      

     To request permission to incorporate this software into commercial
     products contact:  Vice President of Marketing and Business         
     Development;  Mitsubishi Electric Research Laboratories (MERL), 201
     Broadway, Cambridge, MA   02139 or <license@merl.com>.        
                                                                               
     IN NO EVENT SHALL MERL BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, 
     SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST        
     PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS 
     DOCUMENTATION, EVEN IF MERL HAS BEEN ADVISED OF THE POSSIBILITY OF
     SUCH DAMAGES.

     MERL SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
     FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN
     ``AS IS'' BASIS, AND MERL HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE,
     SUPPORT, UPDATES, ENHANCEMENTS OR MODIFICATIONS.
"""

"""
This file contains classes used to automate LDPC quantization
tests and recording of their results.  The main classes are

CodeParams
BECTester
TestStatsReporter

See the documentation for each class for further details.

"""

import sys

from pycodes import pyLDPC
from pycodes.utils.channels import *
from pycodes.utils.converters import *

class CodeParams:
    """
    The CodeParams class is designed to keep track of the
    various parameters used in creating and testing a code.
    """
    
    def __init__(self,N,K,E,L,name,numToErase,numTrials):
        """
        __init__(self,N,K,E,L,name,numToErase,numTrials):
        N,K,E,L:    The number of variables, dimension, number of edges,
                    and link array used to create a DualLDPCCode using the
                    pyLDPC.DualLDPCCode function.
        name:       Name for the test.
        numToErase  A list containing how many of N elements to erase.
        numTrials    A list indicating how many trials for each element
                      in numToErase.
        """
        self.N = N
        self.K = K
        self.E = E
        self.L = L
        self.name = name
        self.numToErase = numToErase
        self.numTrials = numTrials

class TestStatsReporter:
    """
    The TestStatsReporter class is used to report statistics
    generated in testing a code.  There are two main modes
    of use: reading data from a previous run stored in file
    or adding new data.
    """
    
    def __init__(self,filesToReadFrom=[]):
        """
        __init__(self,filesToReadFrom=[]):
        Reads data from the files specified in filesToReadFrom.
        Note that each element in the list filesToReadFrom must
        be a file which can be imported into python via import,
        i.e., each element must a file of the form <name>.py
        which is in sys.path.

        For example, if you have previously generated the files
        test_a.py and test_b.py you can read them in using
        t = TestStatsReporter(['test_a','test_b'])
        """
        self.numSuccesses = []
        self.numTrials = []
        self.N = []
        self.K = []
        self.names = []
        self.numToErase = []
        
        for file in filesToReadFrom:
            self.GetStatsFromFile(file)

    def AddStats(self,N,K,numToErase,name,numSuccesses,numTrials):
        """
        AddStats(self,N,K,numToErase,name,numSuccesses,numTrials):
        
        Add results for running a test of an (N,K) code
        quantizing the erasues in the list numToErase with
        a the number of successes in numSuccesses.
        """
        self.N.append(N)
        self.K.append(K)
        self.numToErase.append(numToErase)
        self.names.append(name)
        self.numSuccesses.append(numSuccesses)
        self.numTrials.append(numTrials)

    def GetStatsFromFile(self,file):
        """
        GetStatsFromFile(self,file):

        Read stats from file and add them to this object.
        See comments in the __init__ function for constrains on file.
        """
        fileData = __import__(file)
        Nlist = fileData.Nlist
        Klist = fileData.Klist
        nElist = fileData.nElist
        nameList = fileData.nameList
        numSuccesses = fileData.numSuccesses
        numTrials = fileData.numTrials

        self.N = self.N + Nlist
        self.K = self.K + Klist
        self.numToErase = self.numToErase + nElist        
        self.names = self.names + nameList
        self.numSuccesses = self.numSuccesses + numSuccesses
        self.numTrials = self.numTrials + numTrials

    def WriteInternalsToFD(self,fd):
        """
        WriteInternalsToFD(self,fd):

        Write internal representation of data to the file descriptor
        fd.  This allows a later TestStatsReporter object to read
        in the data using the GetStatsFromFile function.
        """
        fd.write('Nlist = ' + `self.N` + '\n')
        fd.write('Klist = ' + `self.K` + '\n')
        fd.write('nElist = ' + `self.numToErase` + '\n')
        fd.write('nameList = ' + `self.names` + '\n')
        fd.write('numSuccesses = ' + `self.numSuccesses` + '\n')
        fd.write('numTrials = ' + `self.numTrials` + '\n')

    def WriteStatsToFD(self,codeIndex,title,fd):
        """
        WriteStatsToFD(self,codeIndex,title,fd):

        Write the results for test number codeIndex with the given
        title to file descriptor fd.
        """
        fd.write( '# ' + title + '\n' )
        fd.write( '# ' + 'code title = ' + self.names[codeIndex] + '\n' )      

    def Report(self,title='No title given',file=None):
        """
        Report(self,title='No title given',file=None):

        Report data for all tests using the given title and write
        the data to the given file name.  If file==None, then data
        is printed to the screen but not written to file.

        Note that if you want to be able to read the data back into
        a TestStatsReporter object in the future then file should
        end in .py and satisfy the other constrains listed in the
        comment for the __init__ function.
        """
        if (file == None):
            fd = sys.stdout
        else:
            fd = open(file,'w')

        for codeIndex in range(len(self.names)):
            self.WriteStatsToFD(codeIndex,title,fd)

        if (file != None):
            self.WriteInternalsToFD(fd)
            fd.close()
            print 'Data written to file ' + file
        
    def MakePlot(self,file=None,title=None,xrange=None):
        """
        MakePlot(self,file=None,title=None,xrange=None):

        If file==None then do nothing.  Otherwise use biggles
        to create a plot of the test results and write them in
        Encapsulated PostScript format to file.

        Note that you must have biggles and Numeric installed
        for this function to work.
        """
        if (None==file):
            return None
        if (None==title):
            title = 'Length ' + `self.N` + ' codes'


        try:
            import biggles, Numeric
        except Exception, e:
            print 'Received exception "' +`e`+ '" in import biggles,Numeric.'
            print 'No plot will be made.  Please install biggles and'
            print 'Numeric if you want plotting capability.'
            return

        p = biggles.FramedPlot()
        if (None!=xrange):
            p.xrange=xrange
        p.title = title
        p.xlabel = 'fraction erased'
        p.ylabel = 'failure probability'
        types = ['solid','dotted','dotdashed','longdashed',
                     'shortdashed','dotdotdashed','dotdotdotdashed']
        curves = [0]*len(self.names)

        for i in range(len(self.names)):
            pErased = Numeric.array(map(lambda x: float(x)/float(self.N[i]),
                                        self.numToErase[i]))
            failProbs = Numeric.array(map(lambda x,y:
                                          1-(float(x)/float(y)),
                                          self.numSuccesses[i],
                                          self.numTrials[i]))
            curves[i] = biggles.Curve( pErased, failProbs,
                                       type = types[i % len(types)])
            curves[i].label = self.names[i]
            p.add(curves[i])
        plotKey = biggles.PlotKey( .1, .9, curves)
        p.add(plotKey)

            
        if (file):
            p.write_eps(file)
            print 'Plot written to file ', file
            
        return p
        

class BECTester:
    """
    The BECTester class is used to measure the performance of 
    codes in binary erasure channel quantization.  The idea is that you
    create an instance of the BECTester class and give it the desired
    test parameters.  This causes the instance to run the desired
    tests and record stats.  You can then get the stats using the
    ReportStats member function.
    """
    
    def __init__(self,cpList,verbose=1):
        """
        __init__(self,cpList,verbose=1):
        cpList      A list of CodeParams objects representing the
                    paramters of the tests to do.
        verbose     A 0 or 1 indicating whether the becquantize algorithm
                    should be called in verbose mode.
        """
        self.codeParams = cpList

        self.codes = map(lambda p:pyLDPC.DualLDPCCode(n=p.N,k=p.K,
                                                      H=p.L,e=p.E),cpList)
        self.numSuccesses = [0]*len(self.codes)
        
        for codeIndex in range(len(self.codes)):
            self.numSuccesses[codeIndex] = [0] * len(self.codeParams[
                codeIndex].numToErase)
            for i in range(len(self.codeParams[codeIndex].numToErase)):
                if (verbose):
                    sys.stdout.write('\nStarting tests with '
                                 +`self.codeParams[codeIndex].numToErase[i]`
                                     + ' erasures for code ' 
                                     + `self.codeParams[codeIndex].name`)
                for trial in range(self.codeParams[codeIndex].numTrials[i]):
                    self.DoBECQuantizeTest(i,codeIndex)
                print ('results for test "' +
                       self.codeParams[codeIndex].name +
                       '", N=' + `self.codeParams[codeIndex].N` +
                       ', K=' + `self.codeParams[codeIndex].K` +
                       ' :  ' + 
                       `self.numSuccesses[codeIndex][i]` + '/' +
                       `self.codeParams[codeIndex].numTrials[i]` + ' = ' +
                       `(float(self.numSuccesses[codeIndex][i]) /
                         float(self.codeParams[codeIndex].numTrials[i]))` +
                       ' successes.')

        
    def ReportStats(self,title='Test results:\n',reportFile=None,
                    imgFile=None):
        """
        ReportStats(self,title='Test results:\n',reportFile=None,
                    imgFiles=None):
        title:       A title to use in reporting test results.
        reportFile:  Name of file to write results to.  If
                     reportFile==None, then data is printed to stdout.
        imgFile:     A file to save a plot of results to.  If
                     imgFile==None, then no plot is made.
                     Note that you must have Numeric and biggles installed
                     if you make imgFiles != None.
        
        """
        reporter = TestStatsReporter()
        for i in range(len(self.codes)):
            reporter.AddStats(self.codeParams[i].N,self.codeParams[i].K,
                              self.codeParams[i].numToErase,
                              self.codeParams[i].name,
                              self.numSuccesses[i],
                              self.codeParams[i].numTrials)
        reporter.Report(title=title,file=reportFile)
        reporter.MakePlot(imgFile)
                
    def DoBECQuantizeTest(self,eIndex,codeIndex):
        """
        DoBECQuantizeTest(self,eIndex,codeIndex):

        This is an internal function used to conduct the required
        tests.  THE USER SHOULD NEVER CALL THIS DIRECTLY.
        """
        c = map(lambda x: random.randrange(2),
                range(self.codeParams[codeIndex].N))
        EraseExactlyE(c,self.codeParams[codeIndex].numToErase[eIndex])
        ev = GetLLRsForErasureChannel(c)
        self.codes[codeIndex].setevidence(ev,alg='BECQuant')

        sys.stdout.write('.') # indicates starting iteration
        sys.stdout.flush()

        quantError = self.codes[codeIndex].decode(verbose=0)
        if (quantError == 'stuck'):
            sys.stdout.write('S') # indicates iteration got stuck
            sys.stdout.flush()
            return
        elif (quantError != None):
            sys.stdout.write('I') # indicates contradiction found so
            sys.stdout.flush() # encoding is impossible.
            return
        else:
            self.numSuccesses[codeIndex][eIndex] += 1
            sys.stdout.write('C') # indicates iteration converged
            sys.stdout.flush()                
            return


def BruteForceDecode(c,N,K,L):
    """
    BruteForceDecode(c,N,K,L)

    c:   A list of elements containing 0, 1, and None.
    N:   Length of the erasure correcting code.
    K:   Dimension of the dual of the code.
    L:   A link array describing the dual of the code.

    Returns a vector q such that q * G matches c.

    This function attempts to quantize c to a vector in the given
    code by brute force.  Specifically, an LUP decomposition is
    used to solve the overdetermined set of linear equations.
    The main use of this function is to check/verify
    faster iterative decoding.
    """
    import genericmatrix

    numErased = c.count(None)
    indexTranslation = [0]*N
    reverseIndexTranslation = [0]*(N-numErased)
    word = [0]*(N-numErased)
    j=0
    for i in range(N):
        if (c[i] == None):
            indexTranslation[i] = None
        else:
            word[j] = c[i]
            indexTranslation[i]=j
            reverseIndexTranslation[j]=i
            j=j+1
    m = genericmatrix.GenericMatrix(size=(N-K,N-numErased),
                                    zeroElement=0,
                                    identityElement=1,
                                    add=lambda x,y: x^y,
                                    mul=lambda x,y: x&y,
                                    sub=lambda x,y: x^y,
                                    div=lambda x,y: x )
    for row in range(len(L)):
        for col in L[row]:
            index = indexTranslation[col]
            if (index != None):
                m[row,index] = 1
    m.Transpose()
    sol = m.Solve(word)
    assert m.LeftMulColumnVec(sol) == word
    return sol
    
