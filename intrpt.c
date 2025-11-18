/*
 *  intrpt.c(pp) - low-level routines for interpreting bytecode
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
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "intrpt.h"
#include "bytecd.h"
#include "cclang.h"



// * * * * *  Interpreter globals  * * * * *

cc_interpret_global_struct cc_interpret_globals;     // see intrpt.h


    // Next: the amount of extra space allocated in linked lists.
    // These LLs are used for variable data, member lists, code lists, ... -- almost everything
    // LLFreeSpace is set in initCicada(); the default is 100%, which the user can change (0-255)

const ccFloat LLFreeSpace = 1.;



// * * * * *  Low-level routines for copying and comparing data  * * * * *


// Next 4 routines:  called by def-general (equate, etc.)
// These copy data between two type-matched windows.

void copyWindowData(view *sourceView, view *destView)
{   doCopyCompare(sourceView, destView, &copyWindowData, &copyData, &copyString, copyJumpTable);  }

void copyData(linkedlist *sourceLL, ccInt sourceIndex, linkedlist *destLL, ccInt destIndex, ccInt numberToCopy)
{  copyElements(sourceLL, sourceIndex, destLL, destIndex, numberToCopy);  }

void copyString(window *sourceStringWindow, member *destStringMember)
{
    ccInt rtrn;
    
    
        // if the source and destination variables are the same, then make a new window
        // (we check variables, not windows, in case we have a fake window from encompassMultiView())
    
    if (destStringMember->memberWindow->variable_ptr == sourceStringWindow->variable_ptr)  {
        window *oldDestWindow = destStringMember->memberWindow;
        destStringMember->indices = sourceStringWindow->width;
        rtrn = addWindow(sourceStringWindow->variable_ptr, sourceStringWindow->offset, sourceStringWindow->width,
                    &(destStringMember->memberWindow), true);
        if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
        refWindow(destStringMember->memberWindow);
        derefWindow(&oldDestWindow);        }
    
    
        // otherwise just resize the member and copy the data
    
    else  {
        resizeMember(destStringMember, 1, sourceStringWindow->width);
        if (errCode != passed)  return;
        
        copyElements(&(sourceStringWindow->variable_ptr->mem.data), sourceStringWindow->offset+1,
                &(destStringMember->memberWindow->variable_ptr->mem.data), destStringMember->memberWindow->offset+1,
                sourceStringWindow->width);     }
}

void copyBoolToBool(void *Bool1, void *Bool2)
{  *(bool *) Bool2 = *(bool *) Bool1;  }

void copyCharToChar(void *Char1, void *Char2)
{  *(unsigned char *) Char2 = *(unsigned char *) Char1;  }

void copyCharToInt(void *theChar, void *theInt)
{  *(ccInt *) theInt = ((ccInt) *(char *) theChar);  }

void copyCharToDouble(void *theChar, void *theDouble)
{  *(ccFloat *) theDouble = ((ccFloat) *(char *) theChar);  }

void copyIntToChar(void *theInt, void *theChar)
{
    if (!((*(ccInt *) theInt <= UCHAR_MAX) && (*(ccInt *) theInt >= 0)))  setWarning(out_of_range_err, pcCodePtr-1);
    else  *(char *) theChar = (char) *(ccInt *) theInt;  }

void copyIntToInt(void *Int1, void *Int2)
{  *(ccInt *) Int2 = *(ccInt *) Int1;  }

void copyIntToDouble(void *theInt, void *theDouble)
{  *(ccFloat *) theDouble = ((ccFloat) *(ccInt *) theInt);  }

void copyDoubleToChar(void *theDouble, void *theChar)
{
    if (!((*(ccFloat *) theDouble <= UCHAR_MAX) && (*(ccFloat *) theDouble >= 0)))  setWarning(out_of_range_err, pcCodePtr-1);
    else  *(char *) theChar = (char) *(ccFloat *) theDouble;  }

void copyDoubleToInt(void *theDouble, void *theInt)
{
    if (!((*(ccFloat *) theDouble <= ccIntMax) && (*(ccFloat *) theDouble >= ccIntMin)))  setWarning(out_of_range_err, pcCodePtr-1);
    else  *(ccInt *) theInt = (ccInt) *(ccFloat *) theDouble;  }

void copyDoubleToDouble(void *Double1, void *Double2)
{  *(ccFloat *) Double2 = *(ccFloat *) Double1;  }

void copyStringToString(void *sourceString, void *destString)
{
    copyString((window *) sourceString, (member *) destString);
}

void(*copyJumpTable[])(void *, void *) = {
    copyBoolToBool, setTMerror, setTMerror, setTMerror, setTMerror,
    setTMerror, copyCharToChar, copyCharToInt, copyCharToDouble, setTMerror,
    setTMerror, copyIntToChar, copyIntToInt, copyIntToDouble, setTMerror,
    setTMerror, copyDoubleToChar, copyDoubleToInt, copyDoubleToDouble, setTMerror,
    setTMerror, setTMerror, setTMerror, setTMerror, copyStringToString   };


// Next 4 routines:  called by the comparison operator (==)
// Compare data between two type-matched windows to test for equality.

void compareWindowData(view *sourceView, view *destView)
{   doCopyCompare(sourceView, destView, &compareWindowData, &compareData, &compareString, compareJumpTable);  }

void compareData(linkedlist *sourceLL, ccInt sourceIndex, linkedlist *destLL, ccInt destIndex, ccInt numberToCompare)
{
    if (!boolRegister)  return;
    if (sourceLL->elementSize != destLL->elementSize)  boolRegister = false;
    else if (!compareElements(sourceLL, sourceIndex, destLL, destIndex, numberToCompare))  boolRegister = false;
}

void compareString(window *stringWindow1, member *stringMember2)
{
    window *stringWindow2 = stringMember2->memberWindow;
    
    if (!boolRegister)  return;
    if (stringWindow1->width != stringWindow2->width)  boolRegister = false;
    else if (!compareElements(&(stringWindow1->variable_ptr->mem.data), stringWindow1->offset+1,
            &(stringWindow2->variable_ptr->mem.data), stringWindow2->offset+1, stringWindow1->width))  boolRegister = false;
}

void compareBoolToBool(void *Bool1, void *Bool2)
{  if (*(bool *) Bool1 != *(bool *) Bool2)  boolRegister = false;  }

void compareCharToChar(void *Char1, void *Char2)
{  if (*(unsigned char *) Char1 != *(unsigned char *) Char2)  boolRegister = false;  }

void compareCharToInt(void *theChar, void *theInt)
{  if (((ccInt) *(char *) theChar) != *(ccInt *) theInt)  boolRegister = false;  }

void compareCharToDouble(void *theChar, void *theDouble)
{  if (((ccFloat) *(char *) theChar) != *(ccFloat *) theDouble)  boolRegister = false;  }

void compareIntToChar(void *theInt, void *theChar)
{  if (*(ccInt *) theInt != ((ccInt) *(char *) theChar))  boolRegister = false;  }

void compareIntToInt(void *Int1, void *Int2)
{  if (*(ccInt *) Int1 != *(ccInt *) Int2)  boolRegister = false;  }

void compareIntToDouble(void *theInt, void *theDouble)
{  if (((ccFloat) *(ccInt *) theInt) != *(ccFloat *) theDouble)  boolRegister = false;  }

void compareDoubleToChar(void *theDouble, void *theChar)
{  if (*(ccFloat *) theDouble != ((ccFloat) *(char *) theChar))  boolRegister = false;  }

void compareDoubleToInt(void *theDouble, void *theInt)
{  if (*(ccFloat *) theDouble != ((ccFloat) *(ccInt *) theInt))  boolRegister = false;  }

void compareDoubleToDouble(void *Double1, void *Double2)
{  if (*(ccFloat *) Double1 != *(ccFloat *) Double2)  boolRegister = false;  }

void compareStringToString(void *String1, void *String2)
{
    compareString((window *) String1, (member *) String2);
}

void setTMerror(void *dummy1, void *dummy2)
{  setError(type_mismatch_err, pcCodePtr-1);  }

void(*compareJumpTable[])(void *, void *) = {
    compareBoolToBool, setTMerror, setTMerror, setTMerror, setTMerror,
    setTMerror, compareCharToChar, compareCharToInt, compareCharToDouble, setTMerror,
    setTMerror, compareIntToChar, compareIntToInt, compareIntToDouble, setTMerror,
    setTMerror, compareDoubleToChar, compareDoubleToInt, compareDoubleToDouble, setTMerror,
    setTMerror, setTMerror, setTMerror, setTMerror, compareStringToString    };


// doCopyCompare() explores two window trees simultaneously, in order to perform a given operation between them.
// This is used for copying and comparing data.
// This routine skips hidden members.

void doCopyCompare(view *sourceView, view *destView,
        void(*ccComposite)(view *, view *),
        void(*ccData)(linkedlist *, ccInt, linkedlist *, ccInt, ccInt),
        void(*ccString)(window *, member *),
        void(**ccJumpTable)(void *, void *))
{
    variable *sourceVar = sourceView->windowPtr->variable_ptr, *destVar = destView->windowPtr->variable_ptr;
    member *loopSourceMember, *loopDestMember;
    ccInt sourceType = *sourceVar->types, destType = *destVar->types;
    ccInt sourceMemberNumber, destMemberNumber, indexCounter;
    ccInt sourceOffset, destOffset, sourceWidth, destWidth;
    bool sourceIsString = isVarString(sourceVar), destIsString = isVarString(destVar);
    
    
        // explore composite variables by individually stepping into their members
    
    if (sourceView->width != destView->width)  {  setError(type_mismatch_err, pcCodePtr-1);  return;  }
    
    if (sourceType >= composite_type)  {
        if (destType < composite_type)  setError(type_mismatch_err, pcCodePtr-1);
        if (errCode != passed)  return;
        
        sourceMemberNumber = 0;
        destMemberNumber = 0;
        findNextVisibleMember(sourceVar, &loopSourceMember, &sourceMemberNumber);
        findNextVisibleMember(destVar, &loopDestMember, &destMemberNumber);
        sourceOffset = destOffset = 0;
        
        while (sourceMemberNumber <= sourceVar->mem.members.elementNum)     {
            
            view nextSourceView = *sourceView, nextDestView = *destView;
            
            if (destMemberNumber > destVar->mem.members.elementNum)  setError(type_mismatch_err, pcCodePtr-1);
            if (errCode != passed)  return;
            
            if ((sourceType == destType) && (sourceVar->types[sourceVar->arrayDepth] == destVar->types[destVar->arrayDepth]))  {
                sourceWidth = loopSourceMember->indices;
                destWidth = loopDestMember->indices;        }
            else  {
                sourceWidth = destWidth = 1;
                if ((loopSourceMember->indices == 0) || (loopDestMember->indices == 0))  {
                    sourceWidth = destWidth = 0;
                    if (loopSourceMember->indices != loopDestMember->indices)  {
                        setError(type_mismatch_err, pcCodePtr-1);
                        return;
            }   }   }
            
            if (isBusy(loopSourceMember, busy_source_copy_flag))  {  setError(self_reference_err, pcCodePtr-1);  return;  }
            if (isBusy(loopDestMember, busy_dest_copy_flag))  {  setError(self_reference_err, pcCodePtr-1);  return;  }
            setBusy(loopSourceMember, busy_source_copy_flag);
            setBusy(loopDestMember, busy_dest_copy_flag);
            
            stepView(&nextSourceView, loopSourceMember, sourceOffset, sourceWidth);
            stepView(&nextDestView, loopDestMember, destOffset, destWidth);
            
            if (errCode == passed)
                ccComposite(&nextSourceView, &nextDestView);
            
            clearBusy(loopSourceMember, busy_source_copy_flag);
            clearBusy(loopDestMember, busy_dest_copy_flag);
            
            if (errCode != passed)  {
                if ((nextSourceView.windowPtr == NULL) && (nextDestView.windowPtr == NULL))  errCode = passed;
                else  return;     }
            
            if (sourceOffset+sourceWidth >= loopSourceMember->indices)  {
                findNextVisibleMember(sourceVar, &loopSourceMember, &sourceMemberNumber);
                sourceOffset = 0;        }
            else  sourceOffset += sourceWidth;
            
            if (destOffset+destWidth >= loopDestMember->indices)  {
                findNextVisibleMember(destVar, &loopDestMember, &destMemberNumber);
                destOffset = 0;          }
            else  destOffset += destWidth;      }
        
        if (destMemberNumber <= destVar->mem.members.elementNum)  setError(type_mismatch_err, pcCodePtr-1);    }
    
    
        // if it's a primitive variable then we handle that here
    
    else    {
    
            // the easiest option is to copy/compare the entire array (if it is one) at once, using linked list routines
        
        if ((sourceType == destType) && (!sourceIsString) && (sourceType != list_type))    {
            
            ccData(&(sourceVar->mem.data), sourceView->offset + 1, &(destVar->mem.data), destView->offset + 1, sourceView->width);   }
        
        else if (destType >= composite_type)  setError(type_mismatch_err, pcCodePtr-1);
        
        else   {
        
                // strings are special and we have to copy/compare each one individually
            
            if ((sourceIsString) || (sourceType == list_type))  {
                if (destType != sourceType)  setError(type_mismatch_err, pcCodePtr-1);
                else for (indexCounter = 1; indexCounter <= sourceView->width; indexCounter++)  {
                    ccString(LL_member(sourceVar, sourceView->offset + indexCounter)->memberWindow,
                            LL_member(destVar, destView->offset + indexCounter));
            }   }
            
            
                // finally, if we have mismatched but compatible types (think composite variables having ints versus doubles),
                // then we have to step through them member by member
            
            else   {
                if ((destIsString) || (destType == list_type))  setError(type_mismatch_err, pcCodePtr-1);
                else {
                    void(*ccFunction)(void *, void *) = ccJumpTable[5*(*sourceVar->types) + (*destVar->types)];
                    linkedlist *sourceLL = &(sourceVar->mem.data), *destLL = &(destVar->mem.data);
                    sublistHeader *sourceLLsublist = sourceLL->memory, *destLLsublist = destLL->memory;
                    ccInt sourceSublistLocalIndex = 0, destSublistLocalIndex = 0;
                    
                    if (sourceView->width > 0)  {       // skipElements() will crash if sourceLL or destLL has 0 elements
                        ccFunction( skipElements(sourceLL, &sourceLLsublist, &sourceSublistLocalIndex, sourceView->offset),
                                    skipElements(destLL, &destLLsublist, &destSublistLocalIndex, destView->offset));
                        if (errCode == passed)  {
                        for (indexCounter = 2; indexCounter <= sourceView->width; indexCounter++)  {
                            ccFunction( skipElements(sourceLL, &sourceLLsublist, &sourceSublistLocalIndex, 1),
                                        skipElements(destLL, &destLLsublist, &destSublistLocalIndex, 1)  );
                            if (errCode != passed)  break;
        }   }   }   }   }}
    }
}


bool isVarString(variable *theVar)
{
    ccInt *types = theVar->types;
    if (types[0] == string_type)  return true;
    if (types[0] == list_type)  return (types[1] == char_type);
    return false;
}


// Returns the next non-hidden, non-empty member of a given variable

void findNextVisibleMember(variable *theVariable, member **theMember, ccInt *memberCounter)
{
    do  {
        (*memberCounter)++;
        if (*memberCounter > theVariable->mem.members.elementNum)  return;
        *theMember = LL_member(theVariable, *memberCounter);
    }  while (((*theMember)->ifHidden) || ((*theMember)->indices == 0));
}


// copyCompareVarToList() copies or compares a single num/string/bool to an entire list of numbers/strings/bools

void copyCompareVarToList(void *sourceData, ccInt sourceDataType, view *destView, void(**copyCompareJumpTable)(void *, void *))
{
    void(*ccFunction)(void *, void *);
    linkedlist *destLL = &(destView->windowPtr->variable_ptr->mem.data);
    sublistHeader *destLLsublist = destLL->memory;
    ccInt loopVar, LLsublistLocalIndex = 0, destType = *destView->windowPtr->variable_ptr->types;
    
    if (isVarString(destView->windowPtr->variable_ptr))  destType = 4;
    ccFunction = copyCompareJumpTable[5*sourceDataType + destType];
    
    if (destView->width >= 1)  {
        ccFunction(sourceData, skipElements(destLL, &destLLsublist, &LLsublistLocalIndex, destView->offset));
        if (errCode == passed)  {
        for (loopVar = 2; loopVar <= destView->width; loopVar++)  {
            ccFunction(sourceData, skipElements(destLL, &destLLsublist, &LLsublistLocalIndex, 1));
            if (errCode != passed)  break;
    }   }}
}


// * * * * *  Functions operating on variables, including all members thereof  * * * * *


// Next 3 routines:  used by size(), as well as forced-equate for 'type-checking'.
// Here we calculate the size in bytes of a window (including its members, ...)

void sizeView(view *theView, void *dataSize, void *sizeofStrings)
{  doReadWrite(theView, dataSize, sizeofStrings, false, false, false, &sizeView, &sizeData, &sizeString);  }

void sizeData(view *theView, void *dataSize, void *dummy)
{
    ccInt sizeToAdd = theView->windowPtr->variable_ptr->mem.data.elementSize*theView->width;
    
    if (extCallMode)  sizeToAdd += align(sizeToAdd);
    *(ccInt *) dataSize += sizeToAdd;
}

void sizeString(view *theView, ccInt stringIndex, void *dataSize, void *sizeofStrings)
{
    member *stringMember = LL_member(theView->windowPtr->variable_ptr, stringIndex);
    
    if (*(ccInt *) sizeofStrings == no_string)  *(ccInt *) sizeofStrings = 0;
    *(ccInt *) sizeofStrings += stringMember->indices;
}


// Next 3 routines:  also used by size().
// These differ from the last in that the physical storage of the variables is what is counted.
// not wired in yet -- doesn't work because it can't subtract a window that was already counted
// from a later window that completely encloses it (i.e. the difference is a lower + upper piece)

void storageSizeView(view *theView, void *dataSize, void *dummy)
{  doReadWrite(theView, dataSize, dummy, false, true, false, &storageSizeView, &storageSizeData, &storageSize_String);  }

void storageSizeData(view *theView, void *dataSize, void *ssMode)
{
    ccInt loopWindowNum, mode = *(ccInt *) ssMode;
    window *loopWindow;
    pinned_LL *windowsPLL = &(theView->windowPtr->variable_ptr->windows);
    
    if (mode == 1)  {
        if (isBusy(theView->windowPtr, busy_SS_flag))  return;      }
    
    else if (mode == 2)  {
        bool firstBusyWindow = false;
        ccInt windowTop, toAdd, bestOffset, bestTop, prevOffset = -1, prevTop = 0;
        
        for (loopWindowNum = 1; loopWindowNum <= windowsPLL->data.elementNum; loopWindowNum++)  {
            loopWindow = (window *) element(&(windowsPLL->data), loopWindowNum);
            if (isBusy(loopWindow, busy_SS_flag))  {
                firstBusyWindow = (loopWindow == theView->windowPtr);
                break;
        }   }
        
        if (firstBusyWindow)  {
        while (true)  {
            window *bestWindow = NULL;
            bestTop = prevTop;
            bestOffset = ccIntMax;
            
            for (loopWindowNum = 1; loopWindowNum <= windowsPLL->data.elementNum; loopWindowNum++)  {
                loopWindow = (window *) element(&(windowsPLL->data), loopWindowNum);
                if (isBusy(loopWindow, busy_SS_flag))  {
                    windowTop = loopWindow->offset+loopWindow->width;
                    if ( ((loopWindow->offset > prevOffset) && (loopWindow->offset <= prevTop) && (windowTop > bestTop))  
                                || ((loopWindow->offset < bestOffset) && (bestOffset > prevTop) && (windowTop > prevTop)) )  {
                        bestWindow = loopWindow;
                        bestOffset = loopWindow->offset;
                        bestTop = windowTop;
            }   }   }
            
            if (bestWindow == NULL)  break;
            
            if (bestOffset <= prevTop)  toAdd = bestTop-prevTop;
            else  toAdd = bestTop-bestOffset;
            
            *(ccInt *) dataSize += toAdd*bestWindow->variable_ptr->mem.data.elementSize;
            
            prevOffset = bestOffset;
            prevTop = bestTop;
    }   }}
    
    if (mode == 1)  setBusy(theView->windowPtr, busy_SS_flag);
    else if (mode == 3)  clearBusy(theView->windowPtr, busy_SS_flag);
}

void storageSize_String(view *theView, ccInt stringIndex, void *dataSize, void *dummy)
{
    member *stringMember = LL_member(theView->windowPtr->variable_ptr, stringIndex);
    view charView;
    
    charView.windowPtr = theView->windowPtr;
    charView.offset = 0;
    charView.width = 1;
    
    stepView(&charView, stringMember, 0, stringMember->indices);
    if (errCode == passed)  storageSizeView(&charView, dataSize, dummy);
}


// Next 3 routines:  used by forced-equate.
// These copy data from a source buffer into a window.

void writeView(view *theView, void *bufferPtr, void *sizeofStrings)        // writes the variables
{  doReadWrite(theView, bufferPtr, sizeofStrings, true, false, false, &writeView, &writeData, &writeString);  }

void writeData(view *theView, void *bufferPtr, void *dummy)
{
    linkedlist *theLL = &(theView->windowPtr->variable_ptr->mem.data);
    
    setElements(theLL, theView->offset+1, theView->offset+theView->width, *(void **) bufferPtr);
    *(char **) bufferPtr += theLL->elementSize*theView->width;
    if (extCallMode)  *(char **) bufferPtr += align(theLL->elementSize*theView->width);
}

void writeString(view *theView, ccInt stringIndex, void *bufferPtr, void *sizeofStrings)
{
    member *stringMember;
    view stringView;
    
    stringMember = LL_member(theView->windowPtr->variable_ptr, stringIndex);
    stringView.windowPtr = stringMember->memberWindow;
    resizeMember(stringMember, 1, *(ccInt *) sizeofStrings);
    if (errCode != passed)  return;
    
    if (*(ccInt *) sizeofStrings == 0)  return;
    
    setElements(&(stringMember->memberWindow->variable_ptr->mem.data), stringMember->memberWindow->offset+1,
            stringMember->memberWindow->offset+stringMember->memberWindow->width, *(void **) bufferPtr);
    *(char **) bufferPtr += *(ccInt *) sizeofStrings;
    *(ccInt *) sizeofStrings = 0;
}


// Next 3 routines:  used by forced-equate.
// These copy data from a window into a destination memory buffer.

void readView(view *theView, void *bufferPtr, void *sizeofStrings)
{  doReadWrite(theView, bufferPtr, sizeofStrings, true, false, false, &readView, &readData, &readString);  }

void readData(view *theView, void *bufferPtr, void *dummy)
{
    linkedlist *theLL = &(theView->windowPtr->variable_ptr->mem.data);
    
    getElements(theLL, theView->offset+1, theView->offset+theView->width, *(void **) bufferPtr);
    *(char **) bufferPtr += theLL->elementSize*theView->width;
    if (extCallMode)  *(char **) bufferPtr += align(theLL->elementSize*theView->width);
}

void readString(view *theView, ccInt stringIndex, void *bufferPtr, void *sizeofStrings)
{
    window *stringWindow = LL_member(theView->windowPtr->variable_ptr, stringIndex)->memberWindow;
    linkedlist *stringLL = &(stringWindow->variable_ptr->mem.data);
    
    getElements(stringLL, stringWindow->offset+1, stringWindow->offset+stringWindow->width, *(void **) bufferPtr);
    *(char **) bufferPtr += stringWindow->width;
}


// Next 3 routines:  invoked by C_function()
// These count the number of seperate arguments that will go into the 'argv' array.

void countDataLists(view *theView, void *windowCount, void *stringCount)
{  doReadWrite(theView, windowCount, stringCount, false, false, true, &countDataLists, &countDataView, countStringView);  }

void countDataView(view *theView, void *windowCount, void *dummy)
{  (*(ccInt *) windowCount)++;  }

void countStringView(view *theView, ccInt stringDummyIndex, void *windowCount, void *stringCount)
{
/*    ccInt stringIndex, windowCounter;       // catch any window overlaps before we allocate memory, to avoid crashes
    
    for (stringIndex = theView->offset+1; stringIndex <= theView->offset+theView->width; stringIndex++)  {
        
        window *theStringWindow = LL_member(theView->windowPtr->variable_ptr, stringIndex)->memberWindow;
        
            // make sure the string doesn't have more than one window -- that way the user can modify its size and we can still fix the member
        
        for (windowCounter = 1; windowCounter <= theStringWindow->variable_ptr->windows.data.elementNum; windowCounter++)  {
            window *loopWindow = (window *) element(&(theStringWindow->variable_ptr->windows.data), windowCounter);
            if ((loopWindow != theStringWindow) && (*(loopWindow->references) > 0))  {
                if (loopWindow->jamStatus == cannot_jam)  loopWindow->jamStatus = unjammed;
                else  {
                    setError(overlapping_window_err, pcCodePtr-1);
                    return;
    }   }   }   }*/
    
    (*(ccInt *) stringCount)++;
}


