/*
 *  lnklst.h
 *  
 *  Cicada
 *  Copyright (C) 2017 Brian C. Ross
 *  
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */


#ifndef LinkedList_h
#define LinkedList_h

#include <stdbool.h>
#include "cicada.h"


// misc. definitions

#define passed 0

#define sublistHeaderSize sizeof(sublistHeader)


// shortcuts for accessing linked lists

#define LL_Char(a,b) ((unsigned char *) element(a,b))
#define LL_Double(a,b) ((ccFloat *) element(a,b))
#define LL_int(a,b) ((ccInt *) element(a,b))



// pinned_LL -- used when we don't want the location of existing elements to change when others are inserted/deleted.
// Used by, e.g., VariableList, all windows, search paths, etc. -- anything we take pointers from.

typedef struct {
    linkedlist data;                // the actual data
    linkedlist references;          // # of pointers of each element in use -- unsigned ints
    ccInt lowestFreeSpot;           // the first and last elements in the LL that are unallocated (not deleted, since we need
    ccInt highestFreeSpot;          // to keep memory in the same spot (since variables hold mem_pointers))
} pinned_LL;


// stack -- a kind of list used for PCStack

typedef struct {
    ccInt top;
    linkedlist data;
} stack;



// Prototypes (most linkedlist prototypes are public-facing and put in cicada.h)

extern sublistHeader *newSublist(ccInt, ccInt, ccInt, bool);
extern sublistHeader *findSublist(linkedlist *, ccInt, ccInt *);

extern ccInt newPLL(pinned_LL *, ccInt, ccInt, ccFloat);
extern void deletePLL(pinned_LL *);
extern ccInt addPLLElement(pinned_LL *, void **, ccInt *);
extern void refPLLElement(pinned_LL *, ccInt);
extern ccInt derefPLLElement(pinned_LL *, ccInt);
#define RefPLLPtr(refptr) ((*refptr)++)
extern void derefPLLPtr(pinned_LL *, ccInt, ccInt *);

extern ccInt newStack(stack *, ccInt, ccInt, ccFloat);
extern void deleteStack(stack *);
extern ccInt pushStack(stack *, void **);
extern ccInt popStack(stack *, void **);

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif
