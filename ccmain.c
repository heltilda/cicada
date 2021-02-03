/*
 *  ccmain.c(pp) - Cicada main program
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

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include "lnklst.h"
#include "ccmain.h"
#include "cmpile.h"
#include "intrpt.h"
#include "bytecd.h"


// CicadaTest_ON (in ccmain.h) should always be commented out

#ifdef CicadaTest_ON
extern void TestCicada(void);
extern void test_MM_CountRefs(void);
#endif



// Below:  the lists of error messages for cases where the main script (start.cicada or the argument to Cicada) blows up.
// The provided start.cicada file has its own copy of these lists, since it manually flags errors that occur under its watch.

const char *errorStrings[] = {
    "", "out of memory", "out of range", "initialization error", "mismatched indices",
        "error reading string", "error reading number", "overflow", "underflow", "unknown command",
    "unexpected token", "token expected", "argument expected", "left-hand argument expected", "right-hand argument expected",
        "no left-hand argument allowed", "no right-hand argument allowed", "type mismatch", "illegal command", "code overflow",
    "inaccessible code", "jump to middle of command", "division by zero", "member not found", "variable has no member",
        "no member leads to variable", "member is void", "cannot step to multiple members", "incomplete member", "incomplete variable",
    "invalid index", "multiple indices not allowed", "invalid index", "variable has no parent", "not a variable",
        "not a function", "not composite", "string expected", "illegal target", "target was deleted",
    "unequal data sizes", "not a number", "overlapping alias", "nonexistent Cicada function", "call() can't find C function",
        "wrong number of arguments", "error in argument", "self reference", "recursion depth too high", "I/O error"  };




// * * * * *  The code that boots up Cicada and cleans after it  * * * * *



// runCicada() and cicadaMain() are the outermost functions.  These initializes Cicada's memory,
// load and run the required script start.cicada, flag any error, and then quit.

ccInt runCicada(ccInt argc, char **argv)
{
    ccInt rtrn;
    
    cc_compile_global_struct hold_compile_globals = cc_compile_globals;
    cc_interpret_global_struct hold_interpret_globals = cc_interpret_globals;
    cc_bytecode_global_struct hold_bytecode_globals = cc_bytecode_globals;
    
    rtrn = cicadaMain(argc, argv);
    
    cc_compile_globals = hold_compile_globals;
    cc_interpret_globals = hold_interpret_globals;
    cc_bytecode_globals = hold_bytecode_globals;
    
    return rtrn;
}


ccInt cicadaMain(ccInt argc, char **argv)
{
    ccInt mainErrorCode = 0, startupFileNameLength, loopCompiler, rtrn;
    char *firstChar, *startupFileName;
    const char *defaultStartupFileName = "start.cicada";
    linkedlist sourceCode;
    
#ifdef CicadaTest_ON
    if (argc == 0)  TestCicada();
#endif
    
    
        // Initialize the random number generator.
        
    srand((int) time(0));
    rand();  rand();  rand();  rand();      // otherwise the first couple of random numbers are suspiciously consistent
    
    
        // initialize the compiler
    
    rtrn = newLinkedList(&allCompilers, 1, sizeof(compiler_type *), 1., ccFalse);
    if (rtrn == passed)  {
        whichCompiler = 1;
        currentCompiler = newCompiler(cicadaLanguage, cicadaLanguageNumCommands,
                        cicadaLanguageAssociativity, cicadaNumPrecedenceLevels, &rtrn);
        *(compiler_type **) element(&allCompilers, 1) = currentCompiler;         }
    
    if (rtrn != passed)  {
        printf("Error loading compiler (error code %i on command %i)\n", (int) errCode, (int) errPosition);
        return rtrn;       }
    
    
        // Set up a buffer for loading the script
    
    rtrn = newLinkedList(&sourceCode, 0, sizeof(char), 0, ccFalse);
    if (rtrn != passed)  {  printf("cicada:  %s\n", errorStrings[rtrn]);  return 1;  }
    
    
        // Open the file specified in the first argument, if given; otherwise, open start.cicada
    
    if (argc > 1)  {  printf("usage: cicada [source_file]\n");  return 1;  }
    
    if (argc == 0)  startupFileName = (char *) defaultStartupFileName;
    else  startupFileName = (char *) argv[0];
    
    startupFileNameLength = 0;
    while (startupFileName[startupFileNameLength] != 0)  startupFileNameLength++;
    
    if (loadFile(startupFileName, &sourceCode, ccTrue) != passed)  {
        printf("Error:  Could not open %s\n", startupFileName);
        return 1;      }
    
    rtrn = defragmentLinkedList(&sourceCode);
    if (rtrn != passed)  {  printf("cicada:  %s\n", errorStrings[rtrn]);  return 1;  }
    
    
        // (compile and) transform the starting script.
    
    firstChar = (char *) element(&sourceCode, 1);
    rtrn = compile(currentCompiler, firstChar);
    
    if (rtrn != passed)  {
        errCode = rtrn;
        printError(startupFileName, firstChar, errPosition-1, ccFalse);
        return rtrn;    }
    else if (compilerWarning != passed)  {
        warningCode = compilerWarning;
        printError(startupFileName, firstChar, errPosition-1, ccFalse);     }
    
    defragmentLinkedList(&(currentCompiler->opCharNum));
    
    
        // initialize all the global variables used by Cicada
    
    rtrn = initCicada(LL_int(&(currentCompiler->bytecode), 1), currentCompiler->bytecode.elementNum,
            startupFileName, startupFileNameLength, firstChar, LL_int(&(currentCompiler->opCharNum), 1), sourceCode.elementNum);
    if (rtrn != passed)  {  printf("Cicada error:  initialization returned \"%s\"\n", (char *) errorStrings[rtrn]);  return 1;  }
    
    
        // check the bytecode of the script (should be OK if we're using the 'default' Cicada language)
    
    startCodePtr = PCCodeRef.code_ptr;
    endCodePtr = startCodePtr + currentCompiler->bytecode.elementNum;
    
    pcCodePtr = startCodePtr;
    checkBytecode();
    
    if ((errCode != passed) || (warningCode != passed))  {
        
        printError(startupFileName, firstChar, *LL_int(&(currentCompiler->opCharNum), errIndex)-1, ccFalse);
        
        if (errCode != passed)  return errCode;         }
    
    
        // run the script
    
    beginExecution(&PCCodeRef, ccFalse, 0, 1, 0);
    if (errCode == return_flag)  errCode = passed;
    
    
        // if there was an error in the script, print out the line that caused it (if possible)
    
    if ((errCode != passed) && (errCode != finished_signal))  {
        
        storedCodeType *errorBaseScript = storedCode(errScript.PLL_index);
        ccInt errCharNum = 0;
        
        if (errorBaseScript->opCharNum != NULL)  errCharNum = errorBaseScript->opCharNum[
                    errIndex + (ccInt) (errScript.code_ptr - errorBaseScript->bytecode) - 1] - 1;
        
        printError(errorBaseScript->fileName, errorBaseScript->sourceCode, errCharNum, ccFalse);
        
        mainErrorCode = 1;         }
    
    
        // clean up after ourselves and exit
    
#ifdef CicadaTest_ON
test_MM_CountRefs();
#endif
    
    cleanUp();
    
    for (loopCompiler = 1; loopCompiler <= allCompilers.elementNum; loopCompiler++)
        freeCompiler(*(compiler_type **) element(&allCompilers, loopCompiler));
    deleteLinkedList(&allCompilers);
    
    deleteLinkedList(&sourceCode);
    
    return mainErrorCode;
}


// initCicada() initializes the interpreter when Cicada boots up.
// Allocates memory for things like VariableList, the PCStack, Zero, etc.

ccInt initCicada(ccInt *zeroCode, ccInt zeroCodeLength,
        char *fileName, ccInt fileNameLength, char *sourceCode, ccInt *opCharNum, ccInt sourceCodeLength)
{
    variable *varZero;
    ccInt zeroCodeIndex, *zeroCodeEntryPtr, rtrn;
    
    baseView.windowPtr = NULL;              // these can confuse addMemory if not initialized to NULL first
    searchView.windowPtr = NULL;            // to prevent an error on startup when addMemory checks baseView.windowPtr->variable_ptr
    topView.windowPtr = NULL;
    
    rtrn = newPLL(&VariableList, 0, sizeof(variable), LLFreeSpace);        // make space for variables & codes
    if (rtrn != passed)  return rtrn;
    rtrn = newPLL(&MasterCodeList, 0, sizeof(storedCodeType), LLFreeSpace);
    if (rtrn != passed)  return rtrn;
    rtrn = newStack(&PCStack, sizeof(view), 100, LLFreeSpace);
    if (rtrn != passed)  return rtrn;
    
    rtrn = addVariable(&varZero, composite_type, composite_type, 0, ccTrue);
    if (rtrn != passed)  return rtrn;
    rtrn = addWindow(varZero, 0, 1, &Zero, ccTrue);
    if (rtrn != passed)  return rtrn;
    rtrn = drawPath(&ZeroSuspensor, Zero, NULL, 1, 1);
    if (rtrn != passed)  return rtrn;
    
    rtrn = addCode(zeroCode, &zeroCodeEntryPtr, &zeroCodeIndex, zeroCodeLength,
                fileName, fileNameLength, sourceCode, opCharNum, sourceCodeLength);
    if (rtrn != passed)  return rtrn;
    rtrn = addCodeRef(&(varZero->codeList), ZeroSuspensor, zeroCodeEntryPtr, zeroCodeIndex);
    if (rtrn != passed)  return rtrn;
    PCCodeRef = *(code_ref *) element(&(varZero->codeList), 1);
    
    refWindow(Zero);
    refPath(ZeroSuspensor);
    
    pcCodePtr = zeroCodeEntryPtr;        // should immediately call checkBytecode()
    
    rtrn = newLinkedList(&(GL_Object.arrayDimList), 0, sizeof(ccInt), 2., ccFalse);
    if (rtrn != passed)  return rtrn;
    rtrn = newLinkedList(&codeRegister, 0, sizeof(code_ref), 0., ccFalse);      // used in bytecd.c to pass codes from code blocks
    if (rtrn != passed)  return rtrn;
    
    rtrn = newLinkedList(&stringRegister, 0, sizeof(char), 0., ccFalse);     // set up the string register
    if (rtrn != passed)  return rtrn;
    
    errCode = warningCode = passed;
    errIndex = 1;              // for now
    errScript.code_ptr = NULL;
    warningScript.code_ptr = NULL;
    
    recursionCounter = 0;
    pcSearchPath = NULL;
    BIF_argsView.windowPtr = NULL;
    argsView.windowPtr = NULL;
    returnView.windowPtr = NULL;
    thatView.windowPtr = NULL;
    
    extCallMode = ccFalse;
    doPrintError = ccFalse;
    
    return passed;
}


// cleanUp() deletes the global variables when Cicada exits.  Just to be tidy.

void cleanUp()
{
    derefCodeList(&codeRegister);
    
    deletePLL(&VariableList);
    deletePLL(&MasterCodeList);
    deleteStack(&PCStack);
    deleteLinkedList(&codeRegister);
    deleteLinkedList(&(GL_Object.arrayDimList));
    deleteLinkedList(&stringRegister);
    
    return;
}



// printError() converts the character number of an error into a line number, then outputs
// that line number along with the line of text and an error flagging the offending character.

void printError(char *errFileName, char *errText, ccInt errCharNum, ccBool forceLineNo)
{
    
        // print the error/warning message, if there is one
    
    if (errCode != passed)  {
        if ((errCode < passed) || (errCode > finished_signal))  printf("Unknown error #%i\n", (int) errCode);
        else if ((errCode == return_flag) || (errCode == finished_signal))  return;
        else if (errCode == token_expected_err)  printf("Error:  '%s' expected", expectedTokenName);
        else if (errCode != member_not_found_err)  printf("Error:  %s", errorStrings[errCode]);
        else  {
            const char *defaultMemberName = "";
            char *memberName = (char *) defaultMemberName;
            ccInt soughtMember = *(errScript.code_ptr + errIndex - 1);
            
            if ((soughtMember >= 1) && (soughtMember <= currentCompiler->varNames.elementNum))  {
                memberName = ((varNameType *) element(&(currentCompiler->varNames), soughtMember))->theName;    }
            
            printf("Error:  member '%s' not found", memberName);
    }   }
    
    else if (warningCode != passed)  {
        if ((warningCode < passed) || (warningCode > finished_signal))  printf("Unknown warning #%i\n", (int) warningCode);
        printf("Warning:  %s", errorStrings[warningCode]);
        warningCode = passed;          }
    
    else  return;
    
    
        // print the file that caused the error, if its name was recorded
    
    if (errFileName != NULL)  printf(" in file %s", errFileName);
    printf("\n");
    
    
        // if we have the source text, flag the line that caused the error
    
    if (errText != NULL)  {
        
        ccInt lineNo = 1, loopChar, lineBeginningChar, lineEndChar;
        int numLineNoChars;
        
        lineBeginningChar = lineEndChar = errCharNum;
        
        while ((errText[lineBeginningChar] != '\n') && (lineBeginningChar > 0))  lineBeginningChar--;
        if (errText[lineBeginningChar] == '\n')  lineBeginningChar++;
        
        while ((errText[lineEndChar] != '\n') && (errText[lineEndChar] != '\x00'))  lineEndChar++;
        if (errText[lineEndChar] == '\n')  lineEndChar--;
        
        printf("\n");
        
        
            // calculate and print out the line number (if we have a file name)
        
        if ((errFileName == NULL) && (!forceLineNo))  numLineNoChars = 0;
        else  {
            
            for (loopChar = 0; loopChar < lineBeginningChar; loopChar++)  {
            if (errText[loopChar] == '\n')  {
                lineNo++;
            }}
            
            printf("%i:  %n", (int) lineNo, &numLineNoChars);         }
        
        
            // .. and the line in the script that caused it..
        
        for (loopChar = lineBeginningChar; loopChar <= lineEndChar; loopChar++)  {
        if (lettertype(errText + loopChar) != unprintable)  {
            if (errText[loopChar] == '\t')  printf(" ");
            else  printf("%c", errText[loopChar]);
        }}
        printf("\n");
        
        
            // .. and finally a marker indicating the offending character in the line.
        
        for (loopChar = 1; loopChar <= (ccInt) numLineNoChars; loopChar++)  {
            printf(" ");        }
        
        for (loopChar = lineBeginningChar; loopChar < errCharNum; loopChar++)  {
        if (lettertype(errText + loopChar) != unprintable)  {
            printf(" ");
        }}
        printf("^\n");        }
}
