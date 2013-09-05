
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
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "misc.h"
#include "CodeGraph.h"
#include "SumProductBP.h"
#include "BECQuant.h" 
#include "VPSet.h"
#include "VPStack.h"
#include "DualCodeGraphAlgorithms.h"

#define DONT_KNOW 0
#define ZERO 1
#define ONE 2
#define ERASURE 3
#define BEC_ERR 4

#define NUM_BEC_SYMS 5


/* This file contains code to implement a message passing algorithm 
 * for binary erasure channel (BEC) quantization.  
 *
 * The idea is that we are given a word, w containing 0,1 and ? and
 * must quantize w to a codeword.  In doing the quantization we are
 * allowed to quantize ? to anything but can't change 0->1 or 1->0.
 *
 * The way we implement this is by setting "observation variables"
 * (i.e., variables connected to only a single check) to have
 * channelEvidence ONE, ZERO, or ERASURE.  The "hidden variables"
 * represent the message bits used to produce a codeword and start
 * with channelEvidence DONT_KNOW.  
 *
 * The algorithm tries to iteratively find appropriate values for the
 * hidden variables via message passing.  Essentially we use belief
 * propagation with a special scheduling algorithm and special
 * addition and multiplication rules.
 *
 * To compute the outgoing message from check, c, to var, v, combine
 * all incoming messages from vars except v using BEC_ALG_SUM.  To
 * compute the outgoing message from a variable v, to a check v,
 * combine all incoming messages from checks except c using BEC_ALG_MUL.
 */


/* The following is the "addition table" for combining messages into 
 * a check node and computing outgoing message to a var node.
 *
 * The basic intuition is that vars which are unconstrained send
 * "ERASURE" messages.  So if a check sees at least one "ERASURE"
 * message it knows that the sending var can always be satisfied
 * and thus sends out an ERASURE message to tell connected vars
 * not to worry about this check.  ONE and ZERO message combine
 * by XOR as you would expect.  Finally, if a var does not yet
 * know what value it should be, then the check can't decide
 * either so incoming DONT_KNOW messages overpower everything
 * except an incoming ERASURE.
 */

static int gBEC_SUM_TABLE[] = { 
  /*         DN             Z           O          E      BEC_ERR*/
  /* DN */   DONT_KNOW,  DONT_KNOW, DONT_KNOW,  ERASURE,  BEC_ERR,
  /* Z  */   DONT_KNOW,  ZERO,      ONE,        ERASURE,  BEC_ERR,
  /* O  */   DONT_KNOW,  ONE,       ZERO,       ERASURE,  BEC_ERR,
  /* E  */   ERASURE,    ERASURE,   ERASURE,    ERASURE,  BEC_ERR,
  /* ERR */  BEC_ERR,    BEC_ERR,   BEC_ERR,    BEC_ERR,  BEC_ERR
};

/* The following is the "multiplication table" for combining messages 
 * into a var node and computing outgoing message to a check node 
 *
 * The basic intuition is that if all the incoming messages are
 * all the same, then that must be the outgoing message.  If some
 * checks tell the var it is ERASED or DONT_KNOW, but other checks
 * give a concrete value ONE or ZERO, then we choose the concrete
 * value.  
 */

static int gBEC_MUL_TABLE[] = { 
  /*         DN             Z           O          E     BEC_ERR */
  /* DN */   DONT_KNOW,   ZERO,    ONE,      DONT_KNOW,  BEC_ERR,
  /* Z  */   ZERO,        ZERO,    BEC_ERR,  ZERO,       BEC_ERR,
  /* O  */   ONE,         BEC_ERR, ONE,      ONE,        BEC_ERR,
  /* E  */   DONT_KNOW,   ZERO,    ONE,      ERASURE,    BEC_ERR,
  /* ERR */  BEC_ERR,     BEC_ERR, BEC_ERR,  BEC_ERR,    BEC_ERR
};


#define BEC_ALG_SUM(x,y) gBEC_SUM_TABLE[(((int)x)*NUM_BEC_SYMS)+((int)y)]
#define BEC_ALG_MUL(x,y) gBEC_MUL_TABLE[(((int)x)*NUM_BEC_SYMS)+((int)y)]

/*
static int IsValidBECMessage(const double m) {
  return m == 0 || m == 1 || m == 2 || m == 3;
}

double BEC_ALG_SUM(double x, double y) {
  double result;
  assert( IsValidBECMessage(x) );
  assert( IsValidBECMessage(y) );
  result = gBEC_SUM_TABLE[(((int)x)*NUM_BEC_SYMS)+((int)y)];
  assert( IsValidBECMessage(result) );  
  return result;
}
double BEC_ALG_MUL(double x, double y) {
  double result;
  assert( IsValidBECMessage(x) );
  assert( IsValidBECMessage(y) );
  result = gBEC_MUL_TABLE[(((int)x)*NUM_BEC_SYMS)+((int)y)];
  assert( IsValidBECMessage(result) );  
  return result;
}
*/

/* 
 * A common convention is to specify channel evidence as log
 * likelihood ratios and except answers (i.e., beliefs) as log
 * likelihood ratios.  Therefore the functions ConvertBECQuantMsgToLLR
 * and ConvertLLRToBECQuantMsg convert between log likehood ratios and
 * the messages used by this algorithm.
 */

double ConvertBECQuantMsgToLLR(const double msg) {
  switch ((int)msg) {
  case ONE:
    return -1;
  case ZERO:
    return 1;
  case DONT_KNOW:
  case BEC_ERR:
    return strtod("NAN",NULL);
  case ERASURE:
    return 0;
  }
  assert(0);
}

