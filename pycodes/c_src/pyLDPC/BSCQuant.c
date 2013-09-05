
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "misc.h"
#include "DualCodeGraph.h"
#include "SumProductBP.h"
#include "MinSumBP.h"
#include "VPStack.h"
#include "VPSet.h"
#include "DualCodeGraphAlgorithms.h"

const double BSC_FREEZE_VALUE = 11111;
const double BSC_TIE_BREAK_THRESHOLD = 0;

const double gTIE_BREAK_VAL = 12;

static double gNO_MSG_VAL;

const static double gTolerance=.000001; /*tolerance for equality comparisons*/

static char* BSCSetEvidence(DualCodeGraph* dc, const int verbose,
			    void * evPtr) {
  int curVar;  
  const double*const ev = (const double*const) evPtr;
  for(curVar=0; curVar < dc->n; curVar++) {
    dc->vNodes[dc->k + curVar].channelEvidence = ev[curVar];
  }
  return NULL;
}



void PrintStats(const DualCodeGraph*const dc) {
  int i, counts[7];

  for (i=0; i<=6; i++) counts[i]=0;
  printf("\nstats: ");
  for (i=0; i<dc->k; i++) counts[(int) ( (dc->vNodes[i].llr + 6.0)/2.0)]++;
  for (i=0; i<=6; i++) printf("%i:%f ",i,((double)counts[i])/((double)dc->k));
}

static char* BSCGetBeliefsFromChannelEvidence(DualCodeGraph* dc, const int verbose) {
  int curVar;
  double* ev = SafeCalloc(dc->numVNodes,sizeof(double));

  for(curVar=0; curVar < dc->numVNodes; curVar++) {
    ev[curVar] = dc->vNodes[curVar].channelEvidence;
  }
 
  return (char*) ev;	
}

static char* BSCGetBeliefsFromLLRs(DualCodeGraph* dc, const int verbose) {
  int curVar;
  double* ev = SafeCalloc(dc->numVNodes,sizeof(double));

  for(curVar=0; curVar < dc->numVNodes; curVar++) {
    ev[curVar] = dc->vNodes[curVar].llr;
  }
  return (char*) ev;	
}

static void DCClearMessagesForBSC(DualCodeGraph*const dc) {
  int i;
  for (i=0; i < dc->k; i++) {
    dc->vNodes[i].channelEvidence = 0;
    dc->vNodes[i].varType = 'h';
  }
  for (i=0; i < dc->numVNodes; i++) dc->vNodes[i].llr = 0;
  for (i=0; i < dc->numEdges; i++) {
    dc->edges[i].msgToVar = gNO_MSG_VAL;
    dc->edges[i].msgToCheck = gNO_MSG_VAL;
  }
}

static void PrepareDCForBSCQuantization(DualCodeGraph*const dc) {
  
  gNO_MSG_VAL = gEffectivelyZero;
  /* gNO_MSG_VAL = MINSUM_NO_MESSAGE; */

  DCClearMessagesForBSC(dc);
}


typedef struct SimAnnealingClientData {
  double temperature;
  double temperatureScaling;
} SimAnnealingClientData;

static char* DeallocateSimulatedAnnealingBSCQuant(DualCodeGraph* dc, 
					      const int verbose) {
  assert(0 == strcmp(dc->algorithm->algorithmName,
		     "SIMULATED_ANNEALING_BSC_QUANT"));
  free(dc->algorithm->clientData);
  dc->algorithm->clientData = NULL;
  dc->algorithm = NULL;
  return NULL;
}

