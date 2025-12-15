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
#include <string.h>
#include "lnklst.h"
#include "ciclib.h"
#include "ccmain.h"
#include "cmpile.h"
#include "intrpt.h"
#include "bytecd.h"


// CicadaTest_ON (in ccmain.h) should always be commented out

#ifdef CicadaTest_ON
extern void TestCicada(void);
extern void test_MM_CountRefs(void);
#endif



// Below:  the lists of error messages for cases where the starting script blows up.
// The provided terminal.c file has its own copy of these lists, since it manually flags errors that occur under its watch.

const char *errorStrings[] = {
    "", "out of memory", "out of range", "initialization error", "mismatched indices",
        "error reading string", "error reading number", "overflow", "underflow", "unknown command",
    "unexpected token", "token expected", "argument expected", "left-hand argument expected", "right-hand argument expected",
        "no left-hand argument allowed", "no right-hand argument allowed", "type mismatch", "illegal command", "code overflow",
    "inaccessible code", "jump to middle of command", "division by zero", "member not found", "variable has no member",
        "no member leads to variable", "member is void", "cannot step to multiple members", "incomplete member", "incomplete variable",
    "invalid index", "multiple indices not allowed", "invalid index", "variable has no parent", "not a variable",
        "not a function", "not composite", "string expected", "illegal target", "target was deleted",
    "unequal data sizes", "not a number", "overlapping alias", "thrown here", "nonexistent C function",
        "wrong number of arguments", "error in argument", "self reference", "recursion depth too high", "I/O error"  };

const char *defsScript =
#include "defs.c"
;

const char *terminalScript =
#include "terminal.c"
;

const char *userDefsScript;


// * * * * *  The code that boots up Cicada and cleans after it  * * * * *



// runCicada() and cicadaMain() are the outermost functions.  These initialize Cicada's memory,
// load and run the required script stored in terminal.c, flag any error, and then quit.

ccInt runCicadaMain(const Cfunction *userCfunctions, const ccInt numUserCfunctions, const char *scriptToRun, const bool runTerminal)
{
    ccInt rtrn;
    
    cc_compile_global_struct hold_compile_globals = cc_compile_globals;
    cc_interpret_global_struct hold_interpret_globals = cc_interpret_globals;
    cc_bytecode_global_struct hold_bytecode_globals = cc_bytecode_globals;
    
    rtrn = cicadaMain(userCfunctions, numUserCfunctions, scriptToRun, runTerminal);
    
    cc_compile_globals = hold_compile_globals;
    cc_interpret_globals = hold_interpret_globals;
    cc_bytecode_globals = hold_bytecode_globals;
    
    return rtrn;
}


