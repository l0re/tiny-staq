
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

#ifndef _INC_DUALCODEGRAPHALGORITHMS
#define _INC_DUALCODEGRAPHALGORITHMS

#include "DualCodeGraph.h"

/* ----------------------------------------------------------------------
 *
 * The DualCodeGraphAlgorithm data structure represents an iterative 
 * algorithm which can be run on a DualCodeGraph object.  The idea is that
 * you create an instance of the DualCodeGraphAlgorithm class to represent
 * your algorithm and add that instance to the DualCodeGraphAlgorithms
 * and DualCodeGraphAlgorithmNames arrays.  Once you do this, you can
 * call your algorithm from python.
 *
 * ------------------------------------------------------------------- */

typedef char* (*DCAlgorithmMethod)(DualCodeGraph* dc,const int verbose);
typedef char* (*DCAlgorithmMethod2)(DualCodeGraph* dc,const int verbose,
				  void* arg);

struct DualCodeGraphAlgorithm {
  const char*            algorithmName;
  void*                  clientData;/* This field can be used by algorithms to
				     * store client info about the graph.*/
  const DCAlgorithmMethod2 SetEvidence;
  const DCAlgorithmMethod  GetBeliefs;
  const DCAlgorithmMethod  DeallocateAlgorithm;
  const DCAlgorithmMethod  InitializeAlgorithm;
  const DCAlgorithmMethod  DoIteration;
};

DualCodeGraphAlgorithm* MakeDCAlgInstance(const 
					  DualCodeGraphAlgorithm*const a);
char* DoNothing(DualCodeGraph*const dc, const int verbose);

const extern char*const DualCodeGraphAlgorithmNames[];
const extern DualCodeGraphAlgorithm*const DualCodeGraphAlgorithms[];

#endif
