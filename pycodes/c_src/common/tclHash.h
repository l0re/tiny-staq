
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


#ifndef _INC_TCL_HASH
#define _INC_TCL_HASH

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _ANSI_ARGS_(x)	x
typedef void* ClientData;


/*
 * Structure definition for an entry in a hash table.  No-one outside
 * Tcl should access any of these fields directly;  use the macros
 * defined below.
 */


/*
 * Forward declarations of Tcl_HashTable and related types.
 */
typedef struct Tcl_HashKeyType Tcl_HashKeyType;
typedef struct Tcl_HashSearch Tcl_HashSearch;
typedef struct Tcl_HashTable Tcl_HashTable;
typedef struct Tcl_HashEntry Tcl_HashEntry;

typedef unsigned int (Tcl_HashKeyProc) _ANSI_ARGS_((Tcl_HashTable *tablePtr,
	void *keyPtr));
typedef int (Tcl_CompareHashKeysProc) _ANSI_ARGS_((void *keyPtr,
	Tcl_HashEntry *hPtr));
typedef Tcl_HashEntry *(Tcl_AllocHashEntryProc) _ANSI_ARGS_((
	Tcl_HashTable *tablePtr, void *keyPtr));
typedef void (Tcl_FreeHashEntryProc) _ANSI_ARGS_((Tcl_HashEntry *hPtr));

void Tcl_DeleteHashEntry(Tcl_HashEntry *entryPtr);
Tcl_HashEntry * Tcl_FindHashEntry(Tcl_HashTable *tablePtr,
				  const char *key);
void Tcl_DeleteHashTable(Tcl_HashTable *tablePtr);
Tcl_HashEntry *Tcl_CreateHashEntry(Tcl_HashTable*, const char* key, 
				   int* newPtr);
Tcl_HashEntry *
Tcl_FirstHashEntry(Tcl_HashTable *tablePtr,Tcl_HashSearch *searchPtr);

Tcl_HashEntry *
Tcl_NextHashEntry(register Tcl_HashSearch *searchPtr);

struct Tcl_HashEntry {
    Tcl_HashEntry *nextPtr;		/* Pointer to next entry in this
					 * hash bucket, or NULL for end of
					 * chain. */
    Tcl_HashTable *tablePtr;		/* Pointer to table containing entry. */
#if TCL_HASH_KEY_STORE_HASH
  unsigned int hash;			/* Hash value. */
#else
    Tcl_HashEntry **bucketPtr;		/* Pointer to bucket that points to
					 * first entry in this entry's chain:
					 * used for deleting the entry. */
#endif
    ClientData clientData;		/* Application stores something here
					 * with Tcl_SetHashValue. */
    union {				/* Key has one of these forms: */
	char *oneWordValue;		/* One-word value for key. */
      /*        Tcl_Obj *objPtr;*/		/* Tcl_Obj * key value. */
	int words[1];			/* Multiple integer words for key.
					 * The actual size will be as large
					 * as necessary for this table's
					 * keys. */
	char string[4];			/* String for key.  The actual size
					 * will be as large as needed to hold
					 * the key. */
    } key;				/* MUST BE LAST FIELD IN RECORD!! */
};

/*
 * Flags used in Tcl_HashKeyType.
 *
 * TCL_HASH_KEY_RANDOMIZE_HASH:
 *				There are some things, pointers for example
 *				which don't hash well because they do not use
 *				the lower bits. If this flag is set then the
 *				hash table will attempt to rectify this by
 *				randomising the bits and then using the upper
 *				N bits as the index into the table.
 */
#define TCL_HASH_KEY_RANDOMIZE_HASH 0x1

/*
 * Structure definition for the methods associated with a hash table
 * key type.
 */
#define TCL_HASH_KEY_TYPE_VERSION 1
struct Tcl_HashKeyType {
    int version;		/* Version of the table. If this structure is
				 * extended in future then the version can be
				 * used to distinguish between different
				 * structures. 
				 */

    int flags;			/* Flags, see above for details. */

    /* Calculates a hash value for the key. If this is NULL then the pointer
     * itself is used as a hash value.
     */
    Tcl_HashKeyProc *hashKeyProc;

    /* Compares two keys and returns zero if they do not match, and non-zero
     * if they do. If this is NULL then the pointers are compared.
     */
    Tcl_CompareHashKeysProc *compareKeysProc;