ccInt cicadaMain(const Cfunction *CfunctionsFromUser, const ccInt numUserCfunctions, const char *scriptToRun, const bool runTerminal)
{
    compiler_type *baseCompiler = NULL;
    ccInt mainErrorCode = 0, scriptNameLength, scriptLength, loopCompiler, rtrn, cs, firstScriptToRun;
    ccInt *bytecodePtr = NULL, bytecodeLength = 0, c2, cf;
    const char *scriptPtr[2], *scriptName[2];
    const Cfunction *functionLists[2] = { inbuiltCfunctions, CfunctionsFromUser };
    const int functionsNum[2] = { inbuiltCfunctionsNum, numUserCfunctions };
    const char ***functionArgs[2] = { &inbuiltCfunctionArgs, &userCfunctionArgs };
    
    if ((scriptToRun == NULL) && (!runTerminal))  return passed;
    
#ifdef CicadaTest_ON
    if (scriptToRun == NULL)  TestCicada();
#endif
    
    
        // Initialize the random number generator.
    
    srand((int) time(0));
    rand();  rand();  rand();  rand();      // otherwise the first couple of random numbers are suspiciously consistent
    
    
        // Initialize the C function lists
    
    for (c2 = 0; c2 < 2; c2++)  {           // for functions with a ':' in their names, store the arg types
        *(functionArgs[c2]) = (const char **) malloc(functionsNum[c2]*sizeof(char *));
        if (*(functionArgs[c2]) == NULL)  return out_of_memory_err;
        for (cf = 0; cf < functionsNum[c2]; cf++)  {
            const char **fName = &((*(functionArgs[c2]))[cf]);
            *fName = functionLists[c2][cf].functionName;
            while ((**fName != ':') && (**fName != 0))  (*fName)++;
            if (**fName == ':')  (*fName)++;
    }   }
    
    userCfunctions = CfunctionsFromUser;
    userCfunctionsNum = numUserCfunctions;
    
    numCfunctions = inbuiltCfunctionsNum + userCfunctionsNum;
    numIBCfunctions = inbuiltCfunctionsNum;
    inbuiltCFs = inbuiltCfunctions;
    userCFs = userCfunctions;
    
    
        // Initialize the compiler
    
    rtrn = newLinkedList(&allCompilers, 1, sizeof(compiler_type *), 1., false);
    if (rtrn == passed)  baseCompiler = newCompiler(cicadaLanguage, cicadaLanguageNumCommands,
                    cicadaLanguageAssociativity, cicadaNumPrecedenceLevels, &rtrn);
    if (rtrn != passed)  {
        printf("Error loading compiler (error code %i on command %i)\n", (int) rtrn, (int) errPosition);
        return rtrn;       }
    
    *(compiler_type **) element(&allCompilers, 1) = baseCompiler;
    
    
        // Initialize the interpreter
    
    rtrn = initCicada();
    if (rtrn != passed)  {  printf("Error:  initialization returned \"%s\"\n", (char *) errorStrings[rtrn]);  return 1;  }
    
    
        // Set the script to either the terminal script or a user-provided script
    
    if (runTerminal)  {             // the terminal manually preloads defs.cicada
        scriptName[1] = "terminal script";
        scriptPtr[1] = terminalScript;
        firstScriptToRun = 1;
        userDefsScript = scriptToRun;        }
    else  {
        scriptName[0] = "predefinitions script";
        scriptName[1] = "user script";
        scriptPtr[0] = defsScript;
        scriptPtr[1] = scriptToRun;
        firstScriptToRun = 0;       }
        
        
            // Compile & run all scripts
    
    for (cs = firstScriptToRun; cs < 2; cs++)  {
        scriptLength = scriptNameLength = 0;
        while (scriptPtr[cs][scriptLength] != 0)  scriptLength++;
        while (scriptName[cs][scriptNameLength] != 0)  scriptNameLength++;
        
        rtrn = compile(baseCompiler, scriptPtr[cs]);
        
        if (rtrn != passed)  {
            errCode = rtrn;
            printError(scriptName[cs], scriptNameLength, scriptPtr[cs], errPosition-1, false, 1, false);
            return rtrn;    }
        else if (compilerWarning != passed)  {
            warningCode = compilerWarning;
            printError(scriptName[cs], scriptNameLength, scriptPtr[cs], errPosition-1, false, 1, false);     }
        
        bytecodePtr = LL_int(&baseCompiler->bytecode, 1);
        bytecodeLength = baseCompiler->bytecode.elementNum;
        defragmentLinkedList(&(baseCompiler->opCharNum));
        
        rtrn = attachStartingCode(bytecodePtr, bytecodeLength,
                scriptName[cs], scriptNameLength, scriptPtr[cs], LL_int(&(baseCompiler->opCharNum), 1), scriptLength);
        if (rtrn != passed)  {  printf("Error:  code initialization returned \"%s\"\n", (char *) errorStrings[rtrn]);  return 1;  }
        
        
            // check the bytecode of the script (should be OK if we're using the 'default' Cicada language)
        
        pcCodePtr = startCodePtr = PCCodeRef.code_ptr;
        endCodePtr = startCodePtr + bytecodeLength;
        checkBytecode();
        
        if ((errCode != passed) || (warningCode != passed))  {
            printError(scriptName[cs], scriptNameLength, scriptPtr[cs], *LL_int(&(baseCompiler->opCharNum), errIndex)-1, false, 1, false);
            if (errCode != passed)  return errCode;
        }
        
        
        
            // run the script
        
        beginExecution(&PCCodeRef, false, 0, 1, 0);
        if (errCode == return_flag)  errCode = passed;
        
        
            // if there was an error in the script, print out the line that caused it (if possible)
        
        if ((errCode != passed) && (errCode != finished_signal))  {
            
            storedCodeType *errorBaseScript = storedCode(errScript.PLL_index);
            ccInt errCharNum = 0;
            
            if (errorBaseScript->opCharNum != NULL)  errCharNum = errorBaseScript->opCharNum[
                        errIndex + (ccInt) (errScript.code_ptr - errorBaseScript->bytecode) - 1] - 1;
            
            printError(errorBaseScript->fileName, -1, errorBaseScript->sourceCode, errCharNum, false, errorBaseScript->compilerID, false);
            
            mainErrorCode = 1;
    }   }
    
    
        // Clean up after ourselves and exit
    
#ifdef CicadaTest_ON
test_MM_CountRefs();
#endif
    
    cleanUp();
    
    for (loopCompiler = 1; loopCompiler <= allCompilers.elementNum; loopCompiler++)
        freeCompiler(*(compiler_type **) element(&allCompilers, loopCompiler));
    deleteLinkedList(&allCompilers);
    
    return mainErrorCode;
}