double ConvertLLRToBECQuantMsg(const double llr) {
  if (llr == 0) return ERASURE;
  else if (llr == 1) return ZERO;
  else if (llr == -1) return ONE;
  else {
    printf("bad value seen in ConvertLLRToBECQuantMsg: abort()\n");
    abort();
  }
}

static char* BECSetEvidence(DualCodeGraph* dc, const int verbose,
			    void * evPtr) {
  int curVar;  
  const double*const ev = (const double*const) evPtr;
  for(curVar=0; curVar < dc->n; curVar++) {
    dc->vNodes[dc->k + curVar].channelEvidence = 
      ConvertLLRToBECQuantMsg(ev[curVar]);
  }
  return NULL;
}

static char* BECGetBeliefs(DualCodeGraph* dc, const int verbose) {
  int curVar;
  double* ev = SafeCalloc(dc->numVNodes,sizeof(double));

  for(curVar=0; curVar < dc->numVNodes; curVar++) {
    ev[curVar] = ConvertBECQuantMsgToLLR(dc->vNodes[curVar].llr);
  }
  return (char*) ev;
}


/* ----------------------------------------------------------------------
 *
 * FUNCTION:	InitVarLLRFromEvidence
 *
 * PURPOSE:     This is a bit of a hack.  The way we want belief
 *              propagation to work is that to get the outgoing message 
 *              from a var to a check we multiply the incoming check 
 *              messages against the var's channel evidence.  The
 *              problem is that hidden vars with channel evidence
 *              of DONT_KNOW basically have no channel evidence and
 *              so it should really be ignored.  Therefore, for hidden
 *              vars with channel evidence = DONT_KNOW we use ERASURE
 *              since ERASURE behaves like an identity operation in
 *              BEC_MUL_ALG.
 *
 * MODIFIED:	Tue Jun 17, 2003
 *
 * -------------------------------------------------------------------- */


static double InitVarLLRFromEvidence(const int ev) {
  if (ev != DONT_KNOW) return ev;
  else return ERASURE; /* hidden var so ignore its channel evidence */
}

/* ----------------------------------------------------------------------
//
// FUNCTION:	SendBECMessagesFromVToC
//
// INPUTS:	var:     A variable node to send messages from.
//              badVar:  A pointer to a variable node.  If var 
//                       has inconsistent messages then this function
//                       sets *badVar = var.
//
// PURPOSE:	Computes outgoing check messages
//              by multiplying together all incoming messages (except
//              the message from the check we are computing a message for),
//              and puts the outgoing message on the appropriate edge.
//
// MODIFIED:	Fri Jul 04, 2003 (EM) Took out loop and made this
//              function operate only on a single var.
//
// -------------------------------------------------------------------- */


static void SendBECMessagesFromVToC(VariableNode*const curVarNode,
					     VariableNode** badVar) {
  int curCheck, j;
  
  for (curCheck=0; curCheck < curVarNode->numChecks; curCheck++) {
    curVarNode->edges[curCheck]->msgToCheck = 
      InitVarLLRFromEvidence(curVarNode->channelEvidence);
    for (j=0; j < curVarNode->numChecks; j++)
      if (j != curCheck) {
	curVarNode->edges[curCheck]->msgToCheck = 
	  BEC_ALG_MUL(curVarNode->edges[curCheck]->msgToCheck,
		      curVarNode->edges[j]->msgToVar);
	if (BEC_ERR == curVarNode->edges[curCheck]->msgToCheck) {
	  *badVar = curVarNode;
	  return;
	}
      }
  }
}

/* 
 * The following function sends BEC messages from all vars in an
 * array.  This function is useful if you want to process a bunch
 * of variable nodes in parallel. 
 */
VariableNode* SendBECMessagesFromVarArray(VariableNode*const vars,
					  const int numVars) {
  VariableNode* curVarNode;
  VariableNode* badVar = NULL;
  const VariableNode*const lastVar = vars+numVars;
  for (curVarNode=vars; curVarNode < lastVar; curVarNode++) {
    SendBECMessagesFromVToC(curVarNode,&badVar);
    if (NULL != badVar) return badVar;
  }
  return NULL;
}
  

/* ----------------------------------------------------------------------
//
// FUNCTION:	SendBECMessageFromCheckOnEdge
//
// INPUTS:	check: A check to send messages from.
//              edge:  An edge pointer indicating which edge to send msg on.
//
// PURPOSE:	Computes outgoing message
//              by summing over incoming messages (except for the
//              message from the var we are computing a message to), 
//              and places the outgoing message on the appropriate edge.
//
// MODIFIED:	Mon Jul 07, 2003
//
// ------------------------------------------------------------------- */
static void SendBECMessageFromCheckOnEdge(CheckNode*const check,
					  GraphEdge*const outEdge){
  int i;
  
  outEdge->msgToVar = ZERO;
  /* ZERO is identity element for BEC_ALG_SUM so start with ZERO. */
  for (i=0; i < check->numVars; i++) {
    if (outEdge != check->edges[i])
      outEdge->msgToVar = BEC_ALG_SUM(outEdge->msgToVar,
				      check->edges[i]->msgToCheck);
  }
}

/* ----------------------------------------------------------------------
//
// FUNCTION:	SendBECMessagesFromCToV
//
// INPUTS:	curCheckNode: A check to send messages from.
//
// PURPOSE:	Computes all outgoing messages for curCheckNode
//              by summing over incoming messages (except for the
//              message from the var we are computing a message to), 
//              and places the outgoing message on the appropriate edge.
//
// MODIFIED:	Fri Jul 04, 2003
//              (EM) Made function operate on single check not array.
//              (EM) Put inner loop into separate function.
//
// ------------------------------------------------------------------- */