static char* PrepareForSimulatedAnnealingBSCQuant(DualCodeGraph* dc, 
						  const int verbose) {
  VariableNode* curVarNode;
  const VariableNode*const lastVar = dc->vNodes + dc->k;
  
  assert(0 == strcmp(dc->algorithm->algorithmName,
		     "SIMULATED_ANNEALING_BSC_QUANT"));
  PrepareDCForBSCQuantization(dc);

  dc->algorithm->clientData = SafeMalloc(sizeof(SimAnnealingClientData));
  ((SimAnnealingClientData*) dc->algorithm->clientData)->temperature = 30;
  ((SimAnnealingClientData*) dc->algorithm->clientData)
    ->temperatureScaling = .993;

  for (curVarNode=dc->vNodes; curVarNode < lastVar; curVarNode++) {
    if (0 == curVarNode->channelEvidence || 
	gNO_MSG_VAL == curVarNode->channelEvidence) {
      assert(curVarNode->varType == 'h');
      if (0 == curVarNode->llr) curVarNode->llr = .1;
      curVarNode->channelEvidence = curVarNode->llr;
    }
  }

  return NULL;
}

double ComputeLocalCostForVar(const VariableNode*const);
static char* DoSimulatedAnnealingBSCQuant(DualCodeGraph* dc, 
					  const int verbose) {
  double prob, costForStaying, costForFlipping;
  VariableNode* curVarNode;
  SimAnnealingClientData* cd;
  const VariableNode*const lastVar = dc->vNodes + dc->k;

  cd = (SimAnnealingClientData *) dc->algorithm->clientData;
  cd->temperature *= cd->temperatureScaling;

  for (curVarNode=dc->vNodes; curVarNode < lastVar; curVarNode++) {
    if (0 == curVarNode->llr) {
      curVarNode->llr = 1;
      curVarNode->channelEvidence = curVarNode->llr;
    }
    costForStaying = ComputeLocalCostForVar(curVarNode);
    curVarNode->llr = - curVarNode->llr;
    curVarNode->channelEvidence = curVarNode->llr;
    costForFlipping = ComputeLocalCostForVar(curVarNode);
    curVarNode->llr = - curVarNode->llr;
    curVarNode->channelEvidence = curVarNode->llr;
    prob = exp( (costForStaying-costForFlipping) / 
		cd->temperature );
    if (((double)random())/RAND_MAX < prob) {
      curVarNode->llr = - curVarNode->llr;
      curVarNode->channelEvidence = curVarNode->llr;
    }
  }
  
  return NULL;
}

typedef struct BitFlipClientData {
  VPStack* varsToTryThisIter;
  VPSet* varsToTryNextIter;
} BitFlipClientData;

static void DestroyBitFlipQuantClientData(void*const cd) {
  BitFlipClientData* clientData;

  clientData = (BitFlipClientData*) cd;
  VPStackDestroy(clientData->varsToTryThisIter);
  VPSetDestroy(clientData->varsToTryNextIter);
  free(clientData);
}


static void SendBitFlipMessagesFromEvidenceToVars(VariableNode*const vars,
						  const int numVars) {
  VariableNode* curVarNode;
  VariableNode* lastVar = vars + numVars;
  
  for (curVarNode=vars; curVarNode < lastVar; curVarNode++) {
    curVarNode->llr = curVarNode->channelEvidence;
  }
}

static void SendBitFlipMessagesFromVarsToChecks(VariableNode*const vars,
						const int numVars) {
  int curCheck;
  VariableNode* curVarNode;
  VariableNode* lastVar = vars + numVars;
  
  for (curVarNode=vars; curVarNode < lastVar; curVarNode++) {
    for (curCheck=0; curCheck < curVarNode->numChecks; curCheck++) {
      curVarNode->edges[curCheck]->msgToCheck = curVarNode->channelEvidence;
    }
  }
}

static void SendBitFlipMsgFromCheckToVars(CheckNode*const curCheckNode) {
  int curVar, parity = 0, indexForObsVar=-1;
  double obsVarAbsCost, msgToSend;

  for (curVar=0; curVar < curCheckNode->numVars; curVar++) {
    parity ^= (curCheckNode->edges[curVar]->msgToCheck < 0);
    if ('o' == curCheckNode->edges[curVar]->var->varType) {
      assert(-1 == indexForObsVar);
      indexForObsVar = curVar;
      obsVarAbsCost = fabs(curCheckNode->edges[curVar]->var->channelEvidence);
    }
  }
  assert(-1 != indexForObsVar);
  msgToSend = parity ? obsVarAbsCost : -obsVarAbsCost;
  for (curVar=0; curVar < curCheckNode->numVars; curVar++) {
    curCheckNode->edges[curVar]->var->llr -= 
      curCheckNode->edges[curVar]->msgToVar; /* subtract previous message */
    curCheckNode->edges[curVar]->var->llr += msgToSend; /* add new msg */
    curCheckNode->edges[curVar]->msgToVar = msgToSend;
  }
}
					    

