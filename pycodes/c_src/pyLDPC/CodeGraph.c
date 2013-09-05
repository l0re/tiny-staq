
/*
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
     "AS IS" BASIS, AND MERL HAS NO OBLIGATIONS TO PROVIDE MAINTENANCE,
     SUPPORT, UPDATES, ENHANCEMENTS OR MODIFICATIONS.
*/

#include <assert.h>
#include <math.h>
#include "misc.h"
#include "CodeGraph.h"
#include "CodeGraphAlgorithms.h"
#include "SumProductBP.h"

CodeGraph* CreateCodeGraph(const int numVNodes, const int k,
			   const int numCNodes, const int numEdges) {
  CodeGraph* result = SafeMalloc(sizeof(CodeGraph));
  result->k = k;
  result->maxLLR = 1e10;
  result->minLLR = -1e10;
  result->numVNodes = numVNodes;
  result->numCNodes = numCNodes;
  result->numEdges = numEdges;
  result->vNodes = SafeCalloc(numVNodes,sizeof(VariableNode) );
  result->cNodes = SafeCalloc(numCNodes,sizeof(CheckNode));
  result->edges =  SafeCalloc(numEdges,sizeof(GraphEdge));
  result->algorithm = NULL;

  return result;
}


void DestroyCodeGraph(CodeGraph*const cg) {
  int i;
  if (NULL != cg->algorithm) 
    cg->algorithm->DeallocateAlgorithm(cg,/*verbose=*/0);
  cg->algorithm = NULL;
  for (i=0; i < cg->numCNodes; i++) {
    DestroyCheckNode(&(cg->cNodes[i]));
  }
  for (i=0; i < cg->numVNodes; i++) {
    DestroyVarNode(&(cg->vNodes[i]));
  }
  free(cg->cNodes);
  free(cg->vNodes);
  free(cg->edges);
}

/* ----------------------------------------------------------------------
//
// FUNCTION:	AssertCodeGraphValid
//
// PURPOSE:	Checks if a CodeGraph object is valid by checking to make 
//              sure that all belief, channel evidence, and
//              likelihoods have reasonable values.  Also, checks to
//              make sure that all variables are attached to checks
//              and vice versa.
//
// MODIFIED:	Wed Jun 04, 2003
//
// --------------------------------------------------------------------*/

void AssertCodeGraphValid(const CodeGraph*const cg) {
#ifndef NDEBUG
  int i, j, k, varConnectsBackToCheck;
  const VariableNode*const vNodeStart = cg->vNodes;
  const VariableNode*const vNodeEnd = cg->vNodes + cg->numVNodes;
  unsigned short* varConnected = SafeCalloc(cg->numVNodes,
					    sizeof(unsigned short));


  for (i=0; i < cg->numCNodes; i++) {
    assert( cg->cNodes[i].numVars > 0);
    assert( cg->cNodes[i].edges != NULL);
    for (j=0; j < cg->cNodes[i].numVars; j++) {
      assert( cg->cNodes[i].edges[j]->var >= vNodeStart );
      assert( cg->cNodes[i].edges[j]->var < vNodeEnd );
      varConnected[ (int)(cg->cNodes[i].edges[j]->var-vNodeStart)] += 1;
      assert( ! isnan( cg->cNodes[i].edges[j]->var->llr ) );

      varConnectsBackToCheck=0;
      for(k=0; k < cg->cNodes[i].edges[j]->var->numChecks; k++) {
	if (cg->cNodes[i].edges[j]->var->edges[k]->check == 
	    &(cg->cNodes[i])) {
	  varConnectsBackToCheck = 1;
	  break;
	}
      }
      assert(varConnectsBackToCheck);
    }
  }

  for (i=0; i < cg->numVNodes; i++) {
    assert(varConnected[i] > 0);
  }
  free(varConnected);
#endif
}

/* --------------------------------------------------------------------
//
// FUNCTION:	CreateGraphFromLinkArrays
//
// INPUTS:	n,k,m: Number of terms in code, degrees of freedom, and
//                     number of checks.  For non-redundant codes m=n-k.
//		linkArrayLengths: An array of m integers where
//                                linkArrayLengths[i] is the number of
//                                variables in the ith check.
//              linkArrays: An array of arrays indicating the variables
//                          involved in each check.  Specifically,
//                          linkArrays[i] has exactly linkArrayLengths[i]
//                          integers from 0 to n-1 inclusive and
//                          linkArrays[i][j] = x indicates that there is
//                          a one in row i, column x of the parity check
//                          matrix for this code.  Note the caller owns
//                          linkArrays and linkArrayLenghts so neither of
//                          these arrays will be modified or deallocated.
//
// RETURNS:	A newly constructed CodeGraph object.
//
// MODIFIED:	Fri Jun 06, 2003
//
// --------------------------------------------------------------------*/

CodeGraph* CreateGraphFromLinkArrays(const int n, const int k, const int m,
				     const int numEdges,
				     const int*const linkArrayLengths,
				     int** linkArrays) {
  int vCounter, curCheck, curEdge=0;
  const int numChecks = m;
  CodeGraph* result = CreateCodeGraph(n,k,numChecks,numEdges);
  for(curCheck=0; curCheck < numChecks; curCheck++) {
    InitCheckNode(&(result->cNodes[curCheck]),linkArrayLengths[curCheck]);
    for(vCounter=0; vCounter<linkArrayLengths[curCheck]; vCounter++) {
      result->cNodes[curCheck].edges[vCounter] = &(result->edges[curEdge]);
      result->edges[curEdge].check = &(result->cNodes[curCheck]);
      result->edges[curEdge].var = 
	&(result->vNodes[linkArrays[curCheck][vCounter]]);
      assert(result->edges[curEdge].var->numChecks < (128*sizeof(short))-2);
      result->edges[curEdge].var->numChecks++;
      curEdge++;
    }
  }
  assert(curEdge == result->numEdges);
  LinkVarsBackToChecks(result->numVNodes,result->vNodes,
		       result->numEdges,result->edges);
    
  AssertCodeGraphValid(result);

  return result;
}
