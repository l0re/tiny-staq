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

def EncodeFromLinkArray(m,N,L):
    """
    EncodeFromLinkArray(m,N,L):

    m:    A list of 0s and 1s representing 'message bits'.
    N:    Length of the code.
    L:    A link array representing rows in a binary generator matrix.
          Specifically, L[i] contains a list indicating the positions
          in row i of the generator matrix which are a 1.

          For example, consider the generator matrix

          G = [1 1 1 0 1 0 0]
              [0 1 1 1 0 1 0]
              [0 0 1 1 1 0 1]

          To encode the message vector [1,0,1], you would do

>>> EncodeFromLinkArray([1,0,1],7,[[0,1,2,4],[1,2,3,5],[2,3,4,6]])
[1, 1, 0, 1, 0, 0, 1]

    """
    result = [0]*N
    for i in range(len(m)):
        assert(m[i] == 0 or m[i] == 1)
        if (m[i]):
            for index in L[i]:
                result[index] = result[index] ^ m[i] 
        
    return result