static void SendBitFlipMessagesFromChecksToVars(CheckNode*const checks,
						const int numChecks) {
  CheckNode* curCheckNode;
  CheckNode* lastCheck = checks + numChecks;
  
  for(curCheckNode = checks; curCheckNode < lastCheck; curCheckNode++) {
    SendBitFlipMsgFromCheckToVars(curCheckNode);
  }
}


double ComputeCheckCost(const CheckNode*const check) {
  int curVar, parity = 0;
  double absCostForObsVar = gNO_MSG_VAL;

  for(curVar=0; curVar < check->numVars; curVar++) {
    switch (check->edges[curVar]->var->varType) {
    case 'h':
      parity ^= check->edges[curVar]->var->channelEvidence < 0;
      break;
    case 'o':
      assert (0 != check->edges[curVar]->var->channelEvidence &&
	      gNO_MSG_VAL != check->edges[curVar]->var->channelEvidence);
      parity ^= check->edges[curVar]->var->channelEvidence < 0;
      assert(gNO_MSG_VAL == absCostForObsVar);
      absCostForObsVar = fabs(check->edges[curVar]->var->channelEvidence);
      break;
    default:
      abort();
    }
  }
  assert(gNO_MSG_VAL != absCostForObsVar);
  return parity ? absCostForObsVar : - absCostForObsVar;
}


/* ----------------------------------------------------------------------
//
// FUNCTION:	ComputeLocalCostForVar
//
// INPUTS:	var: A variable node.
//
// RETURNS:	Cost of the variable having current llr value.
//              If var->llr != 0, then negating var->llr will negate
//              the result.
//
// MODIFIED:	Tue Jul 22, 2003
//
// ------------------------------------------------------------------- */

double ComputeLocalCostForVar(const VariableNode*const var) {
  int curCheck;
  double cost=0;

  for (curCheck=0; curCheck<var->numChecks; curCheck++) {
    cost += ComputeCheckCost(var->edges[curCheck]->check); 
  }
  return cost;
}


static void FlipVar(VariableNode*const var) {
  int curCheck;

  assert(fabs(var->llr-ComputeLocalCostForVar(var)) <= gTolerance);
  assert(var->channelEvidence != 0);
  assert('h' == var->varType);
  assert(-1 == var->channelEvidence || 1 == var->channelEvidence);
  var->channelEvidence = - var->channelEvidence;
  
  for(curCheck=0; curCheck < var->numChecks; curCheck++) {
    var->edges[curCheck]->msgToCheck = var->channelEvidence;
    SendBitFlipMsgFromCheckToVars(var->edges[curCheck]->check);
  }
  assert(fabs(var->llr-ComputeLocalCostForVar(var)) <= gTolerance);
}

static int FlipVarIfPossible(VariableNode*curVarNode) {
  assert('h' == curVarNode->varType);
  assert(fabs(curVarNode->llr-ComputeLocalCostForVar(curVarNode))<=gTolerance);
  if (curVarNode->llr >= 0) { /* Note that >= is required not just > */
    FlipVar(curVarNode);
    return 1;
  } else
    return 0;
}