static void SendBECMessagesFromCToV(CheckNode*const curCheckNode) {
  int curVar, numVars;

  assert(curCheckNode->numVars > 1);
  for (curVar=0, numVars = curCheckNode->numVars; curVar<numVars; curVar++) {
    SendBECMessageFromCheckOnEdge(curCheckNode,curCheckNode->edges[curVar]);
  }
}

/* 
 * The following function sends BEC messages from all checks in an
 * array.  This function is useful if you want to process a bunch
 * of check nodes in parallel. 
 */
void SendBECMessagesFromCheckArray(CheckNode*const checks,
				   const int numChecks) {
  CheckNode* curCheckNode;
  const CheckNode*const checkNodesEnd = checks+numChecks;
  for (curCheckNode=checks; curCheckNode < checkNodesEnd; curCheckNode++) 
    SendBECMessagesFromCToV(curCheckNode);
}

/* ----------------------------------------------------------------------
//
// FUNCTION:	UpdateBECBelief
//
// INPUTS:	curVarNode: Pointer to a variable node which has its
//                          incoming messages properly computed.
//              badVar:     Pointer which gets set to curVarNode if
//                          curVarNode gets both a ZERO and a ONE message.
//              numVarsChoosingZeroOrOne: A pointer to an int which gets
//                                        incremented if curVarNode got
//                                        a ZERO or ONE message for the 
//                                        first time.
//
// PURPOSE:	Computes curVarNode->llr by multiplying all incoming 
//              messages for curVarNode.  If curVarNode is a
//              hidden var without a value AND the product of incoming 
//              messages is ZERO or ONE, then the var gets set and
//              we increment numVarsChoosingZeroOrOne.
//
// MODIFIED:	Mon Jul 14, 2003
//
// ------------------------------------------------------------------- */


static void UpdateBECBelief(VariableNode*const curVarNode, 
			    VariableNode** badVar,
			    int*const numVarsChoosingZeroOrOne) {
  int curMsg;

  curVarNode->llr = InitVarLLRFromEvidence(curVarNode->channelEvidence);

  for(curMsg=0; curMsg < curVarNode->numChecks; curMsg++) {
    curVarNode->llr = BEC_ALG_MUL(curVarNode->llr,
				  curVarNode->edges[curMsg]->msgToVar);
    if (BEC_ERR == curVarNode->llr) {
      *badVar = curVarNode;
    }
  }
  if (curVarNode->llr == ZERO || curVarNode->llr == ONE) {
    if (curVarNode->channelEvidence == DONT_KNOW) {
      (*numVarsChoosingZeroOrOne)++;
      curVarNode->channelEvidence = curVarNode->llr;
    } 
  }
}


/* ----------------------------------------------------------------------
//
// FUNCTION:	UpdateBECBeliefsForVarArray
//
// INPUTS:	vars, numVars:  Array of vars to update from check messages.
//              badVar: A reference to a pointer which gets set to the
//                      var with inconsistent messages when -1 is returned.
//
// RETURNS:	Number of hidden vars which end up choosing a value or
//              -1 if a var got inconsistent incoming messages.
//
// PURPOSE:	Goes through all VariableNodes in vars and computes
//              the llr field by multiplying all incoming messages
//              in the msgs array for each var.  If the var was a
//              hidden var without a value AND the product of incoming 
//              messages is ZERO or ONE, then the var gets set and
//              we increase the returned count by one.
//
// MODIFIED:	Mon Jun 16, 2003
//
// ------------------------------------------------------------------- */

int UpdateBECBeliefsForVarArray(VariableNode*const vars,
				const int numVars, VariableNode** badVar) {
  VariableNode* curVarNode;
  int numVarsChoosingZeroOrOne = 0;
  const VariableNode*const lastVarNode = vars+numVars;

  for (curVarNode=vars; curVarNode < lastVarNode; curVarNode++) {  
    UpdateBECBelief(curVarNode,badVar,&numVarsChoosingZeroOrOne);
    if (*badVar != NULL) {
      return -1;
    }
  }

  return numVarsChoosingZeroOrOne;
}

/* ----------------------------------------------------------------------
//
// FUNCTION:	TellVarsIfCheckIsFree
//
// INPUTS:	check: A check node.
//
// PURPOSE:	If the single observation var connected to check has
//              ERASURE for channel evidence, than this check is "free".
//              By that we mean that this check doesn't have to be
//              satisfied.  Therefore we increment the counter field
//              of all connected vars so that they new that at least
//              one of their checks can be ignored.
//
// PRECONDITION: The input, check, should be connected to 1 and only 1
//               observation var.  This should always be the case for
//               duals of LDPC codes where the original code is in
//               the normal factor graph form.  But if you start using
//               funky codes you will have to fix this function.
//
// MODIFIED:	Tue Jun 17, 2003
//
// ------------------------------------------------------------------- */

static void TellVarsIfCheckIsFree(const CheckNode*const check) {
  int curVar;
  const int numVars = check->numVars;
  for(curVar=0; curVar < numVars; curVar++)
    if (check->edges[curVar]->var->channelEvidence == ERASURE) {
      assert(check->edges[curVar]->var->varType == 'o');
      /* only observation vars should ever have evidence == ERASURE */
      for(curVar=0; curVar < numVars; curVar++) 
	check->edges[curVar]->var->counter++;
      return;
    }
}