// initCicada() initializes the interpreter when Cicada boots up.
// Allocates memory for things like VariableList, the PCStack, Zero, etc.

ccInt initCicada()
{
    variable *varZero;
    ccInt rtrn;
    const static ccInt compositeTypeInt = composite_type;
    
    baseView.windowPtr = searchView.windowPtr = topView.windowPtr = NULL;   // these can confuse addMemory if not initialized to NULL first
    
    rtrn = newPLL(&VariableList, 0, sizeof(variable), LLFreeSpace);         // make space for variables & codes
    if (rtrn != passed)  return rtrn;
    rtrn = newPLL(&MasterCodeList, 0, sizeof(storedCodeType), LLFreeSpace);
    if (rtrn != passed)  return rtrn;
    rtrn = newStack(&PCStack, sizeof(view), 100, LLFreeSpace);
    if (rtrn != passed)  return rtrn;
    
    rtrn = addVariable(&varZero, &compositeTypeInt, 0, true);
    if (rtrn != passed)  return rtrn;
    rtrn = addWindow(varZero, 0, 1, &Zero, true);
    if (rtrn != passed)  return rtrn;
    rtrn = drawPath(&ZeroSuspensor, Zero, NULL, 1, 1);
    if (rtrn != passed)  return rtrn;
    
    rtrn = newLinkedList(&(GL_Object.arrayDimList), 0, sizeof(ccInt), 2., false);
    if (rtrn != passed)  return rtrn;
    rtrn = newLinkedList(&codeRegister, 0, sizeof(code_ref), 0., false);      // used in bytecd.c to pass codes from code blocks
    if (rtrn != passed)  return rtrn;
    
    rtrn = newLinkedList(&stringRegister, 0, sizeof(char), 0., false);        // set up the string register
    if (rtrn != passed)  return rtrn;
    
    errCode = warningCode = passed;
    errIndex = 1;              // for now
    errScript.code_ptr = NULL;
    warningScript.code_ptr = NULL;
    
    recursionCounter = 0;
    pcSearchPath = NULL;
    argsView.windowPtr = NULL;
    returnView.windowPtr = NULL;
    thatView.windowPtr = NULL;
    
    extCallMode = false;
    doPrintError = false;
    
    return passed;
}


// attachStartingCode() adds a new script to Zero's code list.