static int FlipVarIfPossibleAndTrackNeighbors(VariableNode* var,
					      VPStack*const varsToPursue) {
  int i, curCheck;
  CheckNode* check;

  assert('h' == var->varType);
  assert(fabs(var->llr-ComputeLocalCostForVar(var)) <= gTolerance);
  if (var->llr >= 0) { /* Note that >= is required not just > */
    FlipVar(var);
    if (0 == var->llr) VPStackPush(varsToPursue,var);
    for (curCheck=0; curCheck < var->numChecks; curCheck++) {
      check = var->edges[curCheck]->check;
      for (i=0; i < check->numVars; i++) {
	if (var == check->edges[i]->var) continue;
	switch (check->edges[i]->var->varType) {
	case 'h':
	  if (check->edges[i]->var->llr >= 0) {
	    VPStackPush(varsToPursue,check->edges[i]->var);
	  } else {
	    ;
	  }
	  break;
	case 'o':
	  break;
	default:
	  printf("Unexpected case in FlipVarIfPossibleAndTrackNeighbors.\n");
	  abort();
	}
      }
    }
    return 1;
  } else
    return 0;
}


static void PutVarInStack(VariableNode*const var, VPStack*const stack) {
  VPStackPush(stack,var);
}

char* DeallocateBitFlipBSCQuant(DualCodeGraph*const dc, const int verbose) {
  assert(0 == strcmp(dc->algorithm->algorithmName,
		     "ACCELERATED_BIT_FLIP_BSC_QUANT")||
	 0 == strcmp(dc->algorithm->algorithmName,"PURE_BIT_FLIP_BSC_QUANT"));
  DestroyBitFlipQuantClientData((BitFlipClientData*)dc->algorithm->clientData);
  dc->algorithm->clientData = NULL;
  dc->algorithm = NULL;

  return NULL;
}

char* PrepareForBitFlipBSCQuant(DualCodeGraph*const dc, const int verbose) {
  VariableNode* curVarNode;
  const VariableNode* lastVar = dc->vNodes + dc->k;
  BitFlipClientData* clientData = SafeMalloc(sizeof(BitFlipClientData));

  assert(0 == strcmp(dc->algorithm->algorithmName,
		     "ACCELERATED_BIT_FLIP_BSC_QUANT")||
	 0 == strcmp(dc->algorithm->algorithmName,"PURE_BIT_FLIP_BSC_QUANT"));
  assert(NULL == dc->algorithm->clientData);
  dc->algorithm->clientData = clientData;

  PrepareDCForBSCQuantization(dc);

  clientData->varsToTryNextIter = VPSetCreate();
  clientData->varsToTryThisIter = VPStackCreate(20);

  SendBitFlipMessagesFromEvidenceToVars(dc->vNodes,dc->numVNodes);

  for (curVarNode=dc->vNodes; curVarNode < lastVar; curVarNode++) {
    assert(curVarNode->varType == 'h');
    assert(0 == curVarNode->channelEvidence);
    assert(gNO_MSG_VAL == curVarNode->llr || 0 == curVarNode->llr);
    curVarNode->channelEvidence = 1;
    curVarNode->llr = 0;
    VPSetPush(clientData->varsToTryNextIter,curVarNode);
  }

  SendBitFlipMessagesFromVarsToChecks(dc->vNodes,dc->numVNodes);
  SendBitFlipMessagesFromChecksToVars(dc->cNodes,dc->numCNodes);

  return NULL;
}

char* DoPureBitFlipBSCQuant(DualCodeGraph* dc, const int verbose) {
  VariableNode* curVarNode;
  VariableNode* lastVar = dc->vNodes + dc->k;
  BitFlipClientData* clientData =(BitFlipClientData*)dc->algorithm->clientData;
  int varsFlipped = 0;

  VPStackClear(clientData->varsToTryThisIter);
  for (curVarNode=dc->vNodes; curVarNode < lastVar; curVarNode++) {
    VPStackPush(clientData->varsToTryThisIter,curVarNode);
  }


  while (VPStackSize(clientData->varsToTryThisIter) > 0) {
    VPStackSwapTopToRandomPos(clientData->varsToTryThisIter);
    curVarNode = VPStackPop(clientData->varsToTryThisIter); 
    if (FlipVarIfPossible(curVarNode))
      varsFlipped++;
  }

  return NULL;
}

