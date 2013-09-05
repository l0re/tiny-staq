
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "misc.h"
#include "SatGraph.h"
#include "VPSet.h"
#include "VPStack.h"
#include "SatGraphAlgorithm.h"

#define DONT_KNOW 0
#define ZERO 1
#define ONE 2
#define ERASURE 3
#define BEC_ERR 4

#define NUM_BEC_SYMS 5


/* This file contains code to implement a message passing algorithm 
 * for the circuit satisfiability problem (aka SAT).  Specifically,
 * an instance of the sat problem is a list of clauses of the form
 *
 *   (x1 \/ x8 \/ !x2) /\ (x5 \/ !x1 \/ x3) /\ ...
 *
 * and the goal is to find an assignment for all the variables which
 * satisfies the AND of OR clauses.
 *
 * Our goal is to represent this problem as a graph and using message
 * passing algorithms to try and solve it.  Specifically, our graph
 * will consist of check nodes representing an OR clause and variable
 * nodes representing variables.
 *
 * The algorithm works by first searching for variable nodes which
 * are connected to 0 or 1 check nodes and then choosing the values
 * for these variables to make sure the corresponding check is
 * satisfied.  Once a given check is satisfied then variables 
 * connected to that check will now be involved in one less equation
 * and thus new degree 0/1 variables may be generated which can
 * immediately be chosen to satisfy more checks and so on.  Also, this
 * process may generate degree 1 checks, that is checks where all the
 * incoming variables except one are fixed and thus the remaining 
 * variable's value must be chosen to satisfy that check.
 *
 */

/* The following is the "addition table" for combining messages into 
 * a "check node" corresponding to an OR clause and computing 
 * outgoing message to a var node.
 *
 * The basic intuition is that a var sends us a ZERO or ONE if
 * it has already chosen a value, otherwise it sends us a DONT_KNOW
 * (vars should never send out an ERASURE message).  If we get
 * any ONE messages coming in then we send out an ERASURE message
 * to vars saying that the clause is satisfied so you can "erase" it.
 * If we get only ZERO messages in then we send out a ZERO indicating
 * to the var that if it doesn't do something we will end up ZERO which
 * would be BAD.  If we get a combination of ZERO and DONT_KNOW in, that
 * means some vars are still undecided and might satisfy this OR clause
 * so we send out a DONT_KNOW.
 */

static int gBEC_SUM_TABLE[] = { 
  /*         DN             Z           O       E     BEC_ERR*/
  /* DN */   DONT_KNOW,  DONT_KNOW, ERASURE, ERASURE, BEC_ERR,
  /* Z  */   DONT_KNOW,  ZERO,      ERASURE, ERASURE, BEC_ERR,
  /* O  */   ERASURE,    ERASURE,   ERASURE, ERASURE, BEC_ERR,
  /* E  */   ERASURE,    ERASURE,   ERASURE, ERASURE, BEC_ERR,
  /* ERR */  BEC_ERR,    BEC_ERR,   BEC_ERR, BEC_ERR, BEC_ERR
};

/* The following is the "multiplication table" for combining messages 
 * into a var node and computing outgoing message to a check node 
 *
 * The basic intuition is that an incoming ERASURE message tells
 * us the corresponding clause is satisfied and we can "erase" it
 * from consideration.  An incoming ZERO indicates the clause will
 * be unsatisfied unless we set ourselves to be a 1 to satisfy it.  
 * Similarly, an incoming ONE indicates the clause will be unsatisfied
 * unless we set ourselves to be a 0 to satisfy it.  (I know this seems
 * kind of backwards, but it makes the table representation more compact).
 * Finally, if we receive a DONT_KNOW then it means that clause does not
 * yet know what this var should be.
 */

