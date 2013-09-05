
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

#include "misc.h"
#include "Nodes.h"
#include "MinSumBP.h"

#include <math.h>
#include <assert.h>

/* ************************************************************
 * 
 * The routines in this file which have headers in MinSumBP.h
 * are designed to be used in min-sum belief propagation.
 * For an example, of how to do this see the example in BSCQuant.c
 * for doing min-sum belief propagation on a DualCodeGraph.
 *
 * ************************************************************ */

/* Note: make sure MINSUM_NO_MESSAGE is positive so that
   fabs(MINSUM_NO_MESSAGE) == MINSUM_NO_MESSAGE */
const double MINSUM_NO_MESSAGE = 987654321;

int IsMinSumErasureP(const double input) {
  return fabs(input) <= 0;
}




/* ---------------------------------------------------------------------
//
// FUNCTION:	SendMinSumMessagesFromChannelEvidenceToVariables
//
// INPUTS:	vars:    An array of variables to get evidence messages.
//              numVars: Number of variables in vars.
//
// PURPOSE:	Take the channel evidence for each variable which is
//              in the form of a cost differential
//                 Cost(channel|var=1)-Cost(channel|var=0) and add it
//              to each variable LLR.  Note that if the llr field
//              of a var is highly positive, then that means the var
//              should be a 0 while if it is highly negative, then
//              the var should be a 1.
//
// MODIFIED:	Mon Jun 09, 2003
//
// --------------------------------------------------------------------*/


void SendMinSumMessagesFromChannelEvidenceToVariables(VariableNode*const vars,
						      const int numVars) {
  int i;

  for (i = 0; i < numVars; i++) {    
    vars[i].llr += vars[i].channelEvidence;
  }
}    


/* ----------------------------------------------------------------------
//
// FUNCTION:	SendMinSumMessagesFromVariablesToChecks
//
// INPUTS:	vars:    Array of vars to send messages from.
//              numVars: Number of vars in array.
//
// PURPOSE:	Go through each var in the input array and compute
//              the message from the var to all checks and put these
//              messages on msgToCheck on the appropriate edges.
//              Normally, we subtract the message a check sent to a var
//              when computing the message form the var to that check.
//              But if the check sent MINSUM_NO_MESSAGE, then the check
//              sent nothing to the var so we don't subtract anything out.
//
// MODIFIED:	Tue Jul 15, 2003
//
// -------------------------------------------------------------------- */

void SendMinSumMessagesFromVariablesToChecks(VariableNode*const vars,
					     const int numVars) {
  int curCheck;
  VariableNode* curVarNode;
  const VariableNode*const varNodesEnd = vars+numVars;

  for(curVarNode=vars; curVarNode < varNodesEnd; curVarNode++) {
    for(curCheck=0; curCheck < curVarNode->numChecks; curCheck++) {
      if (MINSUM_NO_MESSAGE != curVarNode->edges[curCheck]->msgToVar) {
	curVarNode->edges[curCheck]->msgToCheck = curVarNode->llr -
	  curVarNode->edges[curCheck]->msgToVar;
      } else {
	curVarNode->edges[curCheck]->msgToCheck = curVarNode->llr;
      }
    }
  }
}

/* ----------------------------------------------------------------------
//
// FUNCTION:	ComputeTotalAbsAndSignAndSmallestTwo
//
// INPUTS:	check:       A check node to compute outgoing messages for.
//              smallestVar: Gets set to index of var with weakest msg.
//              nextSmallestVar: Gets set to index of var with 2nd weakest msg.
//              parity:      The number of negative messages to check mod 2.
//              numErasures: The number of incoming erasure messages.
//
// PURPOSE:	This function computes the parity, and smallest
//              two vars so that computing the differential benefit
//              for each var can be quickly computed from these values.
//              To compute parity, we start with *parity=0 and toggle
//              it for every negative incoming message.
//
//              An incoming message of MINSUM_NO_MESSAGE is always
//              considered smaller than any other message.  So
//              if more than 1 incoming message is MINSUM_NO_MESSAGE, then
//              check->edges[nextSmallestVar] == MINSUM_NO_MESSAGE.
//              This is useful in determining whether the check can
//              send out a message when doing belief propagation on a tree.
//
// MODIFIED:	Wed Jun 25, 2003
//
// ------------------------------------------------------------------- */