int DCCountBadChecks(const DualCodeGraph*const dc) {
  int curVar, parity, badChecks=0;
  const CheckNode*const lastCheck = dc->cNodes+dc->numCNodes;
  CheckNode* curCheckNode;

  for (curCheckNode=dc->cNodes; curCheckNode < lastCheck; curCheckNode++) {
    parity = 0;
    for (curVar=0; curVar < curCheckNode->numVars; curVar++) {
      if ('o' == curCheckNode->edges[curVar]->var->varType) 
	parity = parity ^ 
	  (curCheckNode->edges[curVar]->var->channelEvidence < 0);
      else
	parity = parity ^ (curCheckNode->edges[curVar]->var->llr < 0);
    }
    badChecks += parity;
  }
  return badChecks;
}


int DCCountBadChecksFromChEv(const DualCodeGraph*const dc) {
  CheckNode* curCheckNode;
  CheckNode* lastCheck = dc->cNodes + dc->numCNodes;
  int count=0;

  for (curCheckNode=dc->cNodes; curCheckNode < lastCheck; curCheckNode++) {
    count += (1 == ComputeCheckCost(curCheckNode));
  }

  return count;
}

char* DoAcceleratedBitFlipBSCQuant(DualCodeGraph*const dc, const int verbose) {
  VariableNode* curVarNode;
  BitFlipClientData* clientData =(BitFlipClientData*)dc->algorithm->clientData;
  VPSet*const alreadyTriedVars = VPSetCreate();
  VPStack*const varsToPursue = VPStackCreate(20);

  assert(VPStackEmpty(clientData->varsToTryThisIter));
  VPSetForeach2(clientData->varsToTryNextIter,(Foreach2Func)PutVarInStack,
		clientData->varsToTryThisIter);
  VPSetClear(clientData->varsToTryNextIter);

  while (VPStackSize(clientData->varsToTryThisIter) > 0) {
    VPStackSwapTopToRandomPos(clientData->varsToTryThisIter);
    curVarNode = VPStackPop(clientData->varsToTryThisIter); 
    VPSetPush(alreadyTriedVars,curVarNode);
    FlipVarIfPossibleAndTrackNeighbors(curVarNode,varsToPursue);
      /* What about the vars which were flippable but became
       * unflippable when we flip curVarNode?  Should we
       * go to the effort of removing them from 
       * clientData->varsToTryNextIter? */
    while (! VPStackEmpty(varsToPursue)) {
      curVarNode = VPStackPop(varsToPursue);
      VPSetPush(clientData->varsToTryNextIter,curVarNode);
      if (! VPSetMemberP(alreadyTriedVars,curVarNode)) {
	VPSetPush(alreadyTriedVars,curVarNode);
	VPStackPush(clientData->varsToTryThisIter,curVarNode);
      }
    }
  }
  if (verbose) printf("bad checks after iter = %i\n",
		      DCCountBadChecksFromChEv(dc));

  VPStackDestroy(varsToPursue);
  VPSetDestroy(alreadyTriedVars);
  free(varsToPursue);
  free(alreadyTriedVars);
  return NULL;
}

static void ForcedTreeMinSumQueryVar(VariableNode*const, GraphEdge*const,
				     VPSet*const );

static void ForcedTreeMinSumQueryCheck(CheckNode*const check,
				       GraphEdge*const askingEdge,
				       VPSet*const checksToQuery) {
  int smallestVar, nextSmallestVar, parity, numErasures, curEdge;
  assert(VPSetMemberP(checksToQuery,check));
  assert(gNO_MSG_VAL == askingEdge->msgToCheck);

  for (curEdge=0; curEdge<check->numVars; curEdge++) 
    if (askingEdge != check->edges[curEdge])
      ForcedTreeMinSumQueryVar(check->edges[curEdge]->var,
			       check->edges[curEdge],checksToQuery);
  
  ComputeParityAndSmallestTwo(check,&smallestVar,&nextSmallestVar,
			      &parity,&numErasures);
  assert(askingEdge == check->edges[smallestVar]);
  assert(gNO_MSG_VAL != check->edges[nextSmallestVar]->msgToCheck);
  askingEdge->msgToVar = parity ? 
    -fabs(check->edges[nextSmallestVar]->msgToCheck) :
    fabs(check->edges[nextSmallestVar]->msgToCheck);

}

