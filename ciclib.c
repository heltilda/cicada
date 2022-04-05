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
#include <stdio.h>
#include "ciclib.h"
#include "userfn.h"
#include "cmpile.h"
#include "bytecd.h"
#include "ccmain.h"
#include "lnklst.h"




// Jump tables for the built-in functions and variables.

void(*BuiltInFunctionJumpTable[])(void) =
         {  &cclib_call, &cclib_setCompiler, &cclib_compile, &cclib_transform, &cclib_load,
                &cclib_save, &cclib_input, &cclib_print, &cclib_read_string, &cclib_print_string,
            &cclib_trap, &cclib_throw, &cclib_top, &cclib_size, &cclib_abs,
                &cclib_floor, &cclib_ceil, &cclib_log, &cclib_cos, &cclib_sin,
            &cclib_tan, &cclib_acos, &cclib_asin, &cclib_atan, &cclib_random,
                &cclib_find, &cclib_type, &cclib_member_ID, &cclib_bytecode, &springCleaning  };

ccInt bi_commands_num = sizeof(BuiltInFunctionJumpTable)/sizeof(void *);




// The built-in Cicada library functions follow.

// cclib_call():  runs a user-defined function in, or referenced in, userfn.c(pp)

void cclib_call()
{
    window *functionNameWindow;
    view argView;
    char **pointerList, **argvHandles;
    ccInt functionID, charCounter, argCounter, numArgs, numDataWindows, numStrings, theType;
    arg_info *infoList, *argvInfo;
    ccBool match;
    
    numArgs = numBIF_args();
    if (numArgs < 1)  {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }
    
    
        // get first argument:  the function name/ID number
    
    functionNameWindow = getBIFmember(1);
    if (functionNameWindow == NULL)  {  setError(void_member_err, pcCodePtr-1);  return;  }
    if (functionNameWindow->width != 1)  {  setError(multiple_indices_not_allowed_err, pcCodePtr-1);  return;  }
    
    
        // if the user passed a name, then find its position in the UserFunctions list
    
    theType = functionNameWindow->variable_ptr->type;
    if (theType == string_type)    {
        
        window *functionNameCharWindow = LL_member(functionNameWindow->variable_ptr, functionNameWindow->offset+1)->memberWindow; 
        linkedlist *functionNameString = &(functionNameCharWindow->variable_ptr->mem.data); 
        if (errCode != passed)  return;   
        
        for (functionID = 0; functionID < userFunctionsNum; functionID++)   {
            match = ccTrue;
            for (charCounter = functionNameCharWindow->offset; 
                            charCounter < functionNameCharWindow->offset+functionNameCharWindow->width; charCounter++)   {
                if ((*(char *) element(functionNameString, charCounter+1) != UserFunctions[functionID].functionName[charCounter]) ||
                                    (UserFunctions[functionID].functionName[charCounter] == 0))   {
                    {  match = ccFalse;  break;  }    // 2nd condition to prevent crash if 0 is in the functionID string
            }   }
            if (UserFunctions[functionID].functionName[charCounter] != 0)  match = ccFalse;
            if (match)  break;     }
        
        if (functionID == userFunctionsNum)
            {  setError(user_function_not_found_err, pcCodePtr-1);  return;  }    }
    
    
        // if the user passed the position directly, we have less work to do
    
    else if ((theType >= int_type) && (theType <= double_type))  {
        
        functionID = (ccInt) getBIFnumArg(1, 1., (ccFloat) userFunctionsNum) - 1;
        if (errCode == out_of_range_err)  errCode = user_function_not_found_err;
        if (errCode != passed)  return;     }
    
    else  {  setError(string_expected_err, pcCodePtr-1);  return;  }
    
    
        // Now count the number of primitive arrays to pass to the function (note that variables count as arrays of 1)
        // These come from arguments 2-the last.
    
    numDataWindows = 0;
    numStrings = 0;
    
    extCallMode = ccTrue;
    
    for (argCounter = 2; argCounter <= numArgs; argCounter++)  {
        argView.windowPtr = getBIFmember(argCounter);
        if (argView.windowPtr != NULL)   {
            argView.offset = argView.windowPtr->offset;
            argView.width = argView.windowPtr->width;
            countDataLists(&argView, (void *) &numDataWindows, (void *) &numStrings);
            if (errCode != passed)  break;
    }   }
    if (errCode != passed)  {  extCallMode = ccFalse;  return;  }
    
    
        // Allocate space both for our argument pointers and for the info list that Cicada provides.
    
    pointerList = (char **) malloc((numDataWindows+numStrings+1)*sizeof(char *));
    if (pointerList == NULL)  {  setError(out_of_memory_err, pcCodePtr-1);  extCallMode = ccFalse;  return;  }
    infoList = (arg_info *) malloc((numDataWindows+numStrings)*sizeof(arg_info) + 4);     //  avoid size-0 malloc
    if (infoList == NULL)  {  setError(out_of_memory_err, pcCodePtr-1);  extCallMode = ccFalse;  return;  }
    
    
        // Now fill the argument pointer/info lists.
    
    argvHandles = pointerList;
    argvInfo = infoList;
    for (argCounter = 2; argCounter <= numArgs; argCounter++)  {
        argView.windowPtr = getBIFmember(argCounter);
        if (argView.windowPtr != NULL)  {
            argView.offset = argView.windowPtr->offset;
            argView.width = argView.windowPtr->width;
            argvFillHandles(&argView, (void *) &argvHandles, (void *) &argvInfo);
            if (errCode != passed)  break;
    }   }
    pointerList[numDataWindows+numStrings] = (char *) infoList;
    
    
        // store the return value of the function in intRegister
    
    if (errCode == passed)
        intRegister = (ccInt) UserFunctions[functionID].functionPtr(numDataWindows+numStrings, (char **) pointerList);
    
    
        // fix the string windows (in case the string linked lists were resized) and clean up
    
    argvHandles = pointerList;
    for (argCounter = 2; argCounter <= numArgs; argCounter++)  {
        argView.windowPtr = getBIFmember(argCounter);
        if (argView.windowPtr != NULL)  {
            argView.offset = argView.windowPtr->offset;
            argView.width = argView.windowPtr->width;
            argvFixStrings(&argView, (void *) &argvHandles, NULL);
    }   }
    
    free((void *) pointerList);
    free((void *) infoList);
    
    extCallMode = ccFalse;
}




// setCompiler() switches the compiler used by compile()
 