/* ----------------------------------------------------------------------
//
// FUNCTION:	SetVarCountersToNumFreeChecks
//
// INPUTS:	Array of checks and vars in the graph.
//
// PURPOSE:	Initialize the counter field for each var and then
//              set this field to the number of "free" checks conntected
//              to the var.  See the comment for the TellVarsIfCheckIsFree
//              function for details.
//
// POSTCONDITION: You shouldn't fool with the counter field of the vars
//                after this function is called.
//
// MODIFIED:	Tue Jun 17, 2003
//
// ------------------------------------------------------------------- */

static void SetVarCountersToNumFreeChecks(CheckNode*const checks, 
					  const int numChecks,
					  VariableNode*const vars, 
					  const int numVars) {
  CheckNode* curCheckNode;
  VariableNode* curVarNode;
  const CheckNode*const checkNodesEnd = checks+numChecks;
  const VariableNode*const varNodesEnd = vars+numVars;

  for (curVarNode=vars; curVarNode < varNodesEnd; curVarNode++) 
    curVarNode->counter = 0;

  for (curCheckNode=checks; curCheckNode < checkNodesEnd; curCheckNode++) 
    TellVarsIfCheckIsFree(curCheckNode);
}


/* ----------------------------------------------------------------------
//
// FUNCTION:	SetFreeVarsToZero
//
// INPUTS:	checks, numChecks:  Array of check nodes in the graph.
//              vars, numVars:      Array of var nodes in the graph.
//
// PURPOSE:	Find all hidden vars who are connected only to checks with 
//              erased observation vars.  These vars can be arbitrarily set
//              so we set them to ZERO.
//
// NOTE:        This function uses the counter field of vars so later
//              callers will need to reset the counter field if they use it.
//
// MODIFIED:	Mon Jun 16, 2003
//
// ------------------------------------------------------------------- */

static void SetFreeVarsToZero(CheckNode*const checks, const int numChecks,
			      VariableNode*const vars, const int numVars) {
  VariableNode* curVarNode;
  const VariableNode*const varNodesEnd = vars+numVars;

  SetVarCountersToNumFreeChecks(checks,numChecks,vars,numVars);

  for (curVarNode=vars; curVarNode < varNodesEnd; curVarNode++) {
    assert(curVarNode->counter >= 0);
    assert(curVarNode->counter <= curVarNode->numChecks);
    if (curVarNode->channelEvidence == DONT_KNOW) {/* if hidden var */
      assert(curVarNode->varType == 'h');
      if ( curVarNode->counter == curVarNode->numChecks ) {
	/* var connected only to checks with erased channel evidence */
	curVarNode->llr = ZERO;
	curVarNode->channelEvidence = ZERO;
	curVarNode->varType = 'f';
      }
    }
  }
}

/* ----------------------------------------------------------------------
//
// FUNCTION:	AssertNoVarInMessagesContradict
//
// INPUTS:	var: A variable to check incoming messages of.
//
// PURPOSE:	Goes through all the incoming messages and makes sure
//              that we don't have a contradiction (i.e. one check
//              telling the var to be a 0 and another check telling
//              it to be a 1).
//
// MODIFIED:	Tue Jun 17, 2003
//
// -------------------------------------------------------------------- */

void AssertNoVarInMessagesContradict(const VariableNode*const var) {
  int i;
  double requiredVal = DONT_KNOW;

  for (i = 0; i < var->numChecks; i++) {
    if ( (var->edges[i]->msgToVar == ONE) ||
	 (var->edges[i]->msgToVar == ZERO) ) {
      if (requiredVal == DONT_KNOW) requiredVal = var->edges[i]->msgToVar;
      else { assert(requiredVal == var->edges[i]->msgToVar); }
    }
  }
}

/* ----------------------------------------------------------------------
//
// FUNCTION:	PrepareBECQuantReturnValue
//
// INPUTS:	dc:     DualCodeGraph that we called DoBECQuant on.
//              badVar: NULL or Variable with possibly conflincting in msgs.
//
// RETURNS:	A string representing the return value for DoBECQuant.
//
// PURPOSE:	If DoBECQuant had to stop because of a var getting 
//              conflicting input messages, then this function describes
//              that variable.  Otherwise, NULL is returned.
//
// MODIFIED:	Thu Jun 19, 2003
//
// ------------------------------------------------------------------- */

char* PrepareBECQuantReturnValue(DualCodeGraph* dc, VariableNode* badVar) {
  static char result[300];

  if (badVar == NULL) return NULL;

  sprintf(result,"variable %i got inconsistent messages",
	  (int) (badVar-dc->vNodes));

  return result;
}


/* Return 1 if all hidden variables have chosen a value and
 * return 0 otherwise. */
int DCConvergedP(const DualCodeGraph*const dc) {
  int curVar;

  for(curVar=0; curVar<dc->k; curVar++)
    if ( (dc->vNodes[curVar].channelEvidence != ONE) &&
	 (dc->vNodes[curVar].channelEvidence != ZERO) )
      return 0;
  return 1;
}

void PrepareDCForBECQuantization(DualCodeGraph*const dc) {
  int i;

  for (i=0; i < dc->k; i++) {
    assert(dc->vNodes[i].varType != 'o');
    dc->vNodes[i].channelEvidence = DONT_KNOW;
    dc->vNodes[i].llr = DONT_KNOW;
    dc->vNodes[i].varType = 'h';
  }
  for (i=dc->k; i < dc->numVNodes; i++) {
    assert(dc->vNodes[i].varType == 'o');
    dc->vNodes[i].llr = DONT_KNOW;
  }
							    

  for (i=0; i < dc->numEdges; i++) {
    dc->edges[i].msgToVar = DONT_KNOW;
    dc->edges[i].msgToCheck = DONT_KNOW;
  }
  SetFreeVarsToZero(dc->cNodes,dc->numCNodes,dc->vNodes,dc->numVNodes);
}