static void ForcedTreeFreezeVar(VariableNode*const var) {
  int curEdge;

  assert('h' == var->varType);
  var->varType = 'F';
  if (0 == var->llr) 
    var->channelEvidence = (random()%2) ? BSC_FREEZE_VALUE : -BSC_FREEZE_VALUE;
  else
    var->channelEvidence = 128 * var->llr;
  var->llr = var->channelEvidence;
  for (curEdge=0; curEdge<var->numChecks; curEdge++) {
    var->edges[curEdge]->msgToCheck = var->llr;
  }
}

static int ForcedTreeFreezeVarIfNecessary(VariableNode*const var,
					  const GraphEdge*const askingEdge,
					  VPSet*const checksToQuery) {
  int curEdge;

  for (curEdge=0; curEdge<var->numChecks; curEdge++) {
    if (askingEdge != var->edges[curEdge] &&
	VPSetMemberP(checksToQuery,var->edges[curEdge]->check)) {
      ForcedTreeFreezeVar(var);
      return 1;
    } 
  }
  return 0;
}

static void ForcedTreeMinSumQueryVar(VariableNode*const var,
				     GraphEdge*const askingEdge,
				     VPSet*const checksToQuery) {
  int curEdge;
  double msgOut = 0;

  switch (var->varType) {
  case 'o':
    assert(1 == var->numChecks);
    var->edges[0]->msgToCheck = var->channelEvidence;
    break;
  case 'F': /* Var is frozen so all its messages have already been placed */
    break; /*  on the appropraite edges.  Thus we do nothing. */
  case 'h':
    if (ForcedTreeFreezeVarIfNecessary(var,askingEdge,checksToQuery)) return;
    else {
      for (curEdge=0; curEdge<var->numChecks; curEdge++) {
	VPSetPush(checksToQuery,var->edges[curEdge]->check);
      }
      for (curEdge=0; curEdge<var->numChecks; curEdge++) {
	if (askingEdge != var->edges[curEdge]) {
	  ForcedTreeMinSumQueryCheck(var->edges[curEdge]->check,
				     var->edges[curEdge], checksToQuery);
	  assert(gNO_MSG_VAL != var->edges[curEdge]->msgToVar);
	  msgOut += var->edges[curEdge]->msgToVar;
	}
      }
      askingEdge->msgToCheck=msgOut;
      assert('h' == var->varType);
      var->varType = 'c';
    }
    break;
  default:
    printf("ForcedTreeMinSumQueryVar: unexpected type for var '%c'.\n",
	   var->varType);
    abort();
  }
}

static void PrepareForForcedTreeMinSumIter(DualCodeGraph*const dc) {
  int i;

  for (i=0; i < dc->k; i++) {
    if ('F' == dc->vNodes[i].varType) {
      if (BSC_FREEZE_VALUE == dc->vNodes[i].channelEvidence)
	dc->vNodes[i].llr = 1;
      else if (-BSC_FREEZE_VALUE == dc->vNodes[i].channelEvidence)
	dc->vNodes[i].llr = -1;
      else 
	dc->vNodes[i].llr = dc->vNodes[i].channelEvidence / 128;
    }
    dc->vNodes[i].varType = 'h';
    dc->vNodes[i].channelEvidence = 0;
  }

  for (i=0; i < dc->numEdges; i++) {
    dc->edges[i].msgToCheck = gNO_MSG_VAL;
    dc->edges[i].msgToVar = gNO_MSG_VAL;
  }
}