/* NOTE THE FOLLOWING IS KIND OF INVERTED!! (SEE COMMENT ABOVE) */
static int gBEC_MUL_TABLE[] = { 
  /*         DN             Z           O          E     BEC_ERR */
  /* DN */   DONT_KNOW,   ONE,     ZERO,    DONT_KNOW,  BEC_ERR,
  /* Z  */   ONE,         ONE,     BEC_ERR, ONE,        BEC_ERR,
  /* O  */   ZERO,        BEC_ERR, ZERO,    ZERO,       BEC_ERR,
  /* E  */   DONT_KNOW,   ONE,     ZERO,    ERASURE,    BEC_ERR,
  /* ERR */  BEC_ERR,     BEC_ERR, BEC_ERR, BEC_ERR,    BEC_ERR
};

const double MaybeInvertMessage(const double m, const unsigned char invert) {
  if (invert) {
    if (ONE == m) return ZERO;
    else if (ZERO == m) return ONE;
    else {
      assert(DONT_KNOW == m);
      return m;
    }
  } else
    return m;
}

#define BEC_ALG_SUM(x,y,z) gBEC_SUM_TABLE[(((int)x)*NUM_BEC_SYMS)+((int)MaybeInvertMessage(y,z))];
#define BEC_ALG_MUL(x,y,z) gBEC_MUL_TABLE[(((int)x)*NUM_BEC_SYMS)+((int)MaybeInvertMessage(y,z))];

/* 
 * A common convention is to specify channel evidence as log
 * likelihood ratios and except answers (i.e., beliefs) as log
 * likelihood ratios.  Therefore the functions ConvertBECQuantMsgToLLR
 * and ConvertLLRToBECQuantMsg convert between log likehood ratios and
 * the messages used by this algorithm.
 */

double ConvertSatMsgToLLR(const double msg) {
  switch ((int)msg) {
  case ONE:
    return 1;
  case ZERO:
    return 0;
  case DONT_KNOW:
  case BEC_ERR:
    return strtod("NAN",NULL);
  case ERASURE:
    return -1;
  }
  assert(0);
}

double ConvertLLRToSatMsg(const double llr) {
  if (llr == 0) return ZERO;
  else if (llr == 1) return ONE;
  else if (llr == -1) return ERASURE;
  else {
    printf("bad value seen in ConvertLLRToBECQuantMsg: abort()\n");
    abort();
  }
}

static char* GetSatBeliefs(SatGraph* sg, const int verbose) {
  int curVar;
  double* ev = SafeCalloc(sg->numVNodes,sizeof(double));

  for(curVar=0; curVar < sg->numVNodes; curVar++) {
    ev[curVar] = ConvertSatMsgToLLR(sg->vNodes[curVar].llr);
  }
  return (char*) ev;
}
  

/* ----------------------------------------------------------------------
//
// FUNCTION:	SendSatMessageFromCheckOnEdge
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
static void SendSatMessageFromCheckOnEdge(SatCheckNode*const check,
					  SatGraphEdge*const outEdge){
  int i;
  
  outEdge->msgToVar = ZERO;
  /* ZERO is identity element for BEC_ALG_SUM so start with ZERO. */
  for (i=0; i < check->numVars; i++) {
    if (outEdge != check->edges[i])
      outEdge->msgToVar = BEC_ALG_SUM(outEdge->msgToVar,
				      check->edges[i]->msgToCheck,
				      outEdge->invert);
  }
}

// ----------------------------------------------------------------------
//
// FUNCTION:	SetVarIfItHasLessThanTwoChecks
//
// INPUTS:	curVarNode: A var to process.
//
// RETURNS:	Edge for check that caused var to get fixed to a value 
//              and -1 otherwise.
//
// PURPOSE:	Look at all the checks connected to the var.  If the var
//              has less than 2 unsatisfied checks then set the value of
//              the var so that all connected checks are satisfied.
//
// ----------------------------------------------------------------------

