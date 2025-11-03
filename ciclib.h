/*
 *  ciclib.h
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
 

#ifndef CicadaLibrary_h
#define CicadaLibrary_h

#include "cclang.h"
#include "lnklst.h"
#include "intrpt.h"



#ifdef __cplusplus
extern "C" {
#endif


// Function prototypes

extern const Cfunction inbuiltCfunctions[];
extern const ccInt inbuiltCfunctionsNum;
extern const char **inbuiltCfunctionArgs;

extern const Cfunction *userCfunctions; 
extern ccInt userCfunctionsNum;
extern const char **userCfunctionArgs;


extern ccInt cc_newCompiler(argsType);
extern ccInt cc_compile(argsType);
extern ccInt cc_getMemberNames(argsType);
extern ccInt cc_transform(argsType);
extern ccInt cc_trap(argsType);
extern ccInt cc_throw(argsType);

extern ccInt cc_top(argsType);
extern ccInt cc_size(argsType);
extern ccInt cc_type(argsType);
extern ccInt cc_member_ID(argsType);
extern ccInt cc_bytecode(argsType);

extern ccInt cc_minmax(argsType);
extern ccInt cc_sum(argsType);
extern ccInt cc_makeLinkList(argsType);
extern ccInt makeLinkList(const ccInt, const ccInt, const ccInt, const ccFloat *, ccInt *);
extern ccInt cc_sort(argsType);

extern ccInt cc_load(argsType);
extern ccInt loadFile(const char *, linkedlist *, bool);
extern ccInt cc_save(argsType);
extern char *LL2Cstr(linkedlist *);
extern ccInt cc_input(argsType);
extern ccInt cc_print(argsType);
extern void printChar(const unsigned char *);

extern ccInt cc_read_string(argsType);
extern bool isWordChar(const char *);
extern ccInt cc_print_string(argsType);
extern ccInt copyStr(const char *, char *);

extern ccInt cc_find(argsType);

extern ccInt cc_random(argsType);

extern ccInt cc_abs(argsType);
extern ccFloat doAbs(ccFloat);
extern ccInt cc_floor(argsType);
extern ccFloat doFloor(ccFloat);
extern ccInt cc_ceil(argsType);
extern ccFloat doCeil(ccFloat);
extern ccInt cc_exp(argsType);
extern ccFloat doExp(ccFloat);
extern ccInt cc_log(argsType);
extern ccFloat doLog(ccFloat);
extern ccInt cc_cos(argsType);
extern ccFloat doCos(ccFloat);
extern ccInt cc_sin(argsType);
extern ccFloat doSin(ccFloat);
extern ccInt cc_tan(argsType);
extern ccFloat doTan(ccFloat);
extern ccInt cc_acos(argsType);
extern ccFloat doArccos(ccFloat);
extern ccInt cc_asin(argsType);
extern ccFloat doArcsin(ccFloat);
extern ccInt cc_atan(argsType);
extern ccFloat doArctan(ccFloat);
extern ccInt mathUnaryOp(argsType, ccFloat(MathFunction)(ccFloat));

extern ccInt cc_add(argsType);
extern ccFloat doAdd(ccFloat, ccFloat);
extern ccInt cc_subtract(argsType);
extern ccFloat doSubtract(ccFloat, ccFloat);
extern ccInt cc_multiply(argsType);
extern ccFloat doMultiply(ccFloat, ccFloat);
extern ccInt cc_divide(argsType);
extern ccFloat doDivide(ccFloat, ccFloat);
extern ccInt cc_pow(argsType);
extern ccFloat doPow(ccFloat, ccFloat);
extern ccInt mathBinaryOp(argsType, ccFloat(f)(ccFloat, ccFloat));

extern ccInt cc_springCleaning(argsType);

#ifdef __cplusplus
}
#endif

#endif
