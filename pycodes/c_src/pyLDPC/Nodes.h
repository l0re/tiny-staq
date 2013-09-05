
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

/* 
   This file creates the structures for various nodes such as
   VariableNode and CheckNode and also for GraphEdge.  These nodes and
   edges can be connected to make a code graph using CodeGraph object.
*/

#ifndef _INC_NODES
#define _INC_NODES

struct VariableNode;
struct CheckNode;

/* ************************************************************
 *
 * The GraphEdge data structure is used to link VariableNodes to
 * CheckNodes in a bipartite graph.  Usually we use messages 
 * which correspond to likelihood or log-likelihood ratios and thus 
 * only need a single double for each message to/from a var.
 *
 * ********************************************************* */
typedef struct GraphEdge {
  struct VariableNode*  var;
  struct CheckNode*     check;
  double                msgToVar; 
  double                msgToCheck; 
} GraphEdge;


/* ************************************************************
 *
 * Usually, the VariableNode stores a log likelihood ratio (llr) for 
 * itself as well as the associated channel evidence.  We store the llr as 
 * log ( Pr[...|0] / Pr[...|1] ) because this makes the tanh rule
 * simpler.  (If we stored the reverse, we would need minus signs in
 * applying the tanh rule).
 *
 * ********************************************************* */

typedef struct VariableNode {
  double         llr; /* log( Pr(observations|0) / Pr(observations|1) ) */
  double         channelEvidence; /* log( Pr(channel|0) / Pr(channel|1) ) */
  short          numChecks; /* number of checks connected to this var */
  GraphEdge**    edges; /* each of the numChecks edges points to a check */
  short          counter; /* used to keep track of various things */
  char           varType; /* represents type of the var */
} VariableNode;

typedef struct CheckNode {
  int            numVars;
  GraphEdge**    edges;
} CheckNode;

void PrintCheckNode(const CheckNode*const check);
void PrintVariableNode(const VariableNode*const var);
void PrintVariableNodeArray(const VariableNode* vars, const int numVars);
void InitCheckNode(CheckNode*const cn, const int numVars);
void InitVarNode(VariableNode*const vn);
void DestroyCheckNode(CheckNode*const cn);
void DestroyVarNode(VariableNode*const vn);

void LinkVarsBackToChecks(const int numVars, VariableNode*const vNodes,
			  const int numEdges, GraphEdge*const edges);

#endif