// Next 3 routines:  invoked by C_function()
// These fill the argv array of a C function with the addresses of the data lists.

void argvFillHandles(view *theView, void *argsPtr, void *dummy)
{  doReadWrite(theView, argsPtr, dummy, false, false, true, &argvFillHandles, &passDataView, &passString);  }

void passDataView(view *theView, void *argsPtr, void *dummy)
{
    variable *theVar = theView->windowPtr->variable_ptr;
    argsType *oneArg = (argsType *) argsPtr;
    ccInt rtrn;
    
    rtrn = defragmentLinkedList(&(theVar->mem.data));
    if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr-1);  return;  }
    
    if (theView->offset == theVar->mem.data.elementNum)  
        *(oneArg->p) = (void *) (((char *) theVar->mem.data.memory) + sublistHeaderSize);
    else  *(oneArg->p) = element(&(theVar->mem.data), theView->offset+1);
    *(oneArg->type) = *theVar->types;
    *(oneArg->indices) = theView->width;
    
    incrementArg(oneArg);
}

void passString(view *theView, ccInt stringDummyIndex, void *argsPtr, void *dummy)
{
    argsType *oneArg = (argsType *) argsPtr;
    ccInt stringIndex, rtrn;
    
    linkedlist *stringListCopy = (linkedlist *) malloc((size_t) (theView->width*sizeof(linkedlist)));
    if (stringListCopy == NULL)  {  setError(out_of_memory_err, pcCodePtr-1);  return;  }
    
    for (stringIndex = theView->offset+1; stringIndex <= theView->offset+theView->width; stringIndex++)  {
        
        window *theStringWindow = LL_member(theView->windowPtr->variable_ptr, stringIndex)->memberWindow;
        linkedlist *theStringLL = &(theStringWindow->variable_ptr->mem.data);
        
        rtrn = defragmentLinkedList(theStringLL);
        if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
        
        if (theStringLL->elementNum != theStringWindow->width)  {  setError(incomplete_variable_err, pcCodePtr-1);  return;  }
        
            // make sure the string doesn't have more than one window -- that way the user can modify its size and we can still fix the member
            // This doesn't work -- doesn't check for overlap based on offset/indices, only whether two windows use the same variable
        /*
        for (windowCounter = 1; windowCounter <= theStringWindow->variable_ptr->windows.data.elementNum; windowCounter++)  {
            window *loopWindow = (window *) element(&(theStringWindow->variable_ptr->windows.data), windowCounter);
            if ((loopWindow != theStringWindow) && (*(loopWindow->references) > 0))  {
                if (loopWindow->jamStatus == cannot_jam)  loopWindow->jamStatus = unjammed;
                else  {
                    setError(overlapping_window_err, pcCodePtr-1);
                    return;
        }   }   }*/
        
            // What was this trying to do?  Does not work
        
        /*
        rtrn = addMemory(theStringWindow, 0, -theStringWindow->offset);
        if (rtrn == passed)
            rtrn = addMemory(theStringWindow, theStringWindow->width, theStringWindow->width-theStringWindow->variable_ptr->instances);
        if (rtrn != passed)  setError(rtrn, pcCodePtr-1);
        */
        
        stringListCopy[stringIndex-theView->offset-1] = *theStringLL;
    }
    
    *(oneArg->p) = (void *) stringListCopy;
    *(oneArg->type) = string_type;
    *(oneArg->indices) = theView->width;
    
    incrementArg(oneArg);
}

