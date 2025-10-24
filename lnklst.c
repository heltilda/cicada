/*
 *  lnklst.c(pp) - This file contains 3 sets of routines:
 *  1) Linked list routines for dynamic memory allocation
 *  2) 'Pinned' linked lists which have static memory
 *  3) Stacks (stored on the heap)
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


#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include "lnklst.h"




// *************** Linked list routines *****************

// Linked lists store lists of variables, of any number of bytes each, in linked lists.
// All routines return "passed" or an error code (or a handle/pointer in cases of newLinkedList/element).
// Memory structure:  linkedlist (followed by optional header) -> (points to) sublist1 -> sublist2 -> etc.




// **************** Private LL routines ****************



// newSublist() makes a new sublist, to be linked to the queue of a linked sublists.

sublistHeader *newSublist(ccInt numberOfElements, ccInt maximumElements, ccInt elementSize, ccBool ifCleared)
{
    sublistHeader *temp;
    
    if (ifCleared)
        temp = (sublistHeader *) calloc((size_t) maximumElements*elementSize + sublistHeaderSize, (size_t) 1);
    else 
        temp = (sublistHeader *) malloc((size_t) maximumElements*elementSize + sublistHeaderSize);
    
    if (temp != NULL)  {
        temp->numElements = numberOfElements;
        temp->maxElements = maximumElements;
        temp->nextSublist = NULL;     }
    
    return temp;
}


// findSublist() returns a pointer to the sublist with the given element in it.

sublistHeader *findSublist(linkedlist *theList, ccInt theElement, ccInt *bottomRtrn)
{
    sublistHeader *currentSublist = theList->memory;
    ccInt bottom = 1, top = currentSublist->numElements;
    
    while (theElement > top)  {
        currentSublist = currentSublist->nextSublist;
        bottom = top+1;
        top += currentSublist->numElements;    }
    
    *bottomRtrn = bottom;
    return currentSublist;
}



// **************** Public LL routines ****************

// newLinkedList() constructs an empty linked list with a given amount of space; returns pointer to LL if successful.
// Passes requested number of elements and size of each (and whether you want all the memory set to zero automatically).
// Returns a pointer in your theList.memory.
// Spare room is the ratio of spare memory in the list to numberOfElements, as a percentage.
// The linked list routines try to maintain this level of extra space when the list is dynamically resized.

ccInt newLinkedList(linkedlist *theList, ccInt numberOfElements, ccInt elementSize, ccFloat spareRoom, ccBool ifCleared)
{
    ccInt maxElements;
    sublistHeader *temp;
    
    if ((numberOfElements < 0) || (elementSize < 0) || (spareRoom < 0.))  return out_of_range_err;
    
    maxElements = numberOfElements + floor(numberOfElements*spareRoom);
    if (maxElements < 1)  maxElements = 1;
    
    temp = newSublist(numberOfElements, maxElements, elementSize, ifCleared);      // allocate one sublist
    theList->memory = temp;
    if (temp == NULL)  return out_of_memory_err;
    
    theList->elementSize = elementSize;
    theList->elementNum = numberOfElements;
    theList->spareRoom = spareRoom;
    
    return passed;
}


// deleteLinkedList() deletes an entire linked list (including all its sub-lists), and frees the associated memory.

void deleteLinkedList(linkedlist *theList)
{
    sublistHeader *currentSublist, *nextSublist;
    
    nextSublist = theList->memory;
    while (nextSublist != NULL)  {
        currentSublist = nextSublist;
        nextSublist = currentSublist->nextSublist;
        free((void *) currentSublist);    }
    
    theList->memory = NULL;
}


// insertElements() makes room for new elements in the middle of a linked list.
// First new element goes at [insertionPoint], so all new elements are inserted before the old element at insertionPoint.

ccInt insertElements(linkedlist *theList, ccInt insertionPoint, ccInt newElements, ccBool ifCleared)
{
    sublistHeader * baseSublist;
    linkedlist dummy;
    ccInt newLL_result;
    ccInt bottom, elementsOnTop, copyPoint, skipAhead, newSublistElements;
    
    if (theList->memory == NULL)  return init_err;
    
    if (insertionPoint == 1)  {
        baseSublist = theList->memory;
        bottom = 1;    }
    else  {
        if (!elementExists(theList, insertionPoint-1))  return out_of_range_err;
        baseSublist = findSublist(theList, insertionPoint-1, &bottom);    }
    
    if (newElements == 0)  return passed;
    elementsOnTop = baseSublist->numElements + bottom - insertionPoint;
    
    
        // If necessary, create another sublist.
        // This is a rather hokey way of doing it -- create a new linked list, and from that extract and insert the sublist.
    
    if (baseSublist->numElements + newElements > baseSublist->maxElements)  {
            
            // Try to balance it so that both sublists have spareRoom number of spare elements.
            // First we allocate more room in a new sublist, then shove everything up to make room for the new inserted elements
            // and then try to fill the lower sublist as full as we want it by copying the top of those higher up into the next sublist
            // and removing the extra elements from the lower one.
            // copyPoint is in 'local' (to the sublist) coordinates -- refers to where we start shoving stuff upwards.
            // skipAhead is how far we ought to push everything up so that the first sublist will be as full/empty as we want it.
            
            // In Cicada, we need to have copyPoint at the point of the new arrivals.  Since many LLs are 'pinned',
            // addElements() expects that the ones below stay put.
        
        copyPoint = baseSublist->numElements + 1;
        skipAhead = baseSublist->numElements - (copyPoint - 1);
        if (ccIntMax-theList->elementNum < newElements+skipAhead)  return out_of_memory_err;
        
        
            // Allocate enough room to hold the last max-filled sublist, plus extra spareRoom.
        
        newSublistElements = skipAhead+newElements;    // thus successive sublists get bigger
        if (baseSublist->maxElements > newSublistElements)  newSublistElements = baseSublist->maxElements;
        
        newLL_result = newLinkedList(&dummy, newSublistElements, theList->elementSize, theList->spareRoom, ifCleared);
        if (newLL_result != passed)  return newLL_result;
        
        if (newSublistElements > skipAhead+newElements)         // here we really can ignore return value -- never have 0 els
            deleteElements(&dummy, skipAhead+newElements+1, dummy.elementNum);
        
        
            // stitch the new list in
        
        dummy.memory->nextSublist = baseSublist->nextSublist;
        baseSublist->nextSublist = dummy.memory;
        theList->elementNum += newElements+skipAhead;         // fool the computer for now
        
        
            // First make room for new arrivals..
        
        copyElements(theList, insertionPoint, theList, insertionPoint+newElements, elementsOnTop);
        if (ifCleared) clearElements(theList, insertionPoint, insertionPoint+newElements-1);
        
        
            // Now push everything off the top of the lower list, and reset the number of variables it holds.
            // copyPoint hasn't changed from last CopyEls, since it is tied to a position in the LLs, not a particular element.
        
        copyElements(theList, copyPoint+bottom-1, theList, copyPoint+skipAhead+bottom-1, skipAhead+newElements);
        baseSublist->numElements -= skipAhead;
        theList->elementNum -= skipAhead;         }       // unfool the computer
    
    else  {         // if we don't need to allocate another list -- just increase numElements closer to maxElements
        if (ccIntMax-theList->elementNum < newElements)  return out_of_memory_err;
        baseSublist->numElements += newElements;
        theList->elementNum += newElements;
        if (bottom + baseSublist->numElements > insertionPoint)
            copyElements(theList, insertionPoint, theList, insertionPoint + newElements, elementsOnTop);   }
    
    
                // subtle point -- if insertionPoint+newElements-1 == 0, clearElements returns param_err, which we throw away.  Fine.
    
    if (ifCleared) clearElements(theList, insertionPoint, insertionPoint + newElements - 1);
    
    return passed;
}


// addElements():  a quick way to insert elements at the end of a linked list -- to stretch the list, in other words

ccInt addElements(linkedlist *theList, ccInt newElements, ccBool ifCleared)
{
    return insertElements(theList, theList->elementNum+1, newElements, ifCleared);
}


// deleteElements() removes the memory space associated with certain elements, and pulls the others down
// (so their positions change in the list).  Empty sublists are removed to free up space.

ccInt deleteElements(linkedlist *theList, ccInt firstDelete, ccInt lastDelete)
{
    sublistHeader *tempSublist, *currentSublist, **parentPtr, *tempPtr;
    ccInt bottom, top, firstCopy, copyBottom;
    
    if (theList->memory == NULL)  return init_err;
    if ((!elementExists(theList, firstDelete)) || (!elementExists(theList, lastDelete))) return out_of_range_err;
    if (firstDelete > lastDelete)  return passed;
    currentSublist = findSublist(theList, firstDelete, &bottom);
    
    
        // First, find the sublist in which we stop deleting, then copy what comes after back down towards the bottom.
    
    while (bottom <= lastDelete)
    {
        top = bottom + currentSublist->numElements - 1;
        copyBottom = bottom;
        if (bottom < firstDelete) copyBottom = firstDelete;      // i.e. if it's the first pass
        
        firstCopy = top + 1;
        if (lastDelete < top)  {
            firstCopy = lastDelete + 1;
            memmove((void *) (((char *) currentSublist) + (copyBottom-bottom)*theList->elementSize + sublistHeaderSize),
                    (void *) (((char *) currentSublist) + (firstCopy-bottom)*theList->elementSize + sublistHeaderSize),
                    (top + 1 - firstCopy)*theList->elementSize);        }
        
        bottom += currentSublist->numElements;         // bottom goes by the old indexing; before the elements were deleted
        currentSublist->numElements -= (firstCopy-copyBottom);
        currentSublist = currentSublist->nextSublist;
    }
    
    
        // Now get rid of null lists.
    
    parentPtr = &(theList->memory);
    while (*parentPtr != NULL)    {
        
        tempSublist = *parentPtr;
        
        if (tempSublist->numElements == 0)   {
            tempPtr = tempSublist->nextSublist;
            free((void *) *parentPtr);
            *parentPtr = tempPtr;        }
        else  {
            parentPtr = &((*parentPtr)->nextSublist);
    }   }
    
    
        // If we went too far and deleted the first sublist, recreate it
    
    if (theList->memory == NULL)  {
        tempPtr = newSublist(0, 1, theList->elementSize, ccFalse);
        theList->memory = tempPtr;
        if (tempPtr == NULL)  return out_of_memory_err;    }
    
    theList->elementNum -= lastDelete - firstDelete + 1;
    
    return passed;
}


// deleteElement() is just the singular form of [F]deleteElements() -- it removes a single element.

ccInt deleteElement(linkedlist *theList, ccInt deletePoint)
{
    return deleteElements(theList, deletePoint, deletePoint);
}



// resizeLinkedList() adds to the end or deletes elements from the end, as necessary

ccInt resizeLinkedList(linkedlist *theList, ccInt newTop, ccBool ifCleared)
{
    if (theList->memory == NULL)  return init_err;
    
    if (theList->elementNum < newTop)
        return addElements(theList, newTop-theList->elementNum, ifCleared);
    else if (theList->elementNum > newTop)
        return deleteElements(theList, newTop+1, theList->elementNum);
    else  return passed;
}



// defragmentLinkedList() reduces a linked list to a single list, with NO spare room (modified 4/3/04), regardless of the
// spareRoom parameter.  Call this (for speed) when the number of elements is fixed, or to get all of the list elements in one block.

ccInt defragmentLinkedList(linkedlist *theList)
{
    linkedlist oldList;
    ccInt defLL_rtrn;
    
    if (theList->memory == NULL)  return init_err;
    if (theList->elementNum == theList->memory->numElements)  return passed;
    
    oldList = *theList;
    defLL_rtrn = newLinkedList(theList, oldList.elementNum, oldList.elementSize, oldList.spareRoom, ccFalse);
    if (defLL_rtrn != passed)  {
        *theList = oldList;
        return defLL_rtrn;    }
    
    theList->spareRoom = oldList.spareRoom;
    
    if (theList->elementNum > 0)  {
        defLL_rtrn = copyElements(&oldList, 1, theList, 1, oldList.elementNum);
        if (defLL_rtrn != passed)  return defLL_rtrn;         }
    
    deleteLinkedList(&oldList);
    
    return passed;
}


// copyElements() copies a string of elements from one position in one linked list to another position in another linked list.
// Lists must have identically sized elements, of course.
// This routine is long-winded because it is designed to be fast, not simple.

ccInt copyElements(linkedlist *sourceList, ccInt sourceBegin, linkedlist *destList, ccInt destBegin, ccInt numberToCopy)
{
    ccInt firstCopied, lastCopied, sourceBottom, destBottom, elementSize;
    sublistHeader *currentSourceSublist, *currentDestSublist;
    
    
        // First, make sure the user is within range.
    
    if ((sourceList->memory == NULL) || (destList->memory == NULL))  return init_err;
    if (sourceList->elementSize != destList->elementSize)  return mismatched_indices_err;
    if (numberToCopy == 0)  return passed;            // put before el-exists-check, since if NTC == 0 we often can't pass a valid element
    if ((!elementExists(sourceList, sourceBegin)) || (!elementExists(destList, destBegin)))  return out_of_range_err;
    if ((!elementExists(sourceList, sourceBegin + numberToCopy - 1)) || (!elementExists(destList, destBegin+numberToCopy-1)))
        return out_of_range_err;
    
    elementSize = sourceList->elementSize;
    
    
        // Now start copying the lists.  If we can do it from lowest to highest without overwriting, that's easiest (first clause of 'if' statement).
        // Either way, we iteratively hop down sublists: updating them, restricting the size of the copied memory block, and copying the memory.
    
    if ((sourceBegin > destBegin) || (sourceBegin+numberToCopy <= destBegin) || (sourceList != destList))
    {
        currentSourceSublist = findSublist(sourceList, sourceBegin, &sourceBottom);
        currentDestSublist = findSublist(destList, destBegin, &destBottom);
        firstCopied = 1;
        lastCopied = numberToCopy - 1;
        while (lastCopied < numberToCopy)
        {
            if (currentSourceSublist->numElements+sourceBottom-1 < sourceBegin+firstCopied-1)   {    //update sublists
                sourceBottom = sourceBottom + currentSourceSublist->numElements;
                currentSourceSublist = currentSourceSublist->nextSublist;       }
            
            if (currentDestSublist->numElements+destBottom-1 < destBegin+firstCopied-1)   {
                destBottom = destBottom + currentDestSublist->numElements;
                currentDestSublist = currentDestSublist->nextSublist;     }
            
            lastCopied = numberToCopy;
            if (sourceBottom+currentSourceSublist->numElements < sourceBegin+lastCopied)      // restrict copy length
                lastCopied = sourceBottom + currentSourceSublist->numElements - sourceBegin;
            if (destBottom+currentDestSublist->numElements < destBegin+lastCopied)
                lastCopied = destBottom + currentDestSublist->numElements - destBegin;

            memmove((void *) (((char *) currentDestSublist) + (firstCopied+destBegin-destBottom-1)*elementSize + sublistHeaderSize),
                    (void *) (((char *) currentSourceSublist) + (firstCopied+sourceBegin-sourceBottom-1)*elementSize + sublistHeaderSize),
                    elementSize*(lastCopied-firstCopied+1));
            
            firstCopied = lastCopied+1;
    }   }
    
    else if (sourceBegin < destBegin)       // No, copy from the top down; otherwise we will overwrite something.
    {
        currentSourceSublist = findSublist(sourceList, sourceBegin+numberToCopy-1, &sourceBottom);
        currentDestSublist = findSublist(destList, destBegin+numberToCopy-1, &destBottom);
        firstCopied = numberToCopy+1;
        lastCopied = numberToCopy;
        
        while(firstCopied > 1)
        {
            firstCopied = 1;          // how much to copy?
            if (sourceBottom > firstCopied+sourceBegin-1)
                firstCopied = sourceBottom - sourceBegin + 1;
            if (destBottom > firstCopied+destBegin-1)
                firstCopied = destBottom - destBegin + 1;
            
                        // here we copy first, then scale the lists
            memmove((void *) ((char *) currentDestSublist + (firstCopied+destBegin-destBottom-1)*elementSize + sublistHeaderSize),
                    (void *) ((char *) currentSourceSublist + (firstCopied+sourceBegin-sourceBottom-1)*elementSize + sublistHeaderSize),
                    elementSize*(lastCopied-firstCopied+1));
            
            lastCopied = firstCopied-1;
            if (firstCopied == sourceBottom-sourceBegin+1)
                currentSourceSublist = findSublist(sourceList, sourceBegin+lastCopied-1, &sourceBottom);
            if (firstCopied == destBottom-destBegin+1)
                currentDestSublist = findSublist(destList, destBegin+lastCopied-1, &destBottom);
    }   }
    
    return passed;
}


// compareElements() compares the two subsets of linked lists (of the same length) byte-by-byte to test for equality.

ccBool compareElements(linkedlist *sourceList, ccInt sourceBegin, linkedlist * destList, ccInt destBegin, ccInt numberToCompare)
{
    ccInt firstCompared, lastCompared, elementSize, sourceBottom, destBottom, rtrn;
    sublistHeader *currentSourceSublist, *currentDestSublist;
    
    
        // make sure params were passed correctly
    
    if ((sourceList->memory == NULL) || (destList->memory == NULL))  return init_err;
    if (sourceList->elementSize != destList->elementSize)  return mismatched_indices_err;
    if (numberToCompare == 0)  return ccTrue;
    if ((!elementExists(sourceList, sourceBegin)) || (!elementExists(destList, destBegin)))
        return out_of_range_err;
    if ((!elementExists(sourceList, sourceBegin+numberToCompare-1)) || (!elementExists(destList, destBegin+numberToCompare-1)))
        return out_of_range_err;
    
    elementSize = sourceList->elementSize;
    
    currentSourceSublist = findSublist(sourceList, sourceBegin, &sourceBottom);
    currentDestSublist = findSublist(destList, destBegin, &destBottom);
    
    firstCompared = 1;
    lastCompared = numberToCompare-1;
    
    
        // Now, by chunks (size of each sublist), do straight comparisons of memory.
    
    while (lastCompared < numberToCompare)
    {
        if (currentSourceSublist->numElements + sourceBottom < sourceBegin+firstCompared)   {
            sourceBottom += currentSourceSublist->numElements;
            currentSourceSublist = currentSourceSublist->nextSublist;    }
        
        if (currentDestSublist->numElements + destBottom < destBegin+firstCompared)    {
            destBottom += currentDestSublist->numElements;
            currentDestSublist = currentDestSublist->nextSublist;    }
        
        lastCompared = numberToCompare;
        if (sourceBottom+currentSourceSublist->numElements < sourceBegin+lastCompared)
            lastCompared = sourceBottom + currentSourceSublist->numElements - sourceBegin;
        if (destBottom+currentDestSublist->numElements < destBegin+lastCompared)
            lastCompared = destBottom + currentDestSublist->numElements - destBegin;
        
        rtrn = memcmp((void *) ((char *) currentSourceSublist + (firstCompared+sourceBegin-sourceBottom-1)*elementSize + sublistHeaderSize),
                        (void *) ((char *) currentDestSublist + (firstCompared+destBegin-destBottom-1)*elementSize + sublistHeaderSize),
                        (size_t) (lastCompared-firstCompared+1)*elementSize);
        if (rtrn != 0)  return ccFalse;
        
        firstCompared = lastCompared+1;
    }
    
    return ccTrue;
}


// fillElements() fills elements of a linked list with a byte pattern (FF FF or 4E 4E, etc.).
// Useful for clearing lists -- set pattern to 0.

ccInt fillElements(linkedlist *theList, ccInt firstElement, ccInt lastElement, char fillValue)
{
    ccInt firstFilled, lastFilled, top, bottom;
    sublistHeader *currentSublist;
    
    if (theList->memory == NULL)  return init_err;
    if ((!elementExists(theList, firstElement)) || (!elementExists(theList, lastElement)))
        return out_of_range_err;
    
    firstFilled = firstElement;
    lastFilled = firstElement-1;
    currentSublist = findSublist(theList, firstElement, &bottom);
    
    while (lastFilled < lastElement)
    {
        top = bottom + currentSublist->numElements - 1;
        lastFilled = lastElement;
        if (top < lastElement) lastFilled = top;
        
        memset((void *) ((char *) currentSublist + (firstFilled-bottom)*theList->elementSize + sublistHeaderSize), fillValue,
                (size_t) theList->elementSize*(lastFilled - firstFilled + 1));
        
        bottom = top+1;
        firstFilled = lastFilled+1;
        currentSublist = currentSublist->nextSublist;
    }
    
    return passed;
}


// clearElements() calls [F]fillElements(.., .., .., 0) -- just for convenience.

ccInt clearElements(linkedlist *theList, ccInt firstElement, ccInt lastElement)
{
    return fillElements(theList, firstElement, lastElement, 0);
}


// setElements() -- reads data from a buffer into a linked list.

ccInt setElements(linkedlist *theList, ccInt firstElement, ccInt lastElement, void *readAddress)
{
    ccInt firstSet, lastSet, top, bottom;
    sublistHeader *currentSublist;
    
    firstSet = firstElement;
    lastSet = 0;
    
    if (theList->memory == NULL)  return init_err;
    if (lastElement < firstElement)  return passed;
    if ((!elementExists(theList, firstElement)) || (!elementExists(theList, lastElement)))
        return out_of_range_err;
    
    currentSublist = findSublist(theList, firstElement, &bottom);
    
    while (lastSet < lastElement)
    {
        top = bottom + currentSublist->numElements - 1;
        lastSet = lastElement;
        if (top < lastElement) lastSet = top;
        
        memmove((void *) ((char *) currentSublist + (firstSet-bottom)*theList->elementSize + sublistHeaderSize),
                (void *) ((char *) readAddress + (firstSet-firstElement)*theList->elementSize),
                theList->elementSize*(lastSet-firstSet+1));
        
        bottom = top + 1;
        firstSet = lastSet + 1;
        currentSublist = currentSublist->nextSublist;
    }
    
    return passed;
}


// setElement() -- sets a single element.

ccInt setElement(linkedlist *theList, ccInt theElement, void *readAddress)
{
    return setElements(theList, theElement, theElement, readAddress);
}


// getElements():  writes data from a linked list into a buffer

ccInt getElements(linkedlist *theList, ccInt firstElement, ccInt lastElement, void *writeAddress)
{
    ccInt firstGet, lastGet, top, bottom;
    sublistHeader *currentSublist;
    
    firstGet = firstElement;
    lastGet = 0;
    
    if (theList->memory == NULL)  return init_err;
    if (lastElement < firstElement)  return passed;
    if ((!elementExists(theList, firstElement)) || (!elementExists(theList, lastElement)))
        return out_of_range_err;
    
    currentSublist = findSublist(theList, firstElement, &bottom);
    
    while (lastGet < lastElement)
    {
        top = bottom + currentSublist->numElements - 1;
        lastGet = lastElement;
        if (top < lastElement) lastGet = top;
        
        memmove((void *) ((char *) writeAddress + (firstGet-firstElement)*theList->elementSize),
                (void *) ((char *) currentSublist + (firstGet-bottom)*theList->elementSize + sublistHeaderSize),
                    theList->elementSize*(lastGet - firstGet + 1));
        
        bottom = top + 1;
        firstGet = lastGet + 1;
        currentSublist = currentSublist->nextSublist;
    }
    
    return passed;
}


// getElement() just copies one single element into a buffer

ccInt getElement(linkedlist *theList, ccInt theElement, void *writeAddress)
{
    return getElements(theList, theElement, theElement, writeAddress);
}


// elementExists() tests to make sure the list exists, and then that the element is within range

ccInt elementExists(linkedlist *theList, ccInt theElement)
{
    if (theList->memory == NULL)  return init_err;
    if ((theElement > theList->elementNum) || (theElement <= 0))  return ccFalse;
    
    return ccTrue;
}


// element() returns a pointer to the memory location of theElement, or NULL if it is nonexistent

void *findElement(linkedlist *theList, ccInt theElement)
{
    if (!elementExists(theList, theElement))  return NULL;
    else  return element(theList, theElement);
}


// element():  a fast version of findElement() that skips the error checking

void *element(linkedlist *theList, ccInt theElement)
{
    ccInt bottom;
    sublistHeader *headerOfSublist;
    
    headerOfSublist = findSublist(theList, theElement, &bottom);
    
    return (void *) (((char *) headerOfSublist) + sublistHeaderSize + theList->elementSize*(theElement-bottom));
}


// skipElements() skips ahead from a starting element/sublist by N elements.
// The purpose is for speed, so we don't need to call element() repeatedly if we are accessing consecutive elements repeatedly.
// Assumes the indices are within range (no range checking).
// localIndex is measured relative to the bottom of the sublist, where 0 is the first element.

void *skipElements(linkedlist *theList, sublistHeader **theSublist, ccInt *localIndex, ccInt numberToSkip)
{
    *localIndex += numberToSkip;
    while (*localIndex >= (*theSublist)->numElements)  {
        *localIndex -= (*theSublist)->numElements;
        *theSublist = (*theSublist)->nextSublist;      }
    
    return (void *) ((char *) (*theSublist) + sublistHeaderSize + theList->elementSize*(*localIndex));
}




// *************** PLL routines *****************

// PLL is for Pinned Linked List.  This is a list whose elements stay fixed at the same memory location.
// We can only add elements to the end of this list (which, by my present LL scheme, does not alter the memory location of elements
// below the insertion point).  When elements are deleted, we simply leave the LL alone, and make a note of the fact that an empty spot
// is there for the taking (more precisely, the lowest and highest empty spots are stored as lowestFreeSpot and highestFreeSpot).
// An element is considered deleted when the corresponding entry in 'references' goes to 0 -- i.e. the element
// is no longer being used by anybody.


// newPLL() initializes a PLL by allocating its linkedlists

ccInt newPLL(pinned_LL *basePLL, ccInt numElements, ccInt dataSize, ccFloat spareRoom)
{
    ccInt rtrn;
    
    rtrn = newLinkedList(&(basePLL->data), numElements, dataSize, spareRoom, ccFalse);
    if (rtrn != passed)  return rtrn;
    rtrn = newLinkedList(&(basePLL->references), numElements, sizeof(ccInt), spareRoom, ccTrue);
    if (rtrn != passed)  return rtrn;
    basePLL->lowestFreeSpot = 1;
    basePLL->highestFreeSpot = numElements;
    
    return passed;
}


// deletePLL() deallocates a PLL by removing its linkedlists

void deletePLL(pinned_LL *basePLL)
{
    deleteLinkedList(&(basePLL->data));
    deleteLinkedList(&(basePLL->references));
}


// addPLLElement() looks for an empty element, adding to the top of the linkedlist if none is to be found in the middle.
// Thus the user has no control over where in the linkedlist the new element actually goes.

ccInt addPLLElement(pinned_LL *basePLL, void **newElementPtr, ccInt *newElementIndex)
{
    ccInt counter, rtrn;
    
    
        // Case 1:  add to the top of the list
    
    if (basePLL->lowestFreeSpot > basePLL->data.elementNum)   {             // shouldn't move what's in the lower sublists
        rtrn = addElements(&(basePLL->data), 1, ccFalse);         // with my current insertElements scheme
        if (rtrn != passed)  return rtrn;
        rtrn = addElements(&(basePLL->references), 1, ccTrue);     // references must be initially set to 0
        if (rtrn != passed)  return rtrn;
        
        *newElementIndex = basePLL->data.elementNum;
        *newElementPtr = element(&(basePLL->data), *newElementIndex);
        basePLL->lowestFreeSpot = *newElementIndex+1;
        basePLL->highestFreeSpot = 1;         }   // cannot equal lowestFreeSpot: we need it to be lower than any spots that open up
    
    
        // Case 2:  go to the lowest free element in the existing list
    
    else  {
        *newElementIndex = basePLL->lowestFreeSpot;
        *newElementPtr = element(&(basePLL->data), *newElementIndex);
        *LL_int(&(basePLL->references), *newElementIndex) = 0;
        
        if (basePLL->lowestFreeSpot >= basePLL->highestFreeSpot)  {
            basePLL->lowestFreeSpot = basePLL->data.elementNum+1;
            basePLL->highestFreeSpot = 1;     }
        
        else  {
        for (counter = basePLL->lowestFreeSpot+1; counter <= basePLL->highestFreeSpot; counter++)  {
        if (*LL_int(&(basePLL->references), counter) == 0)  {
            basePLL->lowestFreeSpot = counter;
            counter = basePLL->highestFreeSpot;
    }   }}}
    
    return passed;
}


// refPLLElement() adds one to the number of references for a given element.

void refPLLElement(pinned_LL *basePLL, ccInt elementIndex)
{
    (*LL_int(&(basePLL->references), elementIndex))++;
    return;
}


// derefPLLElement() decrements the # of references of an element given its index; updates lowest/highestFreeSpot variables if refs = 0.
// Don't need a DeletePLLElement() -- below, if we run out of references, the element is considered free.

ccInt derefPLLElement(pinned_LL *basePLL, ccInt elementIndex)
{
    ccInt *referencePtr = LL_int(&(basePLL->references), elementIndex);
    
    derefPLLPtr(basePLL, elementIndex, referencePtr);
    
    return *referencePtr;
}


// derefPLLPtr() decrements the # of references given its index and a pointer to its reference list.
// (Should be faster than derefPLLElement(), if the ref. list is available).
// Don't need to return the number of references, since the user already has referencePtr.

void derefPLLPtr(pinned_LL *basePLL, ccInt elementIndex, ccInt *referencePtr)
{
    (*referencePtr)--;
    if (*referencePtr == 0)  {
        if (elementIndex < basePLL->lowestFreeSpot)  basePLL->lowestFreeSpot = elementIndex;
        if (elementIndex > basePLL->highestFreeSpot)  basePLL->highestFreeSpot = elementIndex;    }
    
    return;
}



// *************** Stack routines *****************

// The stacks add to/remove from the top of the linked lists; the difference is that when elements are removed from the top,
// their storage space is not freed (to make it faster to push new elements later on).
// The only stack in Cicada is for pcCodePtr, and Cicada checks (via the recursion-depth error) to make sure that this doesn't get too big.
// This is somewhat redundant, since the LL routines also store extra space, so this may change.


// newStack() initializes a stack by opening up its LL.
// InitialAllocation is (roughly) the size of free space in the linked list; it should not imply that the stack top doesn't start at zero.

ccInt newStack(stack *theStack, ccInt elementSize, ccInt InitialAllocation, ccFloat spareRoom)
{
    ccInt rtrn;
    
    rtrn = newLinkedList(&(theStack->data), (InitialAllocation/(1.+spareRoom)), elementSize, spareRoom, ccFalse);
    if (rtrn != passed)  return rtrn;
    
    theStack->top = 0;
    
    return passed;
}


// deleteStack() frees the LL memory in the stack.

void deleteStack(stack *theStack)
{
    deleteLinkedList(&(theStack->data));
    
    return;
}


// pushStack() adds an element to (the top of) the stack

ccInt pushStack(stack *theStack, void **newElementPtr)
{
    ccInt rtrn;
    
    if (theStack->top == theStack->data.elementNum)  {
        rtrn = addElements(&(theStack->data), 1, ccFalse);
        if (rtrn != passed)  return rtrn;         }
    
    (theStack->top)++;
    *newElementPtr = element(&(theStack->data), theStack->top);
    
    return passed;
}


// popStack() decrements the stack top and sets OldElPtr to that element
// Due to the way stacks work, that new element will not be overwritten until pushStack() is called again.

ccInt popStack(stack *theStack, void **OldElPtr)
{
    *OldElPtr = element(&(theStack->data), theStack->top);
    
    (theStack->top)--;
    
    return passed;
}