static void PutMessageForVarOnEdge(CheckNode*const check, const int edgeNum) {
  int smallestVar, nextSmallestVar, parity, numErasures;

  ComputeParityAndSmallestTwo(check,&smallestVar,&nextSmallestVar,
			      &parity,&numErasures);
  if (edgeNum == smallestVar) {
    assert(gNO_MSG_VAL != check->edges[nextSmallestVar]->msgToCheck);
    check->edges[edgeNum]->msgToVar = parity ? 
      -fabs(check->edges[nextSmallestVar]->msgToCheck) :
      fabs(check->edges[nextSmallestVar]->msgToCheck);
  } else {
    assert(gNO_MSG_VAL != check->edges[smallestVar]->msgToCheck);
    check->edges[edgeNum]->msgToVar = parity ? 
      -fabs(check->edges[smallestVar]->msgToCheck) :
      fabs(check->edges[smallestVar]->msgToCheck);
  }
  if (check->edges[edgeNum]->msgToCheck < 0) 
    check->edges[edgeNum]->msgToVar = - check->edges[edgeNum]->msgToVar;
}

static void PropagateMessageFromVarAndComputeLLRs(VariableNode*const var,
						  const GraphEdge*const
						  propagatingEdge);

static void PropagateMessageFromCheckAndComputeLLRs(GraphEdge*const
						    propagatingEdge) {
  int curEdge;
  CheckNode*const check = propagatingEdge->check;
  
  for (curEdge=0; curEdge<check->numVars; curEdge++) {
    if (propagatingEdge != check->edges[curEdge] &&
	gNO_MSG_VAL == check->edges[curEdge]->msgToVar) {
      switch (check->edges[curEdge]->var->varType) {
      case 'c':
	PutMessageForVarOnEdge(check,curEdge);
	PropagateMessageFromVarAndComputeLLRs(check->edges[curEdge]->var,
					      check->edges[curEdge]);
	break;
      case 'F':
      case 'o':
	break;
      default:
	printf("PropagateMessageFromCheckAndComputeLLRs: unexpected type %c\n",
	       check->edges[curEdge]->var->varType);
	abort();
      }
    }
  }
}

static void PropagateMessageFromVarAndComputeLLRs(VariableNode*const var,
						  const GraphEdge*const
						  propagatingEdge) {
  int curEdge;
  VPStack*const checksToPropagateTo = VPStackCreate(8);
  assert('c' == var->varType);
  var->llr = 0;
  for (curEdge=0; curEdge<var->numChecks; curEdge++) {
    assert(gNO_MSG_VAL != var->edges[curEdge]->msgToVar);
    var->llr += var->edges[curEdge]->msgToVar;
  }
  if (0 == var->llr) { 
    /* The following is TRICKY. 
     * var has cost 0 so set it to be consistent with the message it 
     * it sent out if possible or randomly set it otherwise. */
    if (NULL != propagatingEdge) var->llr = propagatingEdge->msgToCheck;
    else (var->llr) = (random()%2) ? gTIE_BREAK_VAL : -gTIE_BREAK_VAL; 
    assert(0 != var->llr); 
    for (curEdge=0; curEdge<var->numChecks; curEdge++) {
      if (propagatingEdge != var->edges[curEdge] &&
	  gNO_MSG_VAL == var->edges[curEdge]->msgToCheck) {
	var->edges[curEdge]->msgToCheck = 0 - var->edges[curEdge]->msgToVar;
	VPStackPush(checksToPropagateTo,var->edges[curEdge]);
      }
    }
  } else {
    for (curEdge=0; curEdge<var->numChecks; curEdge++) {
      if (propagatingEdge != var->edges[curEdge] &&
	  gNO_MSG_VAL == var->edges[curEdge]->msgToCheck) {
	var->edges[curEdge]->msgToCheck = var->llr - 
	  var->edges[curEdge]->msgToVar;
	VPStackPush(checksToPropagateTo,var->edges[curEdge]);
      }
    }
  }
  VPStackForeach(checksToPropagateTo,
		 (ForeachFunc)PropagateMessageFromCheckAndComputeLLRs);
  VPStackDestroy(checksToPropagateTo);
  free(checksToPropagateTo);
}