// Next 3 routines:  again, used by C_function()
// This adjusts the size of our string member windows to match the size of the character arrays (in case the user changed those).

void argvFixStrings(view *theView, void *argsPtr, void *dummy)
{  doReadWrite(theView, argsPtr, dummy, false, false, true, &argvFixStrings, &fixNothing, &fixString);  }

void fixNothing(view *theView, void *argsPtr, void *dummy)
{
    argsType *oneArg = (argsType *) argsPtr;
    (oneArg->p)++;
}

void fixString(view *theView, ccInt stringDummyIndex, void *argsPtr, void *dummy)
{
    ccInt stringIndex;
    argsType *oneArg = (argsType *) argsPtr;
    
    linkedlist *stringListCopy = (linkedlist *) *(oneArg->p);
    
    for (stringIndex = theView->offset+1; stringIndex <= theView->offset+theView->width; stringIndex++)  {
        
        member *theStringMember = LL_member(theView->windowPtr->variable_ptr, stringIndex);
        linkedlist *theStringLL = &(theStringMember->memberWindow->variable_ptr->mem.data);
        
        *theStringLL = stringListCopy[stringIndex-theView->offset-1];
        
        theStringMember->indices = theStringLL->elementNum;
        theStringMember->memberWindow->width = theStringLL->elementNum;
        theStringMember->memberWindow->variable_ptr->instances = theStringLL->elementNum;
    }
    
    free((void *) stringListCopy);
    
    (oneArg->p)++;
}


