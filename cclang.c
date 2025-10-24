/*
 *  cclang.c(pp) - constant variable/array definitions
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
#include "cclang.h"



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
    
    { type1arg "\n" type1arg, sentenceLevel, "1", inbytecode bcArg(1) bcArg(2) },
    { type1arg "," type1arg, sentenceLevel, "1", inbytecode bcArg(1) bcArg(2) },
    
    { "(" typeXarg ")", 0, argXtype, inbytecode bcArg(1) },
    
    { type1arg "|" commentarg optionalargs "\n" type1arg, sentenceLevel, "1", inbytecode bcArg(1) bcArg(3) },
    { "&" commentarg "\n", 0, "", removedexpression },
    { "|*" commentarg "*|", 0, "", removedexpression },
    
    { type6arg "~", sentenceLevel, "1", inbytecode bcArg(1) },
    
    
        // Flow control and function calling
    
    { type1arg ";" type1arg, sentenceLevel, "1", inbytecode bcArg(1) bc(code_marker) bcArg(2) },
    { "code", 0, "1", inbytecode bc(code_marker) },
    
    { "return" type3arg, commandLevel, "1", inbytecode bc(function_return) bcArg(1) },
    { type2arg "(" type1arg ")", stepVarLevel, "23456", inbytecode bc(user_function) bcArg(1)
                    bc_define(defxxFlags) bc(search_member) anonymousmember bc(code_block) bcArg(2) bc(end_of_script) },
    
    { "cicadaLibraryFunction # " type4arg " (" type1arg ")", stepVarLevel, "123456",
                    inbytecode bc(built_in_function) bc(0) bc_define(defxxFlags) bc(search_member) anonymousmember
                    bc(code_block) bcArg(2) bc(end_of_script) },
    { "$" Cfunctionarg "(" type1arg ")", stepVarLevel, "123456",
                    inbytecode bc(built_in_function) bcArg(1) bc_define(defxxFlags) bc(search_member) anonymousmember
                    bc(code_block) bcArg(2) bc(end_of_script) },
    
    
        // Define, equate and forced-equate operators
    
    { type2arg "::" type6arg, defineLevel, "123456", inbytecode bc_define(defFlags) bcArg(1) bcArg(2) },
    { type2arg ":= @" type2arg, defineLevel, "123456", inbytecode bc_define(dqaFlags) bcArg(1) bcArg(2) },
    { type2arg "= @" type2arg, defineLevel, "123456", inbytecode bc_define(eqaFlags) bcArg(1) bcArg(2) },
    { type2arg "<- @" type2arg, defineLevel, "123456", inbytecode bc_define(eqaFlags) bcArg(1) bcArg(2) },
    { type2arg "=" type5arg, defineLevel, "123456", inbytecode bc_define(equFlags) bcArg(1) bcArg(2) },
    { type2arg "<-" type5arg, defineLevel, "123456", inbytecode bc_define(equFlags) bcArg(1) bcArg(2) },
    { type2arg ":=" type5arg, defineLevel, "123456", inbytecode bc_define(deqFlags) bcArg(1) bcArg(2) },
    { type2arg "@ ::" type6arg, defineLevel, "123456", inbytecode bc_define(vdfFlags) bcArg(1) bcArg(2) },
    { type2arg "* ::" type6arg, defineLevel, "123456", inbytecode bc_define(mdfFlags) bcArg(1) bcArg(2) },
    { type2arg "= !" type5arg, defineLevel, "123456", inbytecode bc(forced_equate) bcArg(1) bcArg(2) },
    { type2arg "<- !" type5arg, defineLevel, "123456", inbytecode bc(forced_equate) bcArg(1) bcArg(2) },
    
    
        // Member and array index operators
    
    { type2arg "." type9arg, stepVarLevel, "23456", inbytecode bc(step_to_member_ID) bcArg(2) bcArg(1) },
    { type2arg "[" type4arg "]", stepVarLevel, "23456", inbytecode bc(step_to_index) bcArg(1) bcArg(2) },
    { type2arg "[ <" type4arg "," type4arg "> ]", stepVarLevel, "23456", inbytecode bc(step_to_indices) bcArg(1) bcArg(2) bcArg(3) },
    { type2arg "[ * ]", stepVarLevel, "23456", inbytecode bc(step_to_all) bcArg(1) },
    { type2arg "[ ]", stepVarLevel, "23456", inbytecode bc(step_to_all) bcArg(1) },
    { type2arg "[^" type4arg "]", stepVarLevel, "123456", inbytecode bc(resize_cmd) bcArg(1) bcArg(2) },
    { type2arg "[ +" type4arg "]", stepVarLevel, "123456", inbytecode bc(insert_index) bcArg(1) bcArg(2) },
    { type2arg "[ + <" type4arg "," type4arg "> ]", stepVarLevel, "123456", inbytecode bc(insert_indices) bcArg(1) bcArg(2) bcArg(3) },
    
//    { "top (" type1arg ")", stepVarLevel, "456", "$top(" arg1 ")" },
    { "top (" type1arg ")", stepVarLevel, "456", inbytecode bc(built_in_function) bc(-7) bc_define(defxxFlags) bc(search_member) anonymousmember
                    bc(code_block) bcArg(1) bc(end_of_script) },
    
    { "remove" type2arg, commandLevel, "1", inbytecode bc(remove_cmd) bcArg(1) },
    
    
        // Comparison operators
    
    { type5arg "==" type5arg, compareLevel, "5", inbytecode bc(if_equal) bcArg(1) bcArg(2) },
    { type5arg "/=" type5arg, compareLevel, "5", inbytecode bc(if_not_equal) bcArg(1) bcArg(2) },
    { type4arg ">" type4arg, compareLevel, "5", inbytecode bc(if_greater) bcArg(1) bcArg(2) },
    { type4arg ">=" type4arg, compareLevel, "5", inbytecode bc(if_greater_or_equal) bcArg(1) bcArg(2) },
    { type4arg "<" type4arg, compareLevel, "5", inbytecode bc(if_less) bcArg(1) bcArg(2) },
    { type4arg "<=" type4arg, compareLevel, "5", inbytecode bc(if_less_or_equal) bcArg(1) bcArg(2) },
    { type2arg "== @" type2arg, compareLevel, "5", inbytecode bc(if_at) bcArg(1) bcArg(2) },
    { type2arg "/= @" type2arg, compareLevel, "5", inbytecode bc(if_not_at) bcArg(1) bcArg(2) },
    
    
        // Arithmetic operators
    
    { type4arg "+" type4arg, addsubLevel, "456", inbytecode bc(add_num) bcArg(1) bcArg(2) },
    { type4arg "-" type4arg, addsubLevel, "456", inbytecode bc(subtract_num) bcArg(1) bcArg(2) },
    { type4arg "*" type4arg, multdivLevel, "456", inbytecode bc(multiply_num) bcArg(1) bcArg(2) },
    { type4arg "/" type4arg, multdivLevel, "456", inbytecode bc(divide_num) bcArg(1) bcArg(2) },
    { type4arg "^" type4arg, expLevel, "456", inbytecode bc(raise_to_power) bcArg(1) bcArg(2) },
    { "-" type4arg, negationLevel, "456", inbytecode bc(multiply_num) bc_constant_int(-1) bcArg(1) },
    { type4arg "mod" type4arg, multdivLevel, "456", inbytecode bc(mod_int) bcArg(1) bcArg(2) },
    
    
        // Logical operators
    
    { "not" type5arg, notBoolLevel, "5", inbytecode bc(if_not) bcArg(1) },
    { type5arg "and" type5arg, binaryBoolLevel, "5", inbytecode bc(if_and) bcArg(1) bcArg(2) },
    { type5arg "or" type5arg, binaryBoolLevel, "5", inbytecode bc(if_or) bcArg(1) bcArg(2) },
    { type5arg "xor" type5arg, binaryBoolLevel, "5", inbytecode bc(if_xor) bcArg(1) bcArg(2) },
    
    
        // Operators on code
    
    { type2arg "#" type4arg, stepVarLevel, "23456", inbytecode bc(code_number) bcArg(2) bcArg(1) },
    { type2arg "<<" type6arg, codeSubstitutionLevel, "23456", inbytecode bc(substitute_code) bcArg(2) bcArg(1) },
    { type6arg ":" type6arg, inheritanceLevel, "6", inbytecode bc(append_code) bcArg(1) bcArg(2) },
    
    
        // Predefined variables
    
    { "args", 0, "23456", inbytecode bc(args_variable) },
    { "this", 0, "23456", inbytecode bc(this_variable) },
    { "that", 0, "23456", inbytecode bc(that_variable) },
    { "parent", 0, "23456", inbytecode bc(parent_variable) },
    { "\\", 0, "23456", inbytecode bc(parent_variable) },
    { type2arg ".parent", backstepVarLevel, "23456", inbytecode bcArg(1) bc(parent_variable) },
    { type2arg ".\\", backstepVarLevel, "23456", inbytecode bcArg(1) bc(parent_variable) },
    { "top", 0, "23456", inbytecode bc(top_variable) },
    { "*", 0, "236", inbytecode bc(no_variable) },
    { "nothing", 0, "236", inbytecode bc(no_variable) },
    
    
        // Data types
    
    { "[" type4arg "]" type6arg, arrayLevel, "6", inbytecode bc(type_array) bcArg(1) bcArg(2) },
    { "[ * ]" type6arg, arrayLevel, "6", inbytecode bc(type_array) bc_constant_int(0) bcArg(1) },
    { "[ ]" type6arg, arrayLevel, "6", inbytecode bc(type_array) bc_constant_int(0) bcArg(1) },
    
    { "bool", 0, "6", inbytecode bc(type_bool) },
    { "char", 0, "6", inbytecode bc(type_char) },
    { "int", 0, "6", inbytecode bc(type_int) },
    { "double", 0, "6", inbytecode bc(type_float) },
    { "string", 0, "6", inbytecode bc(type_string) },
    
    
        // Hard-coded constants
    
    { "false", 0, "56", inbytecode bc_constant_bool(0) },
    { "true", 0, "56", inbytecode bc_constant_bool(1) },
    
    { "'" chararg "'", 0, "56", inbytecode bc(constant_char) bcArg(1) },
    { int_constant, 0, "456", inbytecode bc(constant_int) bcArg(1) },
    { double_constant, 0, "456", inbytecode bc(constant_double) bcArg(1) },
    { "\"" stringarg "\"", 0, "56", inbytecode bc(constant_string) bcArg(1) },
    { "{" type1arg "}", 0, "67", inbytecode bc(code_block) bcArg(1) bc(end_of_script) },
    
    { variable_name, 0, "9", inbytecode bcArg(1) },
    
    
        // High-level flow control commands, each several bytecode 'sentences' long
    
    { "for" type2arg "in <" type4arg "," type4arg ">" type1arg, commandLevel, "1",
                    inbytecode bc_define(equFlags) bcArg(1) bcArg(2)
                    bcPosition(1) bc_jump_if_true(2) bc(if_greater) bcArg(1)
                    bcArg(3) bcArg(4) bc_define(equFlags) bcArg(1) bc(add_num) bc(that_variable) bc_constant_int(1)
                    bc_jump_always(1) bcPosition(2) },
    { "backfor" type2arg "in <" type4arg "," type4arg ">" type1arg, commandLevel, "1",
                    inbytecode bc_define(equFlags) bcArg(1) bcArg(3)
                    bcPosition(1) bc_jump_if_true(2) bc(if_less) bcArg(1)
                    bcArg(2) bcArg(4) bc_define(equFlags) bcArg(1) bc(subtract_num) bc(that_variable) bc_constant_int(1)
                    bc_jump_always(1) bcPosition(2) },
    { "if" type5arg "then" type1arg optionalargs "else" type1arg, commandLevel, "1",
                    inbytecode bc_jump_if_false(1) bcArg(1) bcArg(2)
                    bc_jump_always(2) bcPosition(1) bcArg(3) bcPosition(2) },
    { "while" type5arg "do" type1arg, commandLevel, "1",
                    inbytecode bcPosition(2) bc_jump_if_false(1) bcArg(1) bcArg(2)
                    bc_jump_always(2) bcPosition(1) },
    { "loop" type1arg "until" type5arg, commandLevel, "1",
                    inbytecode bcPosition(1) bcArg(1) bc_jump_if_false(1) bcArg(2) },
    
    
        // The 'adapters':  wee bits of unscripted bytecode that make the Cicada syntax work
    
    { noarg_adapter, 0, "0", inbytecode bc(end_of_script) },
    { noarg_adapter, 0, "1", inbytecode },
    { noarg_adapter, 0, "3", inbytecode bc(no_variable) },
    
    { type2arg_adapter, 0, "1", inbytecode bc_define(dqaxFlags) bc(search_member) anonymousmember bcArg(1) },
    { type5arg_adapter, 0, "1", inbytecode bc_define(deqxFlags) bc(search_member) anonymousmember bcArg(1) },
    { type6arg_adapter, 0, "1", inbytecode bc_define(defxFlags) bc(search_member) anonymousmember bcArg(1) },
    { type5arg_adapter, 0, "3", inbytecode bc_define(deqxxFlags) bc(search_member) anonymousmember bcArg(1) },
    { type6arg_adapter, 0, "35", inbytecode bc_define(defxxFlags) bc(search_member) anonymousmember bcArg(1) },
    { type7arg_adapter, 0, "2", inbytecode bc_define(defxxFlags) bc(search_member) anonymousmember bcArg(1) },
    { type9arg_adapter, 0, "1", inbytecode bc_define(dqaxFlags) bc(search_member) anonymousmember bc(search_member) bcArg(1) },
    { type9arg_adapter, 0, "23456", inbytecode bc(search_member) bcArg(1) },
    
    { type1arg_adapter, 0, "0", inbytecode bcArg(1) bc(end_of_script) },
    { type2arg_adapter, 0, "0", inbytecode bc_define(dqaxFlags) bc(search_member) anonymousmember bcArg(1) bc(end_of_script) },
    { type5arg_adapter, 0, "0", inbytecode bc_define(deqxFlags) bc(search_member) anonymousmember bcArg(1) bc(end_of_script) },
    { type6arg_adapter, 0, "0", inbytecode bc_define(defxFlags) bc(search_member) anonymousmember bcArg(1) bc(end_of_script) },
    { type9arg_adapter, 0, "0", inbytecode bc_define(dqaxFlags) bc(search_member) anonymousmember bc(search_member) bcArg(1) bc(end_of_script) },
    
    
        // *************************************************************************************
        // *****  Commands with a scripted definition (using the operators defined above)  *****
        // *************************************************************************************
    
    { type2arg "[ - <" type4arg "," type4arg "> ]", stepVarLevel, "1", "remove " arg1 "[<" arg2 "," arg3 ">]" },
    
    { "call (" type1arg ")", stepVarLevel, "1456", "cicadaLibraryFunction#0(" arg1 ")" },
    { "trap (" type1arg ")", stepVarLevel, "1456", inbytecode bc(built_in_function) bc(-5)
                bc_define(defcxxFlags) bc(search_member) anonymousmember bc(code_block) bcArg(1) bc(end_of_script) },   // trap() is special
    
    { "exit", 0, "1", "throw(51)" }
    
    
        // *********************************************************
        // *****  User-defined commands/operators can go here  *****
        // *********************************************************
    
};


ccInt cicadaLanguageNumCommands = sizeof(cicadaLanguage)/sizeof(commandTokenType);
ccInt cicadaNumPrecedenceLevels = sizeof(cicadaLanguageAssociativity)/sizeof(ccInt);
