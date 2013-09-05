
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
#include "SumProductBP.h"

#include <math.h>
#include <assert.h>

/* ************************************************************
 * 
 * The routines in this file which have headers in SumProductBP.h
 * are designed to be used in sum-product belief propagation.
 * For an example, of how to do this see the example in 
 * CodeGraphSumProductAlgs.c for doing sum-product belief propagation.
 *
 * ************************************************************ */

const double gEffectivelyZero = 1e-50;
const double gEffectivelyInf  = 1e50;

/* ---------------------------------------------------------------------
//
// FUNCTION:	SendBPMessagesFromChannelEvidenceToVariables
//
// INPUTS:	vars:    An array of variables to get evidence messages.
//              numVars: Number of variables in vars.
//
// PURPOSE:	Take the channel evidence for each variable which is
//              in the form of a likelihood ratio
//                 Pr(channel|var=0)/Pr(channel|var=1) and add it
//              to each variable LLR.
//
// MODIFIED:	Mon Jun 09, 2003
//
// --------------------------------------------------------------------*/


void SendBPMessagesFromChannelEvidenceToVariables(VariableNode*const vars,
						  const int numVars) {
  int i;

  for (i = 0; i < numVars; i++) {    
    vars[i].llr += vars[i].channelEvidence;
  }
}    

void ResetVariableLLRs(VariableNode*const vars,const int numVars) {
  int i;

  for (i = 0; i < numVars; i++) {    
    vars[i].llr = 0;
  }
}    


void SendBPMessagesFromVariablesToChecks(VariableNode*const vars,
					 const int numVars) {
  int curCheck;
  VariableNode* curVarNode;
  const VariableNode*const varNodesEnd = vars+numVars;

  for(curVarNode=vars; curVarNode < varNodesEnd; curVarNode++) {
    for(curCheck=0; curCheck < curVarNode->numChecks; curCheck++) {
      curVarNode->edges[curCheck]->msgToCheck = curVarNode->llr -
	curVarNode->edges[curCheck]->msgToVar;
      if (fabs(curVarNode->edges[curCheck]->msgToCheck) <= gEffectivelyZero)
	curVarNode->edges[curCheck]->msgToCheck = 10*gEffectivelyZero;
    }
  }
}

/* ----------------------------------------------------------------------
//
// FUNCTION:	SendBPMessagesFromChecksToVariables
//
// INPUTS:	checks:    An array of check nodes which need to send
//                         messages to their associated variables.
//              numChecks: Number of check nodes in the array.
//              maxLLR,minLLR:  Minimum and maximum values for LLRs.
//                              Values greater than this are clipped
//                              to prevent overflow.
//
// PURPOSE:	Goes through all checks and computes the messages from
//              that check to assoicated variables and updates the 
//              the likelihoods for the variable nodes.
//
//              To update the likelihoods for a variable from a given
//              check, we set that variable value to 0/1 and SUM the
//              probability over all possible values of the other
//              variables which satisfy the check.  This gives us
//              Pr(vars related to check|current var = 0/1).  We then
//              multiply these likelihoods by the current likelihoods
//              for the var.  After this process is completed each
//              variable node will contain products of such
//              likelihoods.  Note that to speed up calcuations we
//              use the "tanh rule".
//
// MODIFIED:	Thu Jun 05, 2003
//
// ---------------------------------------------------------------------*/

/* using tanh rule */
void SendBPMessagesFromChecksToVariables(CheckNode*const checks,
					 const int numChecks,
					 const double maxLLR,
					 const double minLLR) {
  int curVar, numVars, i;
  double rawMesg,  tanhProduct; /* tanh u/2 = - Prod_j tanh v_j/2 */
  double tanhDivisor;
  CheckNode* curCheckNode;
  const CheckNode*const checkNodesEnd = checks+numChecks;

  for(curCheckNode=checks; curCheckNode<checkNodesEnd; curCheckNode++) {
    numVars=curCheckNode->numVars;
    tanhProduct = 1.0;
    for (i=0; i < numVars; i++) {
      assert(fabs(curCheckNode->edges[i]->msgToCheck) >= gEffectivelyZero/2);
      tanhProduct *= tanh( curCheckNode->edges[i]->msgToCheck / 2.0 );
    }
    for (curVar=0; curVar < numVars; curVar++) {
      tanhDivisor = tanh(curCheckNode->edges[curVar]->msgToCheck / 2.0);
      assert(fabs(tanhDivisor) > gEffectivelyZero);
      rawMesg = atanh(tanhProduct/tanhDivisor);
      if (rawMesg > maxLLR) rawMesg = maxLLR;
      else if (rawMesg < minLLR) rawMesg = minLLR;
      rawMesg *= 2;
      curCheckNode->edges[curVar]->msgToVar = rawMesg;
      curCheckNode->edges[curVar]->var->llr += rawMesg;
    }
  }
}

/* *********************************************************************
 *
 * The following are routines for doing the sum-product algorithm
 * where messages are likelihood-ratios instead of log-likelihood-ratios.
 * This is most useful when running the dual sum-product on a dual code
 * since the dual sum-product algorithm can have negative messages
 * which would become complex if you took the logarithm.
 *
 * ********************************************************************* */

void SendLRBPMessagesFromChannelEvidenceToVariables(VariableNode*const vars,
						    const int numVars) {
  int i;

  for (i = 0; i < numVars; i++) {    
    vars[i].llr *= vars[i].channelEvidence;
  }
}    

void ResetVariableLikelihoodRatios(VariableNode*const vars,const int numVars) {
  int i;

  for (i = 0; i < numVars; i++) {    
    vars[i].llr = 1;
  }
}    


