
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

#include<stdlib.h>

#include "VPStack.h"
#include "misc.h"

VPStack* VPStackCreate(const int size) {
  VPStack*const result = SafeMalloc(sizeof(VPStack));
  result->data = SafeCalloc(size,sizeof(void*));
  result->maxLen = size;
  result->curLen = 0;
  return result;
}

void VPStackDestroy(VPStack*const v) {
  free(v->data);
}

void VPStackClear(VPStack*const v) {
  v->curLen = 0;
}

void VPStackPush(VPStack*const v, void* p) {
  int i;
  void** newData;

  if (v->curLen >= v->maxLen) {
    v->maxLen *= 2;
    newData = SafeCalloc(v->maxLen, sizeof(void*));
    for (i=0; i<v->curLen; i++) newData[i] = v->data[i];
    free(v->data);
    v->data = newData;
  }
  v->data[v->curLen++] = p;
}

int VPStackEmpty(const VPStack*const v) {
  return v->curLen==0;
}

void* VPStackPop(VPStack*const v) {
  if (v->curLen == 0) {
    return NULL;
  } else {
    return v->data[--v->curLen];
  }
}

int VPStackSize(const VPStack*const v) {
  return v->curLen;
}

void VPStackSwapTopToRandomPos(VPStack*const v) {
  int newPos = random() % v->curLen;
  void* temp = v->data[newPos];
  v->data[newPos] = v->data[0];
  v->data[0] = temp;
}

void VPStackForeach(const VPStack*const v, void (*func)(void*) ) {
  int i;
  for (i=0; i < v->curLen; i++) func(v->data[i]);
}

int VPStackMemberP(const VPStack*const v, const void*const m) {
  int i;
  for (i=0; i < v->curLen; i++) 
    if (m == v->data[i]) return 1;
  return 0;
}
