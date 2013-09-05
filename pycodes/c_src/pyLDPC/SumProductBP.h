
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

#ifndef _INC_SUMPRODUCTBP
#define _INC_SUMPRODUCTBP

#include "Nodes.h"


/* 
 * The following variable is global.  It represents
 * a number which is effectively zero.  Any message values from
 * variables to checks should have absolute value at least this much.
 * That way, when we do a product like A*B*...*Z, we can recover the
 * product A*C*D*...*Z, A*B*D*...*Z, etc. by dividing out one term
 * without having to worry about division by 0.  See
 * SendBPMessagesFromVariablesToChecks and 
 * SendBPMessagesFromChecksToVariables for usage.
*/

const extern double gEffectivelyZero;
const extern double gEffectivelyInf;


void SendBPMessagesFromChannelEvidenceToVariables(VariableNode*const vars,
						  const int numVars);

void SendBPMessagesFromVariablesToChecks(VariableNode*const vars,
					 const int numVars);

void ResetVariableLLRs(VariableNode*const vars,const int numVars);

void SendBPMessagesFromChecksToVariables(CheckNode*const checks,
					 const int numChecks,
					 const double maxLLR,
					 const double minLLR);


void SendLRBPMessagesFromChannelEvidenceToVariables(VariableNode*const vars,
						    const int numVars);
void ResetVariableLikelihoodRatios(VariableNode*const vars,const int numVars);
void SendLRBPMessagesFromVariablesToChecks(VariableNode*const vars,
					   const int numVars);
void SendLRBPMessagesFromChecksToVariables(CheckNode*const checks,
					   const int numChecks);

double TwoPointDFT(const double lr);
double InverseTwoPointDFT(const double lrFT);
#endif