/* 
   The following prototype is required since 
   PropagateZeroOneMessagesFromCheck and PropagateZeroOneMessagesFromVar
   recursively call each other.
*/
static CheckNode* PropagateZeroOneMessagesFromCheck(CheckNode*const check,
						    VPSet* undecidedVars);


/* ----------------------------------------------------------------------
//
// FUNCTION:	PropagateZeroOneMessagesFromVar
//
// INPUTS:	var: A "hidden variable" which has just been determined 
//                   to be a ONE or a ZERO and needs to propagate this to
//                   all assoicated checks.
//              undecidedVars: A set of vars which have not yet
//                             determined whether they are ONE or ZERO.
//
// RETURNS:     If a check can not be satisfied then a pointer to that
//              CheckNode is returned, otherwise NULL is returned.
//
// PURPOSE:	First, set the channel evidence of var to reflect that 
//              var is now a ONE or ZERO.  Then remove var from
//              undecidedVars.  Finally, send out this value to
//              all associated checks and call 
//              PropagateZeroOneMessagesFromCheck to continue propagating
//              this message.
//
//              The main idea behind this function is that as soon as we
//              decided that a var is a ZERO or a ONE, we need to propagate
//              this information as far as we can in the code graph.
//
// MODIFIED:	Mon Jul 14, 2003
//
// -------------------------------------------------------------------- */


static CheckNode* PropagateZeroOneMessagesFromVar(VariableNode*const var, 
						  VPSet* undecidedVars) {
  int curCheck, inSet;
  CheckNode* contradiction = NULL;

  if (var->channelEvidence == ZERO || var->channelEvidence == ONE) return NULL;
  assert(var->llr == ZERO || var->llr == ONE);

  assert(var->varType == 'h');
  assert(var->channelEvidence == DONT_KNOW);
  var->channelEvidence = var->llr; 
  inSet = VPSetDelete(undecidedVars,var);
  assert(inSet); 
  for (curCheck = 0; curCheck < var->numChecks; curCheck++) {
    assert(DONT_KNOW == var->edges[curCheck]->msgToCheck ||
	   ERASURE == var->edges[curCheck]->msgToCheck);
    var->edges[curCheck]->msgToCheck = var->llr;
  }
  for (curCheck = 0; curCheck < var->numChecks; curCheck++) {
    if ( (contradiction = /* assignment intentional */
	  PropagateZeroOneMessagesFromCheck(var->edges[curCheck]->check,
					    undecidedVars)) )
      return contradiction;
  }
  return NULL;
}

/* ----------------------------------------------------------------------
//
// FUNCTION:	FindEdgeToSendCheckMessageOn
//
// INPUTS:	check: A check node which may need to propagate some
//                     ZERO or ONE messages to assoicated vars.
//              edgeToSendMessageOn: A pointer to an int which gets
//                                   set to the index of the edge that
//                                   the check should send a message on.
//              parity: A pointer to an int which gets set to the 
//                      parity of incoming messages to the check.
//
// PURPOSE:	Find out which edge we need to send a message out on so
//              that we can efficiently propagate ZERO/ONE messages.  If
//              all edges have incoming messages, then we don't need to
//              send out a message on any edge so *edgeToSendMessageOn=-1.
//              If at least two edges have incoming messages of DONT_KNOW
//              then we can't send out on any edge so we set
//              *edgeToSendMessageOn=-2.
//
// MODIFIED:	Mon Jul 14, 2003
//
// -------------------------------------------------------------------- */


static void FindEdgeToSendCheckMessageOn(const CheckNode*const check,
					 int*const edgeToSendMessageOn,
					 int*const parity) {
  int curEdge;
  
  *edgeToSendMessageOn=-1;
  *parity=0;

  for (curEdge=0; curEdge < check->numVars; curEdge++) {
    if ( ERASURE == check->edges[curEdge]->msgToCheck ||
	 DONT_KNOW == check->edges[curEdge]->msgToCheck) {
      if (-1 == *edgeToSendMessageOn) {
	*edgeToSendMessageOn = curEdge;
      } else { /* at least two edges with DONT_KNOW or ERASURE coming in */
	*edgeToSendMessageOn=-2; /* so we can't send any 0/1 messages out. */
	return;
      }
    } else if (ZERO == check->edges[curEdge]->msgToCheck) {
      ;
    } else if (ONE == check->edges[curEdge]->msgToCheck) {
      *parity = *parity ^ 1;
    } else {
      printf("bad message seen in FindEdgeToSendCheckMessageOn\n");
      abort();
    }
  }
}

/* ----------------------------------------------------------------------
//
// FUNCTION:	PropagateZeroOneMessagesFromCheck
//
// INPUTS:	check: A check node which may need to propagate ZERO/ONE
//                     messages out to associated vars.
//              undecidedVars: A set of vars which have not yet decided
//                             to be ONE or ZERO.  This input is only
//                             used in passing it to
//                             PropagateZeroOneMessagesFromVar.
//
// RETURNS:     If a check can not be satisfied then a pointer to that
//              CheckNode is returned, otherwise NULL is returned.
//                              
// PURPOSE:	If all but one incoming messages are ONE/ZERO then
//              send the appropriate message along the remaining edge
//              and call PropagateZeroOneMessagesFromVar on that var.
//
// MODIFIED:	Mon Jul 14, 2003
//
// -------------------------------------------------------------------- */