static void ReportForcedTreeStats(const DualCodeGraph*const dc) {
  static int oldBadChecks=999999;
  int curVar, badChecks, frozenVars = 0, computedVars = 0, isolatedVars = 0;

  for(curVar=0; curVar < dc->k; curVar++) {
    /* printf("%4.3f  ",dc->vNodes[curVar].llr); */
    switch (dc->vNodes[curVar].varType) {
    case 'F': frozenVars++;
      break;
    case 'c': computedVars++;
      break;
    case 'h': isolatedVars++;
      break;
    default:
      printf("ReportForcedTreeStats: unexpected type for var %i:'%c'.\n",
	     curVar,dc->vNodes[curVar].varType);
    }
  }

  assert(dc->k == isolatedVars + computedVars + frozenVars);
  printf("isolated/computed/frozen = %i/%i/%i\n",
	 isolatedVars,computedVars,frozenVars);
  badChecks = DCCountBadChecks(dc);
  printf("bad checks = %i\n",badChecks);
  assert(badChecks<=oldBadChecks);
  oldBadChecks = badChecks;
}

static char* DoForcedTreeMinSumBSCQuant(DualCodeGraph*const dc, 
					const int verbose) {
  GraphEdge sentinel;
  int curVar;
  VariableNode* curVarToQuery;
  VariableNode* firstVarToQuery;
  static int curVarToStartWith = 0;
  VPSet*const checksToQuery = VPSetCreate();
  VPStack*const varsToQuery = VPStackCreate(8);

  curVarToStartWith = (curVarToStartWith+1)%dc->k;
  firstVarToQuery = dc->vNodes + curVarToStartWith;

  gNO_MSG_VAL = MINSUM_NO_MESSAGE;
  
  PrepareForForcedTreeMinSumIter(dc);

  for (curVar=0; curVar<dc->k; curVar++) 
    VPStackPush(varsToQuery,dc->vNodes + curVar);
  VPStackPush(varsToQuery,firstVarToQuery);

  /* The following is TRICKY.  First we query firstVarToQuery and then  *
   * we query any var which was isolated and hasn't gotten a value yet. */
  while ( ! VPStackEmpty(varsToQuery) ) {
    curVarToQuery = VPStackPop(varsToQuery);
    if ('h' == curVarToQuery->varType) {
      VPSetClear(checksToQuery);
      ForcedTreeMinSumQueryVar(curVarToQuery, &sentinel, checksToQuery);
      PropagateMessageFromVarAndComputeLLRs(curVarToQuery, NULL);
    }
  }

  ReportForcedTreeStats(dc);

  VPSetDestroy(checksToQuery);
  VPStackDestroy(varsToQuery);
  free(varsToQuery);
  free(checksToQuery);
  return NULL;
}

const DualCodeGraphAlgorithm ForcedTreeMinSumBSCQuantAlg = {
  "FORCED_TREE_MIN_SUM_BSC_QUANT",
  NULL,
  BSCSetEvidence,
  BSCGetBeliefsFromLLRs,
  DoNothing,
  DoNothing,
  DoForcedTreeMinSumBSCQuant
};


const DualCodeGraphAlgorithm AcceleratedBitFlipBSCQuantAlg = {
  "ACCELERATED_BIT_FLIP_BSC_QUANT",
  NULL,
  BSCSetEvidence,
  BSCGetBeliefsFromChannelEvidence,
  DeallocateBitFlipBSCQuant,
  PrepareForBitFlipBSCQuant,
  DoAcceleratedBitFlipBSCQuant
};


const DualCodeGraphAlgorithm PureBitFlipBSCQuantAlg = {
  "PURE_BIT_FLIP_BSC_QUANT",
  NULL,
  BSCSetEvidence,
  BSCGetBeliefsFromChannelEvidence,
  DeallocateBitFlipBSCQuant,
  PrepareForBitFlipBSCQuant,
  DoPureBitFlipBSCQuant
};


const DualCodeGraphAlgorithm SimulatedAnnealingBSCQuantAlg = {
  "SIMULATED_ANNEALING_BSC_QUANT",
  NULL,
  BSCSetEvidence,
  BSCGetBeliefsFromChannelEvidence,
  DeallocateSimulatedAnnealingBSCQuant,
  PrepareForSimulatedAnnealingBSCQuant,
  DoSimulatedAnnealingBSCQuant
};