void cclib_setCompiler()
{
    ccInt numArgs = numBIF_args(), rtrn;
    
    
        // if we're just trying to switch compilers, allow that
    
    if (numArgs == 1)  {
        whichCompiler = (ccInt) getBIFnumArg(1, 1., (ccFloat) allCompilers.elementNum);  
        if (errCode == passed)  {
            currentCompiler = *(compiler_type **) element(&allCompilers, whichCompiler);
    }   }
    
    
        // if we pass two arguments, that means that we're defining a new compiler
    
    else if (numArgs == 2)  {
        window *argWindows[5], *arg1Window = getBIFmember(1), *arg2Window = getBIFmember(2), *arg1CompositeWindow;
        member *argMembers[5], *oneArgMember, *arg1Member;
        compiler_type *tempCompiler = NULL;
        commandTokenType *cmdTokens;
        ccInt numCommands, numOoOs, loopArg, loopCommand, *precedences, argMemberNumber, entryOffset, windowOffsets[5];
        ccInt expectedType[5] = {  string_type, int_type, string_type, string_type, int_type  };
        
        if (arg1Window == NULL)  setError(void_member_err, pcCodePtr-1);
        else if (arg2Window == NULL)  setError(void_member_err, pcCodePtr-1);
        else if (arg1Window->variable_ptr->type != array_type)  setError(not_composite_err, pcCodePtr-1);
        else if (arg2Window->variable_ptr->type != array_type)  setError(not_composite_err, pcCodePtr-1);
        if (errCode != passed)  return;
        
        arg1Member = LL_member(arg1Window->variable_ptr, 1);
        arg1CompositeWindow = arg1Member->memberWindow;

        if (arg1CompositeWindow == NULL)  setError(void_member_err, pcCodePtr-1);
        if (arg1CompositeWindow->variable_ptr->type != composite_type)  setError(not_composite_err, pcCodePtr-1);
        if (errCode != passed)  return;
        
        for (loopArg = 0; loopArg < 5; loopArg++)  {
            if (loopArg < 4)  {
                findMemberIndex(arg1CompositeWindow->variable_ptr, 0, loopArg+1, &oneArgMember, &argMemberNumber, &entryOffset, ccFalse);
                argWindows[loopArg] = oneArgMember->memberWindow;
                windowOffsets[loopArg] = arg1Window->offset * arg1Member->indices;      }
            else  {
                argWindows[loopArg] = LL_member(arg2Window->variable_ptr, 1)->memberWindow;
                windowOffsets[loopArg] = 0;         }
            windowOffsets[loopArg] += argWindows[loopArg]->offset;
            
            if (errCode != passed)  return;
            if (argWindows[loopArg] == NULL)  {  setError(void_member_err, pcCodePtr-1);  return;  }
            if (argWindows[loopArg]->variable_ptr->type != expectedType[loopArg])  {
                if (expectedType[loopArg] == string_type)  setError(string_expected_err, pcCodePtr-1);
                else  setError(not_a_number_err, pcCodePtr-1);
                return;     }
            
            if (defragmentLinkedList(&(argWindows[loopArg]->variable_ptr->mem.data)) != passed)  {
                setError(void_member_err, pcCodePtr-1);
                return;         }
            if (argWindows[loopArg]->width == 0)  argMembers[loopArg] = NULL;
            else  argMembers[loopArg] = LL_member(argWindows[loopArg]->variable_ptr, windowOffsets[loopArg]+1);
            
            if (expectedType[loopArg] == string_type)  {
            for (loopCommand = 0; loopCommand < argWindows[loopArg]->width; loopCommand++)  {
                if (defragmentLinkedList(&(argMembers[loopArg][loopCommand].memberWindow->variable_ptr->mem.data)) != passed)  {
                    setError(out_of_memory_err, pcCodePtr-1);
                    return;
        }   }}  }
        
        numCommands = arg1Member->indices;
        numOoOs = LL_member(arg2Window->variable_ptr, 1)->indices;
        if (numCommands == 0)  precedences = NULL;
        else  precedences = LL_int(&(argWindows[1]->variable_ptr->mem.data), 1);
        
        cmdTokens = (commandTokenType *) malloc(numCommands*sizeof(commandTokenType));
        
        if (cmdTokens == NULL)  setError(out_of_memory_err, pcCodePtr-1);  
        else  {
            for (loopCommand = 0; loopCommand < numCommands; loopCommand++)  {
                cmdTokens[loopCommand].cmdString =
                        (const char *) getCompilerString(argWindows[0], windowOffsets[0], loopCommand);
                cmdTokens[loopCommand].precedence = precedences[loopCommand];
                cmdTokens[loopCommand].rtrnTypeString =
                        (const char *) getCompilerString(argWindows[2], windowOffsets[2], loopCommand);
                cmdTokens[loopCommand].translation =
                        (const char *) getCompilerString(argWindows[3], windowOffsets[3], loopCommand);    }
            
            if (errCode == passed)  {
                tempCompiler = newCompiler(cmdTokens, numCommands,
                            LL_int(&(argWindows[4]->variable_ptr->mem.data), argWindows[4]->offset+1), numOoOs, &rtrn);
                if (rtrn != passed)  setError(rtrn, pcCodePtr-1);
                
                else  {
                    rtrn = addElements(&allCompilers, 1, ccFalse);
                    if (rtrn != passed)  setError(rtrn, pcCodePtr-1);
                    else  {
                        currentCompiler = tempCompiler;
                        whichCompiler = allCompilers.elementNum;
                        *(compiler_type **) element(&allCompilers, whichCompiler) = tempCompiler;
            }   }   }
            
            for (loopCommand = 0; loopCommand < numCommands; loopCommand++)  {
                free((void *) cmdTokens[loopCommand].cmdString);
                free((void *) cmdTokens[loopCommand].rtrnTypeString);
                free((void *) cmdTokens[loopCommand].translation);        }
            free((void *) cmdTokens);
    }   }
    
        // zero args is allowed -- that means the user is just requesting the current compiler number
    
    else if (numArgs != 0)  setError(wrong_argument_count_err, pcCodePtr-1);
    
    intRegister = whichCompiler;
}


char *getCompilerString(window *stringsWindow, ccInt windowOffset, ccInt whichCommand)
{
    window *charsWindow = LL_member(stringsWindow->variable_ptr, windowOffset+whichCommand+1)->memberWindow;
    linkedlist *charsLL = &(charsWindow->variable_ptr->mem.data);
    
    char *stringCopy = (char *) malloc(charsWindow->width+1);
    if (stringCopy == NULL)  setError(out_of_memory_err, pcCodePtr-1);
    else  {
        getElements(charsLL, charsWindow->offset+1, charsWindow->offset+charsWindow->width, (void *) stringCopy);
        stringCopy[charsWindow->width] = 0;     }
    
    return stringCopy;
}




// compile() generates bytecode from a script (a string).  Writes errors/warnings into the error/warning registers.

