/*
 *  cmpile.h
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

#ifndef Compile_h
#define Compile_h

#include "lnklst.h"


typedef struct {
    linkedlist tokens[128];     // token strings; one LL for each possible starting ASCII character
    ccInt numLanguageOps[128];  // number of each token string list that are language operators (i.e. may not end in a space)
    linkedlist tokenSpecs;      // two possible tokens (with/without a single left-hand argument) for each token string
    linkedlist allCodeWords;    // list of all code words corresponding to all operators
    ccInt *OoOdirections;       // left-to-right or right-to-left, for each OoO level
    ccInt specialTokens[3];     // token IDs corresponding to integer and floating-point constants, and variable names
    ccInt adapters[11][11];     // operators inserted into expressions that fix a type-mismatch error
    
    ccInt numVariables;         // number of user-defined variables
    ccInt anonymousMemberNum;   // number of the last hidden (negative-ID) variable
    linkedlist scriptTokens;    // the symbols comprising the script (negative ID = user variable)
    linkedlist bytecode;        // the final bytecode output
    linkedlist opCharNum;       // the character number from the text script of each operator in the final bytecode
    linkedlist varNames;        // a list of the token IDs that are user-defined variable names
} compiler_type;


typedef struct {
    char *name;                 // pointer to the text representation of an operator
    ccInt tokenID_LHarg;        // the ID number of the operator in compiler->tokenSpecs[] if there is a left-hand argument
    ccInt tokenID_noLHarg;      // the ID number of the operator in compiler->tokenSpecs[] if there is no left-hand argument
    ccInt varID;                // if tokenID_noLHarg points to the variable token type (irrespective of tokenID_LHarg)
} tokenStringType;


typedef struct {
    char *name;                 // a duplicate of the 'name' field in the corresponding opString
    ccBool isOptional;          // if this and subsequent arguments can be skipped (as in the 'else' of an 'if')
    ccBool tobeRemoved;         // set by 'removedexpression' -- the tokens are not written to the scriptTokens list
    ccInt precedence;           // order-of-operations precedence level (low = tightly-bound)
    ccInt leftArgType;          // expected type of left-hand argument; 0 if no LH argument
    ccInt rightArgType;         // expected type of right-hand argument; 0 if no RH argument
    ccBool rtrnTypes[11];       // the types (numeric, string, etc.) that return value can be; type 0 = sentence-starter
    ccInt prevToken;            // the previous token, if it is part of a string (i.e. 'for' before 'in', 'if' before 'endif', etc)
    ccInt nextToken;            // the ending token, if one is expected (i.e. 'in' after 'for', 'endif' after 'if', etc)
    ccInt firstCodeWord;        // the first bytecode word in allCodeWords[] associated with the operator
    ccInt numCodeWords;         // the number of bytecode words in allCodeWords[] from this operator
} tokenSpecType;


typedef struct {
    ccInt tokenID;              // the number of the entry in tokenSpecs
    ccInt tokenCharNum;         // the character number from the text script of each bytecode operator
    ccInt firstArg;             // a link to the first argument of this operator; 0 if it has no arguments
    ccInt nextExpression;       // the next operator (for example, the LH operator will link here to the RH operator)
    ccInt enclosedArgs;         // this is the first linked operator of the right-hand arg between this token and nextToken
    ccInt nextScriptToken;      // the 'next token' ID in scriptTokens; 0 if no next token for this op or a nonexistent optional arg
    ccInt *constData;           // for numeric or string constants, this points to the buffer storing the constant
    ccInt constDataSize;        // size in ints of numeric or string constants
    ccBool *rtrnTypes;          // for typeXarg operators such as (..), this points to the type array of what is inside
    ccInt adapterTokenToFix;    // this, or an enclosed, token # if it contains a typeX adapter that needs fixing
} scriptTokenType;


typedef struct {
    const char *cmdString;      // the written character sequence of the operator, including arguments
    ccInt precedence;           // the order-of-operations precedence level of the operator
    const char *rtrnTypeString; // contains the allowed return types of the operator
    const char *translation;    // the bytecode/script translation of the operator
} commandTokenType;


typedef struct {
    char *theName;              // a pointer to the variable name
    ccInt nameLength;           // number of characters in variable name
} varNameType;



// Compile-time error codes; these follow the linked-list error codes


#define char_read_err 5
#define string_read_err 5
#define read_number_err 6
#define overflow_err 7
#define underflow_err 8

#define unknown_command_err 9
#define unexpected_token_err 10
#define token_expected_err 11

#define arg_expected_err 12
#define left_arg_expected_err 13
#define right_arg_expected_err 14
#define no_left_arg_allowed_err 15
#define no_right_arg_allowed_err 16

#define type_mismatch_err 17





// Order-of-operations for same-OoO operations in series

#define l_to_r 1    /* (a+b)+c  */
#define r_to_l 2    /* a=(b=c)  */


// argument types we can concatenate into strings of command-definition strings

