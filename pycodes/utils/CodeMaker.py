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
The CodeMaker package contains function to create regular and irregular
Gallager codes.  For example, to create a 3,6 Gallager code
of block length 30 and dimension 15, you could do

>>> regL = make_H_gallager(30,3,6)

To create an irregular Gallager code with 4 variables of degree 1,
4 variables of degree 2, 2 variables of degree 3, 3 checks of degree 2,
and 4 checks of degree 3, you could do

>>> iregL = MakeIrregularLDPCCode(10,3,{1:4,2:4,3:2},{2:3,3:4})

To create an irregular Gallager code with degree sequences
lambda(x) = 0.33241 x^2 + 0.24632 x^3 + 0.11014 x^4 + 0.31112 x^6
rho(x) = 0.76611 x^6 + 0.23380 x^7 you could do

>>> iregL = MakeIrregularLDPCCodeFromLambdaRho(30,15,{2:0.33241, 3:.24632, 4:.11014, 6:0.31112},{6:.76611, 7:.23389})

Finally, note that although it is possible to make regular
Gallager codes using the Irregular code functions,
YOU SHOULD NOT DO THAT.  The irregular code functions
only give you approximatley the degree sequence you request
due to issues with randomly adding edges and removing
redundant edges.

"""

import time, copy, random

CodeMakerVersion = 1.0

def DotProduct(x,y):
    return reduce(lambda a,b: a+b,map(lambda a,b: a*b, x,y))

def randperm(N):
    "randperm(N): Return a random permuation of [0,1,...,N-1]."
    result = [None]*N
    for i in range(N):
        index = random.randrange(N)
        while (result[index] != None):
            index = random.randrange(N)
        result[index] = i
    return result

def make_H_gallager(N,col_w,row_w):
    """
    make_H_gallager(N,col_w,row_w):

    N:     Length of code.
    col_w: Column weight of code (i.e., how many 1's in a column).
    row_w: Row weight of code (i.e., how many 1's in a row).
    
    Create a regular LDPC code matrix using the construction in
    Gallager's book and return the result as a link array.  The
    return value, r, is a list of N lists where r[i][j] is the
    ith one in the jth row of H.

    The algorithm works by starting with a sub-matrix of n/row_w rows
    containing col_w 1's in the first positions in the first row,
    col_w 1's in the next set of positions in the second row, etc.
    
    H = [ 1 1 1 1 0 0 0 0 0 0 0 0 ...
          0 0 0 0 1 1 1 1 0 0 0 0 ...
          0 0 0 0 0 0 0 0 1 1 1 1 ...
          ...........................

    Next we create col_w-1 additional submatricies which are permuations of
    this and append them to the bottom of H.
    """
    num_rows = (N * col_w) / row_w
    num_sub_rows = num_rows / col_w
    assert row_w*num_rows == N*col_w, 'N*col_w not divisible by row_w'
    assert (N/row_w)*row_w == N, 'N not divisible by row_w'

    H_sub = [0]*num_sub_rows
    for i in range(num_sub_rows):
        H_sub[i] = map(lambda x,y: x + y,[i*row_w]*row_w,range(row_w))
    H = copy.deepcopy(H_sub)

    for i in range(col_w-1):

        H_new_sub = [0]*num_sub_rows
        for m in range(num_sub_rows):
            H_new_sub[m] = [0]*row_w

        
        rand_perm = randperm(N)
        for j in range(num_sub_rows):
            for k in range(row_w):
                H_new_sub[j][k] = rand_perm[H_sub[j][k]]
        l = list(H_new_sub[j])
        l.sort()
        H_new_sub[j] = l
        H = H + copy.deepcopy(H_new_sub)

    return H

def WriteNewRegularGallagerCode(N,col_w,row_w,fileName):
    """
    Routine to create a new Gallager Code and write it to a file.
    """
    fd = open(fileName,'w')
    fd.write('# Created code using CodeMaker.py version ' + `CodeMakerVersion`
             + '.\n# Random seed = ' + `random.getstate()` + '.\n')
    fd.write('# Command used = WriteNewRegularGallagerCode(%i,%i,%i,"%s") '
             %  (N, col_w, row_w, fileName))
    fd.write('\n\nlinkArray = ' + `make_H_gallager(N,col_w,row_w)`)
    fd.write('\n\n')
    fd.close()
    

def MakeIrregularLDPCCode(N,K,varDegrees,checkDegrees):
    """
    MakeIrregularLDPCCode(N,K,checkDegrees,varDegrees):

    N:               Length of the code.
    K:               Dimension of the code.
    checkDegrees:    Hash indicating how many checks have 1 connection,
                     2 connections, 3 connections, etc.
    varDegrees:      Hash indicating how many vars have 1 connection,
                     2 connections, 3 connections, etc.

    Returns a a list L which is a randomly constructed link
    array for the desired code.  Note the current approach of
    preventing multi-edges is to not add a redundant edge.  This
    ends up producing a graph with slightly lower degrees than
    specified.  If instead we keep trying to add edges to
    exactly match the desired degrees the function sometimes
    gets stuck and doesn't finish.
    """
    
    M = N - K
    numChecks = reduce(lambda x,y:x+y, checkDegrees.values())
    assert numChecks == M, (
        'Number of checks in checkDegrees sums to ' + `numChecks` +
        '!=' + `M` + '.')
    assert reduce(lambda x,y:x+y, varDegrees.values()) == N, (
        'Number of vars in varDegrees does not sum to N.')

    edgesFromChecks = DotProduct(checkDegrees.keys(),checkDegrees.values())
    edgesFromVars = DotProduct(varDegrees.keys(),varDegrees.values())
    assert edgesFromVars == edgesFromChecks, (
        '# edges from vars != # edges from checks:' +
        `edgesFromVars` + ' != ' + `edgesFromChecks`)
    assert N > max(checkDegrees.keys())
    assert M > max(varDegrees.keys())

    result = [0]*M
    
    vars = []
    curVarIndex = 0
    for d in varDegrees.keys(): # for each possible var degree 
        for i in range(varDegrees[d]): # for each of var with that degree
            vars.extend([curVarIndex]*d)
            curVarIndex = curVarIndex+1
    assert curVarIndex==N
    curCheckIndex = 0
    for d in checkDegrees.keys(): # for each possible check degree
        for i in range(checkDegrees[d]): # for each check with that degree
            result[curCheckIndex] = [None]*d
            for connectionForCheck in range(d):
                vIndex = random.randrange(len(vars))
                if (result[curCheckIndex].count(vars[vIndex]) == 0):
                    result[curCheckIndex][connectionForCheck]=vars.pop(vIndex)
                else:
                    print 'warning not adding multi-edge'
                    vars.pop(vIndex)                    
            while (result[curCheckIndex].count(None)>0):
                result[curCheckIndex].pop(result[curCheckIndex].index(None))
            curCheckIndex = curCheckIndex + 1
    assert len(vars)==0, 'vars should be empty but it is' + `vars`
    return result

def IntegrateLambdaOrRho(terms):
    """
    IntegrateLambdaOrRho(terms):
    terms:   A hash table containing lambda or rho coefficients.
             For example, if lambda(x) = .4 x^2 + .6 x^3, then
             the input would be {2:.4, 3:.6}.
    Returns  The integral of the argument from 0 to 1.
    """
    sum = 0
    total = 0
    for i in terms.keys():
        sum = sum + terms[i]
        total = total + terms[i]/float(i)
    assert(abs(sum-1.0) < .0001)
    return total

def LambdaRhoToDegreeSequences(N,M,lam,rho):
    """
    N:     Block size.
    M:     Number of constraints.
    lam:   A hash table specifying the variable degress using the lambda
           notation.  Specifically, lam[i] = p denotes that the fraction
           of EDGES coming from a variable node of degree i is p.
    rho:   A hash table specifying the check degress using the rho
           notation.  Specifically, rho[i] = p denotes that the fraction
           of EDGES coming from a check node of degree i is p.

    Returns a pair of hash tables (varDegrees, checkDegrees) where
    varDegress[i] = y indicates that there are y varialbes of degree i.
    """
    totalVarEdges = float(N)/IntegrateLambdaOrRho(lam)
    totalCheckEdges = float(M)/IntegrateLambdaOrRho(rho)
    
    varDegrees = {}
    for key in lam.keys():
        varDegrees[key] = int(round(totalVarEdges*lam[key]/float(key)))

    checkDegrees = {}
    for key in rho.keys():
        checkDegrees[key] = int(round(totalCheckEdges*rho[key]/float(key)))

    return (varDegrees,checkDegrees)

def ComputeEdgeMismatch(varDegrees,checkDegrees):
    edgesFromChecks = DotProduct(checkDegrees.keys(),checkDegrees.values())
    edgesFromVars = DotProduct(varDegrees.keys(),varDegrees.values())    
    edgeMismatch = edgesFromChecks-edgesFromVars
    return edgeMismatch

def MakeIrregularLDPCCodeFromLambdaRho(N,K,lam,rho):
    """
    MakeIrregularLDPCCodeFromLambdaRho(N,K,lam,rho):
    N:     Block size.
    K:     Dimension.
    lam:   A hash table specifying the variable degress using the lambda
           notation.  Specifically, lam[i] = p denotes that the fraction
           of EDGES coming from a variable node of degree i is p.
    rho:   A hash table specifying the check degress using the rho
           notation.  Specifically, rho[i] = p denotes that the fraction
           of EDGES coming from a check node of degree i is p.

    This function creates an irregular LDPC code for the desired parameters.
    """
    M=N-K
    total = 0

    (varDegrees,checkDegrees) = LambdaRhoToDegreeSequences(N,M,lam,rho)
    for key in checkDegrees.keys():
        total = total + checkDegrees[key]
    cCleanupIndex = checkDegrees.values().index(max(checkDegrees.values()))
    cCleanupIndex = checkDegrees.keys()[cCleanupIndex]
    checkDegrees[cCleanupIndex] = checkDegrees[cCleanupIndex] - (total-M)
    assert checkDegrees[cCleanupIndex] > 0

    total = 0
    for key in varDegrees.keys():
        total = total + varDegrees[key]

    vCleanupIndex = varDegrees.values().index(max(varDegrees.values()))
    vCleanupIndex = varDegrees.keys()[vCleanupIndex]
    varDegrees[vCleanupIndex] = varDegrees[vCleanupIndex] - (total-N)
    assert varDegrees[vCleanupIndex] > 0
    edgeMismatch = ComputeEdgeMismatch(varDegrees, checkDegrees)
    
    print 'Cleaning up edge mismatch of ', edgeMismatch

    k = varDegrees.keys()
    k.sort()

    degreeDiff = k[1]-k[0]
    edgeDiff = edgeMismatch/degreeDiff +1
    varDegrees[k[0]]=varDegrees[k[0]]-edgeDiff
    varDegrees[k[1]]=varDegrees[k[1]]+edgeDiff
    assert varDegrees[k[0]] > 0
    assert varDegrees[k[1]] > 0

    edgeMismatch = ComputeEdgeMismatch(varDegrees,checkDegrees)
    k = checkDegrees.keys()
    k.sort()
    if (edgeMismatch < 0):
        checkDegrees[k[0]] = checkDegrees[k[0]] - 1
        edgeMismatch = edgeMismatch - k[0]
        if (not checkDegrees.has_key(-edgeMismatch)):
            checkDegrees[-edgeMismatch]=0
        checkDegrees[-edgeMismatch]=checkDegrees[-edgeMismatch]+1
        print 'Adding one check of degree', -edgeMismatch, 'to fix mismatch.'
    else:
        # haven't yet implemented this case
        assert 0
    print 'using -->', varDegrees, checkDegrees
    return MakeIrregularLDPCCode(N,K,varDegrees,checkDegrees)