void cclib_compile()
{
    window *textStringWindow, *varNamesWindow;
    member *varNamesStringMember;
    linkedlist *textString;
    ccInt numArgs = numBIF_args(), nextVarName, rtrn;
    char *scriptCopy;
    
    if ((numArgs < 1) || (numArgs > 4))  {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }
    
    
        // make sure the script ends with a 0 (which we will later take out) and lies in one contiguous block of memory
    
    textStringWindow = getBIFstringArg(1);
    if (errCode != passed)  return;
    textString = &(textStringWindow->variable_ptr->mem.data);
    
    
        // add the trailing null character at the end of (our own copy of) the script
    
    scriptCopy = (char *) malloc(textStringWindow->width+1);
    if (scriptCopy == NULL)  {  setError(out_of_memory_err, pcCodePtr-1);  return;  }
    getElements(textString, textStringWindow->offset+1, textStringWindow->offset+textStringWindow->width, (void *) scriptCopy);
    scriptCopy[textStringWindow->width] = 0;
    
    
        // Run the compiler.  Store the error code/index (which may be zero -- no error).
    
    rtrn = compile(currentCompiler, scriptCopy);
    
    if (((rtrn != passed) || (compilerWarning != passed)) && (doPrintError))  {
        
        char *fileString = NULL;
        ccInt fileStringLength = -1;
        
        if (numArgs >= 2)  {
        if (getBIFmember(2) != NULL)  {
            window *fileStringWindow = getBIFstringArg(2);
            if (errCode != passed)  {  free((void *) scriptCopy);  return;  }
            if (fileStringWindow->width > 0)  {
                if (defragmentLinkedList(&(fileStringWindow->variable_ptr->mem.data)) != passed)  rtrn = out_of_memory_err;
                else  {
                    fileString = (char *) element(&(fileStringWindow->variable_ptr->mem.data), fileStringWindow->offset+1);
                    fileStringLength = fileStringWindow->width;
        }}  }   }
        
        if (rtrn != passed)  setError(rtrn, pcCodePtr-1);
        else  setWarning(compilerWarning, pcCodePtr-1);
        
        printError(fileString, fileStringLength, scriptCopy, errPosition-1, ccFalse);
        doPrintError = ccFalse;             }
    
    else if (rtrn != passed)  setError(rtrn, pcCodePtr-1);
    
    
        // delete the null character we put in at the end of the string
    
    free((void *) scriptCopy);
    
    if (errCode != passed)  return;
    
    
        // store the bytecode string
    
    rtrn = resizeLinkedList(&stringRegister, currentCompiler->bytecode.elementNum * sizeof(ccInt), ccFalse);
    if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr-1);  return;  }
    defragmentLinkedList(&stringRegister);
    
    if (currentCompiler->bytecode.elementNum > 0)
        getElements(&(currentCompiler->bytecode), 1, currentCompiler->bytecode.elementNum, element(&stringRegister, 1));
    
    
        // store the bytecode word -> text position mapping in the provided variable, if it was given
    
    if (numArgs >= 3)  {
        window *opCharNumWindow = NULL, *arg3Window = getBIFmember(3);
        linkedlist *opCharNumLL;
        if (errCode != passed)  return;
        
        if (arg3Window != NULL)  {
            resizeMember(LL_member(arg3Window->variable_ptr, arg3Window->offset+1), 1, currentCompiler->opCharNum.elementNum*sizeof(ccInt));
            if (errCode == passed)  opCharNumWindow = getBIFstringArg(3);
            if (errCode != passed)  return;
            
            opCharNumLL = &(opCharNumWindow->variable_ptr->mem.data);
            rtrn = defragmentLinkedList(opCharNumLL);
            if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
            
            if (currentCompiler->opCharNum.elementNum > 0)  {
                getElements(&(currentCompiler->opCharNum), 1, currentCompiler->opCharNum.elementNum,
                        (void *) element(opCharNumLL, opCharNumWindow->offset+1));
    }   }   }
    
    
        // store the bytecode word -> text position mapping in the provided variable, if it was given
    
    if (numArgs >= 4)  {
        varNamesWindow = getBIFmember(4);
        if (errCode != passed)  return;
        
        if (varNamesWindow != NULL)  {
            variable *varNamesVar = varNamesWindow->variable_ptr;
            member *varNamesMember;
            
            if ((varNamesVar->eventualType != string_type) || (varNamesVar->arrayDepth != 1))
                {  setError(library_argument_err, pcCodePtr-1);  return;  }
            varNamesStringMember = LL_member(varNamesWindow->variable_ptr, varNamesWindow->offset+1);
            
            varNamesMember = LL_member(varNamesWindow->variable_ptr, 1);
            nextVarName = varNamesMember->indices+1;
            resizeMember(varNamesMember, 1, currentCompiler->varNames.elementNum);
            if (errCode != passed)  return;
            
            if (varNamesMember->memberWindow != NULL)  {
                variable *varNamesStringVar = varNamesMember->memberWindow->variable_ptr;
                
                while (currentCompiler->varNames.elementNum >= nextVarName)  {
                    
                    member *stringCharsMember = LL_member(varNamesStringVar, varNamesMember->memberWindow->offset+nextVarName);
                    varNameType *oneVarName = (varNameType *) element(&(currentCompiler->varNames), nextVarName);
                    
                    resizeMember(stringCharsMember, 1, oneVarName->nameLength);
                    setElements(&(stringCharsMember->memberWindow->variable_ptr->mem.data), stringCharsMember->memberWindow->offset+1,
                                stringCharsMember->memberWindow->offset+oneVarName->nameLength, (void *) oneVarName->theName);
                    
                    nextVarName++;
    }   }   }   }
}



// cclib_transform() takes bytecode and stores it in Cicada's code registry.
// Checks the code first to make sure it won't crash the interpreter.

void cclib_transform()
{
    linkedlist *inputStringLL, *tempString;
    member *loopMember;
    window *compiledCodeWindow = NULL, *pathWindow = NULL, *inputStringWindow, *drawPathTo;
    variable *pathVariable = NULL;
    searchPath *newCodePath, *newCodePathStem = pcSearchPath;
    ccInt pathVarMemberCounter, firstPathVarMember = 0, counter, loopArg, loopCodeWord, rtrn;
    ccInt codeIndex, numArgs, dummy, memberOffset, fileNameLength = 0, sourceCodeLength = 0, opCharNumLength = 0;
    ccInt *holdPCCodePtr, *codeEntryPtr, *holdStartCodePtr, *holdEndCodePtr, *opCharNum = NULL, theOpChar;
    char *fileName = NULL, *sourceCode = NULL;
    char **lastArgs[3] = { &fileName, &sourceCode, (char **) &opCharNum };
    ccInt *lastArgLengths[3] = { &fileNameLength, &sourceCodeLength, &opCharNumLength };
    code_ref holdPCCodeRef;
    
    numArgs = numBIF_args();
    if ((numArgs < 1) || (numArgs > 6))  {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }
    
    
        // check & defragment the bytecode string
    
    inputStringWindow = getBIFstringArg(1);
    if (errCode != passed)  return;
    if (inputStringWindow->width == 0)  {  intRegister = passed;  return;  }
    
    inputStringLL = &(inputStringWindow->variable_ptr->mem.data);
    rtrn = defragmentLinkedList(inputStringLL);
    if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr-1);  return;  }
    if (inputStringWindow->width % sizeof(ccInt) != 0)  {  setError(library_argument_err, pcCodePtr-1);  return;  }
    
    
        // get the destination function (argument 2)
    
    if (numArgs >= 2)  {
        compiledCodeWindow = getBIFmember(2);
        if (compiledCodeWindow != NULL)  {
            if (compiledCodeWindow->variable_ptr->type != composite_type)  {  setError(not_a_function_err, pcCodePtr-1);  return;  }
            if (compiledCodeWindow->width != 1)  {  setError(multiple_indices_not_allowed_err, pcCodePtr-1);  return;  }
    }   }
    
    
        // if a code path was provided, read that too
    
    if (numArgs >= 3)  {
        pathWindow = getBIFmember(3);
        if (pathWindow != NULL)    {
            pathVariable = pathWindow->variable_ptr;
            
            if (pathWindow->variable_ptr->type < composite_type)  {  setError(not_composite_err, pcCodePtr-1);  return;  }
            if (pathWindow->width != 1)  {  setError(multiple_indices_not_allowed_err, pcCodePtr-1);  return;  }
            
            newCodePathStem = NULL;
            pathVarMemberCounter = 0;
            while (1 == 1)  {
                pathVarMemberCounter++;
                rtrn = findMemberIndex(pathVariable, 0, pathVarMemberCounter, &loopMember, &dummy, &memberOffset, ccFalse);
                if (rtrn == invalid_index_err)  break;
                if (!loopMember->ifHidden)  {
                    if (loopMember->memberWindow != NULL)  {
                        firstPathVarMember = pathVarMemberCounter;
                        break;
    }   }   }   }   }
    
    
        // the last three optional arguments help to flag transform/runtime errors in the code
    
    for (loopArg = 4; loopArg <= numArgs; loopArg++)  {
    if (getBIFmember(loopArg) != NULL)  {
        window *tempStringWindow = getBIFstringArg(loopArg);
        if (errCode != passed)  return;
        
        tempString = &(tempStringWindow->variable_ptr->mem.data);
        rtrn = defragmentLinkedList(tempString);
        if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
        
        *lastArgLengths[loopArg-4] = tempStringWindow->width;
        if (tempStringWindow->width == 0)  {
            *lastArgs[loopArg-4] = (char *) element(tempString, 0);     }
        else  {
            *lastArgs[loopArg-4] = (char *) element(tempString, tempStringWindow->offset+1);
    }}  }
    
    if (opCharNum != NULL)  {
        if (sourceCode == NULL)  {  setError(void_member_err, pcCodePtr-1);  return;  }
        if (inputStringWindow->width != opCharNumLength)  {  setError(out_of_range_err, pcCodePtr-1);  return;  }
        for (loopCodeWord = 0; loopCodeWord < opCharNumLength/sizeof(ccInt); loopCodeWord++)  {
        if ((opCharNum[loopCodeWord] < 1) || (opCharNum[loopCodeWord] > sourceCodeLength))  {
        if (sourceCodeLength > 0)  {            // even a blank script has an end-of-file token --> char #1 
            setError(out_of_range_err, pcCodePtr-1);
            return;
    }   }}}
    
    else if (sourceCode != NULL)  {  setError(void_member_err, pcCodePtr-1);  return;  }
    
    
        // store some important variables since we need to overwrite these temporarily while checking the bytecode
    
    holdPCCodePtr = pcCodePtr;
    holdStartCodePtr = startCodePtr;
    holdEndCodePtr = endCodePtr;
    holdPCCodeRef = PCCodeRef;
    
    
        // set the PC at the beginning of the bytecode, and run it in 'check' mode
    
    startCodePtr = LL_int(inputStringLL, inputStringWindow->offset+1);
    endCodePtr = startCodePtr + (ccInt) ceil(1.*inputStringWindow->width/sizeof(ccInt));
    pcCodePtr = startCodePtr;
    PCCodeRef.anchor = NULL;
    PCCodeRef.code_ptr = startCodePtr;
    
    checkBytecode();
    
    if ((errCode != passed) && (doPrintError))  {
        if (opCharNum == NULL)  theOpChar = 0;
        else  theOpChar = opCharNum[errIndex-1]-1;
        printError(fileName, fileNameLength, sourceCode, theOpChar, ccFalse);
        
        doPrintError = ccFalse;        }
    
    
        // restore the important variables
    
    PCCodeRef = holdPCCodeRef;
    pcCodePtr = holdPCCodePtr;
    startCodePtr = holdStartCodePtr;
    endCodePtr = holdEndCodePtr;
    
    if (errCode != passed)  return;
    
    
        // OK, it worked.  Make a new anchor for the code.
    
    if (newCodePathStem != NULL)  refPath(newCodePathStem);       // temporarily over-reference each new path as we create it
    if (firstPathVarMember != 0)    {
        
        pathVarMemberCounter = firstPathVarMember-1;
        while (1 == 1)  {
            pathVarMemberCounter++;
            rtrn = findMemberIndex(pathVariable, 0, pathVarMemberCounter, &loopMember, &dummy, &memberOffset, ccFalse);
            if (rtrn == invalid_index_err)  break;
            if ((!loopMember->ifHidden) && (loopMember->memberWindow != NULL))  {
                rtrn = drawPath(&newCodePath, loopMember->memberWindow, newCodePathStem, 1, PCCodeRef.PLL_index);
                if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
                refPath(newCodePath);                               // so it doesn't get deleted by a later path in the same window
                derefPath(&newCodePathStem);
                newCodePathStem = newCodePath;
    }   }   }
    
    if (compiledCodeWindow != NULL)  drawPathTo = compiledCodeWindow;
    else  drawPathTo = PCCodeRef.anchor->jamb;
    
    rtrn = drawPath(&newCodePath, drawPathTo, newCodePathStem, 1, PCCodeRef.PLL_index);
    if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
    refPath(newCodePath);
    derefPath(&newCodePathStem);
    
    
        // wipe up any existing codes in our function
    
    if (compiledCodeWindow != NULL)  {
        for (counter = 1; counter <= compiledCodeWindow->variable_ptr->codeList.elementNum; counter++)
            derefCodeRef((code_ref *) element(&(compiledCodeWindow->variable_ptr->codeList), counter));
        deleteElements(&(compiledCodeWindow->variable_ptr->codeList), 1, compiledCodeWindow->variable_ptr->codeList.elementNum);    }
    
    
        // add our new code to the Registry..
    
    rtrn = addCode(LL_int(inputStringLL, inputStringWindow->offset+1), &codeEntryPtr, &codeIndex,
            (ccInt) ceil(1.*inputStringWindow->width/sizeof(ccInt)), fileName, fileNameLength, sourceCode, opCharNum, sourceCodeLength);
    if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
    
    
        // and to the code list of the function
    
    if (compiledCodeWindow != NULL)  {
        rtrn = addCodeRef(&(compiledCodeWindow->variable_ptr->codeList), newCodePath, codeEntryPtr, codeIndex);
        if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }        }
    
    derefCodeList(&codeRegister);
    
    resizeLinkedList(&codeRegister, 0, ccFalse);
    rtrn = addCodeRef(&codeRegister, newCodePath, codeEntryPtr, codeIndex);
    if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }

    derefPath(&newCodePath);
    
    GL_Object.codeList = &codeRegister;
    codeNumber = 1;
}