#define type0arg "\x10"
#define type1arg "\x11"
#define type2arg "\x12"
#define type3arg "\x13"
#define type4arg "\x14"
#define type5arg "\x15"
#define type6arg "\x16"
#define type7arg "\x17"
#define type8arg "\x18"
#define type9arg "\x19"
#define typeXarg "\x1A"
#define chararg "\x1B"
#define stringarg "\x1C"
#define commentarg "\x1D"
#define optionalargs "\x1E"

#define type0arg_adapter "\xf0"
#define type1arg_adapter "\xf1"
#define type2arg_adapter "\xf2"
#define type3arg_adapter "\xf3"
#define type4arg_adapter "\xf4"
#define type5arg_adapter "\xf5"
#define type6arg_adapter "\xf6"
#define type7arg_adapter "\xf7"
#define type8arg_adapter "\xf8"
#define type9arg_adapter "\xf9"
#define noarg_adapter "\xfa"

#define int_constant "\xfb"
#define double_constant "\xfc"
#define variable_name "\xfd"

#define argXtype "\xfe"


// predefined commands in the bytecode string

#ifdef CicadaCompileFile

#define arg1 "\x11"
#define arg2 "\x12"
#define arg3 "\x13"
#define arg4 "\x14"
#define arg5 "\x15"
#define arg6 "\x16"
#define arg7 "\x17"
#define arg8 "\x18"
#define arg9 "\x19"

#endif

#define anonymousmember " \x1a "
#define removedexpression "\x1b"
#define inbytecode "\x1c"



// Numbers corresponding to the define flags; flags word = appropriate sum of these numbers

#define dg_run_constructor (1<<5)
#define dg_relink_target (1<<4)
#define dg_new_target (1<<3)
#define dg_add_members (1<<2)
#define dg_update_members (1<<1)
#define dg_post_equate 1


// The flags words for the various define operators

#define def_flags (dg_add_members+dg_update_members+dg_new_target+dg_run_constructor)
#define vdf_flags (dg_add_members+dg_new_target+dg_run_constructor)
#define mdf_flags (dg_update_members+dg_add_members)
#define dqa_flags (dg_add_members+dg_update_members+dg_relink_target)
#define eqa_flags (dg_relink_target)
#define equ_flags (dg_post_equate)
#define deq_flags (dg_add_members+dg_update_members+dg_new_target+dg_post_equate+dg_run_constructor)



// use this instead of the lettertypeArray[] directly (because of char signage issues)

#define lettertype(a) lettertypeArray[*(unsigned char *) (a)]

#define unprintable 0
#define a_space 1
#define a_eol 2
#define a_letter 3
#define a_number 4
#define a_symbol 5
#define a_null 6




// Global struct definition

typedef struct {
    compiler_type *_currentCompiler;
    ccInt _errPosition;
    ccInt _compilerWarning;
    char *_expectedTokenName;
} cc_compile_global_struct;

extern const int maxPrintableDigits;
extern const int maxFieldWidth;
extern const char *printFloatFormatString;
extern const char *print_stringFloatFormatString;
extern const char *readFloatFormatString;
extern const char *printIntFormatString;
extern const char *readIntFormatString;

extern cc_compile_global_struct cc_compile_globals;

#define currentCompiler cc_compile_globals._currentCompiler
#define errPosition cc_compile_globals._errPosition
#define compilerWarning cc_compile_globals._compilerWarning
#define expectedTokenName cc_compile_globals._expectedTokenName



// External prototypes

#ifdef __cplusplus
extern "C" {
#endif

extern compiler_type *newCompiler(commandTokenType *, ccInt, ccInt *, ccInt, ccInt *);
extern ccInt addTokenSpec(compiler_type *, char **, char **, ccInt, ccBool, ccBool, ccBool, ccInt, ccInt);
extern ccInt findToken(compiler_type *, linkedlist *, char **, ccInt *, ccBool, ccBool);
extern void freeCompiler(compiler_type *);
extern void freeBytecode(compiler_type *);
extern ccInt compile(compiler_type *, char *);
extern ccInt tokenize(compiler_type *, char *, ccInt);
extern ccInt addScriptToken(compiler_type *compiler, ccInt, ccInt, ccInt *, ccInt);
extern ccInt readNum(char **, ccFloat *, ccBool *);
extern ccInt readTextString(char **, char *, ccInt **, ccInt *, ccBool);
extern void nextChar(char **);
extern ccInt reorderTokens(compiler_type *, ccInt, ccBool);
extern ccInt relinkExpression(compiler_type *, ccInt *, ccInt, ccInt, ccInt, ccInt **, ccBool **, ccInt *);
extern ccInt relinkBestToken(compiler_type *, linkedlist *, ccInt, ccInt, ccInt, ccInt **, ccBool **, ccInt *);
extern ccInt writeTokenOps(compiler_type *, ccInt, ccBool);

extern ccInt lettertypeArray[256];

#ifdef __cplusplus
}
#endif

#endif
