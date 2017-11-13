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

extern void(*BuiltInFunctionJumpTable[])();
extern ccInt bi_commands_num;

extern void cclib_call();
extern void cclib_setCompiler();
extern char *getCompilerString(window *, ccInt, ccInt);
extern void cclib_compile();
extern void cclib_transform();
extern void cclib_load();
extern ccInt loadFile(char *, linkedlist *, ccBool);
extern void cclib_save();
extern void cclib_input();
extern void cclib_print();
extern void cclib_read_string();
extern void cclib_print_string();
extern void cclib_trap();
extern void cclib_throw();
extern void cclib_top();
extern void cclib_size();

extern void cclib_abs();
extern ccFloat doAbs(ccFloat);
extern void cclib_floor();
extern ccFloat doFloor(ccFloat);
extern void cclib_ceil();
extern ccFloat doCeil(ccFloat);
extern void cclib_log();
extern ccFloat doLog(ccFloat);
extern void cclib_cos();
extern ccFloat doCos(ccFloat);
extern void cclib_sin();
extern ccFloat doSin(ccFloat);
extern void cclib_tan();
extern ccFloat doTan(ccFloat);
extern void cclib_acos();
extern ccFloat doArccos(ccFloat);
extern void cclib_asin();
extern ccFloat doArcsin(ccFloat);
extern void cclib_atan();
extern ccFloat doArctan(ccFloat);
extern void doMath(ccFloat(*)(ccFloat));

extern void cclib_random();
extern void cclib_find();

extern void cclib_type();
extern void cclib_member_ID();
extern void cclib_bytecode();

extern void springCleaning();

extern ccInt numBIF_args();
extern window *getBIFmember(ccInt);
extern ccFloat getBIFnumArg(ccInt, ccFloat, ccFloat);
extern ccBool getBIFboolArg(ccInt);
extern void writeBIFintArg(ccInt, ccInt);
extern window *getBIFstringArg(ccInt);


#endif