// hexDigit():  converts a number 0-15 into a hexidecimal digit 0-F (used in printing strings)

char hexDigit(unsigned char hexNumber)
{
    if (hexNumber <= 9)  return hexNumber+'0';
    
    return hexNumber-10+'A';
}


// doReadWrite() is used for scanning a single window tree (in comparison to doCopyCompare() which scans two at once).
// Invoked by size(), feq, read_string(), print_string(), C_function().
// This routine skips hidden members; so for example size() will not register these.

void doReadWrite(view *theView, void *globalPtr, void *secondaryPtr, bool doInIndexOrder, bool skipBusyMembers, bool bundleStrings,
        void(*srwComposite)(view *, void *, void *),
        void(*srwData)(view *, void *, void *),
        void(*srwString)(view *, ccInt, void *, void *))
{
    view nextView;
    member *loopMember;
    variable *theVar = theView->windowPtr->variable_ptr;
    ccInt memberCounter, indexCounter, firstMember, lastMember;
//    bool encompassesLists = false;
    
/*    if (theVar->arrayDepth > 0)  {
    if (theVar->types[1] == list_type)  {
        encompassesLists = true;
    }}*/
    
    if (*theVar->types >= composite_type)   {
        
        if (*theVar->types != list_type)  {  firstMember = 1;  lastMember = theVar->mem.members.elementNum;  }
        else  {  firstMember = theView->offset+1; lastMember = theView->offset+theView->width;  }
        
        for (memberCounter = firstMember; memberCounter <= lastMember; memberCounter++)  {
            loopMember = LL_member(theVar, memberCounter);
            
            if (isBusy(loopMember, busy_SRW_flag))  {
                if (!skipBusyMembers)  {
                    setError(self_reference_err, pcCodePtr-1);
                    return;
            }   }
            
            else  {
                setBusy(loopMember, busy_SRW_flag);
                
                if ((loopMember->memberWindow != NULL) && (!loopMember->ifHidden))      {
                    if (doInIndexOrder)  {
                    for (indexCounter = 0; indexCounter < theView->width*loopMember->indices; indexCounter++)  {
                        nextView.windowPtr = theView->windowPtr;
                        nextView.offset = theView->offset;
                        nextView.width = 1;
                        stepView(&nextView, loopMember, indexCounter, 1);
                        if (errCode == passed)  srwComposite(&nextView, globalPtr, secondaryPtr);
                        else  errCode = passed;
                    }}
                    
                    else  {
                        nextView = *theView;
                        if (*theVar->types == list_type)  {  nextView.offset = 0;  nextView.width = 1;  }
                        stepView(&nextView, loopMember, 0, loopMember->indices);
                        if (errCode == passed)  srwComposite(&nextView, globalPtr, secondaryPtr);
                        else  errCode = passed;
                }   }
                
                clearBusy(loopMember, busy_SRW_flag);
                if (errCode != passed)  return;
    }   }   }
    
    else  {        // it's a primitive variable
        if (*theVar->types == string_type)  {
            ccInt viewTop = theView->width;
            if (bundleStrings)  viewTop = 1;  
            
            for (indexCounter = 1; indexCounter <= viewTop; indexCounter++)  {
                srwString(theView, theView->offset+indexCounter, globalPtr, secondaryPtr);
                if (errCode != passed)  break;
        }   }
        else  srwData(theView, globalPtr, secondaryPtr);            }
}


// printNumber() prints a number out in characters, and returns a count of the number of characters.
// This is used by print_string().
// If destString == NULL, we are just getting the size of the string, not writing anything.
// Otherwise, we write destString in the confidence that print_string() has given it the required storage.

void printNumber(char *destString, const ccFloat numberToPrint, ccInt *characterCount, const ccInt numberType, const ccInt maxFloatingDigits)
{
    char theChars[50];
    
    if (numberType != double_type)  *characterCount = sprintf(theChars, printIntFormatString, (ccInt) numberToPrint);
    else  *characterCount = sprintf(theChars, print_stringFloatFormatString, maxFloatingDigits, numberToPrint);
    
    if (destString != NULL)  memcpy((void *) destString, (void *) theChars, *characterCount);
    
    return;
}



// * * * * *  Routines for walking up and down the member/path trees  * * * * *


// The next routine searches up the path path for a variable member with a name of soughtMemberID.
// Returns the (path &) window (in searchView) & member number.
// Note that the window returned is that of the stem variable; user can step from there using DefStepToIndex or just STI.
// Assumes we are starting from a composite variable.

void searchMember(ccInt soughtMemberID, member **stemMember, ccInt *stemMemberNumber, bool allowAddMember, bool ifHiddenMember)
{
    searchPath *searchPosition;
    ccInt rtrn;
    
    searchPosition = pcSearchPath;
    searchView = baseView;
    
    do  {
        searchView.windowPtr = searchPosition->jamb;
        rtrn = findMemberID(searchView.windowPtr->variable_ptr, soughtMemberID, stemMember, stemMemberNumber, allowAddMember, ifHiddenMember);
        if (rtrn == passed)  {
            return;    }
        else if (rtrn != member_not_found_err)  {
            setError(rtrn, pcCodePtr);
            return;    }
        
        if (searchPosition->stem == NULL)  break;
        else if (searchPosition->stemIndices == 0)
            searchView.offset = 0;
        else
            searchView.offset = (searchView.offset - searchView.windowPtr->offset) / searchPosition->stemIndices;
        
        if (searchView.width > 1)  searchView.width /= searchPosition->stemIndices;
        else if (searchPosition->stemIndices == 0)  {           // if we stepped through a 0-sized member, recalculate indices
            searchView.width = 1;
            searchPath *SP2 = searchPosition->stem;
            while (SP2->stem != NULL)  {
                searchView.width *= SP2->stemIndices;
                SP2 = SP2->stem;
        }   }
        if (searchView.multipleIndices)  searchView.multipleIndices = (searchView.width != 1);
        
        searchPosition = searchPosition->stem;
    }  while (true);
    
    setError(member_not_found_err, pcCodePtr);
    return;
}



// findMemberID() returns the member number (not index!) of the member with the sought ID (name).
// Returns 0 if no match found in the current window, or if we're for some reason looking for an ID of zero
// (which is not allowed since the entries in the names LL begin at one) (also since ID 0 is reserved for implicit variable definitions).

ccInt findMemberID(variable *theObject, ccInt soughtMemberID, member **soughtMember, ccInt *soughtMemberNumber,
                bool allowAddMember, bool ifHiddenMember)
{
    ccInt counter, rtrn;
    linkedlist *membersList;
    
    if (*theObject->types < composite_type)  return member_not_found_err;
    membersList = &(theObject->mem.members);
    
    for (counter = 1; counter <= membersList->elementNum; counter++)   {
        *soughtMember = (member *) element(membersList, counter);
        if ((*soughtMember)->memberID == soughtMemberID)   {
            *soughtMemberNumber = counter;
            return passed;
    }   }
    
    if (!allowAddMember)  {
        *soughtMember = NULL;
        rtrn = member_not_found_err;        }
    
    else  {
        *soughtMemberNumber = theObject->mem.members.elementNum + 1;
        rtrn = addMembers(theObject, *soughtMemberNumber, 1, soughtMember, ifHiddenMember, 1, true);
        if (rtrn == passed)  (*soughtMember)->memberID = soughtMemberID;        }
    
    return rtrn;
}


// findMemberIndex() returns the member number corresponding to the given index [n] requested by the user
// Remember that some members (e.g. arrays) have multiple indices, whereas hidden members are skipped entirely from the index count.
// Returns, in addition, the member * and the 'entry offset', an offset from the first index of the member which can be 0.

