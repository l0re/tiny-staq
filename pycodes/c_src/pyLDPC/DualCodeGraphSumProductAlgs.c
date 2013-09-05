
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

#include <string.h>

#include "misc.h"
#include "DualCodeGraphAlgorithms.h"
#include "SumProductBP.h"
#include "DualCodeGraphSumProductAlgs.h"

char* DoNothing(DualCodeGraph*const dc, const int verbose) {
  return NULL;
}


static char* SetEvidenceFromLR(DualCodeGraph*const dc, const int verbose,
			       void*const evPtr) {
  const double inf = 1.0/0.00000001;
  double*const ev = (double*) evPtr;
  int i;
  /* All the hidden nodes shouldn't affect decoding so we set them to
   * have evidence of 1 and start their llr out at inf so that the
   * first message they send doesn't affect any checks and later their
   * evidence doesn't affect any other messages. */
  for (i=0; i < dc->k; i++) {
    dc->vNodes[i].channelEvidence = 1;
    dc->vNodes[i].llr = inf;
  }
  for (i=0; i < dc->n; i++) {
    dc->vNodes[i+dc->k].channelEvidence = ev[i];
    dc->vNodes[i+dc->k].llr = 1;
  }
  return NULL;
}

static char* GetBeliefsFromLR(DualCodeGraph*const dc, const int verbose) {
  int curVar;
  double*const result = SafeCalloc(dc->numVNodes,sizeof(double));
  for (curVar=0; curVar<dc->numVNodes; curVar++) 
    result[curVar] = 1.0/(1.0+dc->vNodes[curVar].llr);
  return (char*) result;
}



static char* PrepareForLRSumProductBPOnDualCodeGraph(DualCodeGraph*const dc, 
						     const int verbose) {
  int curEdge;

  for(curEdge=0; curEdge<dc->numEdges; curEdge++) {
    dc->edges[curEdge].msgToVar = 1;
    dc->edges[curEdge].msgToCheck = 1;
  }
  return NULL;
}

static char* DoLRSumProductBP(DualCodeGraph*const dc, 
			      const int verbose) {
  SendLRBPMessagesFromChannelEvidenceToVariables(dc->vNodes,dc->numVNodes); 
  SendLRBPMessagesFromVariablesToChecks(dc->vNodes,dc->numVNodes);
  ResetVariableLikelihoodRatios(dc->vNodes,dc->numVNodes);
  SendLRBPMessagesFromChecksToVariables(dc->cNodes,dc->numCNodes);

  return NULL;
}

const DualCodeGraphAlgorithm LRSumProductBP = {
  "LR_SUM_PRODUCT_BP",
  NULL,
  SetEvidenceFromLR,
  GetBeliefsFromLR,
  DoNothing,
  PrepareForLRSumProductBPOnDualCodeGraph,
  DoLRSumProductBP
};

