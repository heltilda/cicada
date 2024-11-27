/*
 *  userfn.c(pp) - for the user's own C/C++ functions
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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "intrpt.h"
#include "ccmain.h"
#include "userfn.h"


// **********************************************
// #include any user-defined header files here







// **********************************************
// UserFunctions defines the { names, function addresses } of user's C routines.
// Each routine must be of the form:  ccInt RoutineName(ccInt argc, char **argv)


userFunction UserFunctions[] = { { "cicada", &runCicada }, { "pass2nums", &pass2nums }, 
            { "minmax", &callMinMax }, { "sum", &callSum }, { "makeLinkList", &callMakeLinkList }, { "sort", &callSort },
};


const ccInt userFunctionsNum = (ccInt) (sizeof(UserFunctions)/sizeof(userFunction));      // for Cicada's own records





// **********************************************
// C routines may be inserted here, or included in separate source files.






// Example of a user-written C routine.
// Try it out with:  call("pass2nums", 5, pi)

ccInt pass2nums(ccInt argc, char **argv)
{
    ccInt *a;
    ccFloat b;
    
    getArgs(argc, argv, &a, byValue(&b));
    printf("passed %i by reference and %g by value\n", *(int *) a, (double) b);
    
    return passed;
}






// **********************************************
// C routines used by user.cicada


ccInt callMinMax(ccInt argc, char **argv)
{
    ccInt c1, listTop, *bestIdx, mult;
    ccFloat *theList, *bestValue;
    arg_info *argInfo = (arg_info *) argv[argc];
    
    getArgs(argc, argv, &theList, byValue(&mult), &bestIdx, &bestValue);
    listTop = argInfo[0].argIndices;
    
    *bestIdx = 1;
    *bestValue = mult * theList[0];
    
    for (c1 = 0; c1 < argInfo[0].argIndices; c1++)  {
    if (mult*theList[c1] > *bestValue)  {
        *bestIdx = c1 + 1;
        *bestValue = mult * theList[c1];
    }}
    
    return passed;
}



ccInt callSum(ccInt argc, char **argv)
{
    ccInt c1;
    ccFloat *theList, *theSum;
    arg_info *argInfo = (arg_info *) argv[argc];
    
    getArgs(argc, argv, &theList, &theSum);
    
    *theSum = 0.;
    for (c1 = 0; c1 < argInfo[0].argIndices; c1++)  *theSum += theList[c1];
    
    return passed;
}



ccInt callMakeLinkList(ccInt argc, char **argv)
{
    ccInt *linkList, firstIndex, direction;
    ccFloat *sortingList;
    arg_info *argInfo = (arg_info *) argv[argc];
    
    getArgs(argc, argv, &sortingList, &linkList, byValue(&direction));
    
    firstIndex = makeLinkList(0, argInfo[0].argIndices-1, direction, sortingList, linkList);
    
    return firstIndex;
}


ccInt makeLinkList(const ccInt left, const ccInt right, const ccInt direction, const ccFloat *sortingList, ccInt *linklist)
{
    ccInt middle, smallest, toWrite, side, nextSide, idx[2];
    
    if (right > left)  {
        
        middle = floor((right + left)/2);
        idx[0] = makeLinkList(left, middle, direction, sortingList, linklist);
        idx[1] = makeLinkList(middle + 1, right, direction, sortingList, linklist);
        
        if ((sortingList[idx[0]] - sortingList[idx[1]])*direction <= 0)  side = 0;
        else  side = 1;
        
        smallest = toWrite = idx[side];
        idx[side] = linklist[idx[side]];
        
        while ((idx[0] != -1) && (idx[1] != -1))  {
            
            if ((sortingList[idx[0]] - sortingList[idx[1]])*direction <= 0)  nextSide = 0;
            else  nextSide = 1;
            
            if (side != nextSide)  {
                linklist[toWrite] = idx[nextSide];
                side = nextSide;       }
            
            toWrite = idx[side];
            idx[side] = linklist[idx[side]];    }
        
        linklist[toWrite] = idx[1-side];
        
        return smallest;    }
        
    else  return left;
}


ccInt callSort(ccInt argc, char **argv)
{
    ccInt numIndices, cl, ci, idx, *linkList, numLists, firstIndex, rtrn = passed;
    arg_info *argInfo = (arg_info *) argv[argc];
    
    getArgs(argc, argv, &linkList, byValue(&firstIndex), endArgs);
    numIndices = argInfo[0].argIndices;
    numLists = (argc-2)/2;
    
    for (cl = 0; cl < numLists; cl++)  {
        if (argInfo[cl+2].argType != argInfo[cl+2+numLists].argType)  rtrn = 1;
        else if (argInfo[cl+2].argIndices != numIndices)  rtrn = 2;
        else if ((argInfo[cl+2].argType != int_type) && (argInfo[cl+2].argType != double_type))  rtrn = 3;
        if (rtrn != passed)  break;         }
    
    if (rtrn != passed)  {
    for (ci = 0; ci < numIndices; ci++)  {
        linkList[ci]++;        // convert to Cicada indexing
    }}
    
    else  {
    for (cl = 0; cl < numLists; cl++)  {
        idx = firstIndex;
        if (argInfo[cl+2].argType == int_type)  {
            ccInt *list = (ccInt *) (argv[cl+2]), *sortedList = (ccInt *) (argv[cl+2+numLists]);
            for (ci = 0; ci < numIndices; ci++)  {
                sortedList[ci] = list[idx];
                idx = linkList[idx];
        }   }
        else if (argInfo[cl+2].argType == double_type)  {
            ccFloat *list = (ccFloat *) (argv[cl+2]), *sortedList = (ccFloat *) (argv[cl+2+numLists]);
            for (ci = 0; ci < numIndices; ci++)  {
                sortedList[ci] = list[idx];
                idx = linkList[idx];
    }}  }   }
    
    return rtrn;
}






// Misc -- useful for loading arguments into the user's C routines.
// 
// example for $myFunction(intArg, boolArg1, boolArg2):
//
// ccInt myFunction(ccInt argc, char **argv)
// {
//     ccInt *var1;
//     ccBool var2, *var3;
//     
//     getArgs(argc, argv, &var1, byValue(&var2), &var3)        // load all 3 arguments
//     
//     getArgs(argc, argv, &var1, endArgs)                      // load only the first argument
//     
//     getArgs(argc, argv, fromArg(2), &var3)                   // load only the last argument
//
//     return passed;
// }

void getArgs(ccInt argc, char **argv, ...)
{
    ccInt loopArg;
    va_list theArgs;
    char **nextarg;
    arg_info *myArgInfo = (arg_info *) argv[argc];
    
    va_start(theArgs, argv);
    for (loopArg = 0; loopArg < argc; loopArg++)  {
        nextarg = va_arg(theArgs, char **);
        if (nextarg != NULL)
            *nextarg = argv[loopArg];
        else  {
            nextarg = va_arg(theArgs, char **);
            if (nextarg == NULL)  {
                loopArg = ((ccInt) va_arg(theArgs, int)) - 1;
                if (loopArg < -1)  break;       }
            else  {
                size_t numBytes = (size_t) myArgInfo[loopArg].argIndices*typeSizes[myArgInfo[loopArg].argType];
                if (numBytes > 0)  memcpy((void *) nextarg, (void *) argv[loopArg], numBytes);
    }   }   }
    va_end(theArgs);
    
    return;
}