ccInt findMemberIndex(variable *theVariable, ccInt currentOffset, ccInt soughtMemberIndex,
        member **soughtMember, ccInt *memberNumber, ccInt *entryOffset, bool allowAddMember)
{
    ccInt indexCounter, rtrn;
    
    if (soughtMemberIndex <= 0)  return invalid_index_err;
    
    
        // if it's a composite variable, return the Nth member
    
    if (*theVariable->types == composite_type)  {
        *entryOffset = 0;
        indexCounter = 0;
        for (*memberNumber = 1; *memberNumber <= theVariable->mem.members.elementNum; (*memberNumber)++)  {
            *soughtMember = LL_member(theVariable, *memberNumber);
            if (!(*soughtMember)->ifHidden)  {
                indexCounter++;
                if (indexCounter == soughtMemberIndex)  return passed;
        }   }
        
        if ((!allowAddMember) || (indexCounter != soughtMemberIndex-1))  {
            *entryOffset = *memberNumber;
            return invalid_index_err;         }
        
        else  {
            *memberNumber = theVariable->mem.members.elementNum + 1;
            rtrn = addMembers(theVariable, *memberNumber, 1, soughtMember, false, 1, true);
            if (rtrn == passed)  (*soughtMember)->memberID = 0;
            return rtrn;
    }   }
    
    
        // if it's an array or string variable, return the Nth index of the first member
    
    if ((*theVariable->types == string_type) || (*theVariable->types == list_type))  *memberNumber = currentOffset+1;
    else  *memberNumber = 1;
    *soughtMember = LL_member(theVariable, *memberNumber);
    
    
    if (soughtMemberIndex > (*soughtMember)->indices)  {
        *entryOffset = (*soughtMember)->indices+1;
        return invalid_index_err;    }
    
    *entryOffset = soughtMemberIndex - 1;
    return passed;
}


ccInt numMemberIndices(view *theView)
{
    ccInt memberCounter, topIndex;
    variable *theVariable = theView->windowPtr->variable_ptr;
    
    if (*theVariable->types == composite_type)  {
        topIndex = 0;
        for (memberCounter = 1; memberCounter <= theVariable->mem.members.elementNum; memberCounter++)  {
        if (!(LL_member(theVariable, memberCounter)->ifHidden))  {
            topIndex ++;
    }   }}
    
    else if (*theVariable->types == array_type)  topIndex = (LL_member(theVariable, 1))->indices;
    
    else  topIndex = (LL_member(theVariable, theView->offset+1))->indices;
    
    return topIndex;
}


// This is a general routine for stepping a window/variable pointer to one of its members.

void stepView(view *viewToStep, member *startingMember, ccInt entryOffset, ccInt indexWidth)
{
    window *memberWindow = startingMember->memberWindow;
    ccInt startingOffset = viewToStep->offset, startingWidth = viewToStep->width;
    if (*viewToStep->windowPtr->variable_ptr->types == string_type)  startingOffset = 0;
    
    if (*viewToStep->windowPtr->variable_ptr->types == list_type)  {  startingOffset = 0;  startingWidth = 1;  }
    
    if (memberWindow == NULL)  {  viewToStep->windowPtr = NULL;  setError(void_member_err, pcCodePtr-1);  return;  };
    
    viewToStep->windowPtr = memberWindow;
    if (startingMember->memberWindow->jamStatus == unjammed)  {
        viewToStep->windowPtr = NULL;
        setError(void_member_err, pcCodePtr-1);
        return;         }
    
    viewToStep->offset = startingOffset*startingMember->indices + viewToStep->windowPtr->offset + entryOffset;
    viewToStep->width = startingWidth * indexWidth;
}



// * * * * *  Error and warning routines  * * * * *


// setError() sets an error code that stops execution

void setError(ccInt errorCode, ccInt *errorPtr)
{
    setErrIndex(errorPtr, errorCode, &PCCodeRef, &errCode, &errIndex, &errScript);
}


// setWarning() sets a warning code, but execution continues

void setWarning(ccInt warnCode, ccInt *warnPtr)
{
    setErrIndex(warnPtr, warnCode, &PCCodeRef, &warningCode, &warningIndex, &warningScript);
}


// setErrIndex(), called by both setError() and setWarning(), stores information about the error location in the code

void setErrIndex(ccInt *errPtr, ccInt theError, code_ref *theScript, ccInt *errCodePtr, ccInt *errIndexPtr, code_ref *errScriptPtr)
{
    *errCodePtr = theError;
    
    if (errScriptPtr->code_ptr != NULL)  derefCodeRef(errScriptPtr);
    *errScriptPtr = *theScript;
    errScriptPtr->anchor = NULL;
    refCodeRef(errScriptPtr);
    
    *errIndexPtr = ((ccInt) (errPtr - errScriptPtr->code_ptr)) + 1;
}



// * * * * *  Memory routines  * * * * *


// addVariable() adds a new variable to the variable tree, and sets a number of the fields in the variable record.
// It not allocate memory for members or data -- that is done by each of the two routines manually.
// Business is set to zero.  Assumes zero initial instances of the variable.
// The new variable must be referenced by a window immediately or it will likely be overwritten.

ccInt addVariable(variable **theNewVar, const ccInt *types, ccInt arrayDepth, bool makeSpareRoom)
{
    ccInt newVarIndex, rtrn;
    ccFloat freeSpace;
    
    rtrn = addPLLElement(&VariableList, (void **) theNewVar, &newVarIndex);
    if (rtrn != passed)  return rtrn;
    
    (*theNewVar)->PLL_index = newVarIndex;
    (*theNewVar)->references = LL_int(&(VariableList.references), newVarIndex);
    (*(*theNewVar)->references) = 0;         // the windows are all the references we want
    
    rtrn = newLinkedList(&((*theNewVar)->codeList), 0, sizeof(code_ref), 0., false);
    if (rtrn != passed)  return out_of_memory_err;
    
    rtrn = newPLL(&((*theNewVar)->windows), 0, sizeof(window), LLFreeSpace);
    if (rtrn != passed)  return rtrn;
    rtrn = newPLL(&((*theNewVar)->pathList), 3, sizeof(searchPath), LLFreeSpace);        // why 3?
    if (rtrn != passed)  return rtrn;
    
    if (makeSpareRoom)  freeSpace = LLFreeSpace;
    else  freeSpace = 0.;
    rtrn = newLinkedList(&((*theNewVar)->mem.data), 0, typeSizes[*types], freeSpace, false);
    if (rtrn != passed)  return out_of_memory_err;
    
    (*theNewVar)->types = malloc((arrayDepth+1)*sizeof(ccInt));
    if ((*theNewVar)->types == NULL)  return out_of_memory_err;
    memcpy((*theNewVar)->types, types, (arrayDepth+1)*sizeof(ccInt));
    
    (*theNewVar)->arrayDepth = arrayDepth;
    (*theNewVar)->instances = 0;
    (*theNewVar)->business = 0;
    
    return passed;
}


// References a variable (signifying that it is being used)

void refVariable(variable *theVariable)
{
    RefPLLPtr(theVariable->references);
    
    return;
}


// Dereferences a variable, and deletes it if refs == 0.

void derefVariable(variable *theVariable)
{
    member *loopMember;
    ccInt counter, codeCounter;
    
    derefPLLPtr(&VariableList, theVariable->PLL_index, theVariable->references);
    if (*(theVariable->references) != 0)  return;
    
    if (*theVariable->types >= string_type)  {
    for (counter = 1; counter <= theVariable->mem.members.elementNum; counter++)   {
        loopMember = LL_member(theVariable, counter);
        for (codeCounter = 1; codeCounter <= loopMember->codeList.elementNum; codeCounter++)   {
            derefCodeRef((code_ref *) element(&(loopMember->codeList), codeCounter));   }
        derefWindow(&(loopMember->memberWindow));
    }}
    
    deletePLL(&(theVariable->windows));     // All the windows must already have been deleted, since variables are referenced by the windows.
    deletePLL(&(theVariable->pathList));    // All search paths are already gone, since all the windows are gone.
    
    for (counter = 1; counter <= theVariable->codeList.elementNum; counter++)
        derefCodeRef((code_ref *) element(&(theVariable->codeList), counter));
    
    free(theVariable->types);
    
    return;
}


// addWindow() creates new instances of the variable, and allocates the associated storage space.
// Does not reference the window (and the host variable does not get referenced until the window is first referenced).
// If windowOffset is greater than any possible offset (given by hostVariable->instances), this routine creates new space in the variable;
// otherwise, it is construed as an alias at windowOffset.

ccInt addWindow(variable *hostVariable, ccInt windowOffset, ccInt addedInstances, window **newWindow, bool ifCanJam)
{
    ccInt newIndex, rtrn;
    
    rtrn = addPLLElement(&(hostVariable->windows), (void **) newWindow, &newIndex);
    if (rtrn != passed)  return out_of_memory_err;
    
    (*newWindow)->PLL_index = newIndex;
    (*newWindow)->references = (ccInt *) element(&(hostVariable->windows.references), newIndex);
    (*newWindow)->variable_ptr = hostVariable;
    (*newWindow)->offset = windowOffset;
    (*newWindow)->business = false;
    if (ifCanJam)  (*newWindow)->jamStatus = can_jam;
    else  (*newWindow)->jamStatus = cannot_jam;
    
    *((*newWindow)->references) = 0;
    
    if (windowOffset == hostVariable->instances)   {
        (*newWindow)->width = 0;                            // the 'starting width' before addMemory() adds to it
        rtrn = addMemory(*newWindow, 0, addedInstances);    // no need to check overlap
        unflagVariables(hostVariable, busy_add_flag);
        return rtrn;       }
    else  (*newWindow)->width = addedInstances;
    
    return passed;
}


