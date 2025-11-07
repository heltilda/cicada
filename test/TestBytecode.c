#include <stdlib.h>
#include <stdio.h>
#include "lnklst.h"
#include "cmpile.h"
#include "intrpt.h"
#include "bytecd.h"
#include "ccmain.h"


extern void checkBytecode(void);
extern void test_MM_CountRefs(void);
void test_PrintBytecode(linkedlist *);

// Tests code run by the interpreter, and compiled by the Cicada compiler.
// Tests runtime error codes and pointers.  Tests are cumulative; we don't cleanUp() between them.

#define tYHE_TestRuns 88

void test_YHErrors()
{
    ccInt NullCode = 0, rtrn, counter, cc;
    char *CicadaCodePtr;
    linkedlist CompiledCodes;
    compiler_type *testCompiler;
    
            // Introduce the variables in the same order for each test case, since the compiler starts anew each run.  Only then a=a between any two runs.
            // Start with the definitions that introduce errors.  (This is now not true -- 2/17/05).
    char *TestPrograms[tYHE_TestRuns] = {
        "a:=@b", "a::*, a =@ b", "remove a, a::int", "a :=@a", "a@::a",
            "a::a, b::{a}", "a:=@b", "a::*:{char}", "a=@nothing, a@::*:a", "a::int, b=@b, c::a:*",    // 'another c'    // 5
        "a::*", "a=@a=@a", "remove a, remove b, remove c, remove d",             // 10
                    "a::{b::int}, c::{b::int}, (d::a:c)@::a:c, f::a:c:*:*, f=@d", "a.b::a.b, c=@*, remove c",  // was ::
            "remove a.b, remove a, c@::{b::int}, a::{b::c}, a.b=@c", "a.b=@a", "this.a=@a", "this[4][1]::this[3]", "remove this",    // 15
        "this[3][1][1]@::int", "this[-1]::this", "this@::this[5]", "this[5]::[10]int", "remove this[5][3]",                // 20
            "remove this[5][11]", "remove this[<4, 5>]",                                            // 25
                                "remove c, remove this[1], remove this[1]",
                                "a::{b::*}, (a.b::a):=@a", "a=@a.b",
        "remove a, a::{b::a}", "remove a, a::{b::*}, a.b=@a, remove a.b.b.b.b", "remove a, a::{}, ((b::*)@::{remove a})=@a",        // 30
                                                                    "a::[5][3]int", "a::a, b=@b, c::int, remove c.c.a.b",
            "remove a, remove b, remove c, a::\"12345\", b::char", " a@::b", "remove a, (a::string)::int",      // 35
                                                                    "remove a, remove b, b::string, a::\"t\", b:=@a, a@::\"d\"", "a.b=2",
        "remove a, remove b, a::[5][3][8]int, a[+1], a[<1,6>][+<3,4>]", "a[+<2, 1>]", "a[+7]", "a[+<0,2>]", "a[+9]",        // 40
            "a::a, b:=a[7][5][8]", "a[+0]", "a[3][+2]", "remove a[2][1]", "remove a[<1, 7>][1]",                    // 45
        "a[7][4][1]=1", "a[7][5][1]=1", "a[1][1][1][+3]", "remove a[<1, 7>], a[+1][2]=a[1][2]", "remove a, remove b",        // 50
            "(a::char)::char, b@::int, c:={d::int}", "a = 2, b = 3, a = b, b= a", "b = -1", "b=100000",         // 55
                                                "remove a, remove b, remove c",
        "a::{b::int}, c::{b::char}, d::{f::{g::int}}", "a.b=a.b, a = c, a=d.f, d.f = a", "a.b=c.b, a=d", "a=a.b", "a=@*, (b:=@c)=a",  // 60
            "remove a, b=a", "a=b", "a::[2]int, a[2]=b.b", "a[2]=b", "a[<1, 2>]=b.b",            // 65
        "a[0]=a", "a=a[0]", "a[+1]::char", "remove a, remove b, remove c, remove d, a::int",    // 70
                                                "me:=@this, {me.a::(b:=200)}={c::int}, remove this[1], remove this[1]",
            "a::int, b::[4]char, a=!b[<1,top>], b[<1,top>]=!a", "a=!b[1]",                    // 75
                        "remove a, remove b, a::{b::int,s::string}, b::{c::{d::int}, d::char}, a=!b, b=!a",
                                            "a=!b.d", "remove a, remove b.c, remove b",
        "a::5, b:=a, if (b /= a) then a::\"x\", b=2, if a==b then a::char",                    // 80
                                            "a = 1, b = 1.01, if a > b or -a < -b then a::{}, while not a<=b xor -3.5 == a do a::{}",
                       "remove a, for (a::double) in <1, 2> if a >= 3 or a <= 0.99 then a::{}", "if 1 == 1 and 2 > 1 then a::{}",
                                            "remove a, (a::int) = 1+2*5^2-6/3.0, if a mod 30 /=19 then a::{}, remove a, remove b",
            "a::{code, args(), (b::int)=args[1], b=b+1, return b}, if a(-8) /= -7 then a::int", "if a(a(a(5))) /= 8 then a=0",    // 85
                                "remove this[1], strs := {a::{\"Done\"}, \", at last!  \", 12, .22, \"\\n\\n\"}"
                            };
    
    const ccInt ErrorCodes[tYHE_TestRuns] = {
            member_not_found_err, member_not_found_err, passed, passed, passed,
                passed, type_mismatch_err, type_mismatch_err, type_mismatch_err, passed,
            type_mismatch_err, passed, member_not_found_err, passed, passed,
                passed, type_mismatch_err, passed, passed, undefined_member_err,
            not_composite_err, invalid_index_err, invalid_index_err, passed, passed,
                invalid_index_err, passed, passed, passed, passed,
            recursion_depth_err, passed, member_not_found_err, passed, not_composite_err,   // 32 -- not window_deleted_err since we didn't comb the tree
                passed, type_mismatch_err, type_mismatch_err, passed, not_composite_err,
            passed, passed, passed, invalid_index_err, invalid_index_err,
                passed, invalid_index_err, incomplete_variable_err, incomplete_variable_err, passed,
            passed, invalid_index_err, not_composite_err, passed, passed,    // 50
                passed, passed, passed, passed, passed,
            passed, passed, type_mismatch_err, type_mismatch_err, void_member_err,
                member_not_found_err, member_not_found_err, passed, type_mismatch_err, passed,
            invalid_index_err, invalid_index_err, type_mismatch_err, passed, passed,
                passed, unequal_data_size_err, passed, unequal_data_size_err, passed,
            passed, passed, passed, type_mismatch_err, passed,
                passed, passed, passed
            
                                    };
    
    const ccInt ErrorPtrInts[tYHE_TestRuns] = {        // errIndexes of vars are off by one for c, d, etc. (from{a} of run 5)
            3, 3, 0, 0, 0,
                0, define_equate, define_equate, define_equate, 0,
            define_equate, 0, 5, 0, 0,
                0, define_equate, 0, 0, this_variable,
            step_to_index, -1, 5, 0, 0,
                11, 0, 0, 0, 0,
            0, 0, 2, 0, 4,
                0, define_equate, define_equate, 0, 3,
            0, 0, 0, 0, 9,
                0, 0, 2, 1, 0,
            0, 5, insert_index, 0, 0,        // 50
                0, 0, 0, 0, 0,
            0, 0, 5, step_to_member_ID, 2,
                2, 2, 0, 3, 0,
            0, 0, define_equate, 0, 0,
                0, 1, 0, 3, 0,
            0, 0, 0, define_equate, 0,
                0, 0, 0
    };
    
    rtrn = initCicada();
    if (rtrn == passed)  rtrn = attachStartingCode(&NullCode, 0, NULL, 0, NULL, NULL, 0);
    if (rtrn != passed)  {  printf("test_YHErrors:  initCicada failed (%i)\n", (int) rtrn);  return;  }
    
    baseView.windowPtr = Zero;
    baseView.offset = 0;
    baseView.width = 1;
    
    pcSearchPath = ZeroSuspensor;
    
    refWindow(baseView.windowPtr);
    refPath(pcSearchPath);
    
        // Keep all prior codelists, since they are run when a function or constructor is run.
    rtrn = newLinkedList(&CompiledCodes, tYHE_TestRuns, sizeof(linkedlist), 0., false);
    if (rtrn != passed)  {  printf("test_YHErrors: NewLL returned %i\n", (int) rtrn);    }
    
    PCCodeRef = *(code_ref *) element(&(Zero->variable_ptr->codeList), 1);
    
    for (counter = 1; counter <= tYHE_TestRuns; counter++)
        ((linkedlist *) element(&CompiledCodes, counter))->memory = NULL;
    
    testCompiler = newCompiler(cicadaLanguage, cicadaLanguageNumCommands,
                    cicadaLanguageAssociativity, cicadaNumPrecedenceLevels, &rtrn);
    if (rtrn != passed)  {  printf("newCompile() returned %i\n", (int) rtrn);  return;  }
    
    for (counter = 0; counter < tYHE_TestRuns; counter++)  {
        linkedlist *codeLL = (linkedlist *) element(&CompiledCodes, counter+1);
        
        CicadaCodePtr = (char *) TestPrograms[counter];
        rtrn = compile(testCompiler, CicadaCodePtr);
        if (rtrn != passed)  {
            printf("test_YHErrors: counter = %i:  compile() returned %i at:\n%s\n", (int) counter, (int) rtrn, CicadaCodePtr);
            for (cc = 0; cc < errPosition; cc++)  printf(" ");
            printf("^\n");
            break;          }
        
        rtrn = newLinkedList(codeLL, testCompiler->bytecode.elementNum, sizeof(ccInt), 0., false);
        if (rtrn == passed)  rtrn = defragmentLinkedList(codeLL);
        if (rtrn != passed)  {  printf("test_YHErrors: counter = %i:  newLL gave %i\n", (int) counter, (int) rtrn);  break;  }
        copyElements(&(testCompiler->bytecode), 1, codeLL, 1, testCompiler->bytecode.elementNum);
        
        startCodePtr = PCCodeRef.code_ptr = (ccInt *) findElement(codeLL, 1);
        pcCodePtr = startCodePtr;
        errCode = passed;
        storedCode(1)->bytecode = pcCodePtr;
        PCCodeRef.PLL_index = 1;        // for SetError()
        PCCodeRef.references = element(&(MasterCodeList.references), 1);        // for SetError()
        
        runBytecode();
        test_MM_CountRefs();
        
        if (errCode != ErrorCodes[counter])
            printf("test_YHErrors: errCode on counter = %i was %i / expected %i \n", (int) counter, (int) errCode, (int) ErrorCodes[counter]);
        
        if (errCode != passed)
        if (errScript.code_ptr[errIndex-1] != ErrorPtrInts[counter])
            printf("test_YHErrors: bytecode[errIndex] on counter = %i was %i / expected %i \n", (int) counter,
                    (int) errScript.code_ptr[errIndex-1], (int) ErrorPtrInts[counter]);
    }
    freeCompiler(testCompiler);
    
    for (counter = 1; counter <= tYHE_TestRuns; counter++)    {
    if (((linkedlist *) element(&CompiledCodes, counter))->memory != NULL)  {
        deleteLinkedList((linkedlist *) element(&CompiledCodes, counter));
    }}
    
    deleteLinkedList(&CompiledCodes);
    
    cleanUp();
}