// cclib_load() reads the contents of a file

void cclib_load()
{
    window *fileNameWindow;
    linkedlist *fileNameLL;
    ccInt rtrn;
    char *fileString;
    
    
        // get the name of the file (arg 1)
    
    if (numBIF_args() != 1)  {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }
    
    fileNameWindow = getBIFstringArg(1);
    if (errCode != passed)  return;
    fileNameLL = &(fileNameWindow->variable_ptr->mem.data);
    
    
        // copy the file name, plus a null terminator, into a separate buffer
    
    fileString = (char *) malloc(fileNameWindow->width+1);
    if (fileString == NULL)  {  setError(out_of_memory_err, pcCodePtr-1);  return;  }
    getElements(fileNameLL, fileNameWindow->offset+1, fileNameWindow->width, fileString);
    fileString[fileNameWindow->width] = 0;
    
    
        // try to open the file; throw an error if unsuccessful
    
    rtrn = loadFile(fileString, &stringRegister, ccFalse);
    if (rtrn != passed)  setError(rtrn, pcCodePtr-1);
    
    free((void *) fileString);      // clean up
}


// loadFile():  a routine to load a text file into a linked list

ccInt loadFile(char *fileString, linkedlist *textString, ccBool addFinalNull)
{
    ccInt fileSize, LLsize, bytesRead, rtrn;
    FILE *fileToRead;
    
    
        // try to open the file; throw an error if unsuccessful
    
    fileToRead = fopen(fileString, "rb");
    if (fileToRead == NULL)  return IO_error;
    
    
        // get the size of the file
    
    fseek(fileToRead, 0, SEEK_END);
    fileSize = (ccInt) ftell(fileToRead);
    rewind(fileToRead);
    
    
        // allocate memory and read the file
    
    LLsize = fileSize;
    if (addFinalNull)  LLsize++;
    rtrn = resizeLinkedList(textString, LLsize, ccFalse);
    if (rtrn == passed)  rtrn = defragmentLinkedList(textString);
    if (rtrn != passed)  return rtrn;
    
    if (fileSize > 0)    {
        bytesRead = (ccInt) fread(element(textString, 1), 1, (size_t) fileSize, fileToRead);
        if (bytesRead != fileSize)  {
            fclose(fileToRead);
            return IO_error;
    }   }
    
    rtrn = fclose(fileToRead);
    if (rtrn != 0)  rtrn = IO_error;
    
    if (addFinalNull)  *LL_Char(textString, LLsize) = 0;
    
    return rtrn;
}



// cclib_save() saves a string into a file.  Overwrites the file if it already exists.

void cclib_save()
{
    FILE *fileToWrite;
    window *fileNameWindow, *stringWindow;
    linkedlist *fileNameLL, *stringLL = NULL;
    ccInt bytesWritten, rtrn;
    char *fileString, closeFileRetrn;
    
    if (numBIF_args() != 2)  {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }
    
    
        // get arguments:  file name, string to write
    
    fileNameWindow = getBIFstringArg(1);
    if (errCode != passed)  return;
    stringWindow = getBIFstringArg(2);
    if (errCode != passed)  return;   
    
    fileNameLL = &(fileNameWindow->variable_ptr->mem.data);
    stringLL = &(stringWindow->variable_ptr->mem.data);
    
    
        // defragment the string; copy the file name into a separate buffer
    
    rtrn = defragmentLinkedList(stringLL);
    if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr-1);  return;  }
    
    fileString = (char *) malloc(fileNameWindow->width+1);
    if (fileString == NULL)  {  setError(out_of_memory_err, pcCodePtr-1);  return;  }
    getElements(fileNameLL, fileNameWindow->offset+1, fileNameWindow->offset+fileNameWindow->width, fileString);
    fileString[fileNameWindow->width] = 0;
    
    
        // try to open the file; throw error if unsuccessful
    
    fileToWrite = fopen(fileString, "wb");
    if (fileToWrite == NULL)  {  setError(IO_error, pcCodePtr-1);  free((void *) fileString);  return;  }
    
    
        // write the string to the file
    
    if (stringLL->elementNum > 0)  {
        bytesWritten = (ccInt) fwrite(element(stringLL, stringWindow->offset+1), 1, stringWindow->width, fileToWrite);    }
    else  bytesWritten = 0;
    if (bytesWritten != stringWindow->width)  {  setError(IO_error, pcCodePtr-1);  }
    
    
        // close the file
    
    closeFileRetrn = fclose(fileToWrite);
    if ((errCode == passed) && (closeFileRetrn != 0))  {  setError(IO_error, pcCodePtr-1);  }
    
    free((void *) fileString);    // clean up
}



