
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

/* This file contains the CodeGraph data structure and associated
   sub-structures.  As the name suggests, this data structure
   represents a code via a graph of nodes and edges.  The picture 
   you should have in your mind should be something like

    O  O  O  O 	O  O  O	 O
    \  |   \/  	|   \/ 	 |
     \ |   /\  	|   /\ 	 |
      \|__/  \ 	|__/  \  |__
      |   |   \|   |   \|   |
      | + |    | + |	| + |
      |___|    |___|	|___|
      
   where the O's on top are variable nodes and the boxes on the bottom
   are check nodes.  The CodeGraph data structures stores the check
   nodes, variable nodes, and edges.  This is somewhat redundant, but
   makes it easier to implement different algorithms, do visualization,
   and diagnose problems.

*/

#ifndef __INC_CODEGRAPH
#define __INC_CODEGRAPH

#include "Nodes.h"

typedef struct CodeGraphAlgorithm CodeGraphAlgorithm;

typedef struct CodeGraph {
  double        maxLLR;
  double        minLLR;
  int           k; /* k may be > numVNodes-numCNodes with redundant checks */
  int           numVNodes; /* n */
  int           numCNodes; /* m */
  int           numEdges;
  VariableNode* vNodes;
  CheckNode*    cNodes;
  GraphEdge*    edges;

  CodeGraphAlgorithm* algorithm;
} CodeGraph;

void DestroyCodeGraph(CodeGraph*const);

CodeGraph* CreateGraphFromLinkArrays(const int n, const int k, const int m,
				     const int numEdges,
				     const int*const linkArrayLengths,
				     int** linkArrays);

#endif
