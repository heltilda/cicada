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

// inbuiltCfunctions[] and userCfunctions[] define the { names, function addresses } of user's C routines.
// Each routine must be of the form:  ccInt RoutineName(argsType args)



#define CCname(n) {#n,&cc_n}

const Cfunction inbuiltCfunctions[] = {
    { "newCompiler:adaadd", &cc_newCompiler }, { "compile:dddaaa", &cc_compile },// { "getMemberNames:dad", &cc_getMemberNames },
    { "transform:daaddd", &cc_transform }, { "trap:A", &cc_trap }, { "throw:ddadd", &cc_throw },
    { "top:ad", &cc_top }, { "size:add", &cc_size }, { "type:add", &cc_type }, { "member_ID:add", &cc_member_ID }, { "bytecode:ada", &cc_bytecode },
    { "load:da", &cc_load }, { "save", &cc_save }, { "input:a", &cc_input }, { "print:A", &cc_print },
    { "read_string:da", &cc_read_string}, { "print_string:ada", &cc_print_string }, { "find", &cc_find }, { "random", &cc_random },
    { "abs", &cc_abs }, { "floor", &cc_floor }, { "ceil", &cc_ceil }, { "round", &cc_round }, { "exp", &cc_exp }, { "log", &cc_log },
    { "cos", &cc_cos }, { "sin", &cc_sin }, { "tan", &cc_tan }, { "acos", &cc_acos }, { "asin", &cc_asin }, { "atan", &cc_atan },
    { "add", &cc_add }, { "subtract", &cc_subtract }, { "multiply", &cc_multiply }, { "divide", &cc_divide }, { "pow", &cc_pow },
    { "minmax", &cc_minmax }, { "sum", &cc_sum }, { "makeLinkList", &cc_makeLinkList }, { "sort", &cc_sort },
    { "springCleaning:a", &cc_springCleaning }
};
const ccInt inbuiltCfunctionsNum = (ccInt) (sizeof(inbuiltCfunctions)/sizeof(Cfunction));
const char **inbuiltCfunctionArgs;


const Cfunction *userCfunctions; 
ccInt userCfunctionsNum;
const char **userCfunctionArgs;

ccInt passbackErrCode;
#define returnOnErr(x) if((passbackErrCode=x)!=passed) return passbackErrCode;

    
        // if we pass two arguments, that means that we're defining a new compiler
    
ccInt cc_newCompiler(argsType args)
{
    compiler_type *tempCompiler = NULL;
    commandTokenType *cmdTokens;
    arg *cmdStrings, *cmdReturnTypes, *cmdTranslations;
    ccInt *cmdPrecedences, *precedenceLevelAssociativity, *compilerID;
    ccInt numCommands, nC2, nC3, numPrecedenceLevels, cmdNum, rtrn = passed;
    
    if (args.num != 6)  return wrong_argument_count_err;
    
    returnOnErr(getArgs(args, scalarRef(arrayOf(string_type), &cmdStrings), arrayRef(int_type, &cmdPrecedences),
                scalarRef(arrayOf(string_type), &cmdReturnTypes), scalarRef(arrayOf(string_type), &cmdTranslations),
                arrayRef(int_type, &precedenceLevelAssociativity), scalarRef(int_type, &compilerID)))
    
    numCommands = nC2 = nC3 = 1;
    cmdStrings = stepArg(cmdStrings, 1, &numCommands);
    cmdReturnTypes = stepArg(cmdReturnTypes, 1, &nC2);
    cmdTranslations = stepArg(cmdTranslations, 1, &nC3);
    
    if ((args.indices[1] != numCommands) || (nC2 != numCommands) || (nC3 != numCommands))  return library_argument_err;
    numPrecedenceLevels = args.indices[4];
    
    cmdTokens = (commandTokenType *) malloc(numCommands*sizeof(commandTokenType));
    if (cmdTokens == NULL)  return out_of_memory_err;
    
    for (cmdNum = 0; cmdNum < numCommands; cmdNum++)  {
        cmdTokens[cmdNum].cmdString = argData(stepArg(cmdStrings, cmdNum+1, NULL));
        cmdTokens[cmdNum].precedence = cmdPrecedences[cmdNum];
        cmdTokens[cmdNum].rtrnTypeString = argData(stepArg(cmdReturnTypes, cmdNum+1, NULL));
        cmdTokens[cmdNum].translation = argData(stepArg(cmdTranslations, cmdNum+1, NULL));
        if ((cmdTokens[cmdNum].cmdString == NULL) || (cmdTokens[cmdNum].rtrnTypeString == NULL) || (cmdTokens[cmdNum].translation == NULL))  {
            return out_of_memory_err;
    }   }
    
    tempCompiler = newCompiler(cmdTokens, numCommands, precedenceLevelAssociativity, numPrecedenceLevels, &rtrn);
    if (rtrn == passed)  {
        returnOnErr(addElements(&allCompilers, 1, false))
        *compilerID = allCompilers.elementNum;
        *(compiler_type **) element(&allCompilers, *compilerID) = tempCompiler;
    }
    
    free((void *) cmdTokens);
    
    return rtrn;
}




// compile() generates bytecode from a script (a string).  Writes errors/warnings into the error/warning registers.