static CheckNode* PropagateZeroOneMessagesFromCheck(CheckNode*const check,
						    VPSet* undecidedVars) {
  int parity, edgeToSendMessageOn;
  CheckNode* contradiction = NULL;
  double msgToSend = BEC_ERR;

  FindEdgeToSendCheckMessageOn(check,&edgeToSendMessageOn,&parity);

  if (-2 == edgeToSendMessageOn) { ; /* do nothing */
  } else if (-1 == edgeToSendMessageOn) { 
    if (parity) contradiction = check; /* check not satisfied */
  } else if ('o' != check->edges[edgeToSendMessageOn]->var->varType) {
    msgToSend = (0 == parity) ? ZERO : ONE;
    assert(msgToSend == check->edges[edgeToSendMessageOn]->msgToVar ||
	   ERASURE == check->edges[edgeToSendMessageOn]->msgToVar ||
	   DONT_KNOW == check->edges[edgeToSendMessageOn]->msgToVar);
    assert(msgToSend == check->edges[edgeToSendMessageOn]->var->llr ||
	   ERASURE == check->edges[edgeToSendMessageOn]->var->llr ||
	   DONT_KNOW == check->edges[edgeToSendMessageOn]->var->llr);
    check->edges[edgeToSendMessageOn]->msgToVar = msgToSend;
    check->edges[edgeToSendMessageOn]->var->llr = msgToSend;
    if (msgToSend == ZERO || msgToSend == ONE)
      contradiction = PropagateZeroOneMessagesFromVar
	(check->edges[edgeToSendMessageOn]->var,undecidedVars);
  }
  return contradiction;
}


static int VarUndecidedP(const VariableNode*const v) {
  return ((v->channelEvidence == DONT_KNOW) && 
	  (ONE != v->llr) && (ZERO != v->llr));
}

// RETURNS:     If a check can not be satisfied then a pointer to that
//              CheckNode is returned, otherwise NULL is returned.
static CheckNode* BreakTieAndPropagate(VariableNode*const var,
				       VPSet*const undecidedVars) {
  if (ONE != var->llr && ZERO != var->llr) { /* var is still a tie breaker */
    var->llr = ZERO;
    return PropagateZeroOneMessagesFromVar(var,undecidedVars);
  } else { /* var was a potential tie breaker but propagating other tie */
    return NULL;   /* breakers gave it a value */
  }
}


/* ----------------------------------------------------------------------
//
// FUNCTION:	PursueTieBreaker
//
// INPUTS:	var: A var to break ties with or pursue other tie breakers.
//              alreadyTriedVars: Set of vars we have already tried to
//                                pursue on the current iteration.
//              undecidedVars:    Set of undecided vars.
//              tiesBroken:       Pointer to an int keeping track of how
//                                many ties have been broken.  This var
//                                is incremented whenever we break a tie.
//              verbose:          Indicates whether to print debug messages.
//
// RETURNS:     If a check can not be satisfied then a pointer to that
//              CheckNode is returned, otherwise NULL is returned.
//
// PURPOSE:	This function goes through all the checks connected to
//              var and computes incoming messages to var.  If all
//              incoming messages are ERASURE then var is a tie breaker
//              so the tie is broken, and the new value for var is 
//              propagated out.  Otherwise, if all but one incoming 
//              message is an ERASURE, and the the remaining message 
//              is a DONT_KNOW, we send out an ERASURE on that edge 
//              and recursively PursueTieBreaker on all the vars of the 
//              check sending a DONT_KNOW.
//
//              No matter what happens, var is added to alreadyTriedVars
//              so that the caller can avoid calling PursueTieBreaker
//              on already tried vars.
//
// POSSIBLE IMPROVEMENTS:  
//                         Instead of making a recursive call, we could
//                         keep a stack of recursive calls which need to
//                         be dispatched and iterate through the calls.
//                         The advantage of this approach is that you
//                         wouldn't need to allocate a new stack frame
//                         for each recursive call.
//
// MODIFIED:	Mon Jul 07, 2003
//              Mon Jul 14, 2003 (EM) Added list of POSSIBLE IMPROVMENTS.
//
// ------------------------------------------------------------------- */

static CheckNode* PursueTieBreaker(VariableNode*const var,
				   VPSet*const alreadyTriedVars,
				   VPSet*const undecidedVars,
				   int*const tiesBroken,
				   const int verbose) {
  int curCheck, curVar;
  CheckNode* contradiction;
  GraphEdge* edgeToSendOn = NULL;

  VPSetPush(alreadyTriedVars,var);
  for (curCheck=0; curCheck < var->numChecks; curCheck++) {
    if (DONT_KNOW == var->edges[curCheck]->msgToVar) {
      SendBECMessageFromCheckOnEdge(var->edges[curCheck]->check,
				    var->edges[curCheck]);
      if (var->edges[curCheck]->msgToVar == DONT_KNOW) {
	if (NULL == edgeToSendOn) edgeToSendOn = var->edges[curCheck];
	else {
	  if (verbose) printf("var %x has too many DKs coming in\n",(int)var);
	  return NULL; /* At least two DONT_KNOW messages coming in */
	  /* so this var can't send out anything */
	}
      } else {
	assert(ERASURE == var->edges[curCheck]->msgToVar);
      }
    } else {
      assert(ERASURE == var->edges[curCheck]->msgToVar);
    }
  }
  if (NULL == edgeToSendOn) { /* this var got only erasures in so */
    (*tiesBroken)++;
    return BreakTieAndPropagate(var,undecidedVars);
  } else { /* this var got all ERASUREs in except for edgeToSendOutOn */
    if (verbose) printf("sending out ERASURE messages from var %x\n",(int)var);
    edgeToSendOn->msgToCheck = ERASURE;
    for (curVar = 0; curVar < edgeToSendOn->check->numVars; curVar++) {
      if (edgeToSendOn->check->edges[curVar]->var->channelEvidence == DONT_KNOW
	  && edgeToSendOn->check->edges[curVar]->msgToVar == DONT_KNOW &&
	  edgeToSendOn->check->edges[curVar]->var != var) {
	assert(ZERO != edgeToSendOn->check->edges[curVar]->msgToVar);
	assert(ONE != edgeToSendOn->check->edges[curVar]->msgToVar);
	edgeToSendOn->check->edges[curVar]->msgToVar = ERASURE;
	if ( (contradiction = /* assignment intentional */
	      PursueTieBreaker(edgeToSendOn->check->edges[curVar]->var,
			       alreadyTriedVars,undecidedVars,tiesBroken,
			       verbose)) )
	  return contradiction;
      }
    }
  }
  return NULL;
}