void ComputeParityAndSmallestTwo(const CheckNode*const check,
				 int*const smallestVar,
				 int*const nextSmallestVar,
				 int*const parity,
				 int*const numErasures) {
  int curVar;
  double absVal;
  double smallestValue = 9999999;
  double nextSmallestValue = 9999999+1;
  
  *numErasures=0;
  *parity = 0;
  for (curVar=0; curVar < check->numVars; curVar++) {
    if (MINSUM_NO_MESSAGE == check->edges[curVar]->msgToCheck) {
      absVal = 0; 
      smallestValue = 9999999+1; /* make sure smallextVar <- curVar */
    } else {
      if (check->edges[curVar]->msgToCheck < 0) {
	*parity = *parity ^ 1; /* toggle parity value */
	absVal = - check->edges[curVar]->msgToCheck;
      } else {
	absVal = check->edges[curVar]->msgToCheck;
      }
      if (IsMinSumErasureP(absVal)) {
	(*numErasures)++;
      }
    }
    if (absVal < smallestValue) {
      nextSmallestValue = smallestValue;
      *nextSmallestVar  = *smallestVar;
      smallestValue = absVal;
      *smallestVar = curVar;
    } else if (absVal < nextSmallestValue) {
      nextSmallestValue = absVal;
      *nextSmallestVar = curVar;
    }
  }
  assert(*smallestVar < check->numVars);
  assert(*nextSmallestVar < check->numVars);
}

/* ----------------------------------------------------------------------
//
// FUNCTION:	SendMinSumFromChecksToVariables
//
// INPUTS:	checks:    An array of check nodes which need to send
//                         messages to their associated variables.
//              numChecks: Number of check nodes in the array.
//
// PURPOSE:	Goes through all checks and computes the messages from
//              that check to assoicated variables and updates the 
//              the likelihoods for the variable nodes.
//
//              Imagine that the incoming messages to consider are
//              30, -5, 10, 20 and we are computing the outgoing value
//              for message 0.  To get minVec0 we choose (0,0,0,0) which
//              (assuming the costs for 1 are 0 and the costs for zero
                 are 30, -5, 10, 20) gives a total cost of 
//              -[30-5+10+20 - 30] = -25 since we have
//              to ignore the incoming message from var 0.  To get minVec1
//              we choose (1,1,0,0) which gives a total differential cost of
//              [30  +10+20 - 30] = -30.  Thus the message we should send
//              is -5 to indicate that the cost of choosing var 0 to
//              be a 1 is 5 lower than choosing it to be a 0 if the
//              other elements in the vector are chosen to minimize the cost.
//
//              In general minVec0 and minVec1 will differ only in the sign 
//              of the weakest term so the difference will be 
//              the absolute value of the weakest term.  Whether 0 or 1
//              is favored depends on how many input messages want to
//              be a 1 or 0.  If an odd number of incoming messages want
//              to be a 1 then a 1 is favored.
//
// MODIFIED:	Wed Jun 25, 2003
//
// ---------------------------------------------------------------------*/

void SendMinSumMessagesFromChecksToVariables(CheckNode*const checks,
					     const int numChecks) {
  int curVar, smallestVar, nextSmallestVar, parity, numErasures;
  CheckNode* curCheckNode;
  const CheckNode*const checkNodesEnd = checks+numChecks;

  for(curCheckNode=checks; curCheckNode<checkNodesEnd; curCheckNode++) {
    if (curCheckNode->numVars == 2) {
      curCheckNode->edges[0]->msgToVar = curCheckNode->edges[1]->msgToCheck;
      curCheckNode->edges[1]->msgToVar = curCheckNode->edges[0]->msgToCheck;
      if (MINSUM_NO_MESSAGE != curCheckNode->edges[1]->msgToCheck)
	curCheckNode->edges[0]->var->llr += curCheckNode->edges[1]->msgToCheck;
      if (MINSUM_NO_MESSAGE != curCheckNode->edges[0]->msgToCheck)
	curCheckNode->edges[1]->var->llr += curCheckNode->edges[0]->msgToCheck;
    } else if (curCheckNode->numVars > 2) {
      ComputeParityAndSmallestTwo(curCheckNode,&smallestVar,
				  &nextSmallestVar,&parity,&numErasures);
      for (curVar=0; curVar < curCheckNode->numVars; curVar++) {
	if (curVar != smallestVar) {
	  curCheckNode->edges[curVar]->msgToVar = 
	    fabs(curCheckNode->edges[smallestVar]->msgToCheck);
	} else {
	  curCheckNode->edges[curVar]->msgToVar = 
	    fabs(curCheckNode->edges[nextSmallestVar]->msgToCheck);
	}
	if (MINSUM_NO_MESSAGE != curCheckNode->edges[curVar]->msgToVar) {
	  if (((parity==0) && (curCheckNode->edges[curVar]->msgToCheck < 0)) ||
	      ((parity==1) && (curCheckNode->edges[curVar]->msgToCheck >= 0))){
	    curCheckNode->edges[curVar]->msgToVar =  
	      -curCheckNode->edges[curVar]->msgToVar;
	  }
	  curCheckNode->edges[curVar]->var->llr += 
	    curCheckNode->edges[curVar]->msgToVar;
	}
      }
    } else {
      printf("check had %i vars, aborting!\n",curCheckNode->numVars);
      abort();
    }
  }
}
