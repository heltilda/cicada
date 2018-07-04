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
 

#ifndef Externals_h
#define Externals_h

#include "cicada.h"
#include "lnklst.h"
#include "intrpt.h"



// Prototypes

#ifdef __cplusplus
extern "C" {
#endif

extern void(*BuiltInFunctionJumpTable[])(void);
extern ccInt bi_commands_num;

extern void cclib_call(void);
extern void cclib_setCompiler(void);
extern char *getCompilerString(window *, ccInt, ccInt);
extern void cclib_compile(void);
extern void cclib_transform(void);
extern void cclib_load(void);
extern ccInt loadFile(char *, linkedlist *, ccBool);
extern void cclib_save(void);
extern void cclib_input(void);
extern void cclib_print(void);
extern void cclib_read_string(void);
extern void cclib_print_string(void);
extern void cclib_trap(void);
extern void cclib_throw(void);
extern void cclib_top(void);
extern void cclib_size(void);

extern void cclib_abs(void);
extern ccFloat doAbs(ccFloat);
extern void cclib_floor(void);
extern ccFloat doFloor(ccFloat);
extern void cclib_ceil(void);
extern ccFloat doCeil(ccFloat);
extern void cclib_log(void);
extern ccFloat doLog(ccFloat);
extern void cclib_cos(void);
extern ccFloat doCos(ccFloat);
extern void cclib_sin(void);
extern ccFloat doSin(ccFloat);
extern void cclib_tan(void);
extern ccFloat doTan(ccFloat);
extern void cclib_acos(void);
extern ccFloat doArccos(ccFloat);
extern void cclib_asin(void);
extern ccFloat doArcsin(ccFloat);
extern void cclib_atan(void);
extern ccFloat doArctan(ccFloat);
extern void doMath(ccFloat(*)(ccFloat));

extern void cclib_random(void);
extern void cclib_find(void);

extern void cclib_type(void);
extern void cclib_member_ID(void);
extern void cclib_bytecode(void);

extern void springCleaning(void);

extern ccInt numBIF_args(void);
extern window *getBIFmember(ccInt);
extern ccFloat getBIFnumArg(ccInt, ccFloat, ccFloat);
extern ccBool getBIFboolArg(ccInt);
extern void writeBIFintArg(ccInt, ccInt);
extern window *getBIFstringArg(ccInt);

#ifdef __cplusplus
}
#endif

#endif
