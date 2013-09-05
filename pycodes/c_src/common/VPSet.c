
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

#include "VPSet.h"
#include "misc.h"

VPSet* VPSetCreate() {
  VPSet*result = SafeMalloc(sizeof(VPSet));
  Tcl_InitHashTable(&(result->hashTable),TCL_ONE_WORD_KEYS);
  return result;
}

static void AddItemToVPSet(void*const item, VPSet*const set) {
  VPSetPush(set,item);
}

VPSet* VPSetCopy(VPSet*const v) {
  VPSet* result = VPSetCreate();
  VPSetForeach2(v,(Foreach2Func) AddItemToVPSet,result);
  return result;
}

void VPSetDestroy(VPSet*const v) {
  Tcl_DeleteHashTable(&(v->hashTable));
}

void VPSetClear(VPSet*const v) {
  Tcl_DeleteHashTable(&(v->hashTable));
  Tcl_InitHashTable(&(v->hashTable),TCL_ONE_WORD_KEYS);
}

/* Returns true if added p wasn't already in the set */
int VPSetPush(VPSet*const v, void*const p) {
  int newPtr;
  
  Tcl_CreateHashEntry(&(v->hashTable),(char*)p,&newPtr);
  return newPtr;
}

int VPSetMemberP(VPSet*const v, const void*const p) {
  Tcl_HashEntry* result = Tcl_FindHashEntry(&(v->hashTable),(char*)p);
  return (NULL != result);
}

void* VPSetPop(VPSet*const v) {
  Tcl_HashSearch junk;
  Tcl_HashEntry* entry;

  entry = Tcl_FirstHashEntry(&(v->hashTable),&junk);
  Tcl_DeleteHashEntry(entry);
  return Tcl_GetHashKey( &(v->hashTable),entry);
}

int VPSetDelete(VPSet*const v, const void*const p) {
  Tcl_HashEntry* result = Tcl_FindHashEntry(&(v->hashTable),(char*)p);
  if (NULL != result) {
    Tcl_DeleteHashEntry(result);
    return 1;
  } else {
    return 0;
  }
}


void VPSetForeach(VPSet*const v, void (*func)(void* )) {
  Tcl_HashSearch searchPtr;
  Tcl_HashEntry* entry = Tcl_FirstHashEntry(&(v->hashTable),&searchPtr);
  while (entry != NULL) {
    func(Tcl_GetHashKey((&(v->hashTable)),entry));
    entry = Tcl_NextHashEntry(&searchPtr);
  }
}

void VPSetForeach2(VPSet*const v, void (*func)(void*,void*),
		  void* arg2Value) {
  Tcl_HashSearch searchPtr;
  Tcl_HashEntry* entry = Tcl_FirstHashEntry(&(v->hashTable),&searchPtr);
  while (entry != NULL) {
    func(Tcl_GetHashKey((&(v->hashTable)),entry),arg2Value);
    entry = Tcl_NextHashEntry(&searchPtr);
  }
}

void VPSetForeach3(VPSet*const v, void (*func)(void*,void*,void*),
		  void* arg2Value, void* arg3Value) {
  Tcl_HashSearch searchPtr;
  Tcl_HashEntry* entry = Tcl_FirstHashEntry(&(v->hashTable),&searchPtr);
  while (entry != NULL) {
    func(Tcl_GetHashKey(&(v->hashTable),entry),arg2Value,arg3Value);
    entry = Tcl_NextHashEntry(&searchPtr);
  }
}

int VPSetSize(const VPSet*const v) {
  return v->hashTable.numEntries;
}

int VPSetEmpty(const VPSet*const v) {
  return 0 == v->hashTable.numEntries;
}