// ----------------------------------------------------------------------
//
// FUNCTION:	FindVarToBreakTieAndPropagate
//
// INPUTS:	varsToTry:  Stack containing pointers to var nodes that
//                          we should try to process.
//              alreadyTriedVars: A set containing pointers to vars we
//                                have already tried to process.  This set
//                                should be empty in the first call to
//                                FindVarToBreakTieAndPropagate.
//              undecidedVars:  A set containing pointers to vars which
//                              have not yet decided to be ONE/ZERO.
//              verbose: Indicates whether to print debug messages.
//
// RETURNS:     If a check can not be satisfied then a pointer to that
//              CheckNode is returned, otherwise NULL is returned.
//
// PURPOSE:	This function goes through all the vars in varsToTry
//              and tries to send messages from each var.  Ties are broken
//              as they are found with their results propagated to all
//              neighbors before further ties are broken.
//
// POSSIBLE IMPROVEMENTS:
//              Do we really need to process varsToTry in a random order?
//              Is there any benefit/disadvantage in using a deterministic
//              processing order?
//
//              Instead of processing varsToTry in a random order, we
//              could do the following.  When first starting out and
//              sending ERASURE messages from erased obs vars to checks
//              and then to hidden vars, call PursueTieBreaker on 
//              every hidden var getting an erasure message.  The 
//              advantage of this approach is that it is a nice, simple,
//              depth first search without the complications of our
//              current method.
//
// MODIFIED:	Mon Jul 14, 2003
//
// ----------------------------------------------------------------------
    

static CheckNode* FindVarToBreakTieAndPropagate(VPStack*const varsToTry,
						VPSet*const alreadyTriedVars,
						VPSet*const undecidedVars,
						const int verbose) {
  CheckNode* contradiction;
  VariableNode* var;
  int tiesBroken;
  int varsTried=0;


  while (VPStackSize(varsToTry) > 0) {
    tiesBroken = 0;
    varsTried++;
    /* The following line makes it so that we pop a random var instead
     * of going through the vars in order. */
    /* VPStackSwapTopToRandomPos(varsToTry); */
    var = VPStackPop(varsToTry);
    if (verbose) { 
      printf("trying to pursue tie breaking on var %x:\n",(int)var);
      PrintVariableNode(var);
    }
    if (VPSetMemberP(alreadyTriedVars,var)) continue;
    assert(VarUndecidedP(var));
    if ( (contradiction = /* assignment intentional */
	  PursueTieBreaker(var,alreadyTriedVars,undecidedVars,
			   &tiesBroken,verbose)) )
      return contradiction;
    if (tiesBroken) {
      if (verbose) printf("broke %i ties after %i tries, %i undecided\n",
			  tiesBroken,varsTried,VPSetSize(undecidedVars));
    }
  }
  return NULL;
}

typedef struct CDForProvablyGoodBECQuant {
  char*  error;
  VPSet* undecidedVars;
} CDForProvablyGoodBECQuant;

CDForProvablyGoodBECQuant* CreateCDForProvablyGoodBECQuant(void) {
  CDForProvablyGoodBECQuant*const clientData 
    = SafeMalloc(sizeof(CDForProvablyGoodBECQuant));
  clientData->undecidedVars = VPSetCreate();
  clientData->error = NULL;
  return clientData;
}

void DestroyCDForProvablyGoodBECQuant(void* cd, const char*const type) {
  CDForProvablyGoodBECQuant* clientData = cd;
  assert(0 == strcmp("PROVABLY_GOOD_BEC_QUANT",type));
  VPSetDestroy(clientData->undecidedVars);
  free(clientData->undecidedVars);
  if (NULL != clientData->error) free(clientData->error);
}


char* DeallocateDataForProvablyGoodBECQuant(DualCodeGraph* dc,
					    const int verbose) {
  CDForProvablyGoodBECQuant* clientData = 
    (CDForProvablyGoodBECQuant*) dc->algorithm->clientData;
  DestroyCDForProvablyGoodBECQuant(clientData,dc->algorithm->algorithmName);
  dc->algorithm->clientData = NULL;
  dc->algorithm = NULL;
  return NULL;
}

static char* DescribeContradiction(DualCodeGraph*const dc, 
				   CheckNode*const contradiction);

// ----------------------------------------------------------------------
//
// FUNCTION:	PrepareForProvablyGoodBECQuant
//
// INPUTS:	dc: A DualCodeGraph to prepare for the 
//                  DoProvablyGoodBECQuant function.
//
// PURPOSE:	Initializes client data for dc, sends messages from
//              observation vars and to checks and propagates any
//              necessary messages.
//
// MODIFIED:	Mon Jul 14, 2003
//
// ----------------------------------------------------------------------

