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
BSCTester
TestStatsReporter

See the documentation for each class for further details.

"""

import sys
from math import *

import pycodes, pycodes.pyLDPC
from pycodes.utils.channels import *
from pycodes.utils.converters import *
from pycodes.utils.encoders import *

class CodeParams:
    """
    The CodeParams class is designed to keep track of the
    various parameters used in creating and testing a code.
    """
    
    def __init__(self,N,K,E,L,name,numTimes,maxIter,alg='AccBitFlip'):
        """
        __init__(self,N,K,E,L,name,numTimes,maxIter,alg='AccBitFlip'):
        N,K,E,L:    The number of variables, dimension, number of edges,
                    and link array used to create a DualLDPCCode using the
                    pycodes.DualLDPCCode function.
        name:       Name for the test.
        numTimes    A list indicating how many trials for each code.
        maxIter     Maximum number of iterations to allow in quantization.
        alg         Optional argument indicating the algorithm to use
                    for quantization.
        """
        self.N = N
        self.K = K
        self.E = E
        self.L = L
        self.name = name
        self.numTimes = numTimes
        self.maxIter = maxIter
        self.alg = alg

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
        """
        self.iterationsNeeded = []
        self.flips = []
        self.N = []
        self.K = []
        self.names = []
        
        for file in filesToReadFrom:
            self.GetStatsFromFile(file)

    def AddStats(self,N,K,name,iterationsNeeded,flips):
        """
        AddStats(self,N,K,name,iterationsNeeded,flips):
        
        Add results for running a test of an (N,K) quantization code
        resulting in flips unmatched bits for 
        the number of iterations required given by iterationsNeeded.
        """
        self.N.append(N)
        self.K.append(K)
        self.names.append(name)
        self.iterationsNeeded.append(iterationsNeeded)
        self.flips.append(flips)

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
        iterNeeded = fileData.iterationsNeeded
        fl = fileData.flips

        self.N = self.N + Nlist
        self.K = self.K + Klist
        self.names = self.names + nameList
        self.iterationsNeeded = self.iterationsNeeded + iterNeeded
        self.flips = self.flips + fl

    def WriteInternalsToFD(self,fd):
        """
        WriteInternalsToFD(self,fd):

        Write internal representation of data to the file descriptor
        fd.  This allows a later TestStatsReporter object to read
        in the data using the GetStatsFromFile function.
        """
        fd.write('Nlist = ' + `self.N` + '\n')
        fd.write('Klist = ' + `self.K` + '\n')
        fd.write('nameList = ' + `self.names` + '\n')
        fd.write('iterationsNeeded = ' + `self.iterationsNeeded` + '\n')
        fd.write('flips = ' + `self.flips` + '\n')        

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
            self.WriteFlipStatsToFD(codeIndex,title,fd)

        if (file != None):
            self.WriteInternalsToFD(fd)
            fd.close()
            print 'Data written to file ' + file

    def WriteFlipStatsToFD(self,codeIndex,title,fd):
        tries = reduce(lambda x,y: x+y, self.iterationsNeeded[codeIndex])
        sumFlips = reduce(lambda a,b: a+b,
                          map( lambda k,v: k*v,
                               self.flips[codeIndex].keys(),
                               self.flips[codeIndex].values()))
        sumSqFlips = reduce(lambda a,b: a+b,
                            map(lambda k,v: (k**2)*v,
                                self.flips[codeIndex].keys(),
                                self.flips[codeIndex].values()))
        mean = float(sumFlips)/float(tries)
        print '-->', tries,sumFlips,sumSqFlips,mean
        var = float(sumSqFlips)/float(tries) - mean**2
        fd.write('# ' + title + '\n')
        fd.write('# mean flips = ' + `mean` + '\n')
        fd.write('# variance   = ' + `var` + '\n')
        
    def MakePlot(self,file):
        """
        MakePlot(self,file):

        If file==None then do nothing.  Otherwise use biggles
        to create a plot of the test results and write them in
        Encapsulated PostScript format to file.

        Note that you must have biggles and Numeric installed
        for this function to work.
        """
        if (None==file):
            return None

        try:
            import biggles, Numeric
        except Exception, e:
            print 'Received exception "' +`e`+ '" in import biggles,Numeric.'
            print 'No plot will be made.  Please install biggles and'
            print 'Numeric if you want plotting capability.'
            return

        p = biggles.FramedPlot()
        p.title = 'Length ' + `self.N` + ' code'
        p.xlabel = '% flipped'
        p.ylabel = 'prob'
        types = ['solid','dotted','dotdashed','longdashed',
                     'shortdashed','dotdotdashed','dotdotdotdashed']
        curves = [0]*len(self.names)
        tries = [0]*len(self.names)
        for i in range(len(self.names)):
            keys = self.flips[i].keys()
            keys.sort()
            tries[i] = reduce(lambda x,y: x+y, self.iterationsNeeded[i])
            pFlipped = Numeric.array(map(lambda x: float(x)/float(self.N[i]),
                                         keys))
            prob = Numeric.array(map(lambda x: float(self.flips[i][x])
                                     / tries[i], keys))
                                     

            curves[i] = biggles.Curve( pFlipped, prob,
                                       type = types[i % len(types)])
            curves[i].label = self.names[i]
            p.add(curves[i])
        plotKey = biggles.PlotKey( .1, .9, curves)
        p.add(plotKey)

            
        if (file):
            p.write_eps(file)
            print 'Plot written to file ', file
            
        return p
        

