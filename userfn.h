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



// Comment out the following line if Cicada is to be embedded in a larger C/C++ application
// (i.e. if Cicada's own main() function is not needed).

#define CicadaMainProgram



// The required type of user-defined C or C++ functions

typedef struct {
    const char *functionName;
    int(*functionPtr)(int, char **);
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


extern userFunction UserFunctions[];
extern const ccInt userFunctionsNum;

extern int pass2nums(int, char **);

extern void getArgs(int, char **, ...);

#endif

