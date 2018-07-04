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
#include "intrpt.h"
#include "ccmain.h"
#include "userfn.h"


// **********************************************
// #include any user-defined header files here







// **********************************************
// UserFunctions defines the { names, function addresses } of user's C routines.
// Each routine must be of the form:  ccInt RoutineName(ccInt argc, char **argv)


userFunction UserFunctions[] = { { "pass2nums", &pass2nums }, { "cicada", &runCicada } };


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





// Misc -- useful for loading arguments into the user's C routines.
// 
// example:
//
// ccInt myFunction(ccInt argc, char **argv)
// {
//     ccInt *var1;
//     ccBool var2, *var3;
//     
//     getArgs(argc, argv, &var1, byValue(&var2), &var3)
//     
//     ...     // assumes call() passed (int, bool, bool)
//     
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
