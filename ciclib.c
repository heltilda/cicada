/*
 *  ciclib.c(pp) - built-in Cicada functions
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

#include <math.h>
#include <float.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "ciclib.h"
#include "cmpile.h"
#include "bytecd.h"
#include "ccmain.h"
#include "lnklst.h"


// **********************************************

// inbuiltFunctions[] and userFunctions[] define the { names, function addresses } of user's C routines.
// Each routine must be of the form:  ccInt RoutineName(argType args)



#define CCname(n) {#n,&cc_n}

const Cfunction inbuiltFunctions[] = {
    { "newCompiler", &cc_newCompiler }, { "compile", &cc_compile }, { "getMemberNames", &cc_getMemberNames },
    { "transform:dwwddd", &cc_transform }, { "trap:a", &cc_trap }, { "throw:ddwdd", &cc_throw },
    { "top:wd", &cc_top }, { "size:wdd", &cc_size }, { "type:wdd", &cc_type }, { "member_ID:wdd", &cc_member_ID }, { "bytecode:wdd", &cc_bytecode },
    { "load", &cc_load }, { "save", &cc_save }, { "input", &cc_input }, { "print", &cc_print },
    { "read_string", &cc_read_string}, { "print_string", &cc_print_string }, { "find", &cc_find },
    { "random", &cc_random }, { "abs", &cc_abs }, { "floor", &cc_floor }, { "ceil", &cc_ceil }, { "exp", &cc_exp }, { "log", &cc_log },
    { "cos", &cc_cos }, { "sin", &cc_sin }, { "tan", &cc_tan }, { "acos", &cc_acos }, { "asin", &cc_asin }, { "atan", &cc_atan },
    { "add", &cc_add }, { "subtract", &cc_subtract }, { "multiply", &cc_multiply }, { "divide", &cc_divide }, { "pow", &cc_pow },
    { "minmax", &cc_minmax }, { "sum", &cc_sum }, { "makeLinkList", &cc_makeLinkList }, { "sort", &cc_sort },
    { "springCleaning:w", &cc_springCleaning }
};
const ccInt inbuiltFunctionsNum = (ccInt) (sizeof(inbuiltFunctions)/sizeof(Cfunction));
const char **inbuiltFunctionsArgs;


const Cfunction *userFunctions; 
ccInt userFunctionsNum;
const char **userFunctionsArgs;

ccInt passbackErrCode;
#define returnOnErr(x) if((passbackErrCode=x)!=passed) return passbackErrCode;

    
        // if we pass two arguments, that means that we're defining a new compiler
    
ccInt cc_newCompiler(argsType args)
{
    compiler_type *tempCompiler = NULL;
    commandTokenType *cmdTokens;
    linkedlist *cmdStrings, *cmdReturnTypes, *cmdTranslations;
    ccInt *cmdPrecedences, *precedenceLevelAssociativity, *compilerID;
    ccInt numCommands, numPrecedenceLevels, cmdNum, rtrn = passed;
    
    if (args.num != 6)  return wrong_argument_count_err;
    
    returnOnErr(checkArgs(args, vector(string_type), vector(int_type), vector(string_type),
                    vector(string_type), vector(int_type), scalar(int_type)))
    getArgs(args, &cmdStrings, &cmdPrecedences, &cmdReturnTypes, &cmdTranslations, &precedenceLevelAssociativity, &compilerID);
    
    numCommands = args.indices[0];
    if ((args.indices[1] != numCommands) || (args.indices[2] != numCommands) || (args.indices[3] != numCommands))  return library_argument_err;
    numPrecedenceLevels = args.indices[4];
    
    cmdTokens = (commandTokenType *) malloc(numCommands*sizeof(commandTokenType));
    if (cmdTokens == NULL)  return out_of_memory_err;
      
    for (cmdNum = 0; cmdNum < numCommands; cmdNum++)  {
        cmdTokens[cmdNum].cmdString = LL2Cstr(cmdStrings + cmdNum);
        cmdTokens[cmdNum].precedence = cmdPrecedences[cmdNum];
        cmdTokens[cmdNum].rtrnTypeString = LL2Cstr(cmdReturnTypes + cmdNum);
        cmdTokens[cmdNum].translation = LL2Cstr(cmdTranslations + cmdNum);
        if ((cmdTokens[cmdNum].cmdString == NULL) || (cmdTokens[cmdNum].rtrnTypeString == NULL) || (cmdTokens[cmdNum].translation == NULL))  {
            return out_of_memory_err;
    }   }
    
    tempCompiler = newCompiler(cmdTokens, numCommands, precedenceLevelAssociativity, numPrecedenceLevels, &rtrn);
    if (rtrn == passed)  {
        returnOnErr(addElements(&allCompilers, 1, ccFalse))
        *compilerID = allCompilers.elementNum;
        *(compiler_type **) element(&allCompilers, *compilerID) = tempCompiler;
    }
    
    for (cmdNum = 0; cmdNum < numCommands; cmdNum++)  {
        free((void *) cmdTokens[cmdNum].cmdString);
        free((void *) cmdTokens[cmdNum].rtrnTypeString);
        free((void *) cmdTokens[cmdNum].translation);        }
    free((void *) cmdTokens);
    
    return rtrn;
}




// compile() generates bytecode from a script (a string).  Writes errors/warnings into the error/warning registers.

ccInt cc_compile(argsType args)
{
    compiler_type *theCompiler;
    linkedlist *scriptString, *fileName, *characterPositions, *scriptBytecode;
    ccInt *numMemberNames, compilerID, rtrn;
    char *scriptCopy;
    
    if (args.num != 6)  return wrong_argument_count_err;
    
    returnOnErr(checkArgs(args, scalar(int_type), scalar(string_type), scalar(string_type),
                scalar(string_type), scalar(int_type), scalar(string_type)))
    getArgs(args, byValue(&compilerID), &scriptString, &fileName, &characterPositions, &numMemberNames, &scriptBytecode);
    
    theCompiler = *(compiler_type **) element(&allCompilers, compilerID);
    
    scriptCopy = LL2Cstr(scriptString);
    
    
        // Run the compiler.  Store the error code/index (which may be zero -- no error).
    
    rtrn = compile(theCompiler, (const char *) scriptCopy);
    
    if (((rtrn != passed) || (compilerWarning != passed)) && (doPrintError))  {
        
        const char *fileNamePtr = NULL;
        
        if (fileName->elementNum > 0)  {
            returnOnErr(defragmentLinkedList(fileName))
            fileNamePtr = (const char *) element(fileName, 1);      }
        
        if (rtrn != passed)  setError(rtrn, pcCodePtr-1);
        else  setWarning(compilerWarning, pcCodePtr-1);
        
        printError(fileNamePtr, fileName->elementNum, (const char *) scriptCopy, errPosition-1, ccFalse, compilerID, ccFalse);
        doPrintError = ccFalse;             }
    
    free((void *) scriptCopy);
    
    if (rtrn != passed)  return rtrn;
    
    
        // store the bytecode string
    
    returnOnErr(resizeLinkedList(scriptBytecode, theCompiler->bytecode.elementNum * sizeof(ccInt), ccFalse))
    returnOnErr(defragmentLinkedList(scriptBytecode))
    
    getElements(&(theCompiler->bytecode), 1, theCompiler->bytecode.elementNum, element(scriptBytecode, 1));
    
    returnOnErr(resizeLinkedList(characterPositions, theCompiler->opCharNum.elementNum*sizeof(ccInt), ccFalse))
    returnOnErr(defragmentLinkedList(characterPositions))
    getElements(&(theCompiler->opCharNum), 1, theCompiler->opCharNum.elementNum, (void *) element(characterPositions, 1));
    
    *numMemberNames = theCompiler->varNames.elementNum;
    
    return passed;
}


ccInt cc_getMemberNames(argsType args)
{
    compiler_type *theCompiler;
    linkedlist *memberNames;
    ccInt compilerID, nameID, prevNumNames;
    
    if (args.num != 3)  return wrong_argument_count_err;
    
    returnOnErr(checkArgs(args, scalar(int_type), vector(string_type), scalar(int_type)))
    getArgs(args, byValue(&compilerID), &memberNames, byValue(&prevNumNames));
    
    theCompiler = *(compiler_type **) element(&allCompilers, compilerID);
    
    for (nameID = prevNumNames; (nameID < theCompiler->varNames.elementNum) && (nameID < args.indices[1]); nameID++)  {
        varNameType *compilerVarName = (varNameType *) element(&(theCompiler->varNames), nameID+1);
        returnOnErr(resizeLinkedList(memberNames + nameID, compilerVarName->nameLength, ccFalse))
        setElements(memberNames + nameID, 1, compilerVarName->nameLength, compilerVarName->theName);
    }
    
    return passed;
}


// cc_transform() takes bytecode and stores it in Cicada's code registry.
// Checks the code first to make sure it won't crash the interpreter.

ccInt cc_transform(argsType args)
{
    linkedlist *inputStringLL, *fileNameLL, *sourceCodeLL, *opCharPositionsLL;
    linkedlist **lastArgLLs[3] = { &fileNameLL, &sourceCodeLL, &opCharPositionsLL };
    member *loopMember;
    window *compiledCodeWindow, *pathWindow, *drawPathTo;
    searchPath *newCodePath, *newCodePathStem = pcSearchPath;
    ccInt pathVarMemberCounter, firstPathVarMember = 0, counter, loopCodeWord, rtrn, cs;
    ccInt codeIndex, dummy, memberOffset;
    ccInt *holdPCCodePtr, *codeEntryPtr, *holdStartCodePtr, *holdEndCodePtr, *opCharNum = NULL, theOpChar;
    char *fileName = NULL, *sourceCode = NULL;
    char **lastArgs[3] = { &fileName, &sourceCode, (char **) &opCharNum };
    code_ref holdPCCodeRef;
    
    if (args.num != 6)  return wrong_argument_count_err;
    
    returnOnErr(checkArgs(args, scalar(string_type), scalar(composite_type), endArgs))
    if (args.p[2] != NULL)  returnOnErr(checkArgs(args, fromArg(2), scalar(composite_type), endArgs))
    returnOnErr(checkArgs(args, fromArg(3), scalar(string_type), scalar(string_type), scalar(string_type)))
    getArgs(args, &inputStringLL, &compiledCodeWindow, &pathWindow, &fileNameLL, &sourceCodeLL, &opCharPositionsLL);
    
    returnOnErr(defragmentLinkedList(inputStringLL))
    if (inputStringLL->elementNum == 0)  return passed;
    if (inputStringLL->elementNum % sizeof(ccInt) != 0)  return library_argument_err;
    
    if (pathWindow != NULL)  {
        newCodePathStem = NULL;
        pathVarMemberCounter = 0;
        while (ccTrue)  {
            pathVarMemberCounter++;
            rtrn = findMemberIndex(pathWindow->variable_ptr, 0, pathVarMemberCounter, &loopMember, &dummy, &memberOffset, ccFalse);
            if (rtrn == invalid_index_err)  break;
            if ((!loopMember->ifHidden) && (loopMember->memberWindow != NULL))  {
                firstPathVarMember = pathVarMemberCounter;
                break;
    }   }   }
    
    
        // the last three optional arguments help to flag transform/runtime errors in the code
    
    for (cs = 0; cs < 3; cs++)  {
        if ((*(lastArgLLs[cs]))->elementNum == 0)  *lastArgs[cs] = NULL; //(char *) element(tempString, 0);     }
        else  {
            returnOnErr(defragmentLinkedList(*(lastArgLLs[cs])))
            *lastArgs[cs] = (char *) element(*(lastArgLLs[cs]), 1);
    }   }
    
    if (opCharNum != NULL)  {
        if (sourceCode == NULL)  return void_member_err;
        if (inputStringLL->elementNum != opCharPositionsLL->elementNum)  return out_of_range_err;
        
        if (sourceCodeLL->elementNum > 0)  {            // even a blank script has an end-of-file token --> char #1 
        for (loopCodeWord = 0; loopCodeWord < opCharPositionsLL->elementNum/sizeof(ccInt); loopCodeWord++)  {
        if ((opCharNum[loopCodeWord] < 1) || (opCharNum[loopCodeWord] > sourceCodeLL->elementNum))  {
            return out_of_range_err;
    }   }}}
    
    else if (sourceCode != NULL)  return void_member_err;
    
    
        // store some important variables since we need to overwrite these temporarily while checking the bytecode
    
    holdPCCodePtr = pcCodePtr;
    holdStartCodePtr = startCodePtr;
    holdEndCodePtr = endCodePtr;
    holdPCCodeRef = PCCodeRef;
    
    
        // set the PC at the beginning of the bytecode, and run it in 'check' mode
    
    startCodePtr = LL_int(inputStringLL, 1);
    endCodePtr = startCodePtr + inputStringLL->elementNum/sizeof(ccInt);
    pcCodePtr = startCodePtr;
    PCCodeRef.anchor = NULL;
    PCCodeRef.code_ptr = startCodePtr;
    
    checkBytecode();
    
    if ((errCode != passed) && (doPrintError))  {
        if (opCharNum == NULL)  theOpChar = 0;
        else  theOpChar = opCharNum[errIndex-1]-1;
        printError(fileName, fileNameLL->elementNum, sourceCode, theOpChar, ccFalse, 1, ccFalse);
        
        doPrintError = ccFalse;        }
    
    
        // restore the important variables
    
    PCCodeRef = holdPCCodeRef;
    pcCodePtr = holdPCCodePtr;
    startCodePtr = holdStartCodePtr;
    endCodePtr = holdEndCodePtr;
    
    if (errCode != passed)  return errCode;
    
    
        // OK, it worked.  Make a new anchor for the code.
    
    if (newCodePathStem != NULL)  refPath(newCodePathStem);       // temporarily over-reference each new path as we create it
    if (firstPathVarMember != 0)    {
        
        pathVarMemberCounter = firstPathVarMember-1;
        while (ccTrue)  {
            pathVarMemberCounter++;
            rtrn = findMemberIndex(pathWindow->variable_ptr, 0, pathVarMemberCounter, &loopMember, &dummy, &memberOffset, ccFalse);
            if (rtrn == invalid_index_err)  break;
            if ((!loopMember->ifHidden) && (loopMember->memberWindow != NULL))  {
                returnOnErr(drawPath(&newCodePath, loopMember->memberWindow, newCodePathStem, 1, PCCodeRef.PLL_index))
                refPath(newCodePath);                               // so it doesn't get deleted by a later path in the same window
                derefPath(&newCodePathStem);
                newCodePathStem = newCodePath;
    }   }   }
    
    if (compiledCodeWindow != NULL)  drawPathTo = compiledCodeWindow;
    else  drawPathTo = PCCodeRef.anchor->jamb;
    
    returnOnErr(drawPath(&newCodePath, drawPathTo, newCodePathStem, 1, PCCodeRef.PLL_index))
    refPath(newCodePath);
    derefPath(&newCodePathStem);
    
    
        // wipe up any existing codes in our function
    
    if (compiledCodeWindow != NULL)  {
        for (counter = 1; counter <= compiledCodeWindow->variable_ptr->codeList.elementNum; counter++)
            derefCodeRef((code_ref *) element(&(compiledCodeWindow->variable_ptr->codeList), counter));
        deleteElements(&(compiledCodeWindow->variable_ptr->codeList), 1, compiledCodeWindow->variable_ptr->codeList.elementNum);    }
    
    
        // add our new code to the Registry..
    
    returnOnErr(addCode(LL_int(inputStringLL, 1), &codeEntryPtr, &codeIndex, inputStringLL->elementNum/sizeof(ccInt),
            fileName, fileNameLL->elementNum, sourceCode, opCharNum, sourceCodeLL->elementNum, 1))
    
    
        // .. and to the code list of the function
    
    if (compiledCodeWindow != NULL)  {
        returnOnErr(addCodeRef(&(compiledCodeWindow->variable_ptr->codeList), newCodePath, codeEntryPtr, codeIndex))    }
    
    derefCodeList(&codeRegister);
    
    resizeLinkedList(&codeRegister, 0, ccFalse);
    returnOnErr(addCodeRef(&codeRegister, newCodePath, codeEntryPtr, codeIndex))

    derefPath(&newCodePath);
    
    GL_Object.codeList = &codeRegister;
    codeNumber = 1;
    
    return passed;
}



// cc_trap() catches runtime errors in its constructor, and allows execution to resume after the trap.
// Uses the registers to return error/warning codes/indices/offsets.

ccInt cc_trap(argsType args)
{
    code_ref argCodeRef;
    view *allArgs;
    ccInt loopCode, codeNo, *codeMarkers, *scoutAhead;
    ccBool ifErrorWasTrapped = ccFalse, holdDoPrintError = doPrintError, doClearError = ccTrue;
    
    getArgs(args, &allArgs);
    
    
        // run each code in our args constructor
    
    for (loopCode = 1; loopCode <= allArgs->windowPtr->variable_ptr->codeList.elementNum; loopCode++)  {
        
        code_ref *errScriptToPrint = NULL;
        
            // get a handle on the code
        
        argCodeRef = *(code_ref *) element(&(allArgs->windowPtr->variable_ptr->codeList), loopCode);
        refCodeRef(&argCodeRef);   // in case bE() changes something
        
        codeMarkers = argCodeRef.code_ptr;
        
        if ((*argCodeRef.code_ptr != end_of_script) && (*argCodeRef.code_ptr != code_marker))  {
            ccInt *holdPC = pcCodePtr;
            pcCodePtr = argCodeRef.code_ptr;
            runSkipMode(1);
            
            if (*pcCodePtr == code_marker)  {           // run in the args so the user can specify a target script
                
                codeMarkers = pcCodePtr;
                
                beginExecution(&argCodeRef, ccTrue, allArgs->offset, allArgs->width, 0);
                
                if (allArgs->windowPtr->variable_ptr->type == composite_type)  {
                if (allArgs->windowPtr->variable_ptr->mem.members.elementNum > 0)  {
                    window *errScriptWindow = getViewMember(1);
                    if (errScriptWindow != NULL)  {
                    if (errScriptWindow->variable_ptr->type == composite_type)  {
                        errScriptToPrint = (code_ref *) element(&(errScriptWindow->variable_ptr->codeList), 1);
                        refCodeRef(errScriptToPrint);
                }}  }}
            }
            pcCodePtr = holdPC;
        }
        derefCodeRef(&argCodeRef);   // in case bE() changes something
        
        scoutAhead = codeMarkers;
        while (*scoutAhead == code_marker)  scoutAhead++;
        codeNo = (ccInt) (scoutAhead-codeMarkers);
        doPrintError = ((codeNo == 1) || (codeNo == 3));
        doClearError = ((codeNo == 0) || (codeNo == 1));
        
        
            // run the code
        
        warningCode = passed;
        argCodeRef.anchor = pcSearchPath;               // run in the calling script
        refCodeRef(&argCodeRef);
        beginExecution(&argCodeRef, ccTrue, allArgs->offset, allArgs->width, codeNo);
        if (errCode == return_flag)  errCode = passed;
        
        if (errScriptToPrint != NULL)  {
            derefCodeRef(errScriptToPrint);
            if (errScriptToPrint->references == 0)  errScriptToPrint = NULL;    }
        
        
            // print errors/warnings if we are instructed to do so
        
        if ((errCode != passed) || (warningCode != passed))  {
            
            code_ref *errScriptToPassBack = NULL;
            
            if ((!doClearError) && (errScriptToPrint != NULL))  {
                errScriptToPassBack = errScriptToPrint;
                errScriptToPrint = NULL;     }
            
            if (doPrintError)  {
                
                storedCodeType *errorBaseScript;
                ccInt errIndexToPrint, errCharNum = 0;
                if (errScriptToPrint != NULL)  errIndexToPrint = 1;
                else if (errCode != passed)  {  errIndexToPrint = errIndex;  errScriptToPrint = &errScript;  }
                else  {  errIndexToPrint = warningIndex;  errScriptToPrint = &warningScript;  }
                
                errorBaseScript = storedCode(errScriptToPrint->PLL_index);
                if (errorBaseScript->opCharNum != NULL)  errCharNum = errorBaseScript->opCharNum[
                                   errIndexToPrint + (ccInt) (errScriptToPrint->code_ptr - errorBaseScript->bytecode) - 1] - 1;
                
                printError(errorBaseScript->fileName, -1, errorBaseScript->sourceCode, errCharNum, ccFalse, errorBaseScript->compilerID, errIndexToPrint==1);
            }
            
            if ((codeNo == 3) || (errScriptToPassBack != NULL))  {
                ccBool doErr = (errCode != passed);
                if (errScriptToPassBack == NULL)  {
                    if (doErr)  errCode = thrown_to_err;
                    else  warningCode = thrown_to_err;      }
                else  {
                    setErrIndex(errScriptToPassBack->code_ptr, codeNo==3? thrown_to_err : errCode, errScriptToPassBack,
                                doErr? &errCode : &warningCode, doErr? &errIndex : &warningIndex, doErr? &errScript : &warningScript);
            }   }
            
            if (errCode != passed)  intRegister = errCode;
            else  intRegister = -warningCode;
            if (doClearError)  errCode = warningCode = passed;
            
            ifErrorWasTrapped = ccTrue;
        }
        
        derefCodeRef(&argCodeRef);
        if ((ifErrorWasTrapped) && (intRegister > passed))  break;
    }
    
    
        // clear intRegister if no error and no warning
    
    if (!ifErrorWasTrapped)  intRegister = passed;
    
    doPrintError = holdDoPrintError;
    
    return intRegister;
}


// throw() generates an error.  throw(errCode[, errScript[, errIndex[, ifWarning]]])

ccInt cc_throw(argsType args)
{
    window *eWindow;
    code_ref *targetCodeRef = &PCCodeRef;
    ccInt *holdPC, eCode, eCodeNumber = 1, eIndex, maxIndex;
    code_ref *errorScript = &errScript;
    ccBool doWarning;
    
    
        // read in the info, throwing real errors if args are wrong with the argument
    
    if (args.num != 5)  return wrong_argument_count_err;
    
    returnOnErr(checkArgs(args, scalar(int_type), scalar(bool_type), vector(composite_type), scalar(int_type), scalar(int_type)))
    getArgs(args, byValue(&eCode), byValue(&doWarning), &eWindow, byValue(&eCodeNumber), byValue(&eIndex));

    if ((eCodeNumber < 1) || (eCodeNumber > eWindow->variable_ptr->codeList.elementNum))  return index_argument_err;
    targetCodeRef = (code_ref *) element(&(eWindow->variable_ptr->codeList), eCodeNumber);
    
    holdPC = pcCodePtr;
    pcCodePtr = targetCodeRef->code_ptr;
    runSkipMode(0);
    maxIndex = (ccInt) (pcCodePtr - targetCodeRef->code_ptr);
    pcCodePtr = holdPC;
    if ((eIndex < 1) || (eIndex > maxIndex))  return index_argument_err;
    
    
        // throw the error/warning
    
    if (doWarning)  {
        warningCode = eCode;
        warningIndex = eIndex;
        errorScript = &warningScript;       }
    
    else  {
        errCode = eCode;
        errIndex = eIndex;
        errorScript = &errScript;       }
    
    if (errorScript->code_ptr != NULL)  derefCodeRef(errorScript);
    *errorScript = *targetCodeRef;
    errorScript->anchor = NULL;
    refCodeRef(errorScript);
    
    return passed;
}


// cc_top() returns the number of indices (things that can be rereferenced by [..]) contained in the given variable.
// This is not the same as the number of members!  Arrays have only one width-N member; strings only one per searchView.offset. 

ccInt cc_top(argsType args)
{
    view viewToTop;
    
    if (args.num != 1)  return wrong_argument_count_err;
    
    returnOnErr(checkArgs(args, fromArg(1)))
    getArgs(args, &(viewToTop.windowPtr));
    
    if (viewToTop.windowPtr == NULL)  {  setError(void_member_err, pcCodePtr-1);  return 0;  }
    if (viewToTop.windowPtr->variable_ptr->type < string_type)  {  setError(not_composite_err, pcCodePtr-1);  return 0;  }
    viewToTop.offset = viewToTop.windowPtr->offset;
    
    return numMemberIndices(&viewToTop);
}


// cc_size() returns the total number of bytes contained in its argument

ccInt cc_size(argsType args)
{
    view viewToSize;
    ccInt dataSize = 0, sizeofStrings = 0, ssMode, *sizeRtrn;
    ccBool storageSize;
    
    if (args.num != 3)  return wrong_argument_count_err;
    
    returnOnErr(checkArgs(args, fromArg(1), scalar(bool_type), scalar(int_type)))
    getArgs(args, &(viewToSize.windowPtr), byValue(&storageSize), &sizeRtrn);
    
    if (viewToSize.windowPtr != NULL)  {
        viewToSize.offset = viewToSize.windowPtr->offset;
        viewToSize.width = viewToSize.windowPtr->width;
        
        if (!storageSize)  sizeView(&viewToSize, &dataSize, &sizeofStrings);
        else  {
        for (ssMode = 1; ssMode <= 3; ssMode++)  {
            storageSizeView(&viewToSize, (void *) &dataSize, (void *) &ssMode);
        }}
        
        *sizeRtrn = dataSize+sizeofStrings;       }
    
    else  *sizeRtrn = 0;
    
    return passed;
}



// type() returns the type of a given variable, or of one of the members of that variable (member index is argument 2)

ccInt cc_type(argsType args)
{
    window *hostWindow;
    ccInt whichMember, *theType;
    
    if (args.num != 3)  return wrong_argument_count_err;
    
    returnOnErr(checkArgs(args, fromArg(1), scalar(int_type), scalar(int_type)))
    getArgs(args, &hostWindow, byValue(&whichMember), &theType);
    
    if (whichMember <= 0)  *theType = hostWindow->variable_ptr->type;
    
    else if (hostWindow->variable_ptr->type == composite_type)  {
        if (whichMember > hostWindow->variable_ptr->mem.members.elementNum)  return invalid_index_err;
        *theType = LL_member(hostWindow->variable_ptr, whichMember)->type;  }
    
    else  return not_composite_err;
    
    return passed;
}



// member_ID() returns the ID number of a given member
// (corresponding to the respective entry in the allNames string returned by the compiler)

ccInt cc_member_ID(argsType args)
{
    window *hostWindow;
    ccInt memberIndex, memberNumber, entryOffset, *theID;
    member *soughtMember;
    
    if (args.num != 3)  return wrong_argument_count_err;
    
    returnOnErr(checkArgs(args, vector(composite_type), scalar(int_type), scalar(int_type)))
    getArgs(args, &hostWindow, byValue(&memberIndex), &theID);
    
    returnOnErr(findMemberIndex(hostWindow->variable_ptr, 0, memberIndex, &soughtMember, &memberNumber, &entryOffset, ccFalse))
    
    *theID = soughtMember->memberID;
    
    return passed;
}



// cclib_bytecode() extracts variable or member codes

ccInt cc_bytecode(argsType args)
{
    window *hostWindow;
    linkedlist *codeRefsList = NULL, *bytecodeString;
    ccInt memberNumber, loopCode, *holdPC;
    
    if (args.num != 3)  return wrong_argument_count_err;
    
    returnOnErr(checkArgs(args, vector(composite_type), scalar(int_type), scalar(string_type)))
    getArgs(args, &hostWindow, byValue(&memberNumber), &bytecodeString);
    
    if (memberNumber <= 0)  {
        codeRefsList = (linkedlist *) &(hostWindow->variable_ptr->codeList);    }
    else  {
        if (memberNumber > hostWindow->variable_ptr->mem.members.elementNum)  return invalid_index_err;
        codeRefsList = (linkedlist *) &(LL_member(hostWindow->variable_ptr, memberNumber)->codeList);       }
    
    returnOnErr(resizeLinkedList(bytecodeString, 0, ccFalse))
    
    
        // copy the bytecode of each code block into a string (including the final 0)
    
    for (loopCode = 1; loopCode <= codeRefsList->elementNum; loopCode++)  {
        
        code_ref *soughtCodeRef = (code_ref *) element(codeRefsList, loopCode);
        ccInt oldTop = bytecodeString->elementNum;
        
        holdPC = pcCodePtr;
        pcCodePtr = soughtCodeRef->code_ptr;
        runSkipMode(0);    // should always work (code's already been checked); just in case, check errCode (index would be off, though)
        if (errCode != passed)  {  pcCodePtr = holdPC;  return errCode;  }
        pcCodePtr++;
        
        returnOnErr(addElements(bytecodeString, (ccInt) (pcCodePtr-soughtCodeRef->code_ptr)*sizeof(ccInt), ccFalse))
        pcCodePtr = holdPC;
        
        setElements(bytecodeString, oldTop+1, oldTop+bytecodeString->elementNum, (void *) soughtCodeRef->code_ptr);
    }
    
    return passed;
}



ccInt cc_minmax(argsType args)
{
    ccInt c1, listTop, *bestIdx, mult;
    ccFloat *theList, *bestValue;
    
    getArgs(args, &theList, byValue(&mult), &bestIdx, &bestValue);
    listTop = args.indices[0];
    
    *bestIdx = 1;
    *bestValue = mult * theList[0];
    
    for (c1 = 0; c1 < args.indices[0]; c1++)  {
    if (mult*theList[c1] > *bestValue)  {
        *bestIdx = c1 + 1;
        *bestValue = mult * theList[c1];
    }}
    
    return passed;
}



ccInt cc_sum(argsType args)
{
    ccInt c1;
    ccFloat *theList, *theSum;
    
    getArgs(args, &theList, &theSum);
    
    *theSum = 0.;
    for (c1 = 0; c1 < args.indices[0]; c1++)  *theSum += theList[c1];
    
    return passed;
}



ccInt cc_makeLinkList(argsType args)
{
    ccInt *linkList, *firstIndex, direction, cl;
    ccFloat *sortingList;
    
    getArgs(args, &sortingList, &linkList, byValue(&direction), &firstIndex);
    
    for (cl = 0; cl < args.indices[0]; cl++)  linkList[cl] = -1;
    
    *firstIndex = makeLinkList(0, args.indices[0]-1, direction, sortingList, linkList);
    
    return passed;
}


ccInt makeLinkList(const ccInt left, const ccInt right, const ccInt direction, const ccFloat *sortingList, ccInt *linklist)
{
    ccInt middle, first, toWrite, side, nextSide, idx[2];
    
    if (right > left)  {
        
        middle = floor((right + left)/2);
        idx[0] = makeLinkList(left, middle, direction, sortingList, linklist);
        idx[1] = makeLinkList(middle + 1, right, direction, sortingList, linklist);
        
        if ((sortingList[idx[0]] - sortingList[idx[1]])*direction <= 0)  side = 0;
        else  side = 1;
        
        first = toWrite = idx[side];
        idx[side] = linklist[idx[side]];
        
        while ((idx[0] != -1) && (idx[1] != -1))  {
            
            if ((sortingList[idx[0]] - sortingList[idx[1]])*direction <= 0)  nextSide = 0;
            else  nextSide = 1;
            
            if (side != nextSide)  {
                linklist[toWrite] = idx[nextSide];
                side = nextSide;       }
            
            toWrite = idx[side];
            idx[side] = linklist[idx[side]];    }
        
        linklist[toWrite] = idx[1-side];
        
        return first;    }
        
    else  return left;
}


ccInt cc_sort(argsType args)
{
    ccInt numIndices, cl, ci, idx, *linkList, numLists, firstIndex, rtrn = passed;
    
    getArgs(args, &linkList, byValue(&firstIndex), endArgs);
    numIndices = args.indices[0];
    numLists = (args.num-2)/2;
    
    for (cl = 0; cl < numLists; cl++)  {
        if (args.type[cl+2] != args.type[cl+2+numLists])  rtrn = 1;
        else if (args.indices[cl+2] != numIndices)  rtrn = 2;
        else if ((args.type[cl+2] != int_type) && (args.type[cl+2] != double_type))  rtrn = 3;
        if (rtrn != passed)  break;         }
    
    if (rtrn != passed)  {
    for (ci = 0; ci < numIndices; ci++)  {
        linkList[ci]++;        // convert to Cicada indexing
    }}
    
    else  {
    for (cl = 0; cl < numLists; cl++)  {
        idx = firstIndex;
        if (args.type[cl+2] == int_type)  {
            ccInt *list = (ccInt *) (args.p[cl+2]), *sortedList = (ccInt *) (args.p[cl+2+numLists]);
            for (ci = 0; ci < numIndices; ci++)  {
                sortedList[ci] = list[idx];
                idx = linkList[idx];
        }   }
        else if (args.type[cl+2] == double_type)  {
            ccFloat *list = (ccFloat *) (args.p[cl+2]), *sortedList = (ccFloat *) (args.p[cl+2+numLists]);
            for (ci = 0; ci < numIndices; ci++)  {
                sortedList[ci] = list[idx];
                idx = linkList[idx];
    }}  }   }
    
    return rtrn;
}



// ****** I/O ROUTINES ******


// cc_load() reads the contents of a file

ccInt cc_load(argsType args)
{
    linkedlist *fileName, *fileContents;
    ccInt scriptNo, rtrn;
    size_t scriptSize;
    char *fileNameC;
    const char *scriptString;
    
    
        // get the name of the file (arg 1)
    
    if (args.num != 2)  return wrong_argument_count_err;
    
    if (args.type[0] == int_type)  {
        returnOnErr(checkArgs(args, scalar(int_type), scalar(string_type)))
        getArgs(args, byValue(&scriptNo), &fileContents);
        
        if (scriptNo == 1)  scriptString = defsScript;
        else if (scriptNo == 2)  scriptString = terminalScript;
        else if (scriptNo == 3)  scriptString = userDefsScript;
        else  return out_of_range_err;
        if (scriptString == NULL)  return library_argument_err;
        
        scriptSize = strlen(scriptString);
        returnOnErr(resizeLinkedList(fileContents, (ccInt) scriptSize, ccFalse))
        setElements(fileContents, 1, (ccInt) scriptSize, (void *) scriptString);
        
        return passed;
    }
    
    else  {
        returnOnErr(checkArgs(args, scalar(string_type), scalar(string_type)))
        getArgs(args, &fileName, &fileContents);
        
        fileNameC = LL2Cstr(fileName);
        
        rtrn = loadFile(fileNameC, fileContents, ccFalse);
        
        free((void *) fileNameC);
        return rtrn;
    }
}


// loadFile():  a routine to load a text file into a linked list

ccInt loadFile(const char *fileNameC, linkedlist *textString, ccBool addFinalNull)
{
    ccInt fileSize, LLsize, bytesRead = 0;
    FILE *fileToRead;
    
    if ((fileToRead = fopen(fileNameC, "rb")) == NULL)  return IO_error;
    
    fseek(fileToRead, 0, SEEK_END);              // get the size of the file
    fileSize = (ccInt) ftell(fileToRead);
    rewind(fileToRead);
    
    LLsize = fileSize;
    if (addFinalNull)  LLsize++;
    returnOnErr(resizeLinkedList(textString, LLsize, ccFalse))
    returnOnErr(defragmentLinkedList(textString))
    
    if (fileSize > 0)  bytesRead = (ccInt) fread(element(textString, 1), 1, (size_t) fileSize, fileToRead);
    
    if (addFinalNull)  *LL_Char(textString, LLsize) = 0;
    
    if (fclose(fileToRead) != 0)  return IO_error;
    if (bytesRead != fileSize)  return IO_error;
    
    return passed;
}



// cc_save() saves a string into a file.  Overwrites the file if it already exists.

ccInt cc_save(argsType args)
{
    FILE *fileToWrite;
    linkedlist *fileName, *stringLL;
    ccInt bytesWritten;
    char *fileNameC;
    
    if (args.num != 2)  return wrong_argument_count_err;
    
    returnOnErr(checkArgs(args, scalar(string_type), scalar(string_type)))
    getArgs(args, &fileName, &stringLL);
    
    returnOnErr(defragmentLinkedList(stringLL))
    fileNameC = LL2Cstr(fileName);
    
    
        // try to open the file; throw error if unsuccessful
    
    fileToWrite = fopen(fileNameC, "wb");
    free((void *) fileNameC);
    
    if (fileToWrite == NULL)  return IO_error;
    
    
        // write the string to the file
    
    if (stringLL->elementNum == 0)  bytesWritten = 0;
    else  bytesWritten = (ccInt) fwrite(element(stringLL, 1), 1, stringLL->elementNum, fileToWrite);
    
    if (fclose(fileToWrite) != 0)  return IO_error;
    if (bytesWritten != stringLL->elementNum)  return IO_error;
    
    return passed;
}


char *LL2Cstr(linkedlist *LL)
{
    char *Cstring = (char *) malloc(LL->elementNum+1);
    if (Cstring == NULL)  return NULL;
    
    getElements(LL, 1, LL->elementNum, (void *) Cstring);
    Cstring[LL->elementNum] = 0;
    
    return Cstring;
}


// input() copies input from the keyboard, up until the first end-of-line

ccInt cc_input(argsType args)
{
    ccInt counter, bytesRead;
    const ccInt fileReadBufferSize = 1000;
    char charBuffer[1001], *readRetrn;
    linkedlist *LL;
    ccBool done;
    
    if (args.num == 0)  return wrong_argument_count_err;
    if (args.num > 1)  {  args.num--;  cc_print(args);  args.num++;  }
    
    returnOnErr(checkArgs(args, fromArg(args.num-1), scalar(string_type)))
    getArgs(args, fromArg(args.num-1), &LL);
    
    returnOnErr(resizeLinkedList(LL, 0, ccFalse))
    
    
        // loop until all of the data has been read
    
    done = ccFalse;
    do  {
        
            // load the data into the inlined buffer (on the stack)
        
        bytesRead = fileReadBufferSize;
        readRetrn = fgets(charBuffer, fileReadBufferSize+1, stdin);
        if (readRetrn == NULL)  {  setError(IO_error, pcCodePtr-1);  break;  }
        for (counter = 0; counter < fileReadBufferSize; counter++)  {
            if (*(charBuffer+counter) == '\n')  {
                done = ccTrue;
                bytesRead = counter;    // don't include the terminator
                break;
        }   }
        
        
            // copy from the buffer into the string register
        
        returnOnErr(addElements(LL, bytesRead, ccFalse))           // will be contiguous if spareRoom = 0
        setElements(LL, LL->elementNum-bytesRead+1, LL->elementNum, charBuffer);
    }  while (!done);
    
    return defragmentLinkedList(LL);
}


ccInt cc_print(argsType args)  {
    
    ccInt a, idx, idx2;
    
    for (a = 0; a < args.num; a++)  {
        
        if (args.type[a] == bool_type)  {
            ccBool *p = (ccBool *) args.p[a];
            for (idx = 0; idx < args.indices[a]; idx++)  {
                if (!p[idx])  printf("false");
                else  printf("true");
        }   }
        
        else if (args.type[a] == char_type)  {
            unsigned char *p = (unsigned char *) args.p[a];
            for (idx = 0; idx < args.indices[a]; idx++)  {
                printChar(p+idx);
        }   }
        
        else if (args.type[a] == int_type)  {
            ccInt *p = (ccInt *) args.p[a];
            for (idx = 0; idx < args.indices[a]; idx++)  {
                printf(printIntFormatString, p[idx]);
        }   }
        
        else if (args.type[a] == double_type)  {
            ccFloat *p = (ccFloat *) args.p[a];
            for (idx = 0; idx < args.indices[a]; idx++)  {
                printf(printFloatFormatString, p[idx]);
        }   }
        
        else if (args.type[a] == string_type)  {
            linkedlist *strs = (linkedlist *) args.p[a];
            for (idx = 0; idx < args.indices[a]; idx++)  {
                linkedlist *L = strs + idx;
                if (L->elementNum > 0)  {
                    sublistHeader *LLsublist = L->memory;
                    ccInt localIndex = 0;
                    printChar((const unsigned char *) skipElements(L, &LLsublist, &localIndex, 0));        // trick to get the first sublist
                    for (idx2 = 1; idx2 < L->elementNum; idx2++)  {
                        printChar((const unsigned char *) skipElements(L, &LLsublist, &localIndex, 1));
    }   }   }   }   }
    
    if (fflush(stdout) != passed)  return IO_error;
    return passed;
}

void printChar(const unsigned char *c)
{
    if (lettertype(c) != unprintable)  printf("%c", *c);
    else  printf("\\%c%c", hexDigit((*c)/16), hexDigit((*c) % 16));
}




// ****** STRING OPERATIONS ******

// read_string() copies data from a string into its subsequent argument variables

ccInt cc_read_string(argsType args)
{
    linkedlist *theString;
    ccInt a, idx;
    ccFloat oneNum;
    char *holdString;
    const char *c, *warningChar = NULL;
    
    if (args.num < 1)  return wrong_argument_count_err;
    returnOnErr(checkArgs(args, scalar(string_type), endArgs))
    getArgs(args, &theString, endArgs);
    
    returnOnErr(defragmentLinkedList(theString))
    
    c = holdString = (char *) malloc(theString->elementNum+1);
    if (holdString == NULL)  return out_of_memory_err;
    getElements(theString, 1, theString->elementNum, (void *) holdString);
    holdString[theString->elementNum] = 0;
    
    
        // Read the string into the subsequent argument variables.  String variables are read up through the next space.
    
    for (a = 1; a < args.num; a++)   {
        void *oneArg = args.p[a];
        for (idx = 0; idx < args.indices[a]; idx++)  {
            
            while ((!isWordChar(c)) && (lettertype(c) != a_null))  c++;
            if (lettertype(c) == a_null)  {  if (warningChar == NULL)  warningChar = c;  break;  }
            
            if ((args.type[a] == int_type) || (args.type[a] == double_type))  {
                returnOnErr(readNum(&c, &oneNum, NULL));          // deal with overflow
                if (args.type[a] == int_type)  ((ccInt *) oneArg)[idx] = (ccInt) oneNum;
                else  ((ccFloat *) oneArg)[idx] = oneNum;       }
            else if (args.type[a] == char_type)  {
                ((char *) oneArg)[idx] = *c;
                c++;        }
            else if (args.type[a] == bool_type)  {
                if (strcmp(c, "true") == 0)  {  ((ccBool *) oneArg)[idx] = ccTrue;  c+=4;  }
                else if (strcmp(c, "false") == 0)  {  ((ccBool *) oneArg)[idx] = ccFalse;  c+=5;  }
                else  warningChar = c;          }
            else if (args.type[a] == string_type)  {
                linkedlist *destStr = (linkedlist *) args.p[a];
                const char *strStart = c;
                while (isWordChar(c))  c++;
                returnOnErr(resizeLinkedList(destStr, (ccInt) (c-strStart), ccFalse))
                setElements(destStr, 1, destStr->elementNum, (void *) strStart);
    }   }   }
    
        // Finally, check to see if there seem to be any left-over useful fields in the string -- if so, set a warning.
    
    while ((!isWordChar(c)) && (lettertype(c) != a_null))  c++;
    
    if ((lettertype(c) != a_null) && (warningChar == NULL))  warningChar = c;
    
    if (warningChar != NULL)  {
        setWarning(string_read_err, pcCodePtr-1);
        if (doPrintError)  printError(NULL, -1, holdString, (ccInt) (warningChar-holdString), ccTrue, 1, ccFalse);      }
    
    free(holdString);
    return passed;
}


ccBool isWordChar(const char *c)  {
    ccInt charType = lettertype(c);
    return ((charType == a_letter) || (charType == a_number) || (charType == a_symbol));
}



// print_string() writes the contents of its arguments into a string.
// Numeric data is written out in ASCII.

ccInt cc_print_string(argsType args)
{
    char *strPtr = NULL, *toWritePtr = NULL;
    ccInt a, idx, c2, n, numChars, maxFloatingDigits;
    ccFloat oneNum;
    linkedlist *theString;
    
    returnOnErr(checkArgs(args, scalar(string_type), scalar(numeric_type), endArgs))
    getArgs(args, &theString, byValue(&maxFloatingDigits), endArgs);
    
    if (maxFloatingDigits < 0)  return out_of_range_err;
    if (maxFloatingDigits > maxPrintableDigits)  maxFloatingDigits = maxPrintableDigits;
    
    
        // compute the byte size of the final string / print it
    
    for (c2 = 0; c2 < 2; c2++)  {
        n = 0;
        for (a = 2; a < args.num; a++)  {
            if (args.type[a] == bool_type)  {
                ccBool *p = (ccBool *) args.p[a];
                for (idx = 0; idx < args.indices[a]; idx++)  {
                    if (c2 == 1)  toWritePtr = strPtr+n;
                    if (!p[idx])  n += copyStr("false", toWritePtr);
                    else  n += copyStr("true", toWritePtr);
            }   }
            
            else if (args.type[a] == char_type)  {
                if (c2 == 1)  memcpy(strPtr+n, args.p[a], args.indices[a]);
                n += args.indices[a];        }
            
            else if (args.type[a] == string_type)  {
                linkedlist *strs = (linkedlist *) args.p[a];
                for (idx = 0; idx < args.indices[a]; idx++)  {
                    if (c2 == 1)  getElements(strs+idx, 1, strs[idx].elementNum, strPtr+n);
                    n += strs[idx].elementNum;
            }   }
            
            else  {
                void *p = args.p[a];
                for (idx = 0; idx < args.indices[a]; idx++)  {
                    if (c2 == 1)  toWritePtr = strPtr+n;
                    if (args.type[a] == int_type)  oneNum = ((ccInt *) p)[idx];
                    else  oneNum = ((ccFloat *) p)[idx];
                    printNumber(toWritePtr, oneNum, &numChars, args.type[a], maxFloatingDigits);
                    n += numChars;
        }   }   }
        
        if (c2 == 0)  {
            if (n == 0)  return passed;
            returnOnErr(resizeLinkedList(theString, n, ccFalse))
            returnOnErr(defragmentLinkedList(theString))
            strPtr = (char *) element(theString, 1);
    }   }
    
    return passed;
}

ccInt copyStr(const char *source, char *dest)
{
    ccInt nStr = 0;
    while (lettertype(source+nStr) != a_null)  {
        if (dest != NULL)  dest[nStr] = source[nStr];
        nStr++;
    }
    return nStr;
}


// find() searches a string for a substring, and returns its first/last appearance,
// or the number of times it appears (without overlapping).

ccInt cc_find(argsType args)
{
    linkedlist *str, *substr;
    char *strPtrToSearch, *soughtStrPtr;
    ccInt strLen, substrLen, startingPosition, numMatches, charCounter, mode, matchStart, *position;
    
    if (args.num != 5)  return wrong_argument_count_err;
    returnOnErr(checkArgs(args, scalar(string_type), scalar(string_type), scalar(int_type), scalar(int_type), scalar(int_type)))
    getArgs(args, &str, &substr, byValue(&mode), byValue(&startingPosition), &position);
    
    strLen = str->elementNum;
    substrLen = substr->elementNum;
    
    *position = 0;
    if (strLen == 0)  return passed;
    if (substrLen == 0)  {
        if (mode == 0)  *position = 0;
        else  *position = startingPosition;
        return passed;     }
    
    soughtStrPtr = (char *) element(substr, 1);
    strPtrToSearch = (char *) element(str, 1);
    returnOnErr(defragmentLinkedList(str))
    returnOnErr(defragmentLinkedList(substr))
    
    numMatches = 0;
    if (mode > -1)  {               // search forwards
    for (matchStart = startingPosition-1; matchStart <= strLen-substrLen; matchStart++)  {
        
        for (charCounter = 0; charCounter < substrLen; charCounter++)  {
        if (strPtrToSearch[matchStart+charCounter] != soughtStrPtr[charCounter])   {
            break;
        }}
        
        if (charCounter == substrLen)  {
            if (mode == 1)   {  *position = matchStart+1;  return passed;   }
            else  numMatches++;     // count
    }}  }
    
    else    {                       // search backwards
    for (matchStart = startingPosition-1; matchStart >= 0; matchStart--)  {
        
        for (charCounter = 0; charCounter < substrLen; charCounter++)  {
        if (strPtrToSearch[matchStart+charCounter] != soughtStrPtr[charCounter])   {
            break;
        }}
        
        if (charCounter == substrLen)  {  *position = matchStart+1;  return passed;   }
    }}
    
    if (mode == 0)  *position = numMatches;
    else  *position = 0;     // if we were searching, rather than counting, then if we made it this far we found nothing
    
    return passed;
}



// cc_random() generates a random number and returns it in doubleRegister

ccInt cc_random(argsType args)
{
    ccFloat *array;
    ccInt el;
    
    if (args.num != 1)  return wrong_argument_count_err;
    
    returnOnErr(checkArgs(args, vector(double_type)))
    getArgs(args, &array);
    
    for (el = 0; el < args.indices[0]; el++)  array[el] = ((ccFloat) rand())/RAND_MAX + ((ccFloat) rand())/RAND_MAX/RAND_MAX;
    
    return passed;
}



// The following mathematical functions all invoke doMath(),
// and each is paired with a second function that does the actual computation.
// All math is done in floating point.

ccInt cc_abs(argsType args)  {  return mathUnaryOp(args, &doAbs);  }
ccFloat doAbs(ccFloat argument)  {  return (ccFloat) fabs((double) argument);  }

ccInt cc_floor(argsType args)  {  return mathUnaryOp(args, &doFloor);  }
ccFloat doFloor(ccFloat argument)  {  return floor(argument);  }

ccInt cc_ceil(argsType args)  {  return mathUnaryOp(args, &doCeil);  }
ccFloat doCeil(ccFloat argument)  {  return ceil(argument);  }

ccInt cc_exp(argsType args)  {  return mathUnaryOp(args, &doExp);  }
ccFloat doExp(ccFloat argument)  {  return exp(argument);  }

ccInt cc_log(argsType args)  {  return mathUnaryOp(args, &doLog);  }
ccFloat doLog(ccFloat argument)  {  return log(argument);  }

ccInt cc_cos(argsType args)  {  return mathUnaryOp(args, &doCos);  }
ccFloat doCos(ccFloat argument)  {  return cos(argument);  }

ccInt cc_sin(argsType args)  {  return mathUnaryOp(args, &doSin);  }
ccFloat doSin(ccFloat argument)  {  return sin(argument);  }

ccInt cc_tan(argsType args)  {  return mathUnaryOp(args, &doTan);  }
ccFloat doTan(ccFloat argument)  {  return tan(argument);  }

ccInt cc_acos(argsType args)  {  return mathUnaryOp(args, &doArccos);  }
ccFloat doArccos(ccFloat argument)  {  return acos(argument);  }

ccInt cc_asin(argsType args)  {  return mathUnaryOp(args, &doArcsin);  }
ccFloat doArcsin(ccFloat argument)  {  return asin(argument);  }

ccInt cc_atan(argsType args)  {  return mathUnaryOp(args, &doArctan);  }
ccFloat doArctan(ccFloat argument)  {  return atan(argument);  }

ccInt mathUnaryOp(argsType args, ccFloat(f)(ccFloat x))
{
    ccFloat *source, *dest;
    ccInt el;
    
    if (args.num != 2)  return wrong_argument_count_err;
    
    returnOnErr(checkArgs(args, vector(double_type), vector(double_type)))
    if (args.indices[1] != args.indices[0])  return mismatched_indices_err;
    getArgs(args, &source, &dest);
    
    for (el = 0; el < args.indices[0]; el++)  dest[el] = f(source[el]);
    
    return passed;
}


ccInt cc_add(argsType args)  {  return mathBinaryOp(args, &doAdd);  }
ccFloat doAdd(ccFloat arg1, ccFloat arg2)  {  return arg1 + arg2;  }

ccInt cc_subtract(argsType args)  {  return mathBinaryOp(args, &doSubtract);  }
ccFloat doSubtract(ccFloat arg1, ccFloat arg2)  {  return arg1 - arg2;  }

ccInt cc_multiply(argsType args)  {  return mathBinaryOp(args, &doMultiply);  }
ccFloat doMultiply(ccFloat arg1, ccFloat arg2)  {  return arg1 * arg2;  }

ccInt cc_divide(argsType args)  {  return mathBinaryOp(args, &doDivide);  }
ccFloat doDivide(ccFloat arg1, ccFloat arg2)  {  return arg1 / arg2;  }

ccInt cc_pow(argsType args)  {  return mathBinaryOp(args, &doPow);  }
ccFloat doPow(ccFloat arg1, ccFloat arg2)  {  return pow(arg1, arg2);  }

ccInt mathBinaryOp(argsType args, ccFloat(f)(ccFloat x, ccFloat y))
{
    ccFloat *source1, *source2, *dest;
    ccInt stride1 = 1, stride2 = 1, vectorLen, el;
    
    if (args.num != 3)  return wrong_argument_count_err;
    
    returnOnErr(checkArgs(args, vector(double_type), vector(double_type), vector(double_type)))
    getArgs(args, &source1, &source2, &dest);
    
    if (args.indices[1] > args.indices[0])  vectorLen = args.indices[1];
    else  vectorLen = args.indices[0];
    
    if (args.indices[0] == 1)  stride1 = 0;
    else  if (args.indices[0] != vectorLen)  return mismatched_indices_err;
    if (args.indices[1] == 1)  stride2 = 0;
    else  if (args.indices[1] != vectorLen)  return mismatched_indices_err;
    if (args.indices[2] != vectorLen)  return mismatched_indices_err;
    
    for (el = 0; el < vectorLen; el++)  dest[el] = f(source1[stride1*el], source2[stride2*el]);
    
    return passed;
}



// springCleaning() removes inaccessible memory (with 0 arguments), or alternatively prunes hidden members from a variable (1 arg).

ccInt cc_springCleaning(argsType args)
{
    if (args.num == 0)  combVariables();
    
    else if (args.num == 1)  {
        window *theWindow;
        ccInt loopMember;
        
        returnOnErr(checkArgs(args, vector(composite_type)));
        getArgs(args, &theWindow);
        
        for (loopMember = theWindow->variable_ptr->mem.members.elementNum; loopMember >= 1; loopMember--)  {
        if (LL_member(theWindow->variable_ptr, loopMember)->ifHidden)  {
            removeMember(theWindow->variable_ptr, loopMember);
            if (errCode != passed)  return errCode;
    }   }}
    
    else  return wrong_argument_count_err;
    
    return passed;
}




// Misc -- useful for loading arguments into the user's C routines.

void getArgs(argsType args, ...)
{
    ccInt a;
    va_list theArgs;
    char **nextarg;
    
    va_start(theArgs, args);
    for (a = 0; a < args.num; a++)  {
        nextarg = va_arg(theArgs, char **);
        if (nextarg != NULL)
            *nextarg = (char *) args.p[a];
        else  {
            nextarg = va_arg(theArgs, char **);
            if (nextarg == NULL)  {
                a = ((ccInt) va_arg(theArgs, int)) - 1;
                if (a < -1)  break;       }
            else  {
                size_t numBytes = (size_t) args.indices[a]*typeSizes[args.type[a]];
                if (numBytes > 0)  memcpy((void *) nextarg, (void *) args.p[a], numBytes);
    }   }   }
    va_end(theArgs);
    
    return;
}


#define brk(e) {rtrn=e;break;}
ccInt checkArgs(argsType args, ...)
{
    ccInt a, nextarg, rtrn = passed;
    va_list theArgs;
    
    va_start(theArgs, args);
    for (a = 0; a < args.num; a++)  {
        nextarg = (ccInt) va_arg(theArgs, int);
        if (nextarg != 0)  {
            ccInt expectedType = nextarg;
            ccBool expectScalar = (expectedType < 0);
            if (expectScalar)  expectedType *= -1;
            expectedType--;
            
            if (expectedType == numeric_type)  {
                if ((args.type[a] != int_type) && (args.type[a] != double_type))  brk(type_mismatch_err)        }
            else if (expectedType != args.type[a])  brk(type_mismatch_err)
            if ((expectScalar) && (args.indices[a] != 1))  brk(multiple_indices_not_allowed_err)
        }
        else  {
            nextarg = va_arg(theArgs, int);         // get past the NULL
            a = ((ccInt) va_arg(theArgs, int)) - 1;
            if (a < -1)  break;
    }   }
    va_end(theArgs);
    
    return rtrn;
}
