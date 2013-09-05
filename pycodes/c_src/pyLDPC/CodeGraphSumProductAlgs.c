
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

#include <math.h>
#include <string.h>

#include "misc.h"
#include "CodeGraph.h"
#include "CodeGraphAlgorithms.h"
#include "CodeGraphSumProductAlgs.h"
#include "SumProductBP.h"


static char* FreeAlgrorithm(CodeGraph* cg, const int verbose) {
  free(cg->algorithm);
  cg->algorithm = NULL;
  return NULL;
}

static char* CGSetEvidence(CodeGraph*const cg, const int verbose,
			   void*const evPtr) {
  double*const ev = (double*) evPtr;
  int i;
  for (i=0; i < cg->numVNodes; i++) {
    cg->vNodes[i].channelEvidence = ev[i];
  }
  return NULL;
}

static char* CGGetBeliefsFromLLRs(CodeGraph*const cg, const int verbose) {
  int curVar;
  if (verbose) printf("Doing getbeliefs.\n");
  double*const result = SafeCalloc(cg->numVNodes,sizeof(double));
  for (curVar=0; curVar<cg->numVNodes; curVar++) 
    result[curVar] = 1.0/(1.0+exp(cg->vNodes[curVar].llr));
  return (char*) result;
}

static char* CGGetBeliefsFromLikelihoodRatios(CodeGraph*const cg, 
					      const int verbose) {
  int curVar;
  double*const result = SafeCalloc(cg->numVNodes,sizeof(double));
  for (curVar=0; curVar<cg->numVNodes; curVar++) 
    result[curVar] = 1.0/(1.0+cg->vNodes[curVar].llr);
  return (char*) result;
}

static char* PrepareForSumProductBP(CodeGraph*const cg, const int verbose) {
  int curEdge;
  if (verbose) printf("Preparing for SumProductBP.\n");
  for (curEdge=0; curEdge < cg->numEdges; curEdge++) {
    cg->edges[curEdge].msgToVar = 0;
    cg->edges[curEdge].msgToCheck = 0;
  }
  ResetVariableLLRs(cg->vNodes,cg->numVNodes);
  SendBPMessagesFromChannelEvidenceToVariables(cg->vNodes,cg->numVNodes);
  return NULL;
}

static char* PrepareForLRSumProductBP(CodeGraph*const cg, const int verbose) {
  int curEdge;
  if (verbose) printf("Preparing for LRSumProductBP.\n");
  for (curEdge=0; curEdge < cg->numEdges; curEdge++) {
    cg->edges[curEdge].msgToVar = 1;
    cg->edges[curEdge].msgToCheck = 1;
  }
  ResetVariableLikelihoodRatios(cg->vNodes,cg->numVNodes);
  SendLRBPMessagesFromChannelEvidenceToVariables(cg->vNodes,cg->numVNodes);
  return NULL;
}

static char* DoSumProductBPOnCodeGraph(CodeGraph*const cg, const int verbose){
  if (verbose) printf("Doing SumProductBP.\n");
  if (verbose) printf("-->SendBPMessagesFromVariablesToChecks\n");
  SendBPMessagesFromVariablesToChecks(cg->vNodes,cg->numVNodes);
  if (verbose) printf("-->ResetVariableLLRs\n");
  ResetVariableLLRs(cg->vNodes,cg->numVNodes);
  if (verbose) printf("-->SendBPMessagesFromChecksToVariables\n");
  SendBPMessagesFromChecksToVariables(cg->cNodes,cg->numCNodes,
				      cg->maxLLR,cg->minLLR);
  if (verbose) printf("-->SendBPMessagesFromVariablesToChecks\n");
  SendBPMessagesFromChannelEvidenceToVariables(cg->vNodes,cg->numVNodes); 

  return NULL;
}

static char* CountBadChecksWithLogLikelihoodMsgs(const CodeGraph*const cg,
						 const int verbose) {
  int curCheck, parity, curVar;
  long badChecks;

  badChecks = 0;
  for (curCheck=0; curCheck < cg->numCNodes; curCheck++) {
    parity = 0;
    for(curVar=0; curVar < cg->cNodes[curCheck].numVars; curVar++) {
      parity += cg->cNodes[curCheck].edges[curVar]->var->llr < 0;
    }
    badChecks += (parity % 2);
  }
  return (char*) badChecks;
}

static char* CountBadChecksWithLikelihoodRatioMsgs(const CodeGraph*const cg,
						   const int verbose) {
  int curCheck, parity, curVar;
  long badChecks;
  
  badChecks = 0;
  for (curCheck=0; curCheck < cg->numCNodes; curCheck++) {
    parity = 0;
    for(curVar=0; curVar < cg->cNodes[curCheck].numVars; curVar++) {
      parity += cg->cNodes[curCheck].edges[curVar]->var->llr < 1;
    }
    badChecks += (parity % 2);
  }
  return (char*) badChecks;
}


static char* DoLRSumProductBPOnCodeGraph(CodeGraph*const cg, 
					 const int verbose) {
  if (verbose) printf("Doing LRSumProductBP.\n");
  if (verbose) printf("-->Doing SendLRBPMessagesFromVariablesToChecks.\n");
  SendLRBPMessagesFromVariablesToChecks(cg->vNodes,cg->numVNodes);
  if (verbose) printf("-->Doing ResetVariableLikelihoodRatios.\n");
  ResetVariableLikelihoodRatios(cg->vNodes,cg->numVNodes);
  if (verbose) printf("-->Doing SendLRBPMessagesFromChecksToVariables\n");
  SendLRBPMessagesFromChecksToVariables(cg->cNodes,cg->numCNodes);
  if (verbose)
    printf("-->Doing SendLRBPMessagesFromChannelEvidenceToVariables.\n");
  SendLRBPMessagesFromChannelEvidenceToVariables(cg->vNodes,cg->numVNodes); 

  return NULL;
}


const CodeGraphAlgorithm SumProductBPAlg = {
  "SUM_PRODUCT_BP",
  NULL,
  CGSetEvidence,
  CGGetBeliefsFromLLRs,
  FreeAlgrorithm,
  PrepareForSumProductBP,
  DoSumProductBPOnCodeGraph,
  CountBadChecksWithLogLikelihoodMsgs
};

const CodeGraphAlgorithm LRSumProductBPAlg = {
  "SUM_PRODUCT_BP",
  NULL,
  CGSetEvidence,
  CGGetBeliefsFromLikelihoodRatios,
  FreeAlgrorithm,
  PrepareForLRSumProductBP,
  DoLRSumProductBPOnCodeGraph,
  CountBadChecksWithLikelihoodRatioMsgs
};