ccInt attachStartingCode(ccInt *zeroCode, ccInt zeroCodeLength,
        const char *fileName, ccInt fileNameLength, const char *sourceCode, ccInt *opCharNum, ccInt sourceCodeLength)
{
    variable *varZero = Zero->variable_ptr;
    ccInt zeroCodeIndex, *zeroCodeEntryPtr, rtrn;
    
    rtrn = addCode(zeroCode, &zeroCodeEntryPtr, &zeroCodeIndex, zeroCodeLength,
                fileName, fileNameLength, sourceCode, opCharNum, sourceCodeLength, 1);
    if (rtrn != passed)  return rtrn;
    rtrn = addCodeRef(&(varZero->codeList), ZeroSuspensor, zeroCodeEntryPtr, zeroCodeIndex);
    if (rtrn != passed)  return rtrn;
    PCCodeRef = *(code_ref *) element(&(varZero->codeList), varZero->codeList.elementNum);
    
    if (varZero->codeList.elementNum == 1)  {
        refWindow(Zero);
        refPath(ZeroSuspensor);     }
    
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
    
    free(inbuiltCfunctionArgs);
    free(userCfunctionArgs);
    
    return;
}



// printError() converts the character number of an error into a line number, then outputs
// that line number along with the line of text and an error flagging the offending character.

void printError(const char *errScriptName, ccInt errFileNameLength,
        const char *errText, ccInt errCharNum, bool forceLineNo, const ccInt compilerID, const bool ignoreMemberName)
{
    bool fromTerminal = false;
    
    if (errScriptName != NULL)  fromTerminal = (strcmp(errScriptName, "terminal command") == 0);
    
    
        // print the error/warning message, if there is one
    
    if (errCode != passed)  {
        if ((errCode < passed) || (errCode > finished_signal))  printf("Unknown error #%i\n", (int) errCode);
        else if ((errCode == return_flag) || (errCode == finished_signal))  return;
        else if (errCode == token_expected_err)  printf("Error:  '%s' expected", expectedTokenName);
        else if (errCode == thrown_to_err)  {
            if (errScriptName != NULL)  printf("Error was thrown");
            else  printf("Error was thrown at unnamed script");     }
        else if (errCode != member_not_found_err)  printf("Error:  %s", errorStrings[errCode]);
        else  {
            const char *defaultMemberName = "";
            char *memberName = (char *) defaultMemberName, *str0 = (char *) defaultMemberName, *strf = (char *) defaultMemberName;
            ccInt soughtMember = *(errScript.code_ptr + errIndex - 1);
            compiler_type *theCompiler = *(compiler_type **) element(&allCompilers, compilerID);
            
            if (!ignoreMemberName)  {
            if ((soughtMember >= 1) && (soughtMember <= theCompiler->varNames.elementNum))  {
                str0 = " '";  strf = "'";
                memberName = ((varNameType *) element(&(theCompiler->varNames), soughtMember))->theName;
            }}
            
            printf("Error:  member%s%s%s not found", str0, memberName, strf);
    }   }
    
    else if (warningCode != passed)  {
        if ((warningCode < passed) || (warningCode > finished_signal))  printf("Unknown warning #%i\n", (int) warningCode);
        printf("Warning:  %s", errorStrings[warningCode]);
        warningCode = passed;          }
    
    else  return;
    
    
        // print the file that caused the error, if its name was recorded
    
    if ((errScriptName != NULL) && (errFileNameLength != 0))  {
    if ((errCode == thrown_to_err) || (!fromTerminal))  {
        if (errFileNameLength < 0)  printf(" in %s", errScriptName);
        else  printf(" in %.*s", errFileNameLength, errScriptName);
    }}
    printf("\n");
    
    
        // if we have the source text, flag the line that caused the error
    
    if (errText != NULL)  {
    if ((errCode == thrown_to_err) || (!fromTerminal))  {
        
        ccInt lineNo = 1, loopChar, lineBeginningChar, lineEndChar;
        int numLineNoChars;
        
        lineBeginningChar = lineEndChar = errCharNum;
        
        while ((errText[lineBeginningChar] != '\n') && (lineBeginningChar > 0))  lineBeginningChar--;
        if (errText[lineBeginningChar] == '\n')  lineBeginningChar++;
        
        while ((errText[lineEndChar] != '\n') && (errText[lineEndChar] != '\x00'))  lineEndChar++;
        if (errText[lineEndChar] == '\n')  lineEndChar--;
        
        printf("\n");
        
        
            // calculate and print out the line number (if we have a file name)
        
        if ((errScriptName == NULL) && (!forceLineNo))  numLineNoChars = 0;
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
        printf("^\n");
    }}
}