static int SetVarIfItHasLessThanTwoChecks(SatVarNode*const curVarNode) {
  int curEdge;
  int edgeToSetFrom=-1;

  if ('f' == curVarNode->varType) return -1;
  for (curEdge=0; curEdge < curVarNode->numChecks; curEdge++) {
    if (DONT_KNOW == curVarNode->edges[curEdge]->msgToVar) {
      if (-1 == edgeToSetFrom) edgeToSetFrom = curEdge;
      else return -1; /* at least two unsatisfied checks found */
    } else { assert(ERASURE == curVarNode->edges[curEdge]->msgToVar); }
  }
  if (-1 == edgeToSetFrom) {
    curVarNode->llr = ZERO;
    edgeToSetFrom = 0;
  } else {
    curVarNode->llr = MaybeInvertMessage(ONE,curVarNode->edges[edgeToSetFrom]->
					 invert);
  }
  curVarNode->varType = 'f';
  curVarNode->channelEvidence = curVarNode->llr;
  for (curEdge=0; curEdge < curVarNode->numChecks; curEdge++) {
    curVarNode->edges[curEdge]->msgToCheck = 
      MaybeInvertMessage(curVarNode->llr,curVarNode->edges[curEdge]->invert);
  }
  return edgeToSetFrom; /* indicates we fixed the var */
}

static void SetVarsWithLessThanTwoChecks(SatVarNode*const vars,
					 const int numVars) {
  const SatVarNode*const lastVar = vars+numVars;
  SatVarNode* curVarNode;
  
  for (curVarNode=vars; curVarNode < lastVar;curVarNode++) {
    SetVarIfItHasLessThanTwoChecks(curVarNode);
  }
}

static void SendErasureMessagesToAllVarsForCheck(SatCheckNode*const check,
						 VPSet*const undecidedVars) {
  const int numEdges = check->numVars;
  int curEdge;
  int inSet;

  for(curEdge=0; curEdge < numEdges; curEdge++) {
    check->edges[curEdge]->msgToVar = ERASURE;
    if (SetVarIfItHasLessThanTwoChecks(check->edges[curEdge]->var) >= 0) {
      inSet = VPSetDelete(undecidedVars,check->edges[curEdge]->var);
      assert(inSet);
    }
  }
}


void PrepareSGForMesgPassSolveSat(SatGraph*const sg) {
  int i;

  for (i=0; i < sg->numVNodes; i++) {
    sg->vNodes[i].channelEvidence = DONT_KNOW;
    sg->vNodes[i].llr = DONT_KNOW;
    sg->vNodes[i].varType = 'n';
  }

  for (i=0; i < sg->numEdges; i++) {
    sg->edges[i].msgToVar = DONT_KNOW;
    sg->edges[i].msgToCheck = DONT_KNOW;
  }
  SetVarsWithLessThanTwoChecks(sg->vNodes,sg->numVNodes);
}

/* 
   The following prototype is required since 
   PropagateZeroOneMessagesFromCheck and PropagateZeroOneMessagesFromVar
   recursively call each other.
*/
static SatCheckNode *PropagateZeroOneMessagesFromSatCheck
(SatCheckNode*const check,VPSet* undecidedVars);


/* ----------------------------------------------------------------------
//
// FUNCTION:	PropagateZeroOneMessagesFromSatVar
//
// INPUTS:	var: A variable which has just been determined 
//                   to be a ONE or a ZERO and needs to propagate this to
//                   all assoicated checks.
//              undecidedVars: A set of vars which have not yet
//                             determined whether they are ONE or ZERO.
//
// RETURNS:     If a check can not be satisfied then a pointer to that
//              SatCheckNode is returned, otherwise NULL is returned.
//
// PURPOSE:	First, set the channel evidence of var to reflect that 
//              var is now a ONE or ZERO.  Then remove var from
//              undecidedVars.  Finally, send out this value to
//              all associated checks and call 
//              PropagateZeroOneMessagesFromSatCheck to continue propagating
//              this message.
//
//              The main idea behind this function is that as soon as we
//              decided that a var is a ZERO or a ONE, we need to propagate
//              this information as far as we can in the graph.
//
// MODIFIED:	Mon Jul 14, 2003
//
// -------------------------------------------------------------------- */


