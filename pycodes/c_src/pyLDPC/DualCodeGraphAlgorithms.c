
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

#include <string.h>

#include "misc.h"
#include "DualCodeGraphAlgorithms.h"
#include "DualCodeGraphSumProductAlgs.h"
#include "BSCQuant.h"
#include "BECQuant.h"

DualCodeGraphAlgorithm* MakeDCAlgInstance(const 
					  DualCodeGraphAlgorithm*const a) {
  DualCodeGraphAlgorithm* result = SafeMalloc(sizeof(DualCodeGraphAlgorithm));
  memcpy(result,a,sizeof(DualCodeGraphAlgorithm));
  return result;
}

const char*const DualCodeGraphAlgorithmNames[] = {
  "default","LRSumProductBP","BECQuant","AccBitFlip", 
  "PureBitFlip", "SimAnneal", "ForcedTreeMinSum",
  NULL /* sentinel */
};


const DualCodeGraphAlgorithm*const DualCodeGraphAlgorithms[] = {
  &LRSumProductBP,
  &LRSumProductBP,
  &ProvablyGoodBECQuantAlgorithm,
  &AcceleratedBitFlipBSCQuantAlg, 
  &PureBitFlipBSCQuantAlg, 
  &SimulatedAnnealingBSCQuantAlg, 
  &ForcedTreeMinSumBSCQuantAlg,
  NULL /*sentinel */
};