// Used for debugging
void test_PrintBytecode(linkedlist *CCode)
{
    ccInt counter;
    
    for (counter = 1; counter <= CCode->elementNum; counter++)    {
        printf("%i ", (int) *LL_int(CCode, counter));    }
        
    printf("\n");
}

// Tries to load various Cicada 'programs' (three ints each) into memory; tests the error codes & error pointers.
void test_checkBytecode()
{
    const ccInt TestBytecodeCodes[][3] = {
        {jump_always, -1, end_of_script}, {jump_always, 1, end_of_script}, {jump_always, 0, end_of_script},        // 0-2
        {jump_always, 2, end_of_script}, {jump_always, -2, end_of_script}, {jump_always, 1, jump_always},     // 3-5
        {end_of_script, end_of_script, end_of_script}, {280, end_of_script, end_of_script}, {commands_num, end_of_script, end_of_script},    // 6-8
        {-1, 0, 1}      };
    
    const ccInt ErrCodes[] = {
        0, 0, bad_jump_err,
        bad_jump_err, bad_jump_err, code_overflow_err,
        inaccessible_code_warning, illegal_command_err, illegal_command_err,
        illegal_command_err     };
    
    const ccInt ErrPtrs[] = {
        0, 0, 2,
        2, 2, 4,
        2, 1, 1,
        1           };
    
    ccInt counter, tCH_TestRuns = sizeof(ErrPtrs) / sizeof(ccInt);
    
    for (counter = 0; counter < tCH_TestRuns; counter++)    {
        ccInt dummyRef;
        startCodePtr = (ccInt *) &(TestBytecodeCodes[counter][0]);
        endCodePtr = startCodePtr + 3;
        PCCodeRef.code_ptr = (ccInt *) &(TestBytecodeCodes[counter][0]);
        PCCodeRef.anchor = NULL;
        PCCodeRef.references = &dummyRef;
        errCode = passed;
        warningCode = passed;
        
        pcCodePtr = startCodePtr;
        checkBytecode();
        if ((errCode == passed) && (warningCode != passed))  {  errCode = warningCode;  errIndex = warningIndex;  }
        
        if (errCode != ErrCodes[counter])
            printf("test_checkBytecode:  counter = %i; checkBytecode returned %i; expected %i\n", (int) counter, (int) errCode, (int) ErrCodes[counter]);
        if ((errIndex != ErrPtrs[counter]) && (errCode != passed))
            printf("test_checkBytecode:  counter = %i; checkBytecode had errIndex %i; expected %i\n", 
                            (int) counter, (int) errIndex, (int) ErrPtrs[counter]);
    }
}


void test_scripts()
{
    const char *testFile = "/Users/brianross/Desktop/cicada/cicada/Test/TestScript.txt";
    linkedlist sourceCode;
    
    newLinkedList(&sourceCode, 0, sizeof(char), 0, false);
    loadFile(testFile, &sourceCode, true);
    
    runCicada(NULL, element(&sourceCode, 1), NULL);
}