// References a window, and its host variable as well if this is the first reference.

void refWindow(window *theWindow)
{
    if (theWindow == NULL)  return;
    
    RefPLLPtr(theWindow->references);
    if (*(theWindow->references) == 1)   {
        refVariable(theWindow->variable_ptr);    }
        
    return;
}


// Dereferences a window, and deletes it if refs == 0.

void derefWindow(window **theWindow)
{
    if (*theWindow == NULL)  return;
    
    derefPLLPtr(&((*theWindow)->variable_ptr->windows), (*theWindow)->PLL_index, (*theWindow)->references);
    if (*((*theWindow)->references) == 0)  derefVariable((*theWindow)->variable_ptr);
    
    *theWindow = NULL;
    
    return;
}


// combVariables() removes any windows/variables/etc. that cannot be accessed.
// Inaccessible memory can occur if A references B and B references A, but neither are connected
// to the memory tree where baseView, or any baseWindows on the PCStack, reside.
// This is the rigorous, manual garbage-collection routine invoked by springCleaning().

void combVariables()
{
    ccInt variableCounter, memberCounter, windowCounter, fibrilCounter, codeCounter, codeTop, membersTop;
    variable *loopVariable;
    member *loopMember;
    
    
        // First flag all accessible regions of memory
    
    combBranch(baseView.windowPtr->variable_ptr, true);
    for (windowCounter = 1; windowCounter <= PCStack.top; windowCounter++)
        combBranch(((view *) element(&(PCStack.data), windowCounter))->windowPtr->variable_ptr, true);
        
        
        // Now scan for variables that have their flags cleared
    
    for (variableCounter = VariableList.data.elementNum; variableCounter >= 1; variableCounter--)   {
    if (*LL_int(&(VariableList.references), variableCounter) != 0)   {
        loopVariable = (variable *) element(&(VariableList.data), variableCounter);
        
        if (!isBusy(loopVariable, busy_comb_flag))  {
        
            refVariable(loopVariable);            // make sure the variable won't be deleted while we're still dismembering it
            
            codeTop = loopVariable->codeList.elementNum;     // remove its code, since the anchor references the variable indirectly
            for (codeCounter = 1; codeCounter <= codeTop; codeCounter++)   {
                derefCodeRef((code_ref *) element(&(loopVariable->codeList), codeCounter));   }
            deleteElements(&(loopVariable->codeList), 1, codeTop);
            
            if (*loopVariable->types >= composite_type)  {      // composite?  then remove its members' code lists and window *s
                membersTop = loopVariable->mem.members.elementNum;      // have it stored separately for deleteElements()
                for (memberCounter = 1; memberCounter <= membersTop; memberCounter++)  {
                    loopMember = LL_member(loopVariable, memberCounter);
                    for (codeCounter = 1; codeCounter <= loopMember->codeList.elementNum; codeCounter++)  {
                        derefCodeRef((code_ref *) element(&(loopMember->codeList), codeCounter));   }
                    if (loopMember->memberWindow != NULL)  unlinkWindow(loopVariable, &(loopMember->memberWindow), memberCounter);
                    deleteLinkedList(&(loopMember->codeList));
                }
                deleteElements(&(loopVariable->mem.members), 1, membersTop);      // ?
            }
            
            for (fibrilCounter = 1; fibrilCounter <= loopVariable->pathList.data.elementNum; fibrilCounter++)  {
            if (*LL_int(&(loopVariable->pathList.references), fibrilCounter) != 0)  {
                derefPath(&(((searchPath *) element(&(loopVariable->pathList.data), fibrilCounter))->stem));
            }}
            derefVariable(loopVariable);      // the mortal blow
        }
    }}
    
    
        // remove the accessibility flags
    
    combBranch(baseView.windowPtr->variable_ptr, false);
    for (windowCounter = 1; windowCounter <= PCStack.top; windowCounter++)
        combBranch(((view *) element(&(PCStack.data), windowCounter))->windowPtr->variable_ptr, false);
    
    return;
}


// combBranch() recursively scans the variable tree to find all accessible variables, and set their business comb flags.
// This is called by CombVariables().

void combBranch(variable *oneVariable, bool doSetFlag)
{
    ccInt counter;
    member *loopMember;
    searchPath *loopPath;
    
    if (oneVariable == NULL)  return;
    if (doSetFlag)  {
        if (isBusy(oneVariable, busy_comb_flag))  return;
        setBusy(oneVariable, busy_comb_flag);       }
    else  {
        if (!isBusy(oneVariable, busy_comb_flag))  return;
        clearBusy(oneVariable, busy_comb_flag);       }
    
    if (*oneVariable->types >= composite_type)  {
        for (counter = 1; counter <= oneVariable->mem.members.elementNum; counter++)  {
            loopMember = LL_member(oneVariable, counter);
            if ((!isBusy(loopMember, busy_comb_flag)) && (loopMember->memberWindow != NULL))  {
                setBusy(loopMember, busy_comb_flag);
                combBranch(loopMember->memberWindow->variable_ptr, doSetFlag);
                clearBusy(loopMember, busy_comb_flag);
    }   }   }
    
    for (counter = 1; counter <= oneVariable->pathList.data.elementNum; counter++)  {
    if (*LL_int(&(oneVariable->pathList.references), counter) != 0)  {
        loopPath = (searchPath *) element(&(oneVariable->pathList.data), counter);
        if (loopPath->stem != NULL)  combBranch(loopPath->stem->variablePtr, doSetFlag);      // no need to do loopPath->variablePtr
    }}
    
    for (counter = 1; counter <= oneVariable->codeList.elementNum; counter++)  {
        loopPath = ((code_ref *) element(&(oneVariable->codeList), counter))->anchor;
        if (loopPath != NULL)  combBranch(loopPath->variablePtr, doSetFlag);         // anchors are NULL in err/w_script, thus usually in R_*_script
    }
    
    return;
}


// unlinkWindow() cuts all 'matching' search paths in unlinkedWindow whose stems lie in stemVariable, then removes the unlinkedWindow pointer.
// Derefs both the search paths and unlinkedWindow.

void unlinkWindow(variable *stemVariable, window **unlinkedWindow, ccInt stemMemberNumber)
{
    pinned_LL *unlinkedFibrilList = &((*unlinkedWindow)->variable_ptr->pathList);
    window *holdWindowPtr;
    searchPath *loopPath, *clippedStem;
    ccInt counter;
    
    for (counter = 1; counter <= unlinkedFibrilList->data.elementNum; counter++)  {
    if (*LL_int(&(unlinkedFibrilList->references), counter) != 0)  {
        loopPath = (searchPath *) element(&(unlinkedFibrilList->data), counter);
        if (loopPath->stem != NULL)  {
        if (loopPath->stem->variablePtr == stemVariable)  {
            clippedStem = loopPath->stem;
            loopPath->stem = NULL;
            derefPath(&clippedStem);
    }}  }}
    
    holdWindowPtr = *unlinkedWindow;
    *unlinkedWindow = NULL;
    derefWindow(&holdWindowPtr);        // We just deleted a link, so in any event dereference unlinkedWindow.    
    
    return;
}


// addMembers()  creates a new member for a variable, at position newMemberNumber in the member linked-list and having newIndices.
// The memberID, type and constant flag are set separately, since it's awkward to pass the appropriate variables.
// A window in the target variable is created automatically if destWindow == NULL --> new space is being created (i.e. done with a :=).
// Or, new space is created if destIndex > DestVariable->instances.

ccInt addMembers(variable *hostVariable, ccInt newMemberNumber, ccInt newIndices, member **newMember, bool ifHidden,
        ccInt membersToAdd, const bool doWrite)
{
    linkedlist *memberLL = &(hostVariable->mem.members);
    ccInt cm, rtrn;
    
    
        // add the new member and write its fields
    
    if (membersToAdd > 0)  {
        rtrn = insertElements(memberLL, newMemberNumber, membersToAdd, false);
        if (rtrn != passed)  return out_of_memory_err;      }
    else  membersToAdd = 1;
    
    if (doWrite)  {
    for (cm = 0; cm < membersToAdd; cm++)  {
        *newMember = (member *) element(memberLL, newMemberNumber+cm);
        (*newMember)->indices = newIndices;
        (*newMember)->memberWindow = NULL;
        (*newMember)->type = no_type;
        (*newMember)->eventualType = no_type;
        (*newMember)->arrayDepth = 0;
        (*newMember)->ifHidden = ifHidden;
        (*newMember)->business = 0;
        
        rtrn = newLinkedList(&((*newMember)->codeList), 0, sizeof(code_ref), 0., false);
        if (rtrn != passed)  return out_of_memory_err;
    }}
    
    return passed;
}


// removeMember() deletes a member from the members LL of theVariable.
// Readjusts path stem_members.
// Assumes the member has already been unlinked.

