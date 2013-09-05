
#include <math.h>
#include <string.h>

#include "misc.h"
#include "SatGraph.h"
#include "SatSolve.h"
#include "SatGraphAlgorithm.h"

/* See comments in SatGraphAlgorithm.h */

char* SatAlgorithmDoNothing(SatGraph* sg, const int verbose) {
  return NULL;
}

SatGraphAlgorithm* MakeSGAlgInstance(const SatGraphAlgorithm*const a) {
  SatGraphAlgorithm* result = SafeMalloc(sizeof(SatGraphAlgorithm));
  memcpy(result,a,sizeof(SatGraphAlgorithm));
  return result;
}

const SatGraphAlgorithm*const SatGraphAlgorithms[] = {
  &MesgPassSolveSatAlgorithm,
  &MesgPassSolveSatAlgorithm,
  NULL /* sentinel */
};

const char*const SatGraphAlgorithmNames[] = {
  "default", 
  "MYBP",
  NULL /* sentinel */
};