ccInt cc_compile(argsType args)
{
    compiler_type *theCompiler;
    arg *characterPositionsArg, *scriptBytecodeArg, *memberNameArray, *memberNameStrings;
    ccInt compilerID, nameID, prevNumNames, rtrn;
    char *scriptString, *fileName, *characterPositions, *scriptBytecode, *oneName;
    
    if (args.num != 6)  return wrong_argument_count_err;
    
    returnOnErr(getArgs(args, scalarValue(int_type, &compilerID), arrayRef(char_type, &scriptString), arrayRef(char_type, &fileName),
                scalarRef(listOf(int_type), &scriptBytecodeArg), scalarRef(listOf(int_type), &characterPositionsArg), endArgs))
    if (args.p[5] != NULL)  {  returnOnErr(getArgs(args, fromArg(5), scalarRef(arrayOf(string_type), &memberNameArray)))  }
    else  memberNameArray = NULL;
    
    theCompiler = *(compiler_type **) element(&allCompilers, compilerID);
    
    
        // Run the compiler.  Store the error code/index (which may be zero -- no error).
    
    rtrn = compile(theCompiler, (const char *) scriptString);
    
    if (((rtrn != passed) || (compilerWarning != passed)) && (doPrintError))  {
        
        if (rtrn != passed)  setError(rtrn, pcCodePtr-1);
        else  setWarning(compilerWarning, pcCodePtr-1);
        
        printError(fileName, args.indices[2], (const char *) scriptString, errPosition-1, false, compilerID, false);
        doPrintError = false;             }
    
    if (rtrn != passed)  return rtrn;
    
    
        // store the bytecode string, op character positions, and member names
    
    returnOnErr(setMemberTop(scriptBytecodeArg, 1, theCompiler->opCharNum.elementNum, &scriptBytecode))
    getElements(&(theCompiler->bytecode), 1, theCompiler->bytecode.elementNum, scriptBytecode);
    
    returnOnErr(setMemberTop(characterPositionsArg, 1, theCompiler->opCharNum.elementNum, &characterPositions))
    getElements(&(theCompiler->opCharNum), 1, theCompiler->opCharNum.elementNum, characterPositions);
    
    if (memberNameArray == NULL)  return passed;
    
    prevNumNames = getArgTop(memberNameArray); //getArgTop(stepArg(memberNameArray, 1));
    returnOnErr(setMemberTop(memberNameArray, 1, theCompiler->varNames.elementNum, NULL))
    
    memberNameStrings = stepArg(memberNameArray, 1, NULL);
    if (memberNameStrings == NULL)  return void_member_err;
    
    for (nameID = prevNumNames+1; nameID <= theCompiler->varNames.elementNum; nameID++)  {
        varNameType *compilerVarName = (varNameType *) element(&(theCompiler->varNames), nameID);
        returnOnErr(setStringSize(memberNameStrings, nameID, compilerVarName->nameLength, &oneName))
        memcpy(oneName, compilerVarName->theName, compilerVarName->nameLength);
    }
    
    return passed;
}


// cc_transform() takes bytecode and stores it in Cicada's code registry.
// Checks the bytecode first to make sure it won't crash the interpreter.