void removeMember(variable *theVariable, ccInt doomedMemberNumber)
{
    ccInt counter;
    member *doomedMember;
    
    doomedMember = LL_member(theVariable, doomedMemberNumber);
    
    
        // delete the member & associated codes & references
    
    if (doomedMember->memberWindow != NULL)  derefWindow(&(doomedMember->memberWindow));
    
    for (counter = 1; counter <= doomedMember->codeList.elementNum; counter++)  {
        derefCodeRef((code_ref *) element(&(doomedMember->codeList), counter));   }
    deleteLinkedList(&(doomedMember->codeList));
    
    deleteElement(&(theVariable->mem.members), doomedMemberNumber);
    
    return;     // we ignored return values from FDeleteElement -- an error is possible, but seems implausible to me.
}



// References a search path; if first ref. then it also references its code, window & stem.

void refPath(searchPath *thePath)
{
    if (thePath == NULL)  return;
    
    RefPLLPtr(thePath->references);
    
    if (*(thePath->references) == 1)  {
        refPath(thePath->stem);
        refPLLElement(&MasterCodeList, thePath->sourceCode);
        refWindow(thePath->jamb);     }
    
    return;
}


// Derefs the search path; if refs == 0 it deletes the search path & dereferences the associated code, window & stem.

void derefPath(searchPath **thePath)
{
    if (*thePath == NULL)  return;
    
    derefPLLPtr(&((*thePath)->variablePtr->pathList), (*thePath)->PLL_index, (*thePath)->references);
    if (*((*thePath)->references) != 0)  {  *thePath = NULL;  return;  }
    
            // if we're being deleted, we aren't the anchor for any other paths
    
    derefPath(&((*thePath)->stem));
    derefPLLElement(&MasterCodeList, (*thePath)->sourceCode);
    derefWindow(&((*thePath)->jamb));
    
    *thePath = NULL;
    
    return;
}


// drawPath() creates a new path leading from the given stem to destWindow.
// It is used when we splice paths with a <<, or when transform() creates a suspensor (stem == NULL).

ccInt drawPath(searchPath **newPath, window *destWindow, searchPath *newPathStem, ccInt indices, ccInt sourceCode)
{
    ccInt newPathIndex, rtrn;
    
    rtrn = addPLLElement(&(destWindow->variable_ptr->pathList), (void **) newPath, &newPathIndex);
    if (rtrn != passed)  return rtrn;
    
    (*newPath)->PLL_index = newPathIndex;
    (*newPath)->references = LL_int(&(destWindow->variable_ptr->pathList.references), newPathIndex);
    (*newPath)->stemIndices = indices;
    (*newPath)->stem = newPathStem;
    (*newPath)->variablePtr = destWindow->variable_ptr;
    (*newPath)->sourceCode = sourceCode;
    (*newPath)->jamb = destWindow;
    
    *((*newPath)->references) = 0;
    return passed;
}


// addCode() adds an entry to the global MasterCodeList, copying the code from codeSource.
// The reason to copy the code is that Cicada will have a personal copy regardless
// of what happens to codeSource (which may be stored in a user variable).

ccInt addCode(ccInt *codeSource, ccInt **newCodePtr, ccInt *newCodeElement, ccInt codeLength, const char *fileName,
        ccInt fileNameLength, const char *sourceCode, ccInt *opCharNum, ccInt sourceCodeLength, const ccInt compilerID)
{
    ccInt rtrn;
    storedCodeType *newCodeEntry;
    
    rtrn = addPLLElement(&MasterCodeList, (void **) &newCodeEntry, newCodeElement);
    if (rtrn != passed)  return out_of_memory_err;
    
    
        // the code copy is stored in a buffer, not a linked list, since it can't be resized
    
    newCodeEntry->fileName = newCodeEntry->sourceCode = NULL;
    newCodeEntry->opCharNum = newCodeEntry->bytecode = NULL;
    newCodeEntry->compilerID = compilerID;
    
    *newCodePtr = (ccInt *) malloc((codeLength+1)*sizeof(ccInt));
    if (*newCodePtr == NULL)  return out_of_memory_err;
    newCodeEntry->bytecode = *newCodePtr;
    memcpy((void *) *newCodePtr, codeSource, codeLength*sizeof(ccInt));
    (*newCodePtr)[codeLength] = 0;
    
    if (fileName != NULL)  {
        newCodeEntry->fileName = (char *) malloc((size_t) fileNameLength+1);
        if (newCodeEntry->fileName == NULL)  return out_of_memory_err;
        memcpy((void *) newCodeEntry->fileName, (void *) fileName, fileNameLength);
        newCodeEntry->fileName[fileNameLength] = 0;     }
    
    if (sourceCode != NULL)  {
        newCodeEntry->sourceCode = (char *) malloc((size_t) sourceCodeLength+1);
        if (newCodeEntry->sourceCode == NULL)  return out_of_memory_err;
        memcpy((void *) newCodeEntry->sourceCode, (void *) sourceCode, sourceCodeLength);
        newCodeEntry->sourceCode[sourceCodeLength] = 0;
        
        newCodeEntry->opCharNum = (ccInt *) malloc((size_t) codeLength*sizeof(ccInt));
        if (newCodeEntry->opCharNum == NULL)  return out_of_memory_err;
        memcpy((void *) newCodeEntry->opCharNum, (void *) opCharNum, codeLength*sizeof(ccInt));       }
    
    return passed;
}


// References an entry in the global code list.

void refCode(ccInt *refsPtr)
{  RefPLLPtr(refsPtr);  return;  }


// De-references an entry in the global code list; frees its memory if refs == 0.

void derefCode(ccInt *refsPtr, ccInt refsID)
{
    derefPLLPtr(&MasterCodeList, refsID, refsPtr);
    if (refsPtr == NULL)  {
        if (storedCode(refsID)->fileName != NULL)  free((void *) storedCode(refsID)->fileName);
        if (storedCode(refsID)->sourceCode != NULL)  free((void *) storedCode(refsID)->sourceCode);
        if (storedCode(refsID)->opCharNum != NULL)  free((void *) storedCode(refsID)->opCharNum);
        free((void *) storedCode(refsID)->bytecode);        }
}


// addCodeRef() adds an entry into the destination code list.  References it once (this assumes that the variable is referenced).

ccInt addCodeRef(linkedlist *destCodeLL, searchPath *newAnchor, ccInt *codePtr, ccInt codeID)
{
    code_ref *newCodeRef;
    ccInt rtrn;
    
    rtrn = addElements(destCodeLL, 1, false);
    if (rtrn != passed)  return out_of_memory_err;
    
    newCodeRef = (code_ref *) element(destCodeLL, destCodeLL->elementNum);
    newCodeRef->PLL_index = codeID;
    newCodeRef->references = (ccInt *) element(&(MasterCodeList.references), codeID);
    newCodeRef->code_ptr = codePtr;
    newCodeRef->anchor = newAnchor;
    
    refCodeRef(newCodeRef);
    
    return passed;
}


// References a code_ref, and its anchor.
// By convention, we always reference/dereference the anchor with each ref/deref of the code_ref,
// probably because each invocation of refCodeRef corresponds to the making of a full new copy of that code ref.

void refCodeRef(code_ref *theCodeRef)
{
    refCode(theCodeRef->references);
    refPath(theCodeRef->anchor);     // only if a variable code_ref
    
    return;
}


// Dereferences a code_ref.

void derefCodeRef(code_ref *theCodeRef)
{
    derefCode(theCodeRef->references, theCodeRef->PLL_index);
    derefPath(&(theCodeRef->anchor));    // will be nonzero if from a variable; zero if from a member code_ref
    
    return;
}


// Makes sure no windows occupy the same set of indices that will be resized.
// This is called by resizeIndices() (in bytecd.c(pp)), four times sequentially, incrementing the mode parameter.
// mode = cbo_set_flags (1) sets the busy_resize_flags; cbo_check (2) returns error upon finding an overlap;
// cbo_unjam (3) unjams any unjammable windows in the way; cbo_unset_flags (4) clears the busy_resize_flags.

ccInt checkMemberOverlap(window *theWindow, ccInt newOffset, ccInt newWidth, ccInt mode)
{
    ccInt counter, rtrn;
    member *loopMember;
    pinned_LL *windowsPLL;
    window *loopWindow;
    
    if (*theWindow->variable_ptr->types >= composite_type)  {
        if (isBusy(theWindow, busy_overlap_flag))  return passed;
        setBusy(theWindow, busy_overlap_flag);
        
        for (counter = 1; counter <= theWindow->variable_ptr->mem.members.elementNum; counter++)  {
            
            loopMember = LL_member(theWindow->variable_ptr, counter);
            if (loopMember->memberWindow != NULL)  {
            if ((loopMember->memberWindow->jamStatus == can_jam) && (!isBusy(loopMember, busy_overlap_flag)))  {
                setBusy(loopMember, busy_overlap_flag);
                rtrn = checkMemberOverlap(loopMember->memberWindow,
                            newOffset*loopMember->indices, newWidth*loopMember->indices, mode);
                if (rtrn != passed)  return rtrn;
        }   }}
        
        if ((mode == cbo_set_flags) || (mode == cbo_unset_flags))  return passed;
    }
    
    else  {
        if (mode == cbo_set_flags)  {
            setBusy(theWindow, busy_resize_flag);
            return passed;       }
        else if (mode == cbo_unset_flags)  {
            clearBusy(theWindow, busy_resize_flag);
            return passed;
    }   }
    
    windowsPLL = &(theWindow->variable_ptr->windows);
    
    for (counter = 1; counter <= windowsPLL->data.elementNum; counter++)  {
    if ((counter != theWindow->PLL_index) && (*LL_int(&(windowsPLL->references), counter) != 0))  {
        loopWindow = (window *) element(&(windowsPLL->data), counter);
        
        if ((loopWindow->offset < newOffset + newWidth) &&
            (loopWindow->offset + loopWindow->width > newOffset) && (~isBusy(loopWindow, busy_resize_flag)))  {
            
            if ((mode == cbo_check) && (loopWindow->jamStatus == can_jam))  {
                return overlapping_window_err;         }
            else if ((mode == cbo_unjam) && (loopWindow->jamStatus == cannot_jam))  {
                loopWindow->jamStatus = unjammed;
                clearBusy(loopWindow, busy_resize_flag);        }
    }}  }
    
    return passed;
}


