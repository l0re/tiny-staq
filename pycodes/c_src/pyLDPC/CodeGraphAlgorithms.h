
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

#include "CodeGraph.h"

/* ----------------------------------------------------------------------
 *
 * The CodeGraphAlgorithm data structure represents an iterative 
 * algorithm which can be run on a CodeGraph object.  The idea is that
 * you create an instance of the CodeGraphAlgorithm class to represent
 * your algorithm and add that instance to the CodeGraphAlgorithms
 * and CodeGraphAlgorithmNames arrays.  Once you do this, you can
 * call your algorithm from python.
 *
 * ------------------------------------------------------------------- */

typedef char* (*CGAlgorithmMethod)(CodeGraph* cg,const int verbose);
typedef char* (*CGConstAlgorithmMethod)(const CodeGraph* cg,const int verbose);
typedef char* (*CGAlgorithmMethod2)(CodeGraph* cg,const int verbose,
				    void* arg);

struct CodeGraphAlgorithm {
  const char*            algorithmName;
  void*                  clientData;/* This field can be used by algorithms to
				     * store client info about the graph.*/
  const CGAlgorithmMethod2      SetEvidence;
  const CGAlgorithmMethod       GetBeliefs;
  const CGAlgorithmMethod       DeallocateAlgorithm;
  const CGAlgorithmMethod       InitializeAlgorithm;
  const CGAlgorithmMethod       DoIteration;
  const CGConstAlgorithmMethod  CountBadChecks;
};

CodeGraphAlgorithm* MakeCGAlgInstance(const CodeGraphAlgorithm*const a);

extern const CodeGraphAlgorithm*const CodeGraphAlgorithms[];
extern const char*const CodeGraphAlgorithmNames[];