// cclib_input() copies input from the keyboard, up until the first end-of-line

void cclib_input()
{
    ccInt counter, bytesRead, rtrn;
    const ccInt fileReadBufferSize = 1000;
    char charBuffer[1001], *readRetrn;
    ccBool done;
    
    if (numBIF_args() != 0)  {  cclib_print();  }
    
    rtrn = resizeLinkedList(&stringRegister, 0, ccFalse);
    if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr-1);  return;  }
    
    
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
        
        rtrn = addElements(&stringRegister, bytesRead, ccFalse);         // will be contiguous if spareRoom = 0
        if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr-1);  return;  }
        setElements(&stringRegister, stringRegister.elementNum-bytesRead+1, stringRegister.elementNum, charBuffer);
    }  while (!done);
    
    defragmentLinkedList(&stringRegister);
}




// cclib_print() calls printView() to print its entire argument list to the terminal (screen).

void cclib_print()  {
    printView(&BIF_argsView, NULL, NULL);
    if (fflush(stdout) != passed)  {  setError(IO_error, pcCodePtr-1);  }
}



// cclib_read_string() copies data from a string into its subsequent argument variables

void cclib_read_string()
{
    view windowView;
    window *stringWindow;
    linkedlist *theString;
    ccInt counter, numArgs, oneLetterType;
    char *stringPositionMarker, *holdString, *stringOverflow[2];
    
    numArgs = numBIF_args();
    if (numArgs < 1)  {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }
    
    
        // read in the source string:  argument 1
    
    stringWindow = getBIFstringArg(1);
    if (errCode != passed)  return;
    theString = &(stringWindow->variable_ptr->mem.data);   
    
    
        // make a copy of the source string, just in case the copy process would overwrite the source string
    
    holdString = (char *) malloc(stringWindow->width+1);
    if (holdString == NULL)  {  setError(out_of_memory_err, pcCodePtr-1);  return;  }
    getElements(theString, stringWindow->offset+1, stringWindow->offset+stringWindow->width, (void *) holdString);
    holdString[stringWindow->width] = 0;
    stringOverflow[0] = holdString+stringWindow->width;     // the end of the string
    
    
        // get past any weird characters at the beginning
    
    stringPositionMarker = holdString;
    oneLetterType = lettertype(stringPositionMarker);
    while ((oneLetterType != a_symbol) && (oneLetterType != a_number) && (oneLetterType != a_letter)
                    && (stringPositionMarker < stringOverflow[0]))  {
        stringPositionMarker++;
        oneLetterType = lettertype(stringPositionMarker);     }
    
    
        // Read the string into the subsequent argument variables.  String variables are read up through the next space.
    
    for (counter = 2; counter <= numArgs; counter++)   {
        windowView.windowPtr = getBIFmember(counter);
        windowView.offset = windowView.windowPtr->offset;
        windowView.width = windowView.windowPtr->width;
        if (windowView.windowPtr != NULL)  {
            readViewString(&windowView, (void *) &stringPositionMarker, stringOverflow);
            if (warningCode != passed)  errIndex--;
            if (errCode != passed)  {  free((void *) holdString);  return;  }
    }   }
    
    
        // Finally, check to see if there seem to be any left-over useful fields in the string -- if so, set a warning.
    
    oneLetterType = lettertype(stringPositionMarker);
    while ((oneLetterType != a_symbol) && (oneLetterType != a_number)
                    && (oneLetterType != a_letter) && (stringPositionMarker < stringOverflow[0]))  {
        stringPositionMarker++;
        oneLetterType = lettertype(stringPositionMarker);         }
    
    if ((stringPositionMarker != holdString+stringWindow->width) && (warningCode == passed))  {
        stringOverflow[1] = stringPositionMarker;
        if (stringOverflow[1] >= stringOverflow[0])  stringOverflow[1] = stringOverflow[0]-1;
        setWarning(string_read_err, pcCodePtr-1);       }
    
    if ((warningCode != passed) && (doPrintError))  {
        printError(NULL, -1, holdString, (ccInt) (stringOverflow[1]-holdString), ccTrue);
        warningCode = passed;       }
    
    free((void *) holdString);      // clean up
}



// cclib_print_string() writes the contents of its arguments into a string.
// Numeric data is written out in ASCII.

void cclib_print_string()
{
    window *precisionWindow, *fieldWidthWindow, *stringWindow, *charWindow;
    member *charMember;
    view windowView;
    char *stringPositionMarker;
    ccInt dataSize, sizeofStrings, counter, numArgs, stringWindowPosition = 1, rtrn;
    linkedlist tempPrintLL;
    
    numArgs = numBIF_args();
    if (numArgs < 1)  {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }
    
    
        // read in the (optional) field width and floating-point precision arguments; otherwise use default values
    
    fieldWidth = 0;
    maxDigits = maxPrintableDigits;
    
    fieldWidthWindow = getBIFmember(1);
    if (fieldWidthWindow == NULL)  {  setError(void_member_err, pcCodePtr-1);  return;  }
    
    if (fieldWidthWindow->variable_ptr->type <= double_type)  {
        stringWindowPosition++;
        if (numArgs < 2)  {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }
        fieldWidth = (ccInt) getBIFnumArg(1, 0., (ccFloat) maxFieldWidth);
        
        precisionWindow = getBIFmember(2);
        if (precisionWindow == NULL)  {  setError(void_member_err, pcCodePtr-1);  return;  }
        
        if (precisionWindow->variable_ptr->type <= double_type)  {
            stringWindowPosition++;
            if (numArgs < 3)  {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }
            maxDigits = (ccInt) getBIFnumArg(2, 0., (ccFloat) maxPrintableDigits);
    }   }
    
    stringWindow = getBIFmember(stringWindowPosition);
    if (stringWindow->variable_ptr->type != string_type)  {  setError(string_expected_err, pcCodePtr-1);  return;  }
    
    
        // compute the byte size of the final string without printing it yet
    
    dataSize = 0;  sizeofStrings = 0;
    for (counter = stringWindowPosition+1; counter <= numArgs; counter++)  {
        windowView.windowPtr = getBIFmember(counter);
        windowView.offset = windowView.windowPtr->offset;
        windowView.width = windowView.windowPtr->width;
        if (windowView.windowPtr != NULL)  {
            sizeViewString(&windowView, &dataSize, (void *) &sizeofStrings);
    }   }
    
    
        // allocate contiguous memory in the destination string
    
    rtrn = newLinkedList(&tempPrintLL, dataSize+sizeofStrings, sizeof(char), 0, ccFalse);
    if (rtrn == passed)  rtrn = defragmentLinkedList(&tempPrintLL);
    if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr-1);  return;  }
    
    
        // print the string
    
    if (dataSize+sizeofStrings > 0)  {
        stringPositionMarker = (char *) element(&tempPrintLL, 1);
        for (counter = stringWindowPosition+1; counter <= numArgs; counter++)  {
            windowView.windowPtr = getBIFmember(counter);
            windowView.offset = windowView.windowPtr->offset;
            windowView.width = windowView.windowPtr->width;
            if (windowView.windowPtr != NULL)  {
                printViewString(&windowView, (void *) &stringPositionMarker, (void *) &sizeofStrings);
                if (warningCode != passed)  errIndex--;
                if (errCode != passed)  {  deleteLinkedList(&tempPrintLL);  return;  }
    }   }   }
    
    
        // rewire the output string to point to the linked list we just created
    
    charMember = LL_member(stringWindow->variable_ptr, stringWindow->offset+1);
    charWindow = charMember->memberWindow;
    resizeMember(charMember, 1, tempPrintLL.elementNum);
    if (errCode != passed)  return;
    
    copyElements(&tempPrintLL, 1, &(charWindow->variable_ptr->mem.data), charWindow->offset+1, tempPrintLL.elementNum);
    deleteLinkedList(&tempPrintLL);
}