ccInt cc_transform(argsType args)
{
    member *loopMember;
    window *bytecodeTargetWindow, *pathWindow, *drawPathTo;
    searchPath *newCodePath, *newCodePathStem = pcSearchPath;
    ccInt *bytecodePtr, pathVarMemberCounter, firstPathVarMember = 0, counter, loopCodeWord, rtrn; //, cs;
    ccInt codeIndex, dummy, memberOffset;
    ccInt *holdPCCodePtr, *codeEntryPtr, *holdStartCodePtr, *holdEndCodePtr, *opCharNum = NULL, theOpChar;
    char *fileName = NULL, *sourceCode = NULL;
    code_ref holdPCCodeRef;
    
    if (args.num != 6)  return wrong_argument_count_err;
    
    if ((args.p[2] != NULL) && (args.type[2][0] != composite_type))  return not_composite_err;
    returnOnErr(getArgs(args, arrayRef(int_type, &bytecodePtr), scalarRef(composite_type, &bytecodeTargetWindow), &pathWindow,
                arrayRef(char_type, &fileName), arrayRef(char_type, &sourceCode), arrayRef(int_type, &opCharNum)))
    
    if (args.indices[0] == 0)  return passed;
    if (args.indices[3] == 0)  fileName = NULL;     // the last three optional arguments help to flag transform/runtime errors in the code
    if (args.indices[4] == 0)  sourceCode = NULL;
    if (args.indices[5] == 0)  opCharNum = NULL;
    
    if (pathWindow != NULL)  {
        newCodePathStem = NULL;
        pathVarMemberCounter = 0;
        while (true)  {
            pathVarMemberCounter++;
            rtrn = findMemberIndex(pathWindow->variable_ptr, 0, pathVarMemberCounter, &loopMember, &dummy, &memberOffset, false);
            if (rtrn == invalid_index_err)  break;
            if ((!loopMember->ifHidden) && (loopMember->memberWindow != NULL))  {
                firstPathVarMember = pathVarMemberCounter;
                break;
    }   }   }
    
    
    if ((opCharNum != NULL) && (sourceCode != NULL))  {
        if (args.indices[0] != args.indices[5])  return out_of_range_err;
        
        if (sourceCode != NULL)  {            // even a blank script has an end-of-file token --> char #1 
        for (loopCodeWord = 0; loopCodeWord < args.indices[5]; loopCodeWord++)  {
        if ((opCharNum[loopCodeWord] < 1) || (opCharNum[loopCodeWord] > args.indices[4]))  {
            return out_of_range_err;
    }   }}}
    
    
        // store some important variables since we need to overwrite these temporarily while checking the bytecode
    
    holdPCCodePtr = pcCodePtr;
    holdStartCodePtr = startCodePtr;
    holdEndCodePtr = endCodePtr;
    holdPCCodeRef = PCCodeRef;
    
    
        // set the PC at the beginning of the bytecode, and run it in 'check' mode
    
    startCodePtr = bytecodePtr;
    endCodePtr = startCodePtr + args.indices[0]; // inputStringLL->elementNum/sizeof(ccInt);
    pcCodePtr = startCodePtr;
    PCCodeRef.anchor = NULL;
    PCCodeRef.code_ptr = startCodePtr;
    PCCodeRef.PLL_index = 0;
    
    checkBytecode();
    
    if ((errCode != passed) && (doPrintError))  {
        if (opCharNum == NULL)  theOpChar = 0;
        else  theOpChar = opCharNum[errIndex-1]-1;
//        printError(fileName, fileNameLL->elementNum, sourceCode, theOpChar, false, 1, false);
        printError(fileName, args.indices[3], sourceCode, theOpChar, false, 1, false);
        
        doPrintError = false;        }
    
    
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
        while (true)  {
            pathVarMemberCounter++;
            rtrn = findMemberIndex(pathWindow->variable_ptr, 0, pathVarMemberCounter, &loopMember, &dummy, &memberOffset, false);
            if (rtrn == invalid_index_err)  break;
            if ((!loopMember->ifHidden) && (loopMember->memberWindow != NULL))  {
                returnOnErr(drawPath(&newCodePath, loopMember->memberWindow, newCodePathStem, 1, PCCodeRef.PLL_index))
                refPath(newCodePath);                               // so it doesn't get deleted by a later path in the same window
                derefPath(&newCodePathStem);
                newCodePathStem = newCodePath;
    }   }   }
    
    if (bytecodeTargetWindow != NULL)  drawPathTo = bytecodeTargetWindow;
    else  drawPathTo = PCCodeRef.anchor->jamb;
    
    returnOnErr(drawPath(&newCodePath, drawPathTo, newCodePathStem, 1, PCCodeRef.PLL_index))
    refPath(newCodePath);
    derefPath(&newCodePathStem);
    
    
        // wipe up any existing codes in our function
    
    if (bytecodeTargetWindow != NULL)  {
        for (counter = 1; counter <= bytecodeTargetWindow->variable_ptr->codeList.elementNum; counter++)
            derefCodeRef((code_ref *) element(&(bytecodeTargetWindow->variable_ptr->codeList), counter));
        deleteElements(&(bytecodeTargetWindow->variable_ptr->codeList), 1, bytecodeTargetWindow->variable_ptr->codeList.elementNum);    }
    
    
        // add our new code to the Registry..
    
    returnOnErr(addCode(bytecodePtr, &codeEntryPtr, &codeIndex, args.indices[0],
            fileName, args.indices[3], sourceCode, opCharNum, args.indices[4], 1))
    
    
        // .. and to the code list of the function
    
    if (bytecodeTargetWindow != NULL)  {
        returnOnErr(addCodeRef(&(bytecodeTargetWindow->variable_ptr->codeList), newCodePath, codeEntryPtr, codeIndex))    }
    
    derefCodeList(&codeRegister);
    
    resizeLinkedList(&codeRegister, 0, false);
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
    code_ref argCodeRef, errScriptToPrint;
    view *allArgs;
    window *errScriptWindow = NULL;
    ccInt loopCode, codeNo, *codeMarkers, *scoutAhead;
    bool ifErrorWasTrapped = false, holdDoPrintError = doPrintError, doClearError = true;
    
    getArgs(args, &allArgs);
    
    
        // run each code in our args constructor
    
    for (loopCode = 1; loopCode <= allArgs->windowPtr->variable_ptr->codeList.elementNum; loopCode++)  {
        
        errScriptToPrint.code_ptr = NULL;
        
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
                
                beginExecution(&argCodeRef, true, allArgs->offset, allArgs->width, 0);
                
                if (*allArgs->windowPtr->variable_ptr->types == composite_type)  {
                if (allArgs->windowPtr->variable_ptr->mem.members.elementNum > 0)  {
                    errScriptWindow = getViewMember(1);
                    if (errScriptWindow != NULL)  {
                    if (*errScriptWindow->variable_ptr->types == composite_type)  {
                        errScriptToPrint = *(code_ref *) element(&(errScriptWindow->variable_ptr->codeList), 1);
                        refCodeRef(&errScriptToPrint);
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
        beginExecution(&argCodeRef, true, allArgs->offset, allArgs->width, codeNo);
        if (errCode == return_flag)  errCode = passed;
        
        if (errScriptToPrint.code_ptr != NULL)  {
            derefCodeRef(&errScriptToPrint);
            if (errScriptToPrint.references == 0)  errScriptToPrint.code_ptr = NULL;    }
        
        
            // print errors/warnings if we are instructed to do so
        
        if ((errCode != passed) || (warningCode != passed))  {
            
            code_ref *errScriptToPassBack = NULL;
            
            if ((!doClearError) && (errScriptToPrint.code_ptr != NULL))  {
                errScriptToPassBack = (code_ref *) element(&(errScriptWindow->variable_ptr->codeList), 1);
                errScriptToPrint.code_ptr = NULL;     }
            
            if (doPrintError)  {
                
                storedCodeType *errorBaseScript;
                ccInt errIndexToPrint, errCharNum = 0;
                if (errScriptToPrint.code_ptr != NULL)  errIndexToPrint = 1;
                else if (errCode != passed)  {  errIndexToPrint = errIndex;  errScriptToPrint = errScript;  }
                else  {  errIndexToPrint = warningIndex;  errScriptToPrint = warningScript;  }
                
                if (errScriptToPrint.PLL_index == 0)  printError(NULL, 0, NULL, 0, false, 1, true);
                else  {
                    errorBaseScript = storedCode(errScriptToPrint.PLL_index);
                    if (errorBaseScript->opCharNum != NULL)  errCharNum = errorBaseScript->opCharNum[
                                       errIndexToPrint + (ccInt) (errScriptToPrint.code_ptr - errorBaseScript->bytecode) - 1] - 1;
                    
                    printError(errorBaseScript->fileName, -1, errorBaseScript->sourceCode,
                                errCharNum, false, errorBaseScript->compilerID, errIndexToPrint==1);
            }   }
            
            if ((codeNo >= 2) || (errScriptToPassBack != NULL))  {
                bool doErr = (errCode != passed);
                if (errScriptToPassBack == NULL)  {
                    if (codeNo == 3)  {
                        if (doErr)  errCode = thrown_to_err;
                        else  warningCode = thrown_to_err;
                }   }
                else  {
                    setErrIndex(errScriptToPassBack->code_ptr, codeNo==3? thrown_to_err : errCode, errScriptToPassBack,
                                doErr? &errCode : &warningCode, doErr? &errIndex : &warningIndex, doErr? &errScript : &warningScript);
            }   }
            
            if (errCode != passed)  intRegister = errCode;
            else  intRegister = -warningCode;
            if (doClearError)  errCode = warningCode = passed;
            
            ifErrorWasTrapped = true;
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
    bool doWarning;
    
    
        // read in the info, throwing real errors if args are wrong with the argument
    
    if (args.num != 5)  return wrong_argument_count_err;
    
    returnOnErr(getArgs(args, scalarValue(int_type, &eCode), scalarValue(bool_type, &doWarning), arrayRef(composite_type, &eWindow),
                scalarValue(int_type, &eCodeNumber), scalarValue(int_type, &eIndex)))

    if ((eCodeNumber < 1) || (eCodeNumber > eWindow->variable_ptr->codeList.elementNum))  return index_argument_err;
    targetCodeRef = (code_ref *) element(&(eWindow->variable_ptr->codeList), eCodeNumber);
    
    holdPC = pcCodePtr;
    pcCodePtr = targetCodeRef->code_ptr;
    runSkipMode(0);
    maxIndex = (ccInt) (pcCodePtr - targetCodeRef->code_ptr) + 1;
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
    
    getArgs(args, &(viewToTop.windowPtr));
    
    if (viewToTop.windowPtr == NULL)  {  setError(void_member_err, pcCodePtr-1);  return 0;  }
    if (*viewToTop.windowPtr->variable_ptr->types <= double_type)  {  setError(not_composite_err, pcCodePtr-1);  return 0;  }
    viewToTop.offset = viewToTop.windowPtr->offset;
    
    return numMemberIndices(&viewToTop);
}


// cc_size() returns the total number of bytes contained in its argument

ccInt cc_size(argsType args)
{
    view viewToSize;
    ccInt dataSize[2], *ssMode = &(dataSize[1]), sizeofStrings = 0, *sizeRtrn;
    bool storageSize;
    
    if (args.num != 3)  return wrong_argument_count_err;
    
    returnOnErr(getArgs(args, &(viewToSize.windowPtr), scalarValue(bool_type, &storageSize), scalarRef(int_type, &sizeRtrn)))
    
    *dataSize = 0;
    if (viewToSize.windowPtr != NULL)  {
        viewToSize.offset = viewToSize.windowPtr->offset;
        viewToSize.width = viewToSize.windowPtr->width;
        
        if (!storageSize)  {
            sizeViewInfo SVinfo;
            SVinfo.dataSize = SVinfo.listElSize = 0;
            SVinfo.includeDataLists = true;
            sizeView(&viewToSize, &SVinfo, NULL);
            *dataSize = SVinfo.dataSize;
        }
        else  {
        for (*ssMode = 1; *ssMode <= 3; (*ssMode)++)  {
            storageSizeView(&viewToSize, (void *) dataSize, NULL);
        }}
        
        *sizeRtrn = (*dataSize)+sizeofStrings;       }
    
    else  *sizeRtrn = 0;
    
    return passed;
}



// type() returns the type of a given variable, or of one of the members of that variable (member index is argument 2)

ccInt cc_type(argsType args)
{
    window *hostWindow;
    ccInt whichMember, *theType;
    
    if (args.num != 3)  return wrong_argument_count_err;
    
    returnOnErr(getArgs(args, &hostWindow, scalarValue(int_type, &whichMember), scalarRef(int_type, &theType)))
    
    if (whichMember <= 0)  *theType = *hostWindow->variable_ptr->types;
    
    else if (*hostWindow->variable_ptr->types == composite_type)  {
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
    
    returnOnErr(getArgs(args, arrayRef(composite_type, &hostWindow), scalarValue(int_type, &memberIndex), scalarRef(int_type, &theID)))
    
    returnOnErr(findMemberIndex(hostWindow->variable_ptr, 0, memberIndex, &soughtMember, &memberNumber, &entryOffset, false))
    
    *theID = soughtMember->memberID;
    
    return passed;
}



// cclib_bytecode() extracts variable or member codes

ccInt cc_bytecode(argsType args)
{
    window *hostWindow;
    linkedlist *codeRefsList = NULL;
    arg *bytecodeString;
    ccInt memberNumber, loopCode, *holdPC, numBytecodeWords = 0, numNewWords, *newBytecodeWords;
    
    if (args.num != 3)  return wrong_argument_count_err;
    
    returnOnErr(getArgs(args, &hostWindow, scalarValue(int_type, &memberNumber), scalarRef(listOf(int_type), &bytecodeString)))
    
    if (args.type[0][0] < composite_type)  return type_mismatch_err;
    if (hostWindow == NULL)  return void_member_err;
    
    if (memberNumber <= 0)  {
        codeRefsList = (linkedlist *) &(hostWindow->variable_ptr->codeList);    }
    else  {
        if (memberNumber > hostWindow->variable_ptr->mem.members.elementNum)  return invalid_index_err;
        codeRefsList = (linkedlist *) &(LL_member(hostWindow->variable_ptr, memberNumber)->codeList);       }
    
    returnOnErr(setMemberTop(bytecodeString, 1, 0, NULL))
//    returnOnErr(resizeLinkedList(bytecodeString, 0, false))
    
    
        // copy the bytecode of each code block into a string (including the final 0)
    
    for (loopCode = 1; loopCode <= codeRefsList->elementNum; loopCode++)  {
        
        code_ref *soughtCodeRef = (code_ref *) element(codeRefsList, loopCode);
        //ccInt oldTop = bytecodeString->elementNum;
        
        holdPC = pcCodePtr;
        pcCodePtr = soughtCodeRef->code_ptr;
        runSkipMode(0);    // should always work (code's already been checked); just in case, check errCode (index would be off, though)
        if (errCode != passed)  {  pcCodePtr = holdPC;  return errCode;  }
        pcCodePtr++;
        
        numNewWords = (ccInt) (pcCodePtr-soughtCodeRef->code_ptr);
        returnOnErr(setMemberTop(bytecodeString, 1, numBytecodeWords+numNewWords, (char **) &newBytecodeWords))
        memmove(newBytecodeWords+numBytecodeWords, (void *) soughtCodeRef->code_ptr, numNewWords*sizeof(ccInt));
        numBytecodeWords += numNewWords;
/*        returnOnErr(addElements(bytecodeString, (ccInt) (pcCodePtr-soughtCodeRef->code_ptr)*sizeof(ccInt), false))
        pcCodePtr = holdPC;
        
        setElements(bytecodeString, oldTop+1, oldTop+bytecodeString->elementNum, (void *) soughtCodeRef->code_ptr);*/
    }
    
    return passed;
}



ccInt cc_minmax(argsType args)
{
    ccInt c1, listTop, *bestIdx, mult;
    ccFloat *theList, *bestValue;
    
    returnOnErr(getArgs(args, &theList, scalarValue(int_type, &mult), scalarRef(int_type, &bestIdx), scalarRef(double_type, &bestValue)))
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
    
    returnOnErr(getArgs(args, arrayRef(double_type, &theList), scalarRef(double_type, &theSum)))
    
    *theSum = 0.;
    for (c1 = 0; c1 < args.indices[0]; c1++)  *theSum += theList[c1];
    
    return passed;
}



ccInt cc_makeLinkList(argsType args)
{
    ccInt *linkList, *firstIndex, direction, cl;
    ccFloat *sortingList;
    
    returnOnErr(getArgs(args, arrayRef(double_type, &sortingList),
            arrayRef(int_type, &linkList), scalarValue(int_type, &direction), scalarRef(int_type, &firstIndex)))
    
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
    
    returnOnErr(getArgs(args, arrayRef(int_type, &linkList), scalarValue(int_type, &firstIndex), endArgs))
    numIndices = args.indices[0];
    numLists = (args.num-2)/2;
    
    for (cl = 0; cl < numLists; cl++)  {
        if (args.type[cl+2] != args.type[cl+2+numLists])  rtrn = 1;
        else if (args.indices[cl+2] != numIndices)  rtrn = 2;
        else if ((args.type[cl+2][0] != int_type) && (args.type[cl+2][0] != double_type))  rtrn = 3;
        if (rtrn != passed)  break;         }
    
    if (rtrn != passed)  {
    for (ci = 0; ci < numIndices; ci++)  {
        linkList[ci]++;        // convert to Cicada indexing
    }}
    
    else  {
    for (cl = 0; cl < numLists; cl++)  {
        idx = firstIndex;
        if (args.type[cl+2][0] == int_type)  {
            ccInt *list = (ccInt *) (args.p[cl+2]), *sortedList = (ccInt *) (args.p[cl+2+numLists]);
            for (ci = 0; ci < numIndices; ci++)  {
                sortedList[ci] = list[idx];
                idx = linkList[idx];
        }   }
        else if (args.type[cl+2][0] == double_type)  {
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
    ccInt scriptNo, rtrn;
    size_t scriptSize;
    arg *fileContentsArg;
    char *fileName, *fileContents;
    const char *scriptString;
    
    
        // get the name of the file (arg 1)
    
    if (args.num != 2)  return wrong_argument_count_err;
    
    if (args.type[0][0] == int_type)  {
        returnOnErr(getArgs(args, scalarValue(int_type, &scriptNo), scalarRef(string_type, &fileContentsArg)))
        
        if (scriptNo == 1)  scriptString = defsScript;
        else if (scriptNo == 2)  scriptString = terminalScript;
        else if (scriptNo == 3)  scriptString = userDefsScript;
        else  return out_of_range_err;
        if (scriptString == NULL)  return library_argument_err;
        
        scriptSize = strlen(scriptString);
        returnOnErr(setStringSize(fileContentsArg, 1, (ccInt) scriptSize, &fileContents))
        memcpy(fileContents, scriptString, scriptSize);
        
        return passed;
    }
    
    else  {
        returnOnErr(getArgs(args, arrayRef(char_type, &fileName), scalarRef(string_type, &fileContentsArg)))
        
        rtrn = loadFile(fileName, fileContentsArg, false);
        
        return rtrn;
    }
}


// loadFile():  a routine to load a text file into a linked list

ccInt loadFile(const char *fileNameC, arg *textStringArg, bool addFinalNull)
{
    ccInt fileSize, LLsize, bytesRead = 0;
    char *textString;
    FILE *fileToRead;
    
    if ((fileToRead = fopen(fileNameC, "rb")) == NULL)  return IO_error;
    
    fseek(fileToRead, 0, SEEK_END);              // get the size of the file
    fileSize = (ccInt) ftell(fileToRead);
    rewind(fileToRead);
    
    LLsize = fileSize;
    if (addFinalNull)  LLsize++;
    returnOnErr(setStringSize(textStringArg, 1, fileSize, &textString))
    
    if (fileSize > 0)  bytesRead = (ccInt) fread(textString, 1, (size_t) fileSize, fileToRead);
    
    if (addFinalNull)  textString[LLsize] = 0;
    
    if (fclose(fileToRead) != 0)  return IO_error;
    if (bytesRead != fileSize)  return IO_error;
    
    return passed;
}



// cc_save() saves a string into a file.  Overwrites the file if it already exists.

ccInt cc_save(argsType args)
{
    FILE *fileToWrite;
    const char *fileName, *fileString;
    ccInt bytesWritten;
    
    if (args.num != 2)  return wrong_argument_count_err;
    
    returnOnErr(getArgs(args, arrayRef(char_type, &fileName), arrayRef(char_type, &fileString)))
    
    
        // try to open the file; throw error if unsuccessful
    
    fileToWrite = fopen(fileName, "wb");
    
    if (fileToWrite == NULL)  return IO_error;
    
    
        // write the string to the file
    
    if (args.indices[1] == 0)  bytesWritten = 0;
    else  bytesWritten = (ccInt) fwrite(fileString, 1, args.indices[1], fileToWrite);
    
    if (fclose(fileToWrite) != 0)  return IO_error;
    if (bytesWritten != args.indices[1])  return IO_error;
    
    return passed;
}


// input() copies input from the keyboard, up until the first end-of-line

ccInt cc_input(argsType args)
{
    ccInt counter, bytesRead, totalBytesRead = 0;
    const ccInt fileReadBufferSize = 1000;
    char charBuffer[1001], *readRetrn, *newChars;
    arg *inputString;
    bool done;
    
    if (args.num != 1)  return wrong_argument_count_err;
    
    returnOnErr(getArgs(args, scalarRef(string_type, &inputString)))
    
    returnOnErr(setStringSize(inputString, 1, 0, NULL))
    
    
        // loop until all of the data has been read
    
    done = false;
    do  {
        
            // load the data into the inlined buffer (on the stack)
        
        bytesRead = fileReadBufferSize;
        readRetrn = fgets(charBuffer, fileReadBufferSize+1, stdin);
        if (readRetrn == NULL)  {  setError(IO_error, pcCodePtr-1);  break;  }
        for (counter = 0; counter < fileReadBufferSize; counter++)  {
            if (*(charBuffer+counter) == '\n')  {
                done = true;
                bytesRead = counter;    // don't include the terminator
                break;
        }   }
        
        
            // copy from the buffer into the string register
        
        returnOnErr(setStringSize(inputString, 1, totalBytesRead+bytesRead, &newChars))
        memcpy(newChars + totalBytesRead, charBuffer, bytesRead);
        totalBytesRead += bytesRead;
    }  while (!done);
    
    return passed;
}


ccInt cc_print(argsType args)
{
    printView((view *) args.p[0], NULL, NULL);
    if (fflush(stdout) != passed)  return IO_error;
    
    return passed;
}




// ****** STRING OPERATIONS ******

// read_string() copies data from a string into its subsequent argument variables

ccInt cc_read_string(argsType args)
{
    ccInt a;
    view windowView;
    readStringInfo RSinfo;
    char *theString; //holdString;
    const char *c, *warningChar = NULL;
    
    if (args.num < 1)  return wrong_argument_count_err;
    returnOnErr(getArgs(args, arrayRef(char_type, &theString), endArgs))
    
    
        // Read the string into the subsequent argument variables.  String variables are read up through the next space.
    
    RSinfo.sourceString = theString;
    RSinfo.stringEnd = theString + args.indices[0];
    RSinfo.warningMarker = NULL;
    for (a = 1; a < args.num; a++)   {
        arg *oneArg = args.p[a];
        windowView.windowPtr = (window *) oneArg;
        if (oneArg != NULL)  {
            windowView.offset = windowView.windowPtr->offset;
            windowView.width = windowView.windowPtr->width;
            readViewString(&windowView, &RSinfo, NULL);
    }   }
    
        // Finally, check to see if there seem to be any left-over useful fields in the string -- if so, set a warning.
    
    if (RSinfo.warningMarker != NULL)  warningChar = RSinfo.warningMarker;
    else  {
        c = RSinfo.sourceString;
        while ((!isWordChar(c)) && (lettertype(c) != a_null))  c++;
        if ((lettertype(c) != a_null) && (warningChar == NULL))  warningChar = c;
    }
    
    if (warningChar != NULL)  {
        setWarning(string_read_err, pcCodePtr-1);
        if (doPrintError)  printError(NULL, -1, theString, (ccInt) (warningChar-theString), true, 1, false);      }
    
    return passed;
}


bool isWordChar(const char *c)  {
    ccInt charType = lettertype(c);
    return ((charType == a_letter) || (charType == a_number) || (charType == a_symbol));
}



// print_string() writes the contents of its arguments into a string.
// Numeric data is written out in ASCII.

ccInt cc_print_string(argsType args)
{
    printStringInfo PSinfo;
    view windowView;
    ccInt c2, a;
    arg *theString;
    
    if (args.num < 2)  return wrong_argument_count_err;
    if ((args.type[1][0] != int_type) && (args.type[1][0] != double_type))  return type_mismatch_err;
    
    returnOnErr(getArgs(args, scalarRef(string_type, &theString), byValue(&(PSinfo.maxFloatingDigits)), endArgs))
    
    if (PSinfo.maxFloatingDigits < 0)  return out_of_range_err;
    if (PSinfo.maxFloatingDigits > maxPrintableDigits)  PSinfo.maxFloatingDigits = maxPrintableDigits;
    
    
        // compute the byte size of the final string / print it
    
    PSinfo.stringPtr = NULL;
    for (c2 = 0; c2 < 2; c2++)  {
        PSinfo.numChars = 0;
        for (a = 2; a < args.num; a++)  {
            windowView.windowPtr = (window *) args.p[a];
            if (windowView.windowPtr != NULL)  {
                windowView.offset = windowView.windowPtr->offset;
                windowView.width = windowView.windowPtr->width;
                printViewString(&windowView, &PSinfo, NULL);
        }   }
        if (c2 == 0)  {
            if (PSinfo.numChars == 0)  return passed;
            returnOnErr(setStringSize(theString, 1, PSinfo.numChars, &(PSinfo.stringPtr))) //&strPtr))
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
    char *str, *substr;
    ccInt strLen, substrLen, startingPosition, numMatches, charCounter, mode, matchStart, *position;
    
    if (args.num != 5)  return wrong_argument_count_err;
    returnOnErr(getArgs(args, arrayRef(char_type, &str), arrayRef(char_type, &substr), scalarValue(int_type, &mode),
            scalarValue(int_type, &startingPosition), scalarRef(int_type, &position)))
    
    strLen = args.indices[0];
    substrLen = args.indices[1];
    
    *position = 0;
    if (strLen == 0)  return passed;
    if (substrLen == 0)  {
        if (mode == 0)  *position = 0;
        else  *position = startingPosition;
        return passed;     }
    
    numMatches = 0;
    if (mode > -1)  {               // search forwards
    for (matchStart = startingPosition-1; matchStart <= strLen-substrLen; matchStart++)  {
        
        for (charCounter = 0; charCounter < substrLen; charCounter++)  {
        if (str[matchStart+charCounter] != substr[charCounter])   {
            break;
        }}
        
        if (charCounter == substrLen)  {
            if (mode == 1)   {  *position = matchStart+1;  return passed;   }
            else  numMatches++;     // count
    }}  }
    
    else    {                       // search backwards
    for (matchStart = startingPosition-1; matchStart >= 0; matchStart--)  {
        
        for (charCounter = 0; charCounter < substrLen; charCounter++)  {
        if (str[matchStart+charCounter] != substr[charCounter])   {
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
    
    returnOnErr(getArgs(args, arrayRef(double_type, &array)))
    
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

ccInt cc_round(argsType args)  {  return mathUnaryOp(args, &doRound);  }
ccFloat doRound(ccFloat argument)  {  return floor(argument + 0.5);  }

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
    
    if (args.indices[1] != args.indices[0])  return mismatched_indices_err;
    returnOnErr(getArgs(args, arrayRef(double_type, &source), arrayRef(double_type, &dest)))
    
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
    
    returnOnErr(getArgs(args, arrayRef(double_type, &source1), arrayRef(double_type, &source2), arrayRef(double_type, &dest)))
    
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
        
        returnOnErr(getArgs(args, arrayRef(composite_type, &theWindow)))
        
        for (loopMember = theWindow->variable_ptr->mem.members.elementNum; loopMember >= 1; loopMember--)  {
        if (LL_member(theWindow->variable_ptr, loopMember)->ifHidden)  {
            removeMember(theWindow->variable_ptr, loopMember);
            if (errCode != passed)  return errCode;
    }   }}
    
    else  return wrong_argument_count_err;
    
    return passed;
}



// Functions for arg-type variables

ccInt getArgTop(arg *theArg)
{
    window *argWindow = (window *) theArg;
    variable *argVar = argWindow->variable_ptr;
    
    if (argVar->types[0] == array_type)  {
        member *argMember = LL_member(argVar, 1);
        return argMember->indices;
    }
    
    else if (argVar->types[0] == list_type)  {
        member *argMember = LL_member(argVar, argWindow->width);
        return argMember->indices;
    }
    
    return argVar->mem.members.elementNum;
}

ccInt setMemberTop(arg *theArg, const ccInt memberNumber, const ccInt listSize, char **listData)
{
    window *argWindow = (window *) theArg;
    member *theArgMember;
    ccInt *varTypes = argWindow->variable_ptr->types, width = 1;
    
    if (varTypes[0] != list_type)  width = argWindow->width;
    
    if (varTypes[0] == composite_type)  theArgMember = LL_member(argWindow->variable_ptr, memberNumber);
    else if (varTypes[0] == array_type)  theArgMember = LL_member(argWindow->variable_ptr, 1);
    else  theArgMember = LL_member(argWindow->variable_ptr, argWindow->offset+memberNumber);
    
    resizeMember(theArgMember, width, (ccInt) listSize);
    if (errCode != passed)  return errCode;
    
    if ((varTypes[1] < composite_type) && (listData != NULL) && (listSize > 0))  {
        linkedlist *listLL = &(theArgMember->memberWindow->variable_ptr->mem.data);
        returnOnErr(defragmentLinkedList(listLL));
        *listData = (char *) element(listLL, theArgMember->memberWindow->offset+1);
    }
    
    return passed;
}


ccInt setStringSize(arg *theString, const ccInt stringNumber, const ccInt stringSize, char **stringData)
{
    return setMemberTop(theString, stringNumber, stringSize, stringData);
}


arg *stepArg(arg *theArg, ccInt memberNumber, ccInt *numIndices)
{
    variable *argVar = ((window *) theArg)->variable_ptr;
    member *argMember;
    
    if ((memberNumber < 1) || (memberNumber > argVar->mem.members.elementNum))  return NULL;
    
    argMember = LL_member(argVar, memberNumber);
    if (numIndices != NULL)  *numIndices *= argMember->indices;
    
    return (arg *) argMember->memberWindow;
}

void *argData(arg *theArg)
{
    window *argWindow = (window *) theArg;
    linkedlist *argDataLL = &(argWindow->variable_ptr->mem.data);
    
    if (defragmentLinkedList(argDataLL) != passed)  {
        setError(out_of_memory_err, pcCodePtr-1);
        return NULL;        }
    
    return element(argDataLL, argWindow->offset+1);
}




// Misc -- useful for loading arguments into the user's C routines.

#define brk(e) {rtrn=e;break;}
ccInt getArgs(argsType args, ...)
{
    ccInt a, mode, expectedType, *argType, rtrn = passed;
    va_list theArgs;
    char **nextarg;
    const static bool checkArgType[6] = { false, false, true, true, true, true };
    const static bool expectScalar[6] = { false, false, true, true, false, false };
    const static bool copyValue[6] = { false, true, true, false, true, false };
    
    va_start(theArgs, args);
    for (a = 0; a < args.num; a++)  {
        nextarg = va_arg(theArgs, char **);
        if (nextarg != NULL)  {
            if ((nextarg < &argDummies[0]) || (nextarg > &argDummies[4]))  mode = 0;
            else  mode = (ccInt) (nextarg-&argDummies[0])+1;
            
            if (checkArgType[mode])  {
                if (args.p[a] == NULL)  brk(void_member_err)
                if ((expectScalar[mode]) && (args.indices[a] != 1))  brk(multiple_indices_not_allowed_err)
                argType = args.type[a];
                do  {
                    expectedType = va_arg(theArgs, int);
                    if (expectedType != argType[0])  brk(type_mismatch_err)
                    argType++;
                }  while (expectedType > composite_type);
                if (rtrn != passed)  break;
            }
            
            if (mode > 0)  nextarg = va_arg(theArgs, char **);
            if (!copyValue[mode])  *nextarg = (char *) args.p[a];
            else  {
                size_t numBytes = (size_t) args.indices[a]*typeSizes[args.type[a][0]];
                if (numBytes > 0)  memcpy((void *) nextarg, (void *) args.p[a], numBytes);
        }   }
        
        else  {
            a = ((ccInt) va_arg(theArgs, int)) - 1;
            if (a < -1)  break;
    }   }
    va_end(theArgs);
    
    return rtrn;
}

char *argDummies[5];
