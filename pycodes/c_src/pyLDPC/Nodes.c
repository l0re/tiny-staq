
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

#include "Nodes.h"
#include "misc.h"

void InitCheckNode(CheckNode*const cn, const int numVars) {
  cn->numVars = numVars;
  cn->edges = SafeCalloc(numVars,sizeof(GraphEdge*));
}

/* The numChecks field must already be set */
void InitVarNode(VariableNode*const vn) {
  vn->edges = SafeCalloc(vn->numChecks,sizeof(GraphEdge*));
}

void DestroyCheckNode(CheckNode*const cn) {
  free(cn->edges); 
}


void DestroyVarNode(VariableNode*const vn) {
  free(vn->edges); 
}


void PrintCheckNode(const CheckNode*const check) {
  int i;
  for (i=0; i < check->numVars; i++) {
    printf("messagesIn[%i]=%g, ",i,check->edges[i]->msgToCheck);
  }
  printf("\n");
  for (i=0; i < check->numVars; i++) {
    printf("messagesOut[%i]=%g, ",i,check->edges[i]->msgToVar);
  }
  printf("\n\n");
}

void PrintVariableNode(const VariableNode*const var) {
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

void PrintVariableNodeArray(const VariableNode* vars, const int numVars) {
  const VariableNode*const lastVar = vars+numVars;
  while(vars < lastVar) PrintVariableNode(vars++);
}
  

void LinkVarsBackToChecks(const int numVars, VariableNode*const vNodes,
			  const int numEdges, GraphEdge*const edges) {
  int curVar, curEdge;
  VariableNode* curVarNode;

  for (curVar=0; curVar < numVars; curVar++) 
    InitVarNode(&(vNodes[curVar]));
  for (curEdge=0; curEdge < numEdges; curEdge++) {
    curVarNode = edges[curEdge].var;
    curVarNode->edges[curVarNode->counter++] = &(edges[curEdge]);
  }
}