// cclib_trap() catches runtime errors in its constructor, and allows execution to resume after the trap.
// Uses the registers to return error/warning codes/indices/offsets.

void cclib_trap()
{
    code_ref argCodeRef;
    ccInt counter, codeNo;
    ccBool ifErrorWasTrapped = ccFalse, holdDoPrintError = doPrintError, doClearError = ccTrue;
    
    
        // run each code in our args constructor
    
    for (counter = 1; counter <= BIF_argsView.windowPtr->variable_ptr->codeList.elementNum; counter++)  {
        
        
            // get a handle on the code
        
        argCodeRef = *(code_ref *) element(&(BIF_argsView.windowPtr->variable_ptr->codeList), counter);
        argCodeRef.anchor = pcSearchPath;
        refCodeRef(&argCodeRef);   // in case bE() changes something
        
        doPrintError = (argCodeRef.code_ptr[0] == code_marker);
        if (doPrintError)  {
            doClearError = (argCodeRef.code_ptr[1] != code_marker);
            if (doClearError)  codeNo = 1;
            else  codeNo = 2;       }
        else  codeNo = 0;
        
        
            // run the code
        
        warningCode = passed;
        beginExecution(&argCodeRef, ccTrue, BIF_argsView.offset, BIF_argsView.width, codeNo);
        derefCodeRef(&argCodeRef);
        if (errCode == return_flag)  errCode = passed;
        
        
            // print errors/warnings if we are instructed to do so
        
        if ((errCode != passed) || (warningCode != passed))  {
            
            if (doPrintError)  {
                
                storedCodeType *errorBaseScript;
                ccInt errIndexToPrint, errCharNum = 0;
                code_ref *errScriptToPrint;
                
                if (errCode != passed)  {  errIndexToPrint = errIndex;  errScriptToPrint = &errScript;  }
                else  {  errIndexToPrint = warningIndex;  errScriptToPrint = &warningScript;  }
                
                errorBaseScript = storedCode(errScriptToPrint->PLL_index);
                if (errorBaseScript->opCharNum != NULL)  errCharNum = errorBaseScript->opCharNum[
                                   errIndexToPrint + (ccInt) (errScriptToPrint->code_ptr - errorBaseScript->bytecode) - 1] - 1;
                
                printError(errorBaseScript->fileName, -1, errorBaseScript->sourceCode, errCharNum, ccFalse);       }
            
            if (errCode != passed)  {  intRegister = errCode;  if (doClearError)  errCode = passed;  }
            else  intRegister = -warningCode;
            if (doClearError)  warningCode = passed;
            
            ifErrorWasTrapped = ccTrue;
            if (intRegister > passed)  break;       // break if it was an error that stops execution
    }   }
    
    
        // clear intRegister if no error and no warning
    
    if (!ifErrorWasTrapped)  intRegister = passed;
    
    doPrintError = holdDoPrintError;
}



// throw() generates an error.  throw(errCode[, errScript[, errIndex[, ifWarning]]])

void cclib_throw()
{
    window *paramWindows[] = { NULL, NULL, NULL, NULL, NULL };
    code_ref *targetCodeRef = &PCCodeRef;
    ccInt numArgs, cA, *holdPC;
    ccInt theArgs[] = { 0, 0, 1, (ccInt) (pcCodePtr-1 - PCCodeRef.code_ptr + 1), ccFalse };
    ccInt argsMin[] = { ccIntMin, 0, 1, 1, 0 };
    ccInt argsMax[] = { ccIntMax, 0, 1, 1, 1 };
    code_ref *errorScript = &errScript;
    
    numArgs = numBIF_args();
    if ((numArgs < 1) || (numArgs > 5))  {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }
    
    
        // read in the error code, throwing real errors if something's wrong with the argument
    
    for (cA = 0; cA < numArgs; cA++)    {
        paramWindows[cA] = getBIFmember(cA+1);
        if (paramWindows[cA] != NULL)  {
            
            if (cA == 3)  {
                holdPC = pcCodePtr;
                pcCodePtr = targetCodeRef->code_ptr;
                runSkipMode(0);
                argsMax[3] = (ccInt) (pcCodePtr - targetCodeRef->code_ptr);
                pcCodePtr = holdPC;
                if (errCode != passed)  return;     }
            
            if (cA != 1)    {
                if (cA < 4)  theArgs[cA] = (ccInt) getBIFnumArg(cA+1, argsMin[cA], argsMax[cA]);
                else  theArgs[cA] = (ccInt) getBIFboolArg(cA+1);
                if (errCode != passed)  return;     }
            else    {
                argsMax[2] = paramWindows[cA]->variable_ptr->codeList.elementNum;
                theArgs[3] = 1;       }
            
            if (((cA == 1) || (cA == 2)) && (paramWindows[1] != NULL))  {
                targetCodeRef = (code_ref *) element(&(paramWindows[1]->variable_ptr->codeList), theArgs[2]);
    }   }   }
    
    
        // throw the error/warning
    
    if (theArgs[4] == 0)  {
        errCode = theArgs[0];
        errIndex = theArgs[3];
        errorScript = &errScript;       }
    
    else  {
        warningCode = theArgs[0];
        warningIndex = theArgs[3];
        errorScript = &warningScript;       }
    
    if (errorScript->code_ptr != NULL)  derefCodeRef(errorScript);
    *errorScript = *targetCodeRef;
    errorScript->anchor = NULL;
    refCodeRef(errorScript);
}




// cclib_top() returns the number of indices (things that can be rereferenced by [..]) contained in the given variable.
// This is not the same as the number of members!  Arrays have only one width-N member; strings only one per searchView.offset. 

void cclib_top()
{
    view viewToTop;
    
    if (numBIF_args() != 1)  {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }
    
    
        // get the argument variable
    
    viewToTop.windowPtr = getBIFmember(1);
    if (viewToTop.windowPtr == NULL)  {  setError(void_member_err, pcCodePtr-1);  return;  }
    if (viewToTop.windowPtr->variable_ptr->type < string_type)  {  setError(not_composite_err, pcCodePtr-1);  return;  }
    viewToTop.offset = viewToTop.windowPtr->offset;
    
    intRegister = numMemberIndices(&viewToTop);
}


// cclib_size() returns the total number of bytes contained in its argument.  bytes_num = size(window_to_size)

void cclib_size()
{
    view viewToSize;
    ccInt dataSize = 0, sizeofStrings = 0, ssMode, numArgs = numBIF_args();
    ccBool trueSize = ccFalse;
    
    
        // read in the arguments
        // optional arg 2:  whether or not to double-count overlapping memory
    
    if ((numArgs != 1) && (numArgs != 2))  {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }
    
    viewToSize.windowPtr = getBIFmember(1);
    
    if (numArgs == 2)  {
        trueSize = getBIFboolArg(2);
        if (errCode != passed)  return;     }


        // calculate the size
    
    if (viewToSize.windowPtr != NULL)  {
        viewToSize.offset = viewToSize.windowPtr->offset;
        viewToSize.width = viewToSize.windowPtr->width;
        
        if (!trueSize)  sizeView(&viewToSize, &dataSize, &sizeofStrings);
        else  {
        for (ssMode = 1; ssMode <= 3; ssMode++)  {
            storageSizeView(&viewToSize, (void *) &dataSize, (void *) &ssMode);
        }}
        
        intRegister = dataSize+sizeofStrings;       }
    
    else  intRegister = 0;
}



