
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

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>

#include "misc.h"

/***********************************************************************/
/*  FUNCTION:  SafeMalloc */
/**/
/*    INPUTS:  size is the size to malloc */
/**/
/*    OUTPUT:  returns pointer to allocated memory if succesful */
/**/
/*    EFFECT:  mallocs new memory.  If malloc fails, prints error message */
/*             and terminates program. */
/**/
/*    Modifies Input: none */
/**/
/***********************************************************************/

/* If DMALLOC is defined then misc.h makes SafeMalloc and SafeCalloc
 * just be malloc and calloc */
#ifndef DMALLOC
void * SafeMalloc(size_t size) {
  void * result;

  if ( (result = malloc(size)) ) { /* assignment intentional */
    return(result);
  } else {
    printf("memory overflow: malloc failed in SafeMalloc.");
    printf("  Exiting Program.\n");
    exit(-1);
    return(0);
  }
}

void * SafeCalloc(size_t nmemb, size_t size) {
  void * result;

  if ( (result = calloc(nmemb, size)) ) { /* assignment intentional */
    return(result);
  } else {
    printf("memory overflow: calloc failed in SafeCalloc.");
    printf("  Exiting Program.\n");
    exit(-1);
    return(0);
  }
}
#endif

int SumIntegers(const int size, const int*const data) {
  int i, total;

  for (i=0, total=0; i<size; i++)
    total += data[i];

  return total;
}

char* CopyString(const char*const str) {
  const int len = strlen(str)+1;
  char*const result = SafeCalloc(len,sizeof(char));
  strncpy(result,str,len);
  return result;
}

/* Take an array of strings where the last element in the array is  *
 * NULL and concatenate them into a new string seperated by sep.    */
char* CatStrArray(const char*const*const data,const char*const sep) {
  char*result, *curPos;
  int curItem;
  const int sepLen = strlen(sep);
  int len = 1-sepLen;

  for (curItem=0; data[curItem] != NULL; curItem++) {
    len += strlen(data[curItem]) + sepLen;
  }
  if (len <= 0) return NULL;

  result = SafeCalloc(len+1,sizeof(char));
  curPos = result;
  strcat(curPos,data[0]);
  curPos += strlen(data[0]);

  for (curItem=1; data[curItem] != NULL; curItem++) {
    strcat(curPos,sep);
    curPos += sepLen;
    strcat(curPos,data[curItem]);
    curPos += strlen(data[curItem]);
  }
  *curPos = '\0';
  curPos++;
  assert(curPos-result == len);
  return result;
}
