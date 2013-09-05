
#ifndef _INC_SGALG
#define _INC_SGALG

#include "SatGraph.h"

/* ----------------------------------------------------------------------
 *
 * The SatGraphAlgorithm data structure represents an iterative 
 * algorithm which can be run on a SatGraph object.  The idea is that
 * you create an instance of the SatGraphAlgorithm class to represent
 * your algorithm and add that instance to the SatGraphAlgorithms
 * and SatGraphAlgorithmNames arrays.  Once you do this, you can
 * call your algorithm from python.
 *
 * ------------------------------------------------------------------- */

typedef char* (*SGAlgorithmMethod)(SatGraph* sg,const int verbose);
typedef char* (*SGConstAlgorithmMethod)(const SatGraph* sg,const int verbose);

typedef struct SatGraphAlgorithm {
  const char*            algorithmName;
  void*                  clientData;/* This field can be used by algorithms to
				     * store client info about the graph.*/
  const SGAlgorithmMethod       GetBeliefs;
  const SGAlgorithmMethod       DeallocateAlgorithm;
  const SGAlgorithmMethod       InitializeAlgorithm;
  const SGAlgorithmMethod       DoIteration;
} SatGraphAlgorithm;

SatGraphAlgorithm* MakeSGAlgInstance(const SatGraphAlgorithm*const a);
char* SatAlgorithmDoNothing(SatGraph* sg, const int verbose);

extern const SatGraphAlgorithm*const SatGraphAlgorithms[];
extern const char*const SatGraphAlgorithmNames[];

#endif
