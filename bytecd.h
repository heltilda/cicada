/*
 *  bytecd.h
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

#ifndef Bytecode_h
#define Bytecode_h

#include "lnklst.h"
#include "intrpt.h"
#include "cicada.h"
#include "ciclib.h"



// object_params describes what kind of object we stepped across; set by various commands: int_cmd, search_var, etc.

typedef struct {
    ccInt type;                   // ccInt, string, composite, etc.
    linkedlist arrayDimList;    // stores the dimensions of an array
    linkedlist *codeList;
} object_params;



// step_params describes the last step in a path; used for communicating between def/navigation commands

typedef struct {
    member *stemMember;
    ccInt stemMemberNumber;
    ccInt offset;
    ccInt indices;
    
    view stemView;     // used sometimes when defining variables; stores the coordinate before a step
} step_params;



// *** Famous & global variables ***

typedef struct {
    
    
        // Zero -- the top-level variable, with its entourage of search paths, codes, etc.
    
    window *_Zero, *_Zero_workspace;
    searchPath *_ZeroSuspensor;
    
    
        // these hold the error/warning codes & locations
    
    ccInt _err_code, _err_index, _warning_code, _warning_index;
    code_ref _err_script, _warning_script;
    
    
        // the registers holds numeric quantities from conditional expressions, inlined constants, variables, etc.
    
    ccBool _boolRegister;
    ccInt _intRegister;
    ccFloat _doubleRegister;
    linkedlist _stringRegister;
    linkedlist _codeRegister;
    
    
        // PC stands for Program Counter.
        // pcCodePtr is the standard PC -- points to the active location in the bytecode.
    
    ccInt *_PCCodePtr, *_StartCodePtr, *_EndCodePtr;
    
    
        // the following convey type, path, define-status, etc. of objects that Cicada encounters
    
    ccBool _canAddMembers, _isHiddenMember;
    object_params _GL_Object;
    step_params _GL_Path;
    ccInt _prevIndexWidth;
    
    
        // baseView/searchView are the Cicada memory addresses of the
        // active function/the current search path (a.b) respectively.
    
    view _baseView, _searchView;
    searchPath *_pcSearchPath;
    
    
        // A series of lesser Cicada memory addresses, holding args, that, the return variable, etc.  BIF = built-in-function.
    
    view _BIF_argsView, _argsView, _returnView, _thatView, _topView;
    
    
        // Miscellaneous globals
    
    ccInt _whichCompiler;               // the number of the compiler currently in use
    ccInt _CodeNumber;                  // set by the # operator
    ccInt _GL_MaxDigits;                // for printing numbers
    ccInt _GL_RecursionCounter;         // current nesting of running codes
    code_ref _PCCodeRef;
    linkedlist _JumpList, _JumpFromList, _SentenceList;         // these are used for checking code
    linkedlist _allCompilers;           // the user can switch between these when using compile()
    
    ccBool _extCallMode;                // set if a call() function is running
    ccBool _doPrintError;               // set by trap( ; ... ) to cause errors within to be printed 
} cc_bytecode_global_struct;

extern cc_bytecode_global_struct cc_bytecode_globals;

#define Zero cc_bytecode_globals._Zero
#define Zero_workspace cc_bytecode_globals._Zero_workspace
#define ZeroSuspensor cc_bytecode_globals._ZeroSuspensor

#define errCode cc_bytecode_globals._err_code
#define errIndex cc_bytecode_globals._err_index
#define errScript cc_bytecode_globals._err_script
#define warningCode cc_bytecode_globals._warning_code
#define warningIndex cc_bytecode_globals._warning_index
#define warningScript cc_bytecode_globals._warning_script

#define boolRegister cc_bytecode_globals._boolRegister
#define intRegister cc_bytecode_globals._intRegister
#define doubleRegister cc_bytecode_globals._doubleRegister
#define stringRegister cc_bytecode_globals._stringRegister

#define pcCodePtr cc_bytecode_globals._PCCodePtr
#define startCodePtr cc_bytecode_globals._StartCodePtr
#define endCodePtr cc_bytecode_globals._EndCodePtr

#define canAddMembers cc_bytecode_globals._canAddMembers
#define isHiddenMember cc_bytecode_globals._isHiddenMember
#define GL_Object cc_bytecode_globals._GL_Object
#define GL_Path cc_bytecode_globals._GL_Path

#define pcSearchPath cc_bytecode_globals._pcSearchPath

#define baseView cc_bytecode_globals._baseView
#define searchView cc_bytecode_globals._searchView

#define BIF_argsView cc_bytecode_globals._BIF_argsView
#define argsView cc_bytecode_globals._argsView
#define returnView cc_bytecode_globals._returnView
#define thatView cc_bytecode_globals._thatView
#define topView cc_bytecode_globals._topView

#define whichCompiler cc_bytecode_globals._whichCompiler
#define codeNumber cc_bytecode_globals._CodeNumber
#define maxDigits cc_bytecode_globals._GL_MaxDigits
#define recursionCounter cc_bytecode_globals._GL_RecursionCounter
#define PCCodeRef cc_bytecode_globals._PCCodeRef
#define codeRegister cc_bytecode_globals._codeRegister
#define jumpList cc_bytecode_globals._JumpList
#define jumpFromList cc_bytecode_globals._JumpFromList
#define sentenceList cc_bytecode_globals._SentenceList
#define allCompilers cc_bytecode_globals._allCompilers

#define extCallMode cc_bytecode_globals._extCallMode
#define doPrintError cc_bytecode_globals._doPrintError




// Prototypes

extern void _jump_always();
extern void _jump_if_true();
extern void _jump_if_false();
extern void _code_marker();
extern void _func_return();
extern void _user_function();
extern void _built_in_function();

extern void _def_general();
extern void copyCompareMultiView(void(*)(view *, view *), view *, view *);
extern void encompassMultiView(view *, window *, window *, variable *, ccBool);
extern int relinkGLStemMember(view *, ccBool, ccBool, ccBool);
extern void newStringLL(view *);
extern ccInt checkType(linkedlist *, ccInt *, ccInt *, ccInt *, ccInt, ccInt, view *, ccBool, ccBool);
extern void updateType(linkedlist *, ccInt *, ccInt *, ccInt *, ccInt, ccInt, ccInt, ccBool);

extern void _forced_equate();

extern void _search_member();
extern void _define_search_member();
extern void _object_search_member();
extern void smStep(ccBool);
extern void _step_to_memberID();
extern void _define_step_to_memberID();
extern void _object_step_to_memberID();
extern void sidStep(ccBool);
extern void _step_to_index();
extern void _define_step_to_index();
extern void _object_step_to_index();
extern void stixStep(ccBool);
extern void _step_to_indices();
extern void _define_step_to_indices();
extern void _object_step_to_indices();
extern void sticsStep(ccBool);
extern void _step_to_all_indices();
extern void _define_step_to_all_indices();
extern void staiStep(ccBool);
extern void _resize();
extern void _define_resize();
extern void _resize_start();
extern void doResize(ccBool);
extern void _insert_index();
extern void _define_insert_index();
extern void _insert_indices();
extern void _define_insert_indices();
extern void do_insert_index(ccBool);
extern void do_insert_indices(ccBool);
extern void masterInsert(ccBool, ccBool);

extern void callSearchPathFunction();
extern ccInt callIndexFunction();
extern void navigate(void (*)(ccBool), ccBool, ccBool, ccBool);

extern void resizeMember(member *, ccInt, ccInt);
extern void resizeIndices(member *, ccInt, ccInt, ccInt);

extern void _delete_indices();

extern void _if_eq();
extern void _if_ne();
extern void compareReadArg(void(*)(), ccInt *, ccBool *);
extern void _if_eq_at();
extern void _if_ne_at();
extern void eqaOneArg();

extern void _if_gt();
extern void _if_ge();
extern void _if_lt();
extern void _if_le();

extern void _addf();
extern void _subf();
extern void _mulf();
extern void _divf();
extern void _powf();
extern void _modi();

extern void Math_GT(ccFloat);
extern void Math_GE(ccFloat);
extern void Math_LT(ccFloat);
extern void Math_LE(ccFloat);

extern void Math_Add(ccFloat firstArgument);
extern void Math_Sub(ccFloat firstArgument);
extern void Math_Mul(ccFloat firstArgument);
extern void Math_Div(ccFloat firstArgument);
extern void Math_Pow(ccFloat firstArgument);
extern void Math_Mod(ccFloat firstArgument);

extern void mathOperator(void(*theOp)(ccFloat), ccInt);

extern void _if_not();
extern void _if_and();
extern void _if_or();
extern void _if_xor();

extern void logicAnd(ccBool);
extern void logicOr(ccBool);
extern void logicXor(ccBool);
extern void logicalOperator(void(*logicOp)(ccBool));

extern void _code_number();
extern void _sub_code();
extern void _append_code();
extern void getCurrentCodeList(linkedlist *);

extern void _get_args();
extern void _this_var();
extern void _that_var();
extern void _parent_var();
extern void _top_var();
extern void _no_var();

extern void _array_cmd();

extern void _bool_cmd();
extern void _char_cmd();
extern void _int_cmd();
extern void _double_cmd();
extern void _string_cmd();

extern void _constant_bool();
extern void _constant_char();
extern void _constant_int();
extern void _constant_double();
extern void _constant_string();
extern void _code_block();

extern void _illegal();

extern void refCodeList(linkedlist *);
extern void derefCodeList(linkedlist *);

extern void intAdapter();
extern void doubleAdapter();
extern void numAdapter(ccInt);

extern void skipNoArgs();
extern void skipOneArg();
extern void skipTwoArgs();
extern void skipThreeArgs();
extern void skipInt();
extern void skipDouble();
extern void skipString();
extern void skipIntAndOneArg();
extern void skipIntAndTwoArgs();
extern void skipOneArgAndInt();
extern void skipCode();

extern void checkBytecode();
extern void checkBytecodeSentences();

extern void checkCommand(ccInt);
extern void checkBackCommand(ccInt);
extern void checkFunction(ccInt);
extern void checkInteger(ccInt);
extern void checkDouble(ccInt);
extern void checkString(ccInt);
extern void checkCode(ccInt);
extern void checkJump(ccInt);
extern void checkIndices(ccInt);
extern void checkBytecodeArg(ccInt);
extern void illegalCmd(ccInt);

extern void beginExecution(code_ref *, ccBool, ccInt, ccInt, ccInt);
extern void runBytecode();
extern void runSkipMode(ccInt);

extern void callBytecodeFunction();
extern void callDefineFunction();
extern void callObjectFunction();
extern void callSentenceStartFunction();
extern void callLogicalFunction();
extern void callNumericFunction(ccInt);
extern void callCodeFunction();
extern void callSkipFunction();
extern void callFunction(void(*[commands_num])());

extern void saveBoolRegister(variable *, ccInt);
extern void loadBoolRegister(variable *, ccInt);
extern void saveIntRegister(variable *, ccInt);
extern void loadIntRegister(variable *, ccInt);
extern void saveDoubleRegister(variable *, ccInt);
extern void loadDoubleRegister(variable *, ccInt);
extern void loadStringRegister(variable *, ccInt);
extern void(*loadRegisterFunctions[])(variable *, ccInt);
extern void loadRegister(variable *, ccInt);

extern char leftArgs[commands_num];
extern char rightArgs[commands_num];
extern void(*checkJumpTables[6][commands_num])(ccInt);

extern void(*sentenceStartJumpTable[commands_num])();
extern void(*skipJumpTable[commands_num])();
extern void(*numericJumpTable[commands_num])();
extern void(*codeJumpTable[commands_num])();
extern void(*bytecodeJumpTable[commands_num])();
extern void(*defineJumpTable[commands_num])();

extern ccInt BIF_Types[];

extern void(*loadIntRegJumpTable[])(void *);
extern void(*saveIntRegJumpTable[])(void *);
extern void(*loadDPRegJumpTable[])(void *);
extern void(*saveDPRegJumpTable[])(void *);


// misc

#define var_type -1
#define no_string -1

#endif
