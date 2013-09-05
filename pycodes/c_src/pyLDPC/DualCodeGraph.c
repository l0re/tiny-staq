
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

#include "misc.h"
#include "DualCodeGraph.h"
#include "SumProductBP.h"
#include "DualCodeGraphAlgorithms.h"

/* ----------------------------------------------------------------------
//
// FUNCTION:	CreateDualCodeGraph
//
// INPUTS:	n, k, m: Parameters for normal code
//
// RETURNS:	Newly constructed DualCodeGraph for dual code.
//
// MODIFIED:	Wed Jun 11, 2003
//
// ------------------------------------------------------------------- */


DualCodeGraph* CreateDualCodeGraph(const int n, const int k,
				   const int m, const int numEdges) {
  DualCodeGraph* result = SafeMalloc(sizeof(DualCodeGraph));

  assert(n-k == m); /* haven't handled duals of codes w/redundant checks */
  result->k = n-k;
  result->n = n;
  result->numVNodes = result->n+result->k;
  result->numCNodes = n;
  result->numEdges = numEdges+n;
  result->vNodes = SafeCalloc(result->numVNodes,sizeof(VariableNode) );
  result->cNodes = SafeCalloc(result->numCNodes,sizeof(CheckNode));
  result->edges = SafeCalloc(result->numEdges,sizeof(GraphEdge));
  result->algorithm = NULL;

  return result;
}

/* ----------------------------------------------------------------------
//
// FUNCTION:	CreateDualLinkArrays
//
// INPUTS:	n, k, m, checkLinkArrayLengths, checkLinkArrays: 
//                 Parameters for a code as expected by the function
//                 CreateGraphFromLinkArrays.
//              varLinkArrayLengths, varLinkArrays: Pointers to variables
//                 which get filled in with the link arrays for the dual code.
//
// PURPOSE:	Return link arrays to create the dual code.  
//
//              Specifically, checkLinkArrays[i] corresponds to the
//              ith row in the parity check matrix for the code and
//              says which vars the ith check links to in the normal
//              code.  In the dual code, checks and vars switch places.
//              So instead of having an array saying which vars the ith
//              check is linked to, for the dual code, the input arrays
//              say which checks a var is linked to.  Thus we need to
//              recreate the appropriate arrays.
//
// MODIFIED:	Wed Jun 11, 2003
//
// ------------------------------------------------------------------- */


void CreateDualLinkArrays(const int n, const int k, const int m,
			  const int*const checkLinkArrayLengths,
			  int** checkLinkArrays,
			  int** dualLinkArrayLengths, int*** dualLinkArrays) {
  int row, var, i, varIndex;
  int* nextSpotInArray;

  *dualLinkArrayLengths = SafeCalloc(n,sizeof(int));
  *dualLinkArrays = SafeCalloc(n,sizeof(int*));

  for (row=0; row<m; row++) 
    for (i=0; i<checkLinkArrayLengths[row]; i++)
      (*dualLinkArrayLengths)[checkLinkArrays[row][i]]++;
  for (var=0; var<n; var++) {
    assert((*dualLinkArrayLengths)[var] > 0);
    (*dualLinkArrays)[var] = SafeCalloc((*dualLinkArrayLengths)[var],
				       sizeof(int));
  }

  nextSpotInArray = SafeCalloc(n,sizeof(int));
  for (row=0; row<m; row++) 
    for (i=0; i<checkLinkArrayLengths[row]; i++) {
      varIndex = checkLinkArrays[row][i];
      assert(varIndex < n);
      assert(nextSpotInArray[varIndex] < (*dualLinkArrayLengths)[varIndex]);
      (*dualLinkArrays)[varIndex][nextSpotInArray[varIndex]++] = row;
    }
  free(nextSpotInArray);
}

/* ----------------------------------------------------------------------
//
// FUNCTION:	CreateDualGraphFromLinkArrays
//
// INPUTS:	n,k,m,linkArrayLengths,linkArrays:
//                Parameters which would be used to create a normal
//                ldpc code.
//
// RETURNS:	Dual of the code which would be created by the
//              CreateGraphFromLinkArrays function with the given args.
//
// MODIFIED:	Wed Jun 11, 2003
//
// ------------------------------------------------------------------- */

DualCodeGraph* CreateDualGraphFromLinkArrays(const int n, const int k, 
					     const int m, const int numEdges,
					     const int*const linkArrayLengths,
					     int** linkArrays) {
  int* dualLinkArrayLengths;
  int **dualLinkArrays;
  int vCounter, curCheck, curEdge=0;
  const int numChecks = n;
  const int dualK = n - k;
  DualCodeGraph* result = CreateDualCodeGraph(n,k,m,numEdges);

  CreateDualLinkArrays(n,k,m,linkArrayLengths,linkArrays,
		       &dualLinkArrayLengths,&dualLinkArrays);

  for(curCheck=0; curCheck < numChecks; curCheck++) {
    assert(curCheck < result->numCNodes);
    assert(dualLinkArrayLengths[curCheck] < (128*sizeof(short))-2);
    InitCheckNode(&(result->cNodes[curCheck]),
		  dualLinkArrayLengths[curCheck]+1);
    for(vCounter=0; vCounter<dualLinkArrayLengths[curCheck]; vCounter++) {
      result->cNodes[curCheck].edges[vCounter] = &(result->edges[curEdge]);
      result->edges[curEdge].check = &(result->cNodes[curCheck]);
      result->edges[curEdge].var = 
	&(result->vNodes[dualLinkArrays[curCheck][vCounter]]);
      assert(result->edges[curEdge].var->numChecks < (128*sizeof(short))-3);
      result->edges[curEdge].var->numChecks++;
      curEdge++;
    }
    result->cNodes[curCheck].edges[dualLinkArrayLengths[curCheck]] = 
      &(result->edges[curEdge]);
    result->edges[curEdge].check = &(result->cNodes[curCheck]);
    result->edges[curEdge].var = &(result->vNodes[dualK+curCheck]);
    result->vNodes[dualK+curCheck].numChecks++;
    free(dualLinkArrays[curCheck]);
    curEdge++;
  }
  assert(curEdge == result->numEdges);
  LinkVarsBackToChecks(result->numVNodes,result->vNodes,
		       result->numEdges,result->edges);
  for(vCounter=0; vCounter < dualK; vCounter++) 
    result->vNodes[vCounter].varType = 'h';
  for(vCounter=0; vCounter < numChecks; vCounter++) 
    result->vNodes[vCounter+dualK].varType = 'o';
  free(dualLinkArrays);
  free(dualLinkArrayLengths);

  return result;
}

void DestroyDualCodeGraph(DualCodeGraph*const dc) {
  int i;

  if (NULL != dc->algorithm) 
    dc->algorithm->DeallocateAlgorithm(dc,/*verbose=*/0);
  dc->algorithm = NULL;
  for (i=0; i < dc->numCNodes; i++) {
    DestroyCheckNode(&(dc->cNodes[i]));
  }
  for (i=0; i < dc->numVNodes; i++) {
    DestroyVarNode(&(dc->vNodes[i]));
  }
  free(dc->vNodes);
  free(dc->cNodes);
  free(dc->edges);
}

void PrintVars(const DualCodeGraph* const dc) {
  int i;
  for(i = 0; i < dc->numVNodes; i++)
    PrintVariableNode(&(dc->vNodes[i]));
}

void PrintChecks(const DualCodeGraph* const dc) {
  int i;
  for(i = 0; i < dc->numCNodes; i++)
    PrintCheckNode(&(dc->cNodes[i]));
}