void SendLRBPMessagesFromVariablesToChecks(VariableNode*const vars,
					   const int numVars) {
  int curCheck;
  VariableNode* curVarNode;
  const VariableNode*const varNodesEnd = vars+numVars;

  for(curVarNode=vars; curVarNode < varNodesEnd; curVarNode++) {
    for(curCheck=0; curCheck < curVarNode->numChecks; curCheck++) {
      assert(! isnan(curVarNode->llr/curVarNode->edges[curCheck]->msgToVar));
      curVarNode->edges[curCheck]->msgToCheck = curVarNode->llr /
	curVarNode->edges[curCheck]->msgToVar;
      if (fabs(curVarNode->edges[curCheck]->msgToCheck) <= gEffectivelyZero)
	curVarNode->edges[curCheck]->msgToCheck = 10*gEffectivelyZero;
    }
  }
}

/* ----------------------------------------------------------------------
//
// FUNCTION:	TwoPointDFT
//
// INPUTS:	A likelihood-ratio of the form Pr[0]/Pr[1]
//
// RETURNS:	The likelihood-ratio of the 2 point 
//              Discrete Fourier Transform (DFT).
//
// PURPOSE:	The DFT of a 2-point vector is (p,q) -> (p+q,p-q).
//              Since the input is of the form of the ratio L=p/q and 
//              we want the output in the form of the ratio 
//              L' = (p+q)/(p-q) we note that 
//
//                    (p+q)/q     p/q + 1   L + 1
//              L' =  -------  =  ------- = -----
//                    (p-q)/q     p/q - 1   L - 1
//
//              Note that we can use the same formula to get L from L':
//
//
//              L' + 1   (L+1)/(L-1) + 1   L+1 + (L-1)   2 L
//              ------ = --------------- = ----------- = --- = L.
//              L' - 1   (L+1)/(L-1) - 1   L+1 - (L-1)    2
//
// See Also:    InverseTwoPointDFT
//
// MODIFIED:	Fri Aug 08, 2003
//
// ------------------------------------------------------------------- */

double TwoPointDFT(const double lr) {
  double result;
  if (lr >= gEffectivelyInf)
    result = 1.0;
  else if ( fabs(1-lr) < gEffectivelyZero )
    result = gEffectivelyInf;
  else
    result = (lr + 1.0)/(lr - 1.0); 
  return result;
}

double InverseTwoPointDFT(const double lrFT) {
  return TwoPointDFT(lrFT);
}

// ----------------------------------------------------------------------
//
// FUNCTION:	SendLRBPMessagesFromChecksToVariables
//
// INPUTS:	checks:     An array of check nodes.
//              numChecks:  Number of CheckNode objects in checks array.
//
// PURPOSE:	Compute the outgoing message from each CheckNode in the
//              checks array and send it to the appropriate variable.
//              The rule to compute the output message to var i is to
//              compute the probability that the check is satisfied given
//              var i is 0 and divide by the probability that the check
//              is satisfied given that var i is 1.
//
//              The brute force way to do this is to sum over all
//              configurations where var i is 0 and the check is
//              satisifed (i.e. parity = 0) of the product of the
//              incoming messages divided by the same sum where var i
//              is 1.  Essentially what the brute force approach does
//              is to compute the pdf for v_0 + v_1 + ... + v_n using
//              the standard formula that the pdf of a sum of random
//              variables is the convolution of the pdfs.
//
//              Clearly, a more efficient approach than computing the
//              convolution directly is to Fourier Transform each pdf,
//              multiply the Fourier Transforms and take the Inverse
//              Fourier transform to compute the pdf for 
//              v_0 + v_1 ... + v_n.  But there is one more trick we
//              can apply.  By computing the Discrete Fourier Transform
//              (DFT) instead of the Discrete Time Fourier Transform
//              (DTFT) we get aliasing.  That is we get circular 
//              convolution when we multiply DFTs as opposed to 
//              linear convolution when we multiply DTFTs.  Thus
//              All the even values for v_0 + v_1 ... + v_n alias onto
//              the 0 sample and all the odd values alias onto the odd
//              sample.  Since we are only interested in the parity,
//              though, that is just what we want.  
//
//              Thus we compute the pdf of the parity of v_0+v_1+...+v_n
//              by multiplying DFTs and taking the inverse DFT.  When
//              the messages are represented as log-likelihood-ratios
//              this leads to the "tanh-rule", but when we just have
//              likelihood-ratios, there is no fancy name and no tanh.
//
// MODIFIED:	Fri Aug 08, 2003
//
// ----------------------------------------------------------------------


void SendLRBPMessagesFromChecksToVariables(CheckNode*const checks,
					   const int numChecks) {

  int curVar, numVars, i;
  double rawMesg,  product; 
  CheckNode* curCheckNode;
  const CheckNode*const checkNodesEnd = checks+numChecks;
  for(curCheckNode=checks; curCheckNode<checkNodesEnd; curCheckNode++) {
    numVars=curCheckNode->numVars;
    product = 1.0;
    for (i=0; i < numVars; i++) {
      assert(fabs(curCheckNode->edges[i]->msgToCheck) >= gEffectivelyZero/2);
      product *= TwoPointDFT(curCheckNode->edges[i]->msgToCheck);
    }
    for (curVar=0; curVar < numVars; curVar++) {
      rawMesg = (product / 
		 TwoPointDFT(curCheckNode->edges[curVar]->msgToCheck));
      rawMesg = InverseTwoPointDFT(rawMesg);
      assert(! isnan(rawMesg));
      if (fabs(rawMesg) <= gEffectivelyZero) rawMesg = 10*gEffectivelyZero;
      curCheckNode->edges[curVar]->msgToVar = rawMesg;
      curCheckNode->edges[curVar]->var->llr *= rawMesg;
    }
  }
}