static SatCheckNode* PropagateZeroOneMessagesFromSatVar(SatVarNode*const var, 
							VPSet* undecidedVars) {
  int curCheck, inSet;
  SatCheckNode* contradiction = NULL;

  if (var->channelEvidence == ZERO || var->channelEvidence == ONE) return NULL;
  assert(var->llr == ZERO || var->llr == ONE);
  assert(var->channelEvidence == DONT_KNOW);

  var->channelEvidence = var->llr; 
  inSet = VPSetDelete(undecidedVars,var);
  assert(inSet); 
  for (curCheck = 0; curCheck < var->numChecks; curCheck++) {
    assert(DONT_KNOW == var->edges[curCheck]->msgToCheck ||
	   ERASURE == var->edges[curCheck]->msgToCheck);
    var->edges[curCheck]->msgToCheck = 
      MaybeInvertMessage(var->llr,var->edges[curCheck]->invert);
  }
  for (curCheck = 0; curCheck < var->numChecks; curCheck++) {
    if ( (contradiction = /* assignment intentional */
	  PropagateZeroOneMessagesFromSatCheck(var->edges[curCheck]->check,
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


static void FindEdgeToSendCheckMessageOn(const SatCheckNode*const check,
					 int*const edgeToSendMessageOn,
					 int*const checkSatsified) {
  int curEdge;
  *checkSatsified=0;
  *edgeToSendMessageOn=-1;

  for (curEdge=0; curEdge < check->numVars; curEdge++) {
    assert(ERASURE != check->edges[curEdge]->msgToCheck);
    if ( DONT_KNOW == check->edges[curEdge]->msgToCheck) {
      if (-1 == *edgeToSendMessageOn) {
	*edgeToSendMessageOn = curEdge;
      } else { /* at least two edges with DONT_KNOW or ERASURE coming in */
	*edgeToSendMessageOn=-2; /* so we can't send any 0/1 messages out. */
	return;
      }
    } else if (ZERO == check->edges[curEdge]->msgToCheck) {
      ; /* do nothing */
    } else if (ONE == check->edges[curEdge]->msgToCheck) {
      *checkSatsified=1;
      return;
    } else {
      printf("bad message seen in FindEdgeToSendCheckMessageOn\n");
      abort();
    }
  }
}

static void FreezeVarToValue(SatVarNode*const var, const double value) {
  assert(var->varType = 'n');
  var->llr = value;
  var->varType = 'f';
}

static void FreezeVarOnEdgeToValue(SatCheckNode*const check, 
				   const int edge, const double value) {
  SatVarNode*const var = check->edges[edge]->var;
  assert(value == check->edges[edge]->msgToVar ||
	 DONT_KNOW == check->edges[edge]->msgToVar);
  assert(value == check->edges[edge]->var->llr ||
	 DONT_KNOW == check->edges[edge]->var->llr);
  check->edges[edge]->msgToVar = value;
  FreezeVarToValue(var,value);
}

/* ----------------------------------------------------------------------
//
// FUNCTION:	PropagateZeroOneMessagesFromSatCheck
//
// INPUTS:	check: A check node connected to a variable which just
//                     decided to be ONE ore ZERO.
//              undecidedVars: A set of vars which have not yet decided
//                             to be ONE or ZERO.  This input is only
//                             used in passing it to
//                             PropagateZeroOneMessagesFromVar.
//
// RETURNS:     If a check can not be satisfied then a pointer to that
//              CheckNode is returned, otherwise NULL is returned.
//                              
// PURPOSE:	If the new value the variable selected causes the check
//              to become satisfied, then tell all connected vars that
//              the check is satisfied.
//              If all but one incoming messages are determined and the
//              check is still unsatisfied, then tell the remaning va
//              to satisfy the check and call 
//              PropagateZeroOneMessagesFromVar on that var.
//              Otherwise, do nothing.
//
// -------------------------------------------------------------------- */

static SatCheckNode* PropagateZeroOneMessagesFromSatCheck
(SatCheckNode*const check, VPSet* undecidedVars) {
  int checkSatsified, edgeToSendMessageOn;
  SatCheckNode* contradiction = NULL;

  FindEdgeToSendCheckMessageOn(check,&edgeToSendMessageOn,&checkSatsified);

  if (checkSatsified) {
    SendErasureMessagesToAllVarsForCheck(check,undecidedVars);
  } else if (-2 == edgeToSendMessageOn) { ; /* do nothing */
  } else if (-1 == edgeToSendMessageOn) { 
    contradiction = check; /* found a contradiction because there is nobody 
			    * that will take a message from this check even 
			    * though the check is not satisfied */
  } else {
    FreezeVarOnEdgeToValue(check,edgeToSendMessageOn,
			   MaybeInvertMessage(ONE,check->edges
					      [edgeToSendMessageOn]->invert));
    contradiction = PropagateZeroOneMessagesFromSatVar
      (check->edges[edgeToSendMessageOn]->var,undecidedVars);
  }
  return contradiction;
}

static int VarUndecidedP(const SatVarNode*const v) {
  return ((v->channelEvidence == DONT_KNOW) && 
	  (ONE != v->llr) && (ZERO != v->llr));
}

static double ParityForVar(const SatVarNode*const v) {
  int curEdge;
  double result = ERASURE;
  
  for (curEdge=0; curEdge < v->numChecks; curEdge++) {
    if (DONT_KNOW == v->edges[curEdge]->msgToVar) return DONT_KNOW;
    else if (ERASURE == result) result = v->edges[curEdge]->msgToVar;
    else if (v->edges[curEdge]->msgToVar != result) return DONT_KNOW;
  }
  if (ERASURE == result) return ZERO;
  else return result;
}

/* ----------------------------------------------------------------------
//
// FUNCTION:	FindFreeVar
//
// INPUTS:	var: A var to check for freeness
//              alreadyTriedVars: Set of vars we have already tried to
//                                pursue on the current iteration.
//              undecidedVars:    Set of undecided vars.
//              varsSet:          Pointer to an int keeping track of how
//                                many free vars have been set.  This var
//                                is incremented whenever we set a free var.
//              verbose:          Indicates whether to print debug messages.
//
// RETURNS:     If a check can not be satisfied then a pointer to that
//              CheckNode is returned, otherwise NULL is returned.
//
// PURPOSE:	This function goes through all the checks connected to
//              var and computes incoming messages to var.  If all
//              incoming messages are ERASURE then var is connected only to
//              checks which are satisfied and can be arbitrarily set.
//              If all but one incoming messages are ERAUSRE and the
//              remaning message is DONT_KNOW then we can set the var
//              to satisfy the check sending a DONT_KNOW.
//
//              No matter what happens, var is added to alreadyTriedVars
//              so that the caller can avoid calling FindFreeVar
//              on already tried vars.
//
// ------------------------------------------------------------------- */

static SatCheckNode* FindFreeVar(SatVarNode*const var,
				 VPSet*const alreadyTriedVars,
				 VPSet*const undecidedVars,
				 int*const varsSet, const int verbose) {
  double parity;
  int curCheck, inSet;
  SatGraphEdge* edgeToSendOn = NULL;

  VPSetPush(alreadyTriedVars,var);
  for (curCheck=0; curCheck < var->numChecks; curCheck++) {
    if (DONT_KNOW == var->edges[curCheck]->msgToVar) {
      SendSatMessageFromCheckOnEdge(var->edges[curCheck]->check,
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
  
  if (NULL != edgeToSendOn) { 
    /* this var got all ERASUREs in except for edgeToSendOutOn */
    if (verbose) printf("found var whose value is obvious: %x\n",(int)var);
    var->llr = MaybeInvertMessage(ONE,edgeToSendOn->invert);
    var->varType = 'f';
    (*varsSet)++;
    return PropagateZeroOneMessagesFromSatVar(var,undecidedVars);
  } else if (DONT_KNOW != (parity = ParityForVar(var))) {
    if (verbose) printf("found var w/ constant parity to set: %x\n",(int)var);
    FreezeVarToValue(var,ZERO);
    inSet = VPSetDelete(undecidedVars,var);
    assert(inSet);
    (*varsSet)++;
    return PropagateZeroOneMessagesFromSatVar(var,undecidedVars);
  }
  return NULL;
}

// ----------------------------------------------------------------------
//
// FUNCTION:	ProcessUndecidedVars
//
// INPUTS:	alreadyTriedVars: A set containing pointers to vars we
//                                have already tried to process.  This set
//                                should be empty in the first call to
//                                ProcessUndecidedVars
//              undecidedVars:  A set containing pointers to vars which
//                              have not yet decided to be ONE/ZERO.
//              verbose: Indicates whether to print debug messages.
//
// RETURNS:     If a check can not be satisfied then a pointer to that
//              CheckNode is returned, otherwise NULL is returned.
//
// PURPOSE:	This function goes through all the vars in undecidedVars
//              and tries to find an undecided var connected to 0/1
//              not yet satisfied clause.  If such a var is found, then
//              that var is set to satisfy the clause and this effect is
//              propagated out from that claue.
//
// ----------------------------------------------------------------------
    

static SatCheckNode* ProcessUndecidedVars
(VPSet*const alreadyTriedVars, VPStack*const varsToTry,
 VPSet*const undecidedVars,const int verbose) {
  SatCheckNode* contradiction;
  SatVarNode* var;
  int varsSet;
  int varsTried=0;

  while (VPStackSize(varsToTry) > 0) {
    varsSet = 0;
    varsTried++;
    var = VPStackPop(varsToTry);
    if (verbose) { 
      printf("trying to process var %x:\n",(int)var);
      PrintSatVarNode(var);
    }
    if (VPSetMemberP(alreadyTriedVars,var) || !VarUndecidedP(var)) continue;
    if ( (contradiction = /* assignment intentional */
	  FindFreeVar(var,alreadyTriedVars,undecidedVars,&varsSet,verbose)) )
      return contradiction;
    if (varsSet) {
      if (verbose) printf("set %i free vars after %i tries, %i undecided\n",
			  varsSet,varsTried,VPSetSize(undecidedVars));
    }
  }
  return NULL;
}

typedef struct CDForMesgPassSolveSat {
  char*  error;
  VPSet* undecidedVars;
} CDForMesgPassSolveSat;

CDForMesgPassSolveSat* CreateCDForMesgPassSolveSat(void) {
  CDForMesgPassSolveSat*const clientData 
    = SafeMalloc(sizeof(CDForMesgPassSolveSat));
  clientData->undecidedVars = VPSetCreate();
  clientData->error = NULL;
  return clientData;
}

void DestroyCDForMesgPassSolveSat(void* cd, const char*const type) {
  CDForMesgPassSolveSat* clientData = cd;
  assert(0 == strcmp("MESG_PASS_SOLVE_SAT",type));
  VPSetDestroy(clientData->undecidedVars);
  free(clientData->undecidedVars);
  if (NULL != clientData->error) free(clientData->error);
}


char* DeallocateDataForMesgPassSolveSat(SatGraph* sg, const int verbose) {
  CDForMesgPassSolveSat* clientData = 
    (CDForMesgPassSolveSat*) sg->algorithm->clientData;
  DestroyCDForMesgPassSolveSat(clientData,sg->algorithm->algorithmName);
  sg->algorithm->clientData = NULL;
  sg->algorithm = NULL;
  return NULL;
}

static char* DescribeContradiction(SatGraph*const sg, 
				   SatCheckNode*const contradiction);

// ----------------------------------------------------------------------
//
// FUNCTION:	PrepareForMesgPassSolveSat
//
// INPUTS:	sg: A SatGraph to prepare for the MesgPassSolveSat algorithm.
//
// PURPOSE:	Initializes client data for sg, sends messages from
//              observation vars and to checks and propagates any
//              necessary messages.
//
// MODIFIED:	Mon Jul 14, 2003
//
// ----------------------------------------------------------------------

char* PrepareForMesgPassSolveSat(SatGraph* sg, const int verbose) {

  SatVarNode* curVarNode;
  SatVarNode* lastVarNode;
  CDForMesgPassSolveSat** clientData = 
    (CDForMesgPassSolveSat**) &(sg->algorithm->clientData);
  
  PrepareSGForMesgPassSolveSat(sg);

  assert(NULL == *clientData);
  assert(0 == strcmp("MESG_PASS_SOLVE_SAT",sg->algorithm->algorithmName));

  (*clientData) = CreateCDForMesgPassSolveSat();

  for (curVarNode=sg->vNodes,lastVarNode=sg->vNodes+sg->numVNodes;
       curVarNode < lastVarNode; curVarNode++) {
    if ('n' == curVarNode->varType) {
      assert(DONT_KNOW == curVarNode->channelEvidence);
      VPSetPush((*clientData)->undecidedVars,curVarNode);
    } else assert('f' == curVarNode->varType);
  }

  return NULL;
}

static void AddVarToVPStack(void*varPtr, void*vpStackPtr) {
  VPStackPush((VPStack*) vpStackPtr, varPtr);
}

static void AssertNoIncorrectlyStuckVarsForSatGraph(SatVarNode*const vars,
						    const int numVars) {
  printf("SUPPOSED TO BE ASSERTING NO INCORRECTLY STUCK VARS\n");
  printf("BUT THAT ASSERTION IS NOT IMPLEMENTED YET.\n");
}
 
static char* DescribeStuck(SatGraph*const sg) {
  CDForMesgPassSolveSat*const clientData = 
    (CDForMesgPassSolveSat*) sg->algorithm->clientData;
  assert(NULL == clientData->error);
  clientData->error = CopyString("stuck");
  AssertNoIncorrectlyStuckVarsForSatGraph(sg->vNodes,sg->numVNodes);
  return CopyString(clientData->error);
}

static char* DescribeContradiction(SatGraph*const sg, 
				   SatCheckNode*const contradiction) {
  CDForMesgPassSolveSat*const clientData = 
    (CDForMesgPassSolveSat*) sg->algorithm->clientData;
  char* result= SafeCalloc(strlen("contradiction on check ")+100,sizeof(char));

  sprintf(result,"contradiction on check %i",contradiction-sg->cNodes);
  assert(NULL == clientData->error);
  clientData->error = CopyString(result);
  return result;
}

char* DoMesgPassSolveSat(SatGraph* sg, const int verbose) {
  SatCheckNode* contradiction;
  char* result;
  VPStack* varsToTry = VPStackCreate(20);
  VPSet* alreadyTriedVars = VPSetCreate();
  CDForMesgPassSolveSat* clientData = 
    ((CDForMesgPassSolveSat*)sg->algorithm->clientData);
  VPSet* undecidedVars = clientData->undecidedVars;

  assert(0 == strcmp("MESG_PASS_SOLVE_SAT",sg->algorithm->algorithmName));

  if (NULL != clientData->error) {
    result = CopyString(clientData->error);
  } else {
    VPSetForeach2(undecidedVars,AddVarToVPStack,varsToTry);
    contradiction = ProcessUndecidedVars(alreadyTriedVars,varsToTry,
					 undecidedVars,verbose);
    if (NULL != contradiction) {
      result = DescribeContradiction(sg,contradiction);
    } else if (VPSetSize(undecidedVars) > 0) {
      result = DescribeStuck(sg);
    } else result = NULL;
  }

  VPStackDestroy(varsToTry);
  free(varsToTry);
  VPSetDestroy(alreadyTriedVars);
  free(alreadyTriedVars);
  return result;
}
   
const SatGraphAlgorithm MesgPassSolveSatAlgorithm = {
  "MESG_PASS_SOLVE_SAT",
  NULL,
  GetSatBeliefs,
  DeallocateDataForMesgPassSolveSat,
  PrepareForMesgPassSolveSat,
  DoMesgPassSolveSat
};
