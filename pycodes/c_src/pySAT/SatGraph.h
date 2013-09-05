
/* 
   This file contains the SatGraph data structure and associated
   sub-structures.  As the name suggests, this data structure
   represents a Boolean circuit that we want to test for satisfiablility.
*/

#ifndef __INC_SATGRAPH
#define __INC_SATGRAPH


struct SatVarNode;
struct SatCheckNode;
struct SatGraphAlgorithm;

typedef struct SatGraphEdge {
  struct SatVarNode*   var;
  struct SatCheckNode* check;
  double               msgToVar; /* after potential inversion done */
  double               msgToCheck; /* after potential inversion done */
  unsigned char        invert; /* 1 if invert var, 0 if not */
} SatGraphEdge;

typedef struct SatVarNode {
  char           varType;
  short          numChecks; /* number of checks connected to this var */
  SatGraphEdge**    edges; /* each of the numChecks edges points to a check */
  double         llr;
  double         channelEvidence;
  int            counter;
} SatVarNode;

typedef struct SatCheckNode {
  int            numVars;
  SatGraphEdge** edges;
} SatCheckNode;


typedef struct SatGraph {
  double        maxLLR;
  double        minLLR;
  int           numVNodes; 
  int           numCNodes; 
  int           numEdges;
  SatVarNode*   vNodes;
  SatCheckNode* cNodes;
  SatGraphEdge* edges;

  struct SatGraphAlgorithm* algorithm;
} SatGraph;

void DestroySatGraph(SatGraph*const);

SatGraph* CreateSatGraphFromLinkArrays(const int numVars, /* N=num variables */
				       const int numChecks, /* M=num clauses */
				       const int numEdges,
				       const int*const linkArrayLengths,
				       int** linkArrays);

void PrintSatVarNode(const SatVarNode*const var);
void PrintSatVarNodeArray(const SatVarNode* vars, const int numVars);
#endif