// The following mathematical functions all invoke doMath(),
// and each is paired with a second function that does the actual computation.
// All math is done in floating point.

void cclib_abs()  {  doMath(&doAbs);  }
ccFloat doAbs(ccFloat argument)  {  return (ccFloat) fabs((double) argument);  }

void cclib_floor()  {  doMath(&doFloor);  }
ccFloat doFloor(ccFloat argument)  {  return floor(argument);  }

void cclib_ceil()  {  doMath(&doCeil);  }
ccFloat doCeil(ccFloat argument)  {  return ceil(argument);  }

void cclib_log()  {  doMath(&doLog);  }
ccFloat doLog(ccFloat argument)  {  return log(argument);  }

void cclib_cos()  {  doMath(&doCos);  }
ccFloat doCos(ccFloat argument)  {  return cos(argument);  }

void cclib_sin()  {  doMath(&doSin);  }
ccFloat doSin(ccFloat argument)  {  return sin(argument);  }

void cclib_tan()  {  doMath(&doTan);  }
ccFloat doTan(ccFloat argument)  {  return tan(argument);  }

void cclib_acos()  {  doMath(&doArccos);  }
ccFloat doArccos(ccFloat argument)  {  return acos(argument);  }

void cclib_asin()  {  doMath(&doArcsin);  }
ccFloat doArcsin(ccFloat argument)  {  return asin(argument);  }

void cclib_atan()  {  doMath(&doArctan);  }
ccFloat doArctan(ccFloat argument)  {  return atan(argument);  }

void doMath(ccFloat(MathFunction)(ccFloat argument))
{
    if (numBIF_args() != 1)  {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }
    
    doubleRegister = getBIFnumArg(1, -DBL_MAX, DBL_MAX);
    if (errCode != passed)  return;
    
    doubleRegister = MathFunction(doubleRegister);
}


// cclib_random() generates a random number and returns it in doubleRegister

void cclib_random()
{
    if (numBIF_args() != 0)  {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }

    doubleRegister = ((ccFloat) rand())/RAND_MAX + ((ccFloat) rand())/RAND_MAX/RAND_MAX;
}



// cclib_find() searches a string for a substring, and returns its first/last appearance,
// or the number of times it appears (without overlapping).

void cclib_find()
{
    window *stringToSearchWindow, *soughtStringWindow;
    linkedlist *stringToSearch, *soughtString = NULL;
    char *strPtrToSearch, *soughtStrPtr;
    ccInt numArgs, startingPosition, numMatches, charCounter, mode, matchStart, rtrn;
    
    numArgs = numBIF_args();
    if ((numArgs < 2) || (numArgs > 4))
        {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }
    
    
        // load arguments:  the string to search within, then substring to look for
    
    stringToSearchWindow = getBIFstringArg(1);
    if (errCode != passed)  return;
    soughtStringWindow = getBIFstringArg(2);
    if (errCode != passed)  return;   
    
    stringToSearch = &(stringToSearchWindow->variable_ptr->mem.data);
    soughtString = &(soughtStringWindow->variable_ptr->mem.data);
    
    
        // if argument 3 is present, load it -- it is the 'mode' (search forwards from beginning, backwards from end, or count)
    
    if (numArgs < 3)  {  mode = 1;  startingPosition = 1;  }
    else  {
        
        mode = (ccInt) getBIFnumArg(3, -1., 1.);
        
        
            // if argument 4 is present, load it -- that is the point in the big string to start looking/counting (defaults to beginning/end)
        
        if (numArgs < 4)  {
            if (mode == -1)  startingPosition = stringToSearchWindow->width;
            else  startingPosition = 1;         }
        else  {
            startingPosition = (ccInt) getBIFnumArg(4, 1., (ccFloat) (stringToSearchWindow->width-soughtStringWindow->width+1));
    }   }
    
    if (errCode != passed)  return;
    
    
        // prepare the return variable
    
    rtrn = defragmentLinkedList(stringToSearch);
    if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr-1);  return;  }
    rtrn = defragmentLinkedList(soughtString);
    if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr-1);  return;  }
    
    
        // get pointers to our two strings
    
    if (stringToSearchWindow->width == 0)  {  intRegister = 0;  return;  }
    if (soughtStringWindow->width == 0)  {
        if (mode == 0)  intRegister = 0;
        else  intRegister = startingPosition;
        return;     }
    soughtStrPtr = (char *) element(soughtString, soughtStringWindow->offset+1);
    strPtrToSearch = (char *) element(stringToSearch, stringToSearchWindow->offset+1);
    
    numMatches = 0;
    
    if (mode > -1)  {               // search forwards
    for (matchStart = startingPosition-1; matchStart <= stringToSearchWindow->width-soughtStringWindow->width; matchStart++)  {
        
        for (charCounter = 0; charCounter < soughtStringWindow->width; charCounter++)  {
        if (strPtrToSearch[matchStart+charCounter] != soughtStrPtr[charCounter])   {
            break;
        }}
        
        if (charCounter == soughtStringWindow->width)  {
            if (mode == 1)   {  intRegister = matchStart+1;  return;   }
            else  numMatches++;     // count
    }}  }
    
    else    {                       // search backwards
    for (matchStart = startingPosition-1; matchStart >= 0; matchStart--)  {
        
        for (charCounter = 0; charCounter < soughtStringWindow->width; charCounter++)  {
        if (strPtrToSearch[matchStart+charCounter] != soughtStrPtr[charCounter])   {
            break;
        }}
        
        if (charCounter == soughtStringWindow->width)  {  intRegister = matchStart+1;  return;   }
    }}
    
    if (mode == 0)  intRegister = numMatches;
    else  intRegister = 0;     // if we were searching, rather than counting, then if we made it this far we found nothing
}



// type() returns the type of a given variable, or of one of the members of that variable (member index is argument 2)

void cclib_type()
{
    window *hostWindow;
    ccInt numArgs = numBIF_args();


        // get the variable (arg 1)
    
    if ((numArgs < 1) || (numArgs > 2))  {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }
    hostWindow = getBIFmember(1);
    if (hostWindow == NULL)  {  setError(void_member_err, pcCodePtr-1);  return;  }
    
    
        // return the requested piece of information
    
    if (numArgs == 1)  intRegister = hostWindow->variable_ptr->type;
    
    else if (hostWindow->variable_ptr->type == composite_type)  {
        ccInt memberIndex = getBIFnumArg(2, 1., (ccFloat) hostWindow->variable_ptr->mem.members.elementNum);
        if (errCode == passed)  {
            intRegister = LL_member(hostWindow->variable_ptr, memberIndex)->type;
    }   }
    
    else  setError(not_composite_err, pcCodePtr-1);
}



// member_ID() returns the ID number of a given member
// (corresponding to the respective entry in the allNames string returned by the compiler)

void cclib_member_ID()
{
    window *hostWindow;
    ccInt memberIndex, memberNumber, entryOffset, rtrn;
    member *soughtMember;
    
    if (numBIF_args() != 2)  {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }
    
    
        // get the variable containing the member (arg 1)
    
    hostWindow = getBIFmember(1);
    if (hostWindow == NULL)  {  setError(void_member_err, pcCodePtr-1);  return;  }
    if (hostWindow->variable_ptr->type < composite_type)  {  setError(type_mismatch_err, pcCodePtr-1);  return;  }
    
    
        // get the member index -- arg 2
    
    memberIndex = (ccInt) getBIFnumArg(2, 1., (ccFloat) hostWindow->variable_ptr->mem.members.elementNum);
    if (errCode != passed)  return;
    rtrn = findMemberIndex(hostWindow->variable_ptr, 0, memberIndex, &soughtMember, &memberNumber, &entryOffset, ccFalse);
    if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
    
    
        // return the ID number
    
    intRegister = soughtMember->memberID;
}



