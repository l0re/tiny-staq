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
The converters package contains various routines to convert from
one representation of a code to another.

Type print converters.__dict__.keys() for a list of provided functions.
"""

def AlistToNKH(alistFileName):
    """
    Convert a parity check matrix in alist format in the file alistFileName
    to (N,K,H) format where N is the block length, K is the degrees of
    freedom and H is the parity check matrix represented as a list of rows.

    The alist format is used by Mackay and collaborators in their software.

    NOTE: you probably want to use AlistToNKEL if you want to create
    an LDPCCode or DualLDPDCode object based on an alist.
    """

    fd = open(alistFileName)
    data = fd.read().split('\n')

    line = data[0].split()
    (N,M) = (int(line[0]),int(line[1]))
    K = N-M
    H = [0]*M
    for j in range(M):
        H[j] = [0]*N

    i = 0
    for line in data[(4+N):(4+M+N)]:
        for item in line.split():
            H[i][int(item)-1] = 1
        i = i + 1

    return (N,K,H)


    
def AlistToNKEL(alistFileName):
    """
    Convert a parity check matrix in alist format in the file alistFileName
    to (N,K,E,L) format where N is the block length, K is the degrees of
    freedom, L is a list of links, and E is the total number of edges.

    The alist format is used by Mackay and collaborators in their software.
    """

    fd = open(alistFileName)
    data = fd.read().split('\n')

    line = data[0].split()
    (N,M) = (int(line[0]),int(line[1]))
    K = N-M
    L = [0]*M
    E = 0

    i = 0
    for line in data[(4+N):(4+M+N)]:
        L[i] = map(lambda x: int(x)-1,line.split())
        E = E + len(L[i])
        i = i + 1

    return (N,K,E,L)

def LinkArrayToMatrix(N,linkArray):
    H = [0]*len(linkArray)
    i=0
    for row in linkArray:
        H[i] = [0]*N
        for item in row:
            H[i][item] = 1
        i = i+1
    return H
    
def CountEdgesInLinkArray(linkArray):
    """
    CountEdgesInLinkArray(linkArray):
    Count number of edges in linkArray.  This is useful for getting
    the E parameter required by LDPCCode and DualLDPCCode.
    """
    result = 0
    for row in linkArray:
        result = result + len(row)
    return result
    
