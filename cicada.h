//
//  cicada.h
//  cicada
//
//  Created by Brian Ross on 11/2/25.
//

#ifndef cicada_h
#define cicada_h

#include <stddef.h>
#include <stdbool.h>


// Cicada integer/floating point types

// These can be changed (e.g. int --> long if we need to work with large numbers);
// just remember to also change the 4 read/printXXXFormatStrings variables

typedef int ccInt;
#define ccIntMax INT_MAX
#define ccIntMin INT_MIN

typedef double ccFloat;

#define maxPrintableDigits DBL_DIG

#define printFloatFormatString "%lg"
#define print_stringFloatFormatString "%.*lg"
#define readFloatFormatString "%lg%n"

#define printIntFormatString "%i"
#define readIntFormatString "%i%n"



// C function calls: types, prototypes, & definitions

typedef struct {
    ccInt num;
    void **p;
    ccInt **type;
    ccInt *indices;
} argsType;

typedef struct {
    const char *functionName;
    ccInt(*functionPtr)(argsType);
} Cfunction;

extern ccInt getArgs(argsType, ...);
extern char *argDummies[5];

#define fromArg(n) NULL,n
#define endArgs NULL,-1

#define byValue(p) &argDummies[0],p
#define scalarValue(t,p) &argDummies[1],t,p
#define scalarRef(t,p) &argDummies[2],t,p
#define arrayValue(t,p) &argDummies[3],t,p
#define arrayRef(t,p) &argDummies[4],t,p

//#define arrayArg(t) t+1
#define arrayOf(a) array_type,a
#define listOf(a) list_type,a
#define string_type list_type,char_type
//#define scalarArg(t) 200,t+1



// Variable data types; some internal functions use var_type which is defined as -1 in bytecd.h

#define bool_type 0
#define char_type 1
#define int_type 2
#define double_type 3
#define string_const_type 4
#define composite_type 5
#define array_type 6
#define list_type 7
#define no_type 8



// Run Cicada

extern ccInt runCicadaMain(const Cfunction *, const ccInt, const char *, const bool);

#define runCicada(f,s,r) runCicadaMain(f,(f==NULL)?0:sizeof(f)/sizeof(Cfunction),s,r)


// For strings

typedef struct {            // the first field of the 'window' datatype
    ccInt PLL_index;
} arg;

extern ccInt getArgTop(arg *);
extern ccInt setMemberTop(arg *, const ccInt, const ccInt, char **);
extern ccInt setStringSize(arg *, const ccInt, const ccInt, char **);
extern arg *stepArg(arg *, ccInt, ccInt *);
extern void *argData(arg *);


// Error codes

#define out_of_memory_err 1             // LL error codes
#define out_of_range_err 2
#define init_err 3
#define mismatched_indices_err 4

#define char_read_err 5                 // compiler error codes
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

#define illegal_command_err 18          // additional error codes thrown only at runtime
#define code_overflow_err 19
#define inaccessible_code_warning 20
#define bad_jump_err 21

#define divide_by_zero_warning 22

#define member_not_found_err 23
#define no_member_err 24
#define undefined_member_err 25
#define void_member_err 26
#define step_multiple_members_err 27
#define incomplete_member_err 28
#define incomplete_variable_err 29
#define invalid_index_err 30
#define multiple_indices_not_allowed_err 31
#define index_argument_err 32
#define no_parent_err 33

#define not_a_variable_err 34
#define not_a_function_err 35
#define not_composite_err 36
#define string_expected_err 37

#define illegal_target_err 38
#define target_deleted_err 39

#define unequal_data_size_err 40
#define not_a_number_err 41
#define overlapping_window_err 42

#define thrown_to_err 43
#define nonexistent_C_function_err 44
#define wrong_argument_count_err 45
#define library_argument_err 46

#define self_reference_err 47
#define recursion_depth_err 48
#define IO_error 49

#define return_flag 50
#define finished_signal 51


#endif /* cicada_h */
