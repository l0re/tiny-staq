
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


/* This file contains the DualCodeGraph data structure and associated
   sub-structures.  As the name suggests, this data structure
   represents a dual code via a graph of nodes and edges.  The picture
   you should have in your mind should be something like the following
   for the normal code:

    _  _  _  _	_  _  _	 _
    |  |  |  | 	|  |  |	 |
    =  =  =  = _=  =  =	 =
    \  |  || \/	|  |\ |	 |
     \ |  |\_/\	|  | \|	 |
      \|__|/\  \|__| /\  |__
      |   |  \ |   |/  \|   |
      | + |   \| + |	| + |
      |___|    |___|	|___|
        |        |        |
       _|_      _|_      _|_
             _
   where the |'s on top are variable nodes with channel evidence, the
   _|_'s on the bottom are "hidden" variable nodes which are exactly 0
   and have essentially infintely strong channel evidence and the boxes
   on the bottom are check nodes.

   The dual code can be obtained as described by Forney by exchanging
   check nodes and = nodes to obtain:

    _  _  _  _	_  _  _	 _
    |  |  |  | 	|  |  |	 |
    +  +  +  + _+  +  +	 +
    \  |  || \/	|  |\ |	 |
     \ |  |\_/\	|  | \|	 |
      \|__|/\  \|__| /\  |__
      |   |  \ |   |/  \|   |
      | = |   \| = |	| = |
      |___|    |___|	|___|
        |        |        |
       _|_      _|_      _|_

   where the |'s on top are variable nodes with channel evidence, the
   +'s on the top are check nodes, and the _|_'s on the bottom are 
   variable nodes which we get to choose.

*/


#ifndef __INC_DUALCODEGRAPH
#define __INC_DUALCODEGRAPH

#include "Nodes.h"


typedef struct DualCodeGraphAlgorithm DualCodeGraphAlgorithm;

typedef struct DualCodeGraph {  
  int           n; /* Number of vars w/evidence at top of the figure above.*/
  int           k; /* Number of vars w/out evidence at bottom of figure.*/
  int           numVNodes; /* n+k, vars w/ evidence come last */
  int           numCNodes; 
  int           numEdges;
  VariableNode* vNodes;
  CheckNode*    cNodes;
  GraphEdge*    edges;

  DualCodeGraphAlgorithm* algorithm;
} DualCodeGraph;

void DestroyDualCodeGraph(DualCodeGraph*const);

DualCodeGraph* CreateDualGraphFromLinkArrays(const int n, const int k, 
					     const int m, const int numEdges,
					     const int*const linkArrayLengths,
					     int** linkArrays);
void DoSumProductBPOnDualCodeGraph(DualCodeGraph*const dc, const int numIter);
void PrintVars(const DualCodeGraph* const);

void PrintChecks(const DualCodeGraph* const);
#endif
