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



// Cicada Boolean/integer/floating point types

// These can be changed (e.g. int --> long if we need to work with large numbers);
// just remember to also change the 4 read/printXXXFormatStrings variables at the top of cmpile.c

typedef char ccBool;

typedef int ccInt;
#define ccIntMax INT_MAX
#define ccIntMin INT_MIN

typedef double ccFloat;



// Boolean constants, defined in case 'true' and 'false' aren't defined

#define ccFalse 0
#define ccTrue 1


// misc. definitions

#define sublistHeaderSize sizeof(sublistHeader)

#define passed 0
#define out_of_memory_err 1
#define out_of_range_err 2
#define init_err 3
#define mismatched_indices_err 4


// shortcuts for accessing linked lists

#define LL_Char(a,b) ((unsigned char *) element(a,b))
#define LL_Double(a,b) ((ccFloat *) element(a,b))
#define LL_int(a,b) ((ccInt *) element(a,b))


// There is one header for the entire linked list, which is stored in a variable of type linkedlist (below).
// The header points to the first sublist in the queue; each sublist begins with a 12(?)-byte block of memory
// of type sublistHeader (also below), which gives information specific to that particular sublist.

typedef struct sH_temp {
    ccInt numElements;              // number of elements that are officially registered to be in this sublist
    struct sH_temp *nextSublist;    // link to next sublist (NULL if none)
    ccInt maxElements;              // number of elements that this sublist can hold in the space that was allocated
} sublistHeader;

typedef struct {                    // all these have to do with the whole linked list
    ccInt elementNum;               // number of elements
    sublistHeader *memory;          // first sublist pointer
    ccInt elementSize;              // bytes per element
    double spareRoom;               // _percentage_ of extra room allocated (for speed if elements are added)
} linkedlist;


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



// Prototypes

extern ccInt newLinkedList(linkedlist *, ccInt, ccInt, ccFloat, ccBool);
extern void deleteLinkedList(linkedlist *);
extern ccInt insertElements(linkedlist *, ccInt, ccInt, ccBool);
extern ccInt addElements(linkedlist *, ccInt, ccBool);
extern ccInt deleteElements(linkedlist *, ccInt, ccInt);
extern ccInt deleteElement(linkedlist *, ccInt);
extern ccInt resizeLinkedList(linkedlist *, ccInt, ccBool);
extern ccInt defragmentLinkedList(linkedlist *);
extern ccInt copyElements(linkedlist *, ccInt, linkedlist *, ccInt, ccInt);
extern ccBool compareElements(linkedlist *, ccInt, linkedlist *, ccInt, ccInt);
extern ccInt fillElements(linkedlist *, ccInt, ccInt, char);
extern ccInt clearElements(linkedlist *, ccInt, ccInt);
extern ccInt setElements(linkedlist *, ccInt, ccInt, void *);
extern ccInt setElement(linkedlist*, ccInt, void *);
extern ccInt getElements(linkedlist *, ccInt, ccInt, void *);
extern ccInt getElement(linkedlist *, ccInt, void *);
extern ccInt elementExists(linkedlist *, ccInt);
extern void *findElement(linkedlist *, ccInt);
extern void *element(linkedlist *, ccInt);
extern void *skipElements(linkedlist *, sublistHeader **, ccInt *, ccInt);

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

#endif