// addMemory(), sort of a misnomer, either adds or deletes memory as a result of a resize, or the creation of a new window.
// Adjusts the offsets of the windows and indexes of PC search path accordingly.
// Calls itself recursively for the members of the window's variable if it is composite.
// Outermost invocation:  use offset for insertionOffset, and # of elements of the member pointing to currentWindow to add for newIndices.

ccInt addMemory(window *theWindow, ccInt insertionOffset, ccInt newIndices)
{
    member *loopMember;
    window *loopWindow;
    view *loopView;
    variable *theVariable = theWindow->variable_ptr;
    ccInt counter, rtrn, fullInsertionOffset = insertionOffset + theWindow->offset;
    
    if (newIndices == 0)  return passed;
    
    theVariable->instances += newIndices;
    
    
        // adjust the offset/index_width of any window that could conceivably be affected
    
    for (counter = 1; counter <= theVariable->windows.data.elementNum; counter++)  {
        loopWindow = (window *) element(&(theVariable->windows.data), counter);    // 1st cond below:  if theWindow isn't yet ref'd
        if ((loopWindow == theWindow) || (*LL_int(&(theVariable->windows.references), counter) != 0))  {
            adjustOffsetAndIW(loopWindow == theWindow,
                    &(loopWindow->offset), &(loopWindow->width), fullInsertionOffset, newIndices);
    }   }
    
    if (baseView.windowPtr != NULL)  {
    if (baseView.windowPtr->variable_ptr == theVariable)  {
        adjustOffsetAndIW(baseView.windowPtr->variable_ptr == theWindow->variable_ptr,
                    &(baseView.offset), &(baseView.width), fullInsertionOffset, newIndices);
    }}
    if (searchView.windowPtr != NULL)  {
    if (searchView.windowPtr->variable_ptr == theVariable)  {
        adjustOffsetAndIW(searchView.windowPtr->variable_ptr == theWindow->variable_ptr,
                    &(searchView.offset), &(searchView.width), fullInsertionOffset, newIndices);
    }}
    
    for (counter = 1; counter <= PCStack.top; counter++)  {
        loopView = (view *) element(&(PCStack.data), counter);
        if (loopView->windowPtr != NULL)  {
        if (loopView->windowPtr->variable_ptr == theVariable)     {
            adjustOffsetAndIW(loopView->windowPtr->variable_ptr == theWindow->variable_ptr,
                    &(loopView->offset), &(loopView->width), fullInsertionOffset, newIndices);
    }   }}
    
    
        // recursively call addMemory() for each member, if it is a composite or array variable
    
    if ((*theVariable->types >= composite_type) && (*theVariable->types != list_type))  {
        if (isBusy(theVariable, busy_add_flag))  return passed;
        setBusy(theVariable, busy_add_flag);
        
        for (counter = 1; counter <= theVariable->mem.members.elementNum; counter++)  {
            loopMember = LL_member(theVariable, counter);
            if ((loopMember->memberWindow != NULL) && (loopMember->memberWindow->jamStatus != unjammed))  {
                if (((ccFloat) newIndices)*loopMember->indices > (ccFloat) ccIntMax)  return index_argument_err;
                rtrn = addMemory(loopMember->memberWindow,
                        insertionOffset*(loopMember->indices)+loopMember->memberWindow->offset, newIndices*loopMember->indices);
                if (rtrn != passed)  return rtrn;
        }   }
        
        if (GL_Path.stemView.windowPtr != NULL)  {
        if (GL_Path.stemView.windowPtr->variable_ptr == theVariable)  {
            adjustOffsetAndIW(GL_Path.stemView.windowPtr == theWindow,
                    &(GL_Path.stemView.offset), &(GL_Path.stemView.width), fullInsertionOffset, newIndices);
            GL_Path.stemMember = NULL;       // we can't tell if it was added to or pushed up
    }   }}
    
    
            // if a primitive or list variable, add the memory directly to the linked lists
    
    else  {
        if (newIndices > 0)  {
            
            if ((*theVariable->types != string_type) && (*theVariable->types != list_type))  {
                rtrn = insertElements(&(theVariable->mem.data), fullInsertionOffset+1, newIndices, true);
                if (rtrn != passed)  return rtrn;       }
            
            else  {         // allocate one big sublist in case the members LL isn't set up with spare room
                rtrn = addMembers(theVariable, fullInsertionOffset+1, 0, NULL, false, newIndices, false);
                if (rtrn != passed)  return rtrn;
                
                for (counter = fullInsertionOffset+1; counter <= fullInsertionOffset+newIndices; counter++)  {
                    
                    variable *oneStringVar;
                    window *oneStringWindow = NULL;
                    member *oneStringMember = NULL;
                    const static ccInt charType = char_type;
                    
                    if (*theVariable->types == string_type)  rtrn = addVariable(&oneStringVar, &charType, 0, false);
                    else  rtrn = addVariable(&oneStringVar, theVariable->types + 1, theVariable->arrayDepth-1, false);
                    if (rtrn == passed)  rtrn = addWindow(oneStringVar, 0, 0, &oneStringWindow, true);
                    if (rtrn == passed)  rtrn = addMembers(theVariable, counter, 0, &oneStringMember, false, 0, true);
                    if (rtrn != passed)  return rtrn;
                    
                    oneStringMember->memberWindow = oneStringWindow;
                    refWindow(oneStringWindow);
        }   }   }
        else  {
            if ((*theVariable->types == string_type) || (*theVariable->types == list_type))  {
            for (counter = fullInsertionOffset+(-newIndices); counter >= fullInsertionOffset+1; counter--)  {
                removeMember(theVariable, counter);
            }}
            else  deleteElements(&(theVariable->mem.data), fullInsertionOffset+1, fullInsertionOffset+(-newIndices));
    }   }
    
    return passed;
}


// AO&IW() updates the offset and/or index width of a window after addition/removal of elements of memory in that window's variable.
// The offset is affected if the new/deleted memory is below the current window; the index width if affected if it is in the window.

void adjustOffsetAndIW(bool sameWindow, ccInt *offset, ccInt *indexWidth, ccInt insertionOffset, ccInt newIndices)
{
    if ((sameWindow) && (insertionOffset >= *offset))  { 
        *indexWidth += newIndices;
        if (*indexWidth < insertionOffset - (*offset))  {       // can happen if newIndices < 0 
            *indexWidth = insertionOffset - (*offset);
    }   }
    
    else if (insertionOffset <= *offset) {
        *offset += newIndices;
        if (insertionOffset > *offset)  {                       // possible only if newIndices < 0
            *indexWidth -= insertionOffset - (*offset);
            if (*indexWidth < 0)  *indexWidth = 0;
            *offset = insertionOffset;
    }   }
}


// unflagVariables() removes the busy flag on theVariable, its members' variables, etc.
// This needs to run after each outer addMemory() call, in order for AM() to work again.

void unflagVariables(variable *theVariable, unsigned char theFlag)
{
    member *loopMember;
    ccInt counter;
    
    if (!isBusy(theVariable, theFlag))  return;
    clearBusy(theVariable, theFlag);
    
    if (*theVariable->types >= composite_type)  {
        for (counter = 1; counter <= theVariable->mem.members.elementNum; counter++)  {
            loopMember = LL_member(theVariable, counter);
            if (loopMember->memberWindow != NULL)  unflagVariables(loopMember->memberWindow->variable_ptr, theFlag);  }}
    
    return;
}


// unflagWindow() removes the busy flag on currentWindow, its variable's members' windows, etc.
// This needs to run after each outer checkMemberOverlap() call, in order for CBO() to work again.

void unflagWindow(window *theWindow, unsigned char theFlag)
{
    variable *theVariable = theWindow->variable_ptr;
    ccInt counter;
    
    if (!isBusy(theWindow, theFlag))  return;
    clearBusy(theWindow, theFlag);
    
    if (*theVariable->types >= composite_type)  {
    for (counter = 1; counter <= theVariable->mem.members.elementNum; counter++)  {
        
        member *loopMember = LL_member(theVariable, counter);
        if (loopMember->memberWindow != NULL)  {
        if ((loopMember->memberWindow->jamStatus == can_jam) && (isBusy(loopMember, busy_overlap_flag)))  {
            clearBusy(loopMember, busy_overlap_flag);
            unflagWindow(loopMember->memberWindow, theFlag);
    }}  }}
    
    return;
}


// align() returns the smallest integer that is both >= the_int and is a multiple of size(ccInt).
// Used to ensure memory alignment in various parts of Cicada.

ccInt align(ccInt the_int)
{
    if (the_int % sizeof(ccInt) != 0)
        return sizeof(ccInt) - (the_int % sizeof(ccInt));
    
    return 0;
}


const ccInt typeSizes[] = { sizeof(bool), sizeof(char), sizeof(ccInt), sizeof(ccFloat),
            sizeof(member), sizeof(member), sizeof(member), sizeof(member)  };