    /* Called to allocate memory for a new entry, i.e. if the key is a
     * string then this could allocate a single block which contains enough
     * space for both the entry and the string. Only the key field of the
     * allocated Tcl_HashEntry structure needs to be filled in. If something
     * else needs to be done to the key, i.e. incrementing a reference count
     * then that should be done by this function. If this is NULL then Tcl_Alloc
     * is used to allocate enough space for a Tcl_HashEntry and the key pointer
     * is assigned to key.oneWordValue.
     */
    Tcl_AllocHashEntryProc *allocEntryProc;

    /* Called to free memory associated with an entry. If something else needs
     * to be done to the key, i.e. decrementing a reference count then that
     * should be done by this function. If this is NULL then Tcl_Free is used
     * to free the Tcl_HashEntry.
     */
    Tcl_FreeHashEntryProc *freeEntryProc;
};

/*
 * Structure definition for a hash table.  Must be in tcl.h so clients
 * can allocate space for these structures, but clients should never
 * access any fields in this structure.
 */

#define TCL_SMALL_HASH_TABLE 4
struct Tcl_HashTable {
    Tcl_HashEntry **buckets;		/* Pointer to bucket array.  Each
					 * element points to first entry in
					 * bucket's hash chain, or NULL. */
    Tcl_HashEntry *staticBuckets[TCL_SMALL_HASH_TABLE];
					/* Bucket array used for small tables
					 * (to avoid mallocs and frees). */
    int numBuckets;			/* Total number of buckets allocated
					 * at **bucketPtr. */
    int numEntries;			/* Total number of entries present
					 * in table. */
    int rebuildSize;			/* Enlarge table when numEntries gets
					 * to be this large. */
    int downShift;			/* Shift count used in hashing
					 * function.  Designed to use high-
					 * order bits of randomized keys. */
    int mask;				/* Mask value used in hashing
					 * function. */
    int keyType;			/* Type of keys used in this table. 
					 * It's either TCL_CUSTOM_KEYS,
					 * TCL_STRING_KEYS, TCL_ONE_WORD_KEYS,
					 * or an integer giving the number of
					 * ints that is the size of the key.
					 */

    Tcl_HashKeyType *typePtr;		/* Type of the keys used in the
					 * Tcl_HashTable. */
};

/*
 * Structure definition for information used to keep track of searches
 * through hash tables:
 */

struct Tcl_HashSearch {
    Tcl_HashTable *tablePtr;		/* Table being searched. */
    int nextIndex;			/* Index of next bucket to be
					 * enumerated after present one. */
    Tcl_HashEntry *nextEntryPtr;	/* Next entry to be enumerated in the
					 * the current bucket. */
};

/*
 * Acceptable key types for hash tables:
 *
 * TCL_STRING_KEYS:		The keys are strings, they are copied into
 *				the entry.
 * TCL_ONE_WORD_KEYS:		The keys are pointers, the pointer is stored
 *				in the entry.
 * TCL_CUSTOM_TYPE_KEYS:	The keys are arbitrary types which are copied
 *				into the entry.
 * TCL_CUSTOM_PTR_KEYS:		The keys are pointers to arbitrary types, the
 *				pointer is stored in the entry.
 *
 * While maintaining binary compatability the above have to be distinct
 * values as they are used to differentiate between old versions of the
 * hash table which don't have a typePtr and new ones which do. Once binary
 * compatability is discarded in favour of making more wide spread changes
 * TCL_STRING_KEYS can be the same as TCL_CUSTOM_TYPE_KEYS, and
 * TCL_ONE_WORD_KEYS can be the same as TCL_CUSTOM_PTR_KEYS because they
 * simply determine how the key is accessed from the entry and not the
 * behaviour.
 */

#define TCL_STRING_KEYS		0
#define TCL_ONE_WORD_KEYS	1

#define TCL_CUSTOM_TYPE_KEYS		TCL_STRING_KEYS
#define TCL_CUSTOM_PTR_KEYS		TCL_ONE_WORD_KEYS


/*
 * Macros for clients to use to access fields of hash entries:
 */

#define Tcl_GetHashValue(h) ((h)->clientData)
#define Tcl_SetHashValue(h, value) ((h)->clientData = (ClientData) (value))
#define Tcl_GetHashKey(tablePtr, h) \
	((char *) (((tablePtr)->keyType == TCL_ONE_WORD_KEYS) \
		   ? (h)->key.oneWordValue \
		   : (h)->key.string))


/*
 * Macros to use for clients to use to invoke find and create procedures
 * for hash tables:
 */

void Tcl_InitHashTable(Tcl_HashTable* tablePtr, int keyType);


#endif
