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



// Argument types:  0 = script, 1 = command(s), 2 = function return, 3 = variable/function, 4 = variable with default *,
// 5 = number, 6 = data, 7 = code/type, 8 = inlined code, 9 = variable name

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
    
    { type7arg "~", sentenceLevel, "1", inbytecode bcArg(1) },
    
    
        // Flow control and function calling
    
    { type1arg ";" type1arg, sentenceLevel, "1", inbytecode bcArg(1) bc(code_marker) bcArg(2) },
    { "code", 0, "1", inbytecode bc(code_marker) },
    
    { "return" type4arg, commandLevel, "1", inbytecode bc(function_return) bcArg(1) },
    { type3arg "(" type1arg ")", stepVarLevel, "234567", inbytecode bc(user_function) bcArg(1)
                    bc_define(defxxFlags) bc(search_member) anonymousmember bc(code_block) bcArg(2) bc(end_of_script) },
    { type3arg "{" type1arg "}", stepVarLevel, "234567", inbytecode bc(user_function) bcArg(1)
                    bc_define(defFlags) bc(search_member) anonymousmember bc(code_block) bcArg(2) bc(end_of_script) },
    { type3arg "@" type3arg, negationLevel, "234567", inbytecode bc(user_function) bcArg(1) bcArg(2) },     // negationLevel so it's r_to_l
    { "@" type3arg "(" type1arg ")", stepVarLevel, "34567", inbytecode bc(user_function) bcArg(1)
                    bc_define(defxxFlags) bc(search_member) anonymousmember bc(code_block) bcArg(2) bc(end_of_script) },
    
    { "$" Cfunctionarg "(" type1arg ")", stepVarLevel, "134567",
                    inbytecode bc(C_function) bcArg(1) bc_define(defxxFlags) bc(search_member) anonymousmember
                    bc(code_block) bcArg(2) bc(end_of_script) },
    
    { "trap (" type1arg ")", stepVarLevel, "1567", inbytecode bc(C_function) bc(-5)             // trap() is special
                bc_define(defcxxFlags) bc(search_member) anonymousmember bc(code_block) bcArg(1) bc(end_of_script) },
    
    
        // Define, equate and forced-equate operators
    
    { type3arg "::" type7arg, defineLevel, "134567", inbytecode bc_define(defFlags) bcArg(1) bcArg(2) },
    { type3arg ":= @" type3arg, defineLevel, "134567", inbytecode bc_define(dqaFlags) bcArg(1) bcArg(2) },
    { type3arg "= @" type3arg, defineLevel, "134567", inbytecode bc_define(eqaFlags) bcArg(1) bcArg(2) },
    { type3arg "<- @" type3arg, defineLevel, "134567", inbytecode bc_define(eqaFlags) bcArg(1) bcArg(2) },
    { type3arg "=" type6arg, defineLevel, "134567", inbytecode bc_define(equFlags) bcArg(1) bcArg(2) },
    { type3arg "<-" type6arg, defineLevel, "134567", inbytecode bc_define(equFlags) bcArg(1) bcArg(2) },
    { type3arg ":=" type6arg, defineLevel, "134567", inbytecode bc_define(deqFlags) bcArg(1) bcArg(2) },
    { type3arg "@ ::" type7arg, defineLevel, "134567", inbytecode bc_define(vdfFlags) bcArg(1) bcArg(2) },
    { type3arg "* ::" type7arg, defineLevel, "134567", inbytecode bc_define(mdfFlags) bcArg(1) bcArg(2) },
    { type3arg "= !" type6arg, defineLevel, "134567", inbytecode bc(forced_equate) bcArg(1) bcArg(2) },
    { type3arg "<- !" type6arg, defineLevel, "134567", inbytecode bc(forced_equate) bcArg(1) bcArg(2) },
    
    
        // Member and array index operators
    
    { type3arg "." type9arg, stepVarLevel, "34567", inbytecode bc(step_to_member_ID) bcArg(2) bcArg(1) },
    { type3arg "[" type5arg "]", stepVarLevel, "34567", inbytecode bc(step_to_index) bcArg(1) bcArg(2) },
    { type3arg "[ <" type5arg "," type5arg "> ]", stepVarLevel, "34567", inbytecode bc(step_to_indices) bcArg(1) bcArg(2) bcArg(3) },
    { type3arg "[ * ]", stepVarLevel, "34567", inbytecode bc(step_to_all) bcArg(1) },
    { type3arg "[ ]", stepVarLevel, "34567", inbytecode bc(step_to_all) bcArg(1) },
    { type3arg "[^" type5arg "]", stepVarLevel, "134567", inbytecode bc(resize_cmd) bcArg(1) bcArg(2) },
    { type3arg "[ +" type5arg "]", stepVarLevel, "134567", inbytecode bc(insert_index) bcArg(1) bcArg(2) },
    { type3arg "[ + <" type5arg "," type5arg "> ]", stepVarLevel, "134567", inbytecode bc(insert_indices) bcArg(1) bcArg(2) bcArg(3) },
    
    { "top (" type1arg ")", stepVarLevel, "567", inbytecode bc(C_function) bc(-7) bc_define(defxxFlags) bc(search_member) anonymousmember
                    bc(code_block) bcArg(1) bc(end_of_script) },
    
    { "remove" type3arg, commandLevel, "1", inbytecode bc(remove_cmd) bcArg(1) },
    
    
        // Comparison operators
    
    { type6arg "==" type6arg, compareLevel, "6", inbytecode bc(if_equal) bcArg(1) bcArg(2) },
    { type6arg "/=" type6arg, compareLevel, "6", inbytecode bc(if_not_equal) bcArg(1) bcArg(2) },
    { type5arg ">" type5arg, compareLevel, "6", inbytecode bc(if_greater) bcArg(1) bcArg(2) },
    { type5arg ">=" type5arg, compareLevel, "6", inbytecode bc(if_greater_or_equal) bcArg(1) bcArg(2) },
    { type5arg "<" type5arg, compareLevel, "6", inbytecode bc(if_less) bcArg(1) bcArg(2) },
    { type5arg "<=" type5arg, compareLevel, "6", inbytecode bc(if_less_or_equal) bcArg(1) bcArg(2) },
    { type3arg "== @" type3arg, compareLevel, "6", inbytecode bc(if_at) bcArg(1) bcArg(2) },
    { type3arg "/= @" type3arg, compareLevel, "6", inbytecode bc(if_not_at) bcArg(1) bcArg(2) },
    
    
        // Arithmetic operators
    
    { type5arg "+" type5arg, addsubLevel, "567", inbytecode bc(add_num) bcArg(1) bcArg(2) },
    { type5arg "-" type5arg, addsubLevel, "567", inbytecode bc(subtract_num) bcArg(1) bcArg(2) },
    { type5arg "*" type5arg, multdivLevel, "567", inbytecode bc(multiply_num) bcArg(1) bcArg(2) },
    { type5arg "/" type5arg, multdivLevel, "567", inbytecode bc(divide_num) bcArg(1) bcArg(2) },
    { type5arg "^" type5arg, expLevel, "567", inbytecode bc(raise_to_power) bcArg(1) bcArg(2) },
    { "-" type5arg, negationLevel, "567", inbytecode bc(multiply_num) bc_constant_int(-1) bcArg(1) },
    { type5arg "mod" type5arg, multdivLevel, "567", inbytecode bc(mod_int) bcArg(1) bcArg(2) },
    
    
        // Logical operators
    
    { "not" type6arg, notBoolLevel, "6", inbytecode bc(if_not) bcArg(1) },
    { type6arg "and" type6arg, binaryBoolLevel, "6", inbytecode bc(if_and) bcArg(1) bcArg(2) },
    { type6arg "or" type6arg, binaryBoolLevel, "6", inbytecode bc(if_or) bcArg(1) bcArg(2) },
    { type6arg "xor" type6arg, binaryBoolLevel, "6", inbytecode bc(if_xor) bcArg(1) bcArg(2) },
    
    
        // Operators on code
    
    { type3arg "#" type5arg, stepVarLevel, "34567", inbytecode bc(code_number) bcArg(2) bcArg(1) },
    { type3arg "<<" type7arg, codeSubstitutionLevel, "34567", inbytecode bc(substitute_code) bcArg(2) bcArg(1) },
    { type7arg ":" type7arg, inheritanceLevel, "7", inbytecode bc(append_code) bcArg(1) bcArg(2) },
    
    
        // Predefined variables
    
    { "args", 0, "34567", inbytecode bc(args_variable) },
    { "this", 0, "34567", inbytecode bc(this_variable) },
    { "that", 0, "34567", inbytecode bc(that_variable) },
    { "parent", 0, "34567", inbytecode bc(parent_variable) },
    { "\\", 0, "34567", inbytecode bc(parent_variable) },
    { type3arg ".parent", backstepVarLevel, "34567", inbytecode bcArg(1) bc(parent_variable) },
    { type3arg ".\\", backstepVarLevel, "34567", inbytecode bcArg(1) bc(parent_variable) },
    { "top", 0, "34567", inbytecode bc(top_variable) },
    { "*", 0, "347", inbytecode bc(no_variable) },
    { "nothing", 0, "347", inbytecode bc(no_variable) },
    
    
        // Data types
    
    { "[" type5arg "]" type7arg, arrayLevel, "7", inbytecode bc(type_array) bcArg(1) bcArg(2) },
    { "[ * ]" type7arg, arrayLevel, "7", inbytecode bc(type_array) bc_constant_int(0) bcArg(1) },
    { "[ ]" type7arg, arrayLevel, "7", inbytecode bc(type_array) bc_constant_int(0) bcArg(1) },
    { "[[ ]]" type7arg, arrayLevel, "7", inbytecode bc(type_lists) bcArg(1) },
    
    { "bool", 0, "7", inbytecode bc(type_bool) },
    { "char", 0, "7", inbytecode bc(type_char) },
    { "int", 0, "7", inbytecode bc(type_int) },
    { "double", 0, "7", inbytecode bc(type_float) },
    { "string", 0, "7", inbytecode bc(type_string) },
    
    
        // Hard-coded constants
    
    { "false", 0, "67", inbytecode bc_constant_bool(0) },
    { "true", 0, "67", inbytecode bc_constant_bool(1) },
    
    { "'" chararg "'", 0, "67", inbytecode bc(constant_char) bcArg(1) },
    { int_constant, 0, "567", inbytecode bc(constant_int) bcArg(1) },
    { double_constant, 0, "567", inbytecode bc(constant_double) bcArg(1) },
    { "\"" stringarg "\"", 0, "67", inbytecode bc(constant_string) bcArg(1) },
    { "{" type1arg "}", 0, "78", inbytecode bc(code_block) bcArg(1) bc(end_of_script) },
    
    { variable_name, 0, "9", inbytecode bcArg(1) },
    
    
        // High-level flow control commands, each several bytecode 'sentences' long
    
    { "for" type3arg "in <" type5arg "," type5arg ">" type1arg, commandLevel, "1",
                    inbytecode bc_define(equFlags) bcArg(1) bcArg(2)
                    bcPosition(1) bc_jump_if_true(2) bc(if_greater) bcArg(1)
                    bcArg(3) bcArg(4) bc_define(equFlags) bcArg(1) bc(add_num) bc(that_variable) bc_constant_int(1)
                    bc_jump_always(1) bcPosition(2) },
    { "backfor" type3arg "in <" type5arg "," type5arg ">" type1arg, commandLevel, "1",
                    inbytecode bc_define(equFlags) bcArg(1) bcArg(3)
                    bcPosition(1) bc_jump_if_true(2) bc(if_less) bcArg(1)
                    bcArg(2) bcArg(4) bc_define(equFlags) bcArg(1) bc(subtract_num) bc(that_variable) bc_constant_int(1)
                    bc_jump_always(1) bcPosition(2) },
    { "if" type6arg "then" type1arg optionalargs "else" type1arg, commandLevel, "1",
                    inbytecode bc_jump_if_false(1) bcArg(1) bcArg(2)
                    bc_jump_always(2) bcPosition(1) bcArg(3) bcPosition(2) },
    { "while" type6arg "do" type1arg, commandLevel, "1",
                    inbytecode bcPosition(2) bc_jump_if_false(1) bcArg(1) bcArg(2)
                    bc_jump_always(2) bcPosition(1) },
    { "loop" type1arg "until" type6arg, commandLevel, "1",
                    inbytecode bcPosition(1) bcArg(1) bc_jump_if_false(1) bcArg(2) },
    
    
        // The 'adapters':  wee bits of unscripted bytecode that make the Cicada syntax work
    
    { noarg_adapter, 0, "0", inbytecode bc(end_of_script) },
    { noarg_adapter, 0, "1", inbytecode },
    { noarg_adapter, 0, "4", inbytecode bc(no_variable) },
    
    { type2arg_adapter, 0, "1", inbytecode bc_define(deqxFlags) bc(search_member) anonymousmember bcArg(1) },
    { type3arg_adapter, 0, "1", inbytecode bc_define(dqaxFlags) bc(search_member) anonymousmember bcArg(1) },
    { type6arg_adapter, 0, "1", inbytecode bc_define(deqxFlags) bc(search_member) anonymousmember bcArg(1) },
    { type7arg_adapter, 0, "1", inbytecode bc_define(defxFlags) bc(search_member) anonymousmember bcArg(1) },
    { type6arg_adapter, 0, "4", inbytecode bc_define(deqxxFlags) bc(search_member) anonymousmember bcArg(1) },
    { type7arg_adapter, 0, "46", inbytecode bc_define(defxxFlags) bc(search_member) anonymousmember bcArg(1) },
    { type8arg_adapter, 0, "3", inbytecode bc_define(defxxFlags) bc(search_member) anonymousmember bcArg(1) },
    { type9arg_adapter, 0, "1", inbytecode bc_define(dqaxFlags) bc(search_member) anonymousmember bc(search_member) bcArg(1) },
    { type9arg_adapter, 0, "34567", inbytecode bc(search_member) bcArg(1) },
    
    { type1arg_adapter, 0, "0", inbytecode bcArg(1) bc(end_of_script) },
    { type3arg_adapter, 0, "0", inbytecode bc_define(dqaxFlags) bc(search_member) anonymousmember bcArg(1) bc(end_of_script) },
    { type6arg_adapter, 0, "0", inbytecode bc_define(deqxFlags) bc(search_member) anonymousmember bcArg(1) bc(end_of_script) },
    { type7arg_adapter, 0, "0", inbytecode bc_define(defxFlags) bc(search_member) anonymousmember bcArg(1) bc(end_of_script) },
    { type9arg_adapter, 0, "0", inbytecode bc_define(dqaxFlags) bc(search_member) anonymousmember bc(search_member) bcArg(1) bc(end_of_script) },
    
    
        // *************************************************************************************
        // *****  Commands with a scripted definition (using the operators defined above)  *****
        // *************************************************************************************
    
    { type3arg "[ - <" type5arg "," type5arg "> ]", stepVarLevel, "1", "remove " arg1 "[<" arg2 "," arg3 ">]" },
    
    { "exit", 0, "1", "$throw(51,false,this,1,1)" }
    
    
        // *********************************************************
        // *****  User-defined commands/operators can go here  *****
        // *********************************************************
    
};


ccInt cicadaLanguageNumCommands = sizeof(cicadaLanguage)/sizeof(commandTokenType);
ccInt cicadaNumPrecedenceLevels = sizeof(cicadaLanguageAssociativity)/sizeof(ccInt);
