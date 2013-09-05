
#include <assert.h>
#include <math.h>
#include "misc.h"
#include "SatGraph.h"
#include "SatGraphAlgorithm.h"

void InitSatCheckNode(SatCheckNode*const cn, const int numVars) {
  cn->numVars = numVars;
  cn->edges = SafeCalloc(numVars,sizeof(SatGraphEdge*));
}

/* The numChecks field must already be set */
void InitSatVarNode(SatVarNode*const vn) {
  vn->edges = SafeCalloc(vn->numChecks,sizeof(SatGraphEdge*));
}

void DestroySatCheckNode(SatCheckNode*const cn) {
  free(cn->edges); 
}


void DestroySatVarNode(SatVarNode*const vn) {
  free(vn->edges); 
}

void PrintSatVarNode(const SatVarNode*const var) {
 int i;
 printf("llr=%g, ev=%g\n",var->llr,var->channelEvidence);
 for (i=0; i < var->numChecks; i++) {
   printf("messagesIn[%i]=%g, ",i,var->edges[i]->msgToVar);
 }
 printf("\n");
 for (i=0; i < var->numChecks; i++) {
   printf("messagesOut[%i]=%g, ",i,var->edges[i]->msgToCheck);
 }
 printf("\n\n");
}

void PrintSatVarNodeArray(const SatVarNode* vars, const int numVars) {
  const SatVarNode*const lastVar = vars+numVars;
  while(vars < lastVar) PrintSatVarNode(vars++);
}


void LinkSatVarsBackToSatChecks(const int numVars, SatVarNode*const vNodes,
				const int numEdges, SatGraphEdge*const edges) {
  int curVar, curEdge;
  SatVarNode* curVarNode;

  for (curVar=0; curVar < numVars; curVar++) 
    InitSatVarNode(&(vNodes[curVar]));
  for (curEdge=0; curEdge < numEdges; curEdge++) {
    curVarNode = edges[curEdge].var;
    curVarNode->edges[curVarNode->counter++] = &(edges[curEdge]);
  }
}


SatGraph* CreateSatGraph(const int numVNodes, 
			  const int numCNodes, const int numEdges) {
  SatGraph* result = SafeMalloc(sizeof(SatGraph));
  result->maxLLR = 1e10;
  result->minLLR = -1e10;
  result->numVNodes = numVNodes;
  result->numCNodes = numCNodes;
  result->numEdges = numEdges;
  result->vNodes = SafeCalloc(numVNodes,sizeof(SatVarNode) );
  result->cNodes = SafeCalloc(numCNodes,sizeof(SatCheckNode));
  result->edges =  SafeCalloc(numEdges,sizeof(SatGraphEdge));
  result->algorithm = NULL;

  return result;
}


void DestroySatGraph(SatGraph*const sg) {
  int i;
  if (NULL != sg->algorithm) 
    sg->algorithm->DeallocateAlgorithm(sg,/*verbose=*/0);
  sg->algorithm = NULL;
  for (i=0; i < sg->numCNodes; i++) {
    DestroySatCheckNode(&(sg->cNodes[i]));
  }
  for (i=0; i < sg->numVNodes; i++) {
    DestroySatVarNode(&(sg->vNodes[i]));
  }
  free(sg->cNodes);
  free(sg->vNodes);
  free(sg->edges);
}

/* --------------------------------------------------------------------
//
// FUNCTION:	CreateSatGraphFromLinkArrays
//
// INPUTS:	numVars, numChecks, numEdges:   Number of checks, vars,
                                                and edges in the graph.
//		linkArrayLengths: An array of M integers where
//                                linkArrayLengths[i] is the number of
//                                variables in the ith check.
//              linkArrays: An array of arrays indicating the variables
//                          involved in each check.  Specifically,
//                          linkArrays[i] has exactly linkArrayLengths[i]
//                          integers from [1,N] or [-1,-N] inclusive and
//                          linkArrays[i][j] = x indicates that variable
//                          abs(x)-1 is the jth variable in the ith clause 
//                          with the variable being complemented if x < 0.
//                          Note the caller owns linkArrays and 
//                          linkArrayLenghts so neither of
//                          these arrays will be modified or deallocated.
//
// RETURNS:	A newly constructed SatGraph object.
//
// MODIFIED:	Fri Jun 06, 2003
//
// --------------------------------------------------------------------*/

SatGraph* CreateSatGraphFromLinkArrays(const int numVars, /* N=num variables */
				       const int numChecks, /* M=num clauses */
				       const int numEdges,
				       const int*const linkArrayLengths,
				       int** linkArrays) {
  int vCounter, curCheck, curEdge=0;

  SatGraph* result = CreateSatGraph(numVars,numChecks,numEdges);
  for(curCheck=0; curCheck < numChecks; curCheck++) {
    InitSatCheckNode(&(result->cNodes[curCheck]),linkArrayLengths[curCheck]);
    for(vCounter=0; vCounter<linkArrayLengths[curCheck]; vCounter++) {
      result->cNodes[curCheck].edges[vCounter] = &(result->edges[curEdge]);
      result->edges[curEdge].check = &(result->cNodes[curCheck]);
      result->edges[curEdge].var = 
	&(result->vNodes[abs(linkArrays[curCheck][vCounter])-1]);
      assert(result->edges[curEdge].var->numChecks < (128*sizeof(short))-2);
      result->edges[curEdge].invert = linkArrays[curCheck][vCounter] < 0;
      assert(0 != linkArrays[curCheck][vCounter]);
      result->edges[curEdge].var->numChecks++;
      curEdge++;
    }
  }
  assert(curEdge == result->numEdges);
  LinkSatVarsBackToSatChecks(result->numVNodes,result->vNodes,
			     result->numEdges,result->edges);
    
  return result;
}
