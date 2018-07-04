/*
 *  cicada.c(pp) - constant variable/array definitions
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


#define CicadaCompileFile
#include "cicada.h"



// Cicada default language definition


// Precedence levels

#define sentenceLevel 1
#define commandLevel 2
#define defineLevel 3
#define binaryBoolLevel 4
#define notBoolLevel 5
#define compareLevel 6
#define codeSubstitutionLevel 7
#define arrayLevel 8
#define inheritanceLevel 9
#define addsubLevel 10
#define multdivLevel 11
#define negationLevel 12
#define expLevel 13
#define stepVarLevel 14
#define backstepVarLevel 15


ccInt cicadaLanguageAssociativity[] = {
    r_to_l, r_to_l, r_to_l, l_to_r, r_to_l,
        r_to_l, l_to_r, r_to_l, l_to_r, l_to_r,
    l_to_r, r_to_l, l_to_r, l_to_r, l_to_r      };



// Argument types:  0 = script, 1 = command(s), 2 = variable/function, 3 = variable with default *,
// 4 = number, 5 = data, 6 = code/type, 7 = inlined code, 8 = UNUSED, 9 = variable name

commandTokenType cicadaLanguage[] = {
    
    
        // *****************************************************************
        // *****  Commands and operators defined directly in bytecode  *****
        // *****************************************************************
        
        // First, the basics
    
    { type1arg "\n" type1arg, sentenceLevel, "1", inbytecode "a1 a2" },
    { type1arg "," type1arg, sentenceLevel, "1", inbytecode "a1 a2" },
    
    { "(" typeXarg ")", 0, argXtype, inbytecode "a1" },
    
    { type1arg "|" commentarg optionalargs "\n" type1arg, sentenceLevel, "1", inbytecode "a1 a3" },
    { "&" commentarg "\n", 0, "", removedexpression },
    { "|*" commentarg "*|", 0, "", removedexpression },
    
    
        // Flow control and function calling
    
    { type1arg ";" type1arg, sentenceLevel, "1", inbytecode "a1 4 a2" },
    { "code", 0, "1", inbytecode "4" },
    
    { "return" type3arg, commandLevel, "1", inbytecode "5 a1" },
    { type2arg "(" type1arg ")", stepVarLevel, "23456", inbytecode "6 a1 8 236 10" anonymousmember "57 a2 0" },
    
    { "cicadaLibraryFunction # " type4arg " (" type1arg ")", stepVarLevel, "123456", inbytecode "7 a1 8 236 10" anonymousmember "57 a2 0" },
    { "$" stringarg "(" type1arg ")", stepVarLevel, "123456",
                    inbytecode "7 54 0 8 236 10" anonymousmember "57 8 173 10" anonymousmember "56 a1 a2 0" },
    
    
        // Define, equate and forced-equate operators
    
    { type2arg "::" type6arg, defineLevel, "123456", inbytecode "8 46 a1 a2" },
    { type2arg ":= @" type2arg, defineLevel, "123456", inbytecode "8 22 a1 a2" },
    { type2arg "= @" type2arg, defineLevel, "123456", inbytecode "8 16 a1 a2" },
    { type2arg "<- @" type2arg, defineLevel, "123456", inbytecode "8 16 a1 a2" },
    { type2arg "=" type5arg, defineLevel, "123456", inbytecode "8 1 a1 a2" },
    { type2arg "<-" type5arg, defineLevel, "123456", inbytecode "8 1 a1 a2" },
    { type2arg ":=" type5arg, defineLevel, "123456", inbytecode "8 47 a1 a2" },
    { type2arg "@ ::" type6arg, defineLevel, "123456", inbytecode "8 44 a1 a2" },
    { type2arg "* ::" type6arg, defineLevel, "123456", inbytecode "8 6 a1 a2" },
    { type2arg "= !" type5arg, defineLevel, "123456", inbytecode "9 a1 a2" },
    { type2arg "<- !" type5arg, defineLevel, "123456", inbytecode "9 a1 a2" },
    
    
        // Member and array index operators
    
    { type2arg "." type9arg, stepVarLevel, "23456", inbytecode "11 a2 a1" },
    { type2arg "[" type4arg "]", stepVarLevel, "23456", inbytecode "12 a1 a2" },
    { type2arg "[ <" type4arg "," type4arg "> ]", stepVarLevel, "23456", inbytecode "13 a1 a2 a3" },
    { type2arg "[ * ]", stepVarLevel, "23456", inbytecode "14 a1" },
    { type2arg "[ ]", stepVarLevel, "23456", inbytecode "14 a1" },
    { type2arg "[^" type4arg "]", stepVarLevel, "123456", inbytecode "15 a1 a2" },
    { type2arg "[ +" type4arg "]", stepVarLevel, "123456", inbytecode "16 a1 a2" },
    { type2arg "[ + <" type4arg "," type4arg "> ]", stepVarLevel, "123456", inbytecode "17 a1 a2 a3" },
    
    { "remove" type2arg, commandLevel, "1", inbytecode "18 a1" },
    
    
        // Comparison operators
    
    { type5arg "==" type5arg, compareLevel, "5", inbytecode "19 a1 a2" },
    { type5arg "/=" type5arg, compareLevel, "5", inbytecode "20 a1 a2" },
    { type4arg ">" type4arg, compareLevel, "5", inbytecode "21 a1 a2" },
    { type4arg ">=" type4arg, compareLevel, "5", inbytecode "22 a1 a2" },
    { type4arg "<" type4arg, compareLevel, "5", inbytecode "23 a1 a2" },
    { type4arg "<=" type4arg, compareLevel, "5", inbytecode "24 a1 a2" },
    { type2arg "== @" type2arg, compareLevel, "5", inbytecode "25 a1 a2" },
    { type2arg "/= @" type2arg, compareLevel, "5", inbytecode "26 a1 a2" },
    
    
        // Arithmetic operators
    
    { type4arg "+" type4arg, addsubLevel, "456", inbytecode "27 a1 a2" },
    { type4arg "-" type4arg, addsubLevel, "456", inbytecode "28 a1 a2" },
    { type4arg "*" type4arg, multdivLevel, "456", inbytecode "29 a1 a2" },
    { type4arg "/" type4arg, multdivLevel, "456", inbytecode "30 a1 a2" },
    { type4arg "^" type4arg, expLevel, "456", inbytecode "31 a1 a2" },
    { "-" type4arg, negationLevel, "456", inbytecode "29 54 -1 a1" },
    { type4arg "mod" type4arg, multdivLevel, "456", inbytecode "32 a1 a2" },
    
    
        // Logical operators
    
    { "not" type5arg, notBoolLevel, "5", inbytecode "33 a1" },
    { type5arg "and" type5arg, binaryBoolLevel, "5", inbytecode "34 a1 a2" },
    { type5arg "or" type5arg, binaryBoolLevel, "5", inbytecode "35 a1 a2" },
    { type5arg "xor" type5arg, binaryBoolLevel, "5", inbytecode "36 a1 a2" },
    
    
        // Operators on code
    
    { type2arg "#" type4arg, stepVarLevel, "23456", inbytecode "37 a2 a1" },
    { type2arg "<<" type6arg, codeSubstitutionLevel, "23456", inbytecode "38 a2 a1" },
    { type6arg ":" type6arg, inheritanceLevel, "6", inbytecode "39 a1 a2" },
    
    
        // Predefined variables
    
    { "args", 0, "23456", inbytecode "40" },
    { "this", 0, "23456", inbytecode "41" },
    { "that", 0, "23456", inbytecode "42" },
    { "parent", 0, "23456", inbytecode "43" },
    { "\\", 0, "23456", inbytecode "43" },
    { type2arg ".parent", backstepVarLevel, "23456", inbytecode "a1 43" },
    { type2arg ".\\", backstepVarLevel, "23456", inbytecode "a1 43" },
    { "top", 0, "23456", inbytecode "44" },
    { "*", 0, "236", inbytecode "45" },
    { "nothing", 0, "236", inbytecode "45" },
    
    
        // Data types
    
    { "[" type4arg "]" type6arg, arrayLevel, "6", inbytecode "46 a1 a2" },
    { "[ * ]" type6arg, arrayLevel, "6", inbytecode "46 54 0 a1" },
    { "[ ]" type6arg, arrayLevel, "6", inbytecode "46 54 0 a1" },
    
    { "bool", 0, "6", inbytecode "47" },
    { "char", 0, "6", inbytecode "48" },
    { "int", 0, "6", inbytecode "49" },
    { "double", 0, "6", inbytecode "50" },
    { "string", 0, "6", inbytecode "51" },
    
    
        // Hard-coded constants
    
    { "false", 0, "56", inbytecode "52 0" },
    { "true", 0, "56", inbytecode "52 1" },
    
    { "'" chararg "'", 0, "56", inbytecode "53 a1" },
    { int_constant, 0, "456", inbytecode "54 a1" },
    { double_constant, 0, "456", inbytecode "55 a1" },
    { "\"" stringarg "\"", 0, "56", inbytecode "56 a1" },
    { "{" type1arg "}", 0, "67", inbytecode "57 a1 0" },
    
    { variable_name, 0, "9", inbytecode "a1" },
    
    
        // High-level flow control commands, each several bytecode 'sentences' long
    
    { "for" type2arg "in <" type4arg "," type4arg ">" type1arg, commandLevel, "1",
                    inbytecode "8 1 a1 a2 p1 2 j2 21 a1 a3 a4 8 1 a1 27 42 54 1 1 j1 p2" },
    { "if" type5arg "then" type1arg optionalargs "else" type1arg, commandLevel, "1", inbytecode "3 j1 a1 a2 1 j2 p1 a3 p2" },
    { "while" type5arg "do" type1arg, commandLevel, "1", inbytecode "p2 3 j1 a1 a2 1 j2 p1" },
    { "loop" type1arg "until" type5arg, commandLevel, "1", inbytecode "p1 a1 3 j1 a2" },
    
    
        // The 'adapters':  wee bits of unscripted bytecode that make the Cicada syntax work
    
    { noarg_adapter, 0, "0", inbytecode "0" },
    { noarg_adapter, 0, "1", inbytecode "" },
    { noarg_adapter, 0, "3", inbytecode "45" },
    
    { type2arg_adapter, 0, "1", inbytecode "8 148 10" anonymousmember "a1" },
    { type5arg_adapter, 0, "1", inbytecode "8 173 10" anonymousmember "a1" },
    { type6arg_adapter, 0, "1", inbytecode "8 172 10" anonymousmember "a1" },
    { type5arg_adapter, 0, "3", inbytecode "8 237 10" anonymousmember "a1" },
    { type6arg_adapter, 0, "35", inbytecode "8 236 10" anonymousmember "a1" },
    { type7arg_adapter, 0, "2", inbytecode "8 236 10" anonymousmember "a1" },
    { type9arg_adapter, 0, "1", inbytecode "8 148 10" anonymousmember "10 a1" },
    { type9arg_adapter, 0, "23456", inbytecode "10 a1" },
    
    { type1arg_adapter, 0, "0", inbytecode "a1 0" },
    { type2arg_adapter, 0, "0", inbytecode "8 148 10" anonymousmember "a1 0" },
    { type5arg_adapter, 0, "0", inbytecode "8 173 10" anonymousmember "a1 0" },
    { type6arg_adapter, 0, "0", inbytecode "8 172 10" anonymousmember "a1 0" },
    { type9arg_adapter, 0, "0", inbytecode "8 148 10" anonymousmember "10 a1 0" },
    
    
        // *************************************************************************************
        // *****  Commands with a scripted definition (using the operators defined above)  *****
        // *************************************************************************************
    
    { type2arg "[ - <" type4arg "," type4arg "> ]", stepVarLevel, "1", "remove " arg1 "[<" arg2 "," arg3 ">]" },
    
    { "call (" type1arg ")", stepVarLevel, "1456", "cicadaLibraryFunction#0(" arg1 ")" },
    { "setCompiler (" type1arg ")", stepVarLevel, "56", "cicadaLibraryFunction#1(" arg1 ")" },
    { "compile (" type1arg ")", stepVarLevel, "56", "cicadaLibraryFunction#2(" arg1 ")" },
    { "transform (" type1arg ")", stepVarLevel, "167", "cicadaLibraryFunction#3(" arg1 ")" },
    { "load (" type1arg ")", stepVarLevel, "56", "cicadaLibraryFunction#4(" arg1 ")" },
    { "save (" type1arg ")", stepVarLevel, "156", "cicadaLibraryFunction#5(" arg1 ")" },
    { "input (" type1arg ")", stepVarLevel, "56", "cicadaLibraryFunction#6(" arg1 ")" },
    { "print (" type1arg ")", stepVarLevel, "1", "cicadaLibraryFunction#7(" arg1 ")" },
    { "read_string (" type1arg ")", stepVarLevel, "1", "cicadaLibraryFunction#8(" arg1 ")" },
    { "print_string (" type1arg ")", stepVarLevel, "1", "cicadaLibraryFunction#9(" arg1 ")" },
    { "trap (" type1arg ")", stepVarLevel, "1456", inbytecode "7 54 10 8 204 10" anonymousmember "57 a1 0" },   // trap() is special
    { "throw (" type1arg ")", stepVarLevel, "1", "cicadaLibraryFunction#11(" arg1 ")" },
    { "top (" type1arg ")", stepVarLevel, "456", "cicadaLibraryFunction#12(" arg1 ")" },
    { "size (" type1arg ")", stepVarLevel, "456", "cicadaLibraryFunction#13(" arg1 ")" },
    { "abs (" type1arg ")", stepVarLevel, "456", "cicadaLibraryFunction#14(" arg1 ")" },
    { "floor (" type1arg ")", stepVarLevel, "456", "cicadaLibraryFunction#15(" arg1 ")" },
    { "ceil (" type1arg ")", stepVarLevel, "456", "cicadaLibraryFunction#16(" arg1 ")" },
    { "log (" type1arg ")", stepVarLevel, "456", "cicadaLibraryFunction#17(" arg1 ")" },
    { "cos (" type1arg ")", stepVarLevel, "456", "cicadaLibraryFunction#18(" arg1 ")" },
    { "sin (" type1arg ")", stepVarLevel, "456", "cicadaLibraryFunction#19(" arg1 ")" },
    { "tan (" type1arg ")", stepVarLevel, "456", "cicadaLibraryFunction#20(" arg1 ")" },
    { "acos (" type1arg ")", stepVarLevel, "456", "cicadaLibraryFunction#21(" arg1 ")" },
    { "asin (" type1arg ")", stepVarLevel, "456", "cicadaLibraryFunction#22(" arg1 ")" },
    { "atan (" type1arg ")", stepVarLevel, "456", "cicadaLibraryFunction#23(" arg1 ")" },
    { "random (" type1arg ")", stepVarLevel, "456", "cicadaLibraryFunction#24(" arg1 ")" },
    { "find (" type1arg ")", stepVarLevel, "456", "cicadaLibraryFunction#25(" arg1 ")" },
    { "type (" type1arg ")", stepVarLevel, "456", "cicadaLibraryFunction#26(" arg1 ")" },
    { "member_ID (" type1arg ")", stepVarLevel, "456", "cicadaLibraryFunction#27(" arg1 ")" },
    { "bytecode (" type1arg ")", stepVarLevel, "56", "cicadaLibraryFunction#28(" arg1 ")" },
    { "springCleaning (" type1arg ")", stepVarLevel, "1456", "cicadaLibraryFunction#29(" arg1 ")" },
    
    { "exit", 0, "1", "throw(51)" }
    
    
        // *********************************************************
        // *****  User-defined commands/operators can go here  *****
        // *********************************************************
    
};


ccInt cicadaLanguageNumCommands = sizeof(cicadaLanguage)/sizeof(commandTokenType);
ccInt cicadaNumPrecedenceLevels = sizeof(cicadaLanguageAssociativity)/sizeof(ccInt);