// cclib_bytecode() extracts variable or member codes and returns them in stringRegister

void cclib_bytecode()
{
    window *hostWindow;
    linkedlist *codeRefsList = NULL;
    ccInt memberNumber, loopCode, *holdPC, numArgs = numBIF_args(), rtrn;
    
    if ((numArgs < 1) || (numArgs > 2))
        {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }
    
    
        // load the variable containing the member or variable code (arg 1)
    
    hostWindow = getBIFmember(1);
    if (hostWindow == NULL)  {  setError(void_member_err, pcCodePtr-1);  return;  }
    if (hostWindow->variable_ptr->type != composite_type)  {  setError(not_composite_err, pcCodePtr-1);  return;  }
    
    
        // if it's something about a member we want to know, then we have to provide the member number (arg 2)
    
    if (numArgs == 1)
        codeRefsList = (linkedlist *) &(hostWindow->variable_ptr->codeList);
    
    else  {
        memberNumber = (ccInt) getBIFnumArg(2, 1., (ccFloat) hostWindow->variable_ptr->mem.members.elementNum);
        if (errCode == passed)  {
            codeRefsList = (linkedlist *) &(LL_member(hostWindow->variable_ptr, memberNumber)->codeList);
    }   }
    
    if (errCode != passed)  return;
    
    
        // get each code sequentially into the string (if there is more than one then they will be separated by 0s)
    
    rtrn = resizeLinkedList(&stringRegister, 0, ccFalse);
    if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
    
    for (loopCode = 1; loopCode <= codeRefsList->elementNum; loopCode++)  {
        
        code_ref *soughtCodeRef = (code_ref *) element(codeRefsList, loopCode);
        ccInt oldTop = stringRegister.elementNum;
        
        
            // copy the bytecode into a string (this includes the final 0)
        
        holdPC = pcCodePtr;
        pcCodePtr = soughtCodeRef->code_ptr;
        runSkipMode(0);    // should always work (code's already been checked); just in case, check errCode (index would be off, though)
        if (errCode != passed)  {  pcCodePtr = holdPC;  return;  }
        pcCodePtr++;
        
        rtrn = addElements(&stringRegister, (ccInt) (pcCodePtr-soughtCodeRef->code_ptr)*sizeof(ccInt), ccFalse);
        pcCodePtr = holdPC;
        if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
        
        setElements(&stringRegister, oldTop+1, oldTop+stringRegister.elementNum, (void *) soughtCodeRef->code_ptr);
    }
}


// springCleaning() removes inaccessible memory (with 0 arguments), or alternatively prunes hidden members from a variable (1 arg).

void springCleaning()
{
    if (numBIF_args() == 0)  combVariables();
    
    else if (numBIF_args() == 1)  {
        window *theWindow = getBIFmember(1);
        ccInt loopMember;
        
        if (theWindow == NULL)  {  setError(void_member_err, pcCodePtr-1);  return;  }
        if (theWindow->variable_ptr->type != composite_type)  {  setError(not_composite_err, pcCodePtr-1);  return;  }
        
        for (loopMember = theWindow->variable_ptr->mem.members.elementNum; loopMember >= 1; loopMember--)  {
        if (LL_member(theWindow->variable_ptr, loopMember)->ifHidden)  {
            removeMember(theWindow->variable_ptr, loopMember);
            if (errCode != passed)  return;
    }   }}
    
    else  {  setError(wrong_argument_count_err, pcCodePtr-1);  return;  }
}



// These last three are generic support routines.


// numBIF_args() returns the number of non-hidden members of theVariable.

ccInt numBIF_args()
{
    variable *BIFargsVariable = BIF_argsView.windowPtr->variable_ptr;
    ccInt counter, visibleMemberCounter = 0;
    
    for (counter = 1; counter <= BIFargsVariable->mem.members.elementNum; counter++)   {
    if (!(LL_member(BIFargsVariable, counter)->ifHidden))   {
        visibleMemberCounter++;
    }}
    
    return visibleMemberCounter;
}


// getBIFmember() returns the Nth non-hidden member of BIF_argsView (N being denoted by 'soughtMemberIndex').
// BIF = built-in-function.

window *getBIFmember(ccInt soughtMemberIndex)
{
    variable *BIFargsVariable = BIF_argsView.windowPtr->variable_ptr;
    ccInt counter, memberCounter = 0;
    member *loopMember;
    
    for (counter = 1; counter <= BIFargsVariable->mem.members.elementNum; counter++)   {
        loopMember = LL_member(BIFargsVariable, counter);
        if (!loopMember->ifHidden)  {
            memberCounter++;
            if (memberCounter == soughtMemberIndex)  {
                if (loopMember->memberWindow == NULL)  return NULL;
                else if (loopMember->memberWindow->jamStatus == unjammed)  return NULL;
                else  return loopMember->memberWindow;
    }   }   }
    
    return NULL;    // should never happen
}


ccFloat getBIFnumArg(ccInt argNum, ccFloat argMin, ccFloat argMax)
{
    window *argWindow = getBIFmember(argNum);
    
    if (argWindow == NULL)  setError(void_member_err, pcCodePtr-1);
    else if ((argWindow->variable_ptr->type == bool_type) || (argWindow->variable_ptr->type > double_type))
        setError(not_a_number_err, pcCodePtr-1);
    else if (argWindow->width != 1)  setError(multiple_indices_not_allowed_err, pcCodePtr-1);
    
    else  {
        loadDoubleRegister(argWindow->variable_ptr, argWindow->offset);
        if (!((doubleRegister >= argMin) && (doubleRegister <= argMax)))  setError(out_of_range_err, pcCodePtr-1);     }
    
    return doubleRegister;
}


ccBool getBIFboolArg(ccInt argNum)
{
    window *argWindow = getBIFmember(argNum);
    
    if (argWindow == NULL)  setError(void_member_err, pcCodePtr-1);
    else if (argWindow->variable_ptr->type != bool_type)  setError(type_mismatch_err, pcCodePtr-1);
    else if (argWindow->width != 1)  setError(multiple_indices_not_allowed_err, pcCodePtr-1);
    
    else  loadBoolRegister(argWindow->variable_ptr, argWindow->offset);
    
    return boolRegister;
}


void writeBIFintArg(ccInt argNum, ccInt theNum)
{
    window *argWindow = getBIFmember(argNum);
    
    if (argWindow == NULL)  return;
    else if (argWindow->variable_ptr->type > double_type)  setError(type_mismatch_err, pcCodePtr-1);
    else if (argWindow->width != 1)  setError(multiple_indices_not_allowed_err, pcCodePtr-1);
    
    else  {
        doubleRegister = (ccFloat) theNum;
        saveDoubleRegister(argWindow->variable_ptr, argWindow->offset);   }
}


window *getBIFstringArg(ccInt argNum)
{
    window *argWindow = getBIFmember(argNum);
    variable *argVar;
    
    if (argWindow == NULL)  {  setError(void_member_err, pcCodePtr-1);  return NULL;  }
    argVar = argWindow->variable_ptr;
    
    if (argVar->type == string_type)  {
        if (argWindow->width != 1)  setError(multiple_indices_not_allowed_err, pcCodePtr-1);
        else  return LL_member(argVar, argWindow->offset+1)->memberWindow;     }
    
    else if ((argVar->type == array_type) && (argVar->arrayDepth == 1) && (argVar->eventualType == char_type))  {
        return LL_member(argVar, 1)->memberWindow;     }
    
    else if (argVar->type == char_type)  {
        return argWindow;     }
    
    else  setError(string_expected_err, pcCodePtr-1);
    
    return NULL;   
}
