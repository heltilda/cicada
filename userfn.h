/*
 *  userfn.h
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

#ifndef UserFunctions_h
#define UserFunctions_h

#include "lnklst.h"
#include <stdlib.h>



// The required type of user-defined C or C++ functions

typedef struct {
    const char *functionName;
    ccInt(*functionPtr)(ccInt, char **);
} userFunction;


// arg_info:  a data type for the final argv parameter to each user function call.
// This gives a report on the sizes, etc. of each argument.

typedef struct {
    ccInt argType;
    ccInt argIndices;
} arg_info;



// Prototypes & misc definitions

#define byValue(a) NULL,a
#define fromArg(n) NULL,NULL,n
#define endArgs NULL,NULL,-1


#ifdef __cplusplus
extern "C" {
#endif

extern userFunction UserFunctions[];
extern const ccInt userFunctionsNum;

extern ccInt pass2nums(ccInt, char **);

extern ccInt callMinMax(ccInt, char **);
extern ccInt callSum(ccInt, char **);
extern ccInt callMakeLinkList(ccInt, char **);
extern ccInt makeLinkList(const ccInt, const ccInt, const ccInt, const ccFloat *, ccInt *);
extern ccInt callSort(ccInt, char **);

extern void getArgs(ccInt, char **, ...);

#ifdef __cplusplus
}
#endif

#endif