class BSCTester:
    """
    The BSCTester class is used to measure the performance of 
    codes in binary erasure channel quantization.  The idea is that you
    create an instance of the BSCTester class and give it the desired
    test parameters.  This causes the instance to run the desired
    tests and record stats.  You can then get the stats using the
    ReportStats member function.
    """
    
    def __init__(self,cpList,verbose=1):
        """
        __init__(self,cpList,verbose=1):
        cpList      A list of CodeParams objects representing the
                    paramters of the tests to do.
        verbose     A 0 or 1 indicating whether the bscquantize algorithm
                    should be called in verbose mode.
        """
        self.codeParams = cpList
        self.verbose = verbose
        self.codes = map(lambda p:pycodes.pyLDPC.DualLDPCCode(n=p.N,k=p.K,
                                                              H=p.L,e=p.E),
                         cpList)
        self.iterationsNeeded = [0]*len(self.codes)
        self.flips = []
        for codeIndex in range(len(self.codes)):
            self.flips.append({})
            self.iterationsNeeded[codeIndex] = (
                [0]*(self.codeParams[codeIndex].maxIter+1))
            for i in range(len(self.codeParams)):
                if (verbose):
                    sys.stdout.write('\nStarting tests for code ' 
                                     + `self.codeParams[codeIndex].name`)
                for trial in range(self.codeParams[codeIndex].numTimes):
                    self.DoBSCQuantizeTest(codeIndex)


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
                              self.codeParams[i].name,
                              self.iterationsNeeded[i],self.flips[i])
        reporter.Report(title=title,file=reportFile)
        reporter.MakePlot(imgFile)

    def CountFlips(self,c,codeIndex):
        N = self.codeParams[codeIndex].N
        K = self.codeParams[codeIndex].K
        L = self.codeParams[codeIndex].L
        
        beliefs = self.codes[codeIndex].getbeliefs()[0:(N-K)]
        compressedResult = map(lambda x: x < 0, beliefs)
        quantizedResult = EncodeFromLinkArray(compressedResult,N,L)
        diff = reduce(lambda a,b: a+b, map(lambda x,y: x!=y,c,quantizedResult))

        return diff

    def NoteCodeGotFlips(self,codeIndex,numFlips):

        if (self.flips[codeIndex].has_key(numFlips)):
            self.flips[codeIndex][numFlips] = (
                self.flips[codeIndex][numFlips] + 1)
        else:
            self.flips[codeIndex][numFlips] = 1
    
    def DoBSCQuantizeTest(self,codeIndex):
        """
        DoBSCQuantizeTest(self,codeIndex):

        This is an internal function used to conduct the required
        tests.  THE USER SHOULD NEVER CALL THIS DIRECTLY.
        """
        c = map(lambda x: random.randrange(2),
                range(self.codeParams[codeIndex].N))
        ev = map(lambda x: -(2*x) + 1, c)
        self.codes[codeIndex].setevidence(
            ev,alg=self.codeParams[codeIndex].alg)

        sys.stdout.write('.')
        sys.stdout.flush()
        for i in range(self.codeParams[codeIndex].maxIter):
            self.codes[codeIndex].decode(verbose=self.verbose)
            if (0 and self.codes[codeIndex].converged()):
                self.iterationsNeeded[codeIndex][i+1] = (
                    self.iterationsNeeded[codeIndex][i+1] + 1)
                self.NoteCodeGotFlips(codeIndex,self.CountFlips(c,codeIndex))
                return
        self.iterationsNeeded[codeIndex][0] = (
            self.iterationsNeeded[codeIndex][0]+1)
        flips = self.CountFlips(c,codeIndex)
        self.NoteCodeGotFlips(codeIndex,flips)
        N = self.codeParams[codeIndex].N
        K = self.codeParams[codeIndex].K
        sys.stdout.write('%2.2f%%' % (round(10000.0*float(flips)
                                            /float(N))/100.0))
        
        beliefs = self.codes[codeIndex].getbeliefs()
        dontKnows = beliefs[0:self.codeParams[codeIndex].K].count(0.0)
        if (dontKnows > 0):
            print '\nNumber of don\'t knows = ', dontKnows, '\n\n'
        if (flips > (N-K)):
            print '\n\nWARNING: ' + `flips` + ' flips in quantizing', c
        


