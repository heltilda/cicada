/*
 *  cicada.h -- misc. Cicada-wide definitions
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

#ifndef Cicada_h
#define Cicada_h


#include "lnklst.h"
#include "cmpile.h"




// The bytecode instruction words
// f denotes floating point (default double-precision) arithmetic; i denotes integer

#define null_cmd 0              /*  do nothing; marks ends of {}  */

#define jump_always 1           /*  relative addressing  */
#define jump_if_true 2          /*  jump if test is false  */
#define jump_if_false 3         /*  jump if test is true  */

#define code_marker 4           /*  delineates coding blocks  */
#define func_return 5           /*  return from the function  */
#define user_function 6         /*  (()) call the code of the current variable  */
#define built_in_function 7     /*  call a built-in function (besides the ones listed here)  */

#define def_general 8           /*  set the code (variable & member)  */
#define forced_equate 9         /*  (=!) copy data; only restriction is that data sizes must be the same  */

#define search_member 10        /*  search backwards for var with specified ID  */
#define step_to_memberID 11     /*  (.) step to a var with specified ID  */
#define step_to_index 12        /*  ([]) step to Nth var  */
#define step_to_indices 13      /*  return a range of variables  */
#define step_to_all 14          /*  [*]  */
#define resize_cmd 15           /*  [^#] -- sets a new top  */
#define insert_index 16         /*  same as step_to_index, but adds a new member (to a composite variable) or index (to an array)  */
#define insert_indices 17       /*  same as step_to_indices, but create new members/array indices (plural)  */
#define remove_cmd 18           /*  deletes members or array indices  */

#define if_eq 19                /*  (==)  */
#define if_ne 20                /*  (/=)  */
#define if_gt 21                /*  (>)  */
#define if_ge 22                /*  (>=)  */
#define if_lt 23                /*  (<)  */
#define if_le 24                /*  (<=)  */
#define if_eq_at 25             /*  (== @)  */
#define if_ne_at 26             /*  (/= @)  */

#define addf 27                 /*  (+)  -- P.S.  all numeric commands must be in a row (for _def_general())  */
#define subf 28                 /*  (-)  */
#define mulf 29                 /*  (*)  */
#define divf 30                 /*  (/)  */
#define powerf 31               /*  (^)  */
#define modi 32                 /*  (mod) */

#define if_not 33               /*  (not)  */
#define if_and 34               /*  (and)  */
#define if_or 35                /*  (or)  */
#define if_xor 36               /*  (xor)  */

#define code_number 37          /*  (#) return N'th code block of left arg, where N is given by right arg  */
#define sub_code 38             /*  (<<) match var (left arg) with code (right arg)  */
#define append_code 39          /*  (:) match left var with right arg code  */

#define get_args 40             /*  call the arguments from the passing routine  */
#define this_var 41             /*  return the variable that contains the code being executed  */
#define that_var 42             /*  return the variable to the left of an equate sign  */
#define parent_var 43           /*  return the next variable up the search path  */
#define top_var 44              /*  return the top of the variable currently being stepped though  */
#define no_var 45               /*  returns nothing -- no variable, type, or data  */

#define array_cmd 46            /*  as in myvar :: [dim] vartype  */
#define bool_cmd 47             /*  bool type  */
#define char_cmd 48             /*  char type  */
#define int_cmd 49              /*  int type  */
#define double_cmd 50           /*  double type  */
#define string_cmd 51           /*  string type  */

#define constant_bool 52        /*  a constant integer entered in the code */
#define constant_char 53        /*  a constant integer entered in the code */
#define constant_int 54         /*  a constant integer entered in the code */
#define constant_double 55      /*  a const. float entered in the code */
#define constant_string 56      /*  contained between double quotes */
#define code_block 57           /*  a {} */

#define commands_num 58
#define written_commands_num 60
#define control_commands_num 15
#define OoOLevels 17



// argument types (IDs of jump table) of the left/right arguments to the various operators

#define start_arg 0   /* start_arg is anything that can be used as the first function in an expression */
#define var_arg 1
#define data_arg 2
#define code_arg 3
#define type_arg 4
#define no_arg 5


// flags used after the def_general command

#define unjammable 7
#define hidden_member 6
#define run_constructor 5
#define relink_target 4
#define new_target 3
#define can_add_members 2
#define update_members 1
#define post_equate 0


// Prototypes from cicada.c(pp)

#ifdef __cplusplus
extern "C" {
#endif

extern commandTokenType cicadaLanguage[];
extern ccInt cicadaLanguageAssociativity[];
extern ccInt cicadaLanguageNumCommands;
extern ccInt cicadaNumPrecedenceLevels;

#ifdef __cplusplus
}
#endif

#endif
