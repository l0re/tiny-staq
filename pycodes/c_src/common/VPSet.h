
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

#ifndef _INC_VPSET
#define _INC_VPSET

#include "tclHash.h"

/*
 * The VPSet data structure represents a set of void pointers.  
 * The VPSet is currently implemented via hash tables from Tcl.
 */

typedef struct VPSet {
  Tcl_HashTable hashTable;
} VPSet;

VPSet* VPSetCreate(void);
VPSet* VPSetCopy(VPSet*const v);
void VPSetDestroy(VPSet*const v);
void VPSetClear(VPSet*const v);
int VPSetPush(VPSet*const v, void*const p);
void* VPSetPop(VPSet*const v);
int VPSetDelete(VPSet*const v, const void*const p);
int VPSetSize(const VPSet*const v);
int VPSetMemberP(VPSet*const v, const void*const p);
int VPSetEmpty(const VPSet*const v);

void VPSetForeach(VPSet*const v, void (*func)(void*));
void VPSetForeach2(VPSet*const v, void (*func)(void*,void*),
		   void* arg2Value);
void VPSetForeach3(VPSet*const v, void (*func)(void*,void*,void*),
		   void* arg2Value, void* arg3Value);

typedef void(*ForeachFunc)(void*);
typedef void(*Foreach2Func)(void*,void*);
typedef void(*Foreach3Func)(void*,void*,void*);
#endif