char* PrepareForProvablyGoodBECQuant(DualCodeGraph* dc, const int verbose) {
  int i;
  CheckNode* contradiction;
  VariableNode* curVarNode;
  VariableNode* lastVarNode;
  CDForProvablyGoodBECQuant** clientData = 
    (CDForProvablyGoodBECQuant**) &(dc->algorithm->clientData);
  
  PrepareDCForBECQuantization(dc);

  assert(NULL == *clientData);
  assert(0 == strcmp("PROVABLY_GOOD_BEC_QUANT",dc->algorithm->algorithmName));

  (*clientData) = CreateCDForProvablyGoodBECQuant();

  for (curVarNode=dc->vNodes,lastVarNode=dc->vNodes+dc->k; 
       curVarNode < lastVarNode; curVarNode++) {
    if ('h' == curVarNode->varType) {
      assert(DONT_KNOW == curVarNode->channelEvidence);
      VPSetPush((*clientData)->undecidedVars,curVarNode);
    } else assert('f' == curVarNode->varType);
  }
  for (curVarNode=dc->vNodes+dc->k,lastVarNode=dc->vNodes+dc->k+dc->n; 
       curVarNode < lastVarNode; curVarNode++) {
    assert('o' == curVarNode->varType);
    assert(1 == curVarNode->numChecks);
    curVarNode->edges[0]->msgToCheck = curVarNode->channelEvidence;
  }

  /* If the graph has some checks with degree 2 (i.e., checks connected
   * to exactly one obs var and one hidden var) we need to propagate the
   * 0/1 messages from such checks. */
  for (curVarNode=dc->vNodes,lastVarNode=dc->vNodes+dc->k; 
       curVarNode < lastVarNode; curVarNode++) {
    for (i=0; i < curVarNode->numChecks; i++) {
      contradiction = 
	PropagateZeroOneMessagesFromCheck(curVarNode->edges[i]->check,
					  (*clientData)->undecidedVars);
      if (contradiction) return DescribeContradiction(dc,contradiction);
    }
  }
  return NULL;
}

static void AddVarToVPStack(void*varPtr, void*vpStackPtr) {
  VPStackPush((VPStack*) vpStackPtr, varPtr);
}

static void AssertNoIncorrectlyStuckVars(const VariableNode*const vars,
					 const int numVars) {
  int atLeastOneNonErasureIn, curCheck;
  const VariableNode* curVarNode = vars;
  const VariableNode* const lastVar = vars + numVars;

  for(;curVarNode<lastVar; curVarNode++) {
    if (DONT_KNOW != curVarNode->llr) continue;
    atLeastOneNonErasureIn=0;
    for (curCheck=0; curCheck < curVarNode->numChecks; curCheck++) {
      if (ERASURE != curVarNode->edges[curCheck]->msgToVar) {
	atLeastOneNonErasureIn = 1;
	break;
      }
    }
    assert(atLeastOneNonErasureIn);
  }
}
 
static char* DescribeStuck(DualCodeGraph*const dc) {
  CDForProvablyGoodBECQuant*const clientData = 
    (CDForProvablyGoodBECQuant*) dc->algorithm->clientData;
  assert(NULL == clientData->error);
  clientData->error = CopyString("stuck");
  AssertNoIncorrectlyStuckVars(dc->vNodes,dc->numVNodes);
  return CopyString(clientData->error);
}

static char* DescribeContradiction(DualCodeGraph*const dc, 
				   CheckNode*const contradiction) {
  CDForProvablyGoodBECQuant*const clientData = 
    (CDForProvablyGoodBECQuant*) dc->algorithm->clientData;
  char* result= SafeCalloc(strlen("contradiction on check ")+100,sizeof(char));

  sprintf(result,"contradiction on check %i",contradiction-dc->cNodes);
  assert(NULL == clientData->error);
  clientData->error = CopyString(result);
  return result;
}

char* DoProvablyGoodBECQuant(DualCodeGraph* dc, const int verbose) {
  CheckNode* contradiction;
  char* result;
  VPStack* varsToTry = VPStackCreate(20);  
  VPSet* alreadyTriedVars = VPSetCreate();
  CDForProvablyGoodBECQuant* clientData = 
    ((CDForProvablyGoodBECQuant*)dc->algorithm->clientData);
  VPSet* undecidedVars = clientData->undecidedVars;

  assert(0 == strcmp("PROVABLY_GOOD_BEC_QUANT",dc->algorithm->algorithmName));

  if (NULL != clientData->error) {
    result = CopyString(clientData->error);
  } else {
    VPSetForeach2(undecidedVars,AddVarToVPStack,varsToTry);
    contradiction = FindVarToBreakTieAndPropagate(varsToTry,alreadyTriedVars,
						  undecidedVars,verbose);
    if (NULL != contradiction) {
      result = DescribeContradiction(dc,contradiction);
    } else if (VPSetSize(undecidedVars) > 0) {
      result = DescribeStuck(dc);
    } else result = NULL;
  }

  VPStackDestroy(varsToTry);
  free(varsToTry);
  VPSetDestroy(alreadyTriedVars);
  free(alreadyTriedVars);
  return result;
}
   
const DualCodeGraphAlgorithm ProvablyGoodBECQuantAlgorithm = {
  "PROVABLY_GOOD_BEC_QUANT",
  NULL,
  BECSetEvidence,
  BECGetBeliefs,
  DeallocateDataForProvablyGoodBECQuant,
  PrepareForProvablyGoodBECQuant,
  DoProvablyGoodBECQuant
};
