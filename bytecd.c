/*
 *  bytecd.c(pp) - executes bytecode commands
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
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <math.h>
#include "bytecd.h"



// Globals

cc_bytecode_global_struct cc_bytecode_globals;

const ccInt glMaxRecursions = 100;      // maximum nesting of running codes, either through
                                        // construction {..} or function calls f()




// *** Routines to parse bytecode operators ***
// Each routine name is preceded by a '_', to differentiate it from the actual operator name.
// These should be roughly in the order of their operator ID numbers.


// _jump_always() is an unconditional goto

void _jump_always()
{
    pcCodePtr += *(ccInt *) pcCodePtr;
}


// _jump_if_true() performs a goto by the specified offset if the condition is true

void _jump_if_true()
{
    ccInt *jumpPC = pcCodePtr + *pcCodePtr;
    
    pcCodePtr++;
    callLogicalFunction();
    if (boolRegister)  pcCodePtr = jumpPC;
}


// _jump_if_false() performs a goto by the specified offset if the condition is false

void _jump_if_false()
{
    ccInt *jumpPC = pcCodePtr + *pcCodePtr;
    
    pcCodePtr++;
    callLogicalFunction();
    if (!boolRegister)  pcCodePtr = jumpPC;
}



// A 'code' marker signifies to the interpreter that a code boundary was encountered.

void _code_marker()
{
    returnView.windowPtr = NULL;
    errCode = return_flag;        // stop runBytecode/Constructor
}



// _func_return() -- throws the return flag (a kind of error code) and sets the address of the return variable.

void _func_return()
{
    callCodeFunction();         // this allows a step to a void return variable; callBytecodeFunction() wouldn't 
    if (errCode != passed)  return;
    
    returnView = searchView;
    if (GL_Object.type != var_type)  returnView.windowPtr = NULL;
      
    errCode = return_flag;      // not a real error -- just to get runBytecode/Constructor to stop
}


// _user_function() runs a user-defined function and returns the return variable.

void _user_function()
{
    view holdArgsView = argsView, functionView;
    linkedlist backupCodeList, *theCodeList;
    ccInt counter, holdCodeNumber, rtrn;
    ccBool cleanUpCodeLL = ccFalse;
    
    
        // First determine which code marker N to skip to.  N = 0 would run the constructor.
        // In theory, N can be anything, but the compiler only generates N = 1 function calls.
    
        // get the function variable (sV.windowPtr == NULL if called in constructor mode)
    
    callCodeFunction();
    if (errCode != passed)  return;
    
    functionView = searchView;
    holdCodeNumber = codeNumber;
    
    backupCodeList.memory = NULL;       // BeginExec() will likely change GL_Object.codeList
    theCodeList = GL_Object.codeList;
    if (theCodeList == NULL)  return;
    
    
        // technical point:  make a backup of the codeRegister if that's where our code is stored, since it could be overwritten
    
    if ((theCodeList->elementNum > 1) || (theCodeList == &codeRegister))   {
        rtrn = newLinkedList(&backupCodeList, GL_Object.codeList->elementNum, sizeof(code_ref), 0., ccFalse);
        if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr);  return;  }
        theCodeList = &backupCodeList;
        cleanUpCodeLL = ccTrue;
        
        for (counter = 1; counter <= theCodeList->elementNum; counter++)    {
            *(code_ref *) element(&backupCodeList, counter) = *(code_ref *) element(GL_Object.codeList, counter);
            refCodeRef((code_ref *) element(&backupCodeList, counter));
    }   }
    
    
        // get the function arguments
    
    callBytecodeFunction();
    if (errCode != passed)    {
        if (cleanUpCodeLL)    {
            for (counter = 1; counter <= theCodeList->elementNum; counter++)   {        // deref & delete backup code list
                derefCodeRef((code_ref *) element(&backupCodeList, counter));   }
            deleteLinkedList(&backupCodeList);   }
        return;      }
    
    holdArgsView = argsView;
    argsView = searchView;
    refWindow(argsView.windowPtr);
    
    
        // Run the function.
    
    returnView.windowPtr = NULL;
    if ((GL_Object.type == var_type) && (functionView.windowPtr != NULL))  {
    if (functionView.windowPtr->variable_ptr->type == composite_type)  {        // don't run array variables (they weren't customized)
    for (counter = 1; counter <= theCodeList->elementNum; counter++)  {
        beginExecution((code_ref *) element(theCodeList, counter), ccTrue, functionView.offset,
                                    functionView.width, holdCodeNumber);
        if (errCode != passed)  break;           // return_flag shows up in errCode
    }}}
    
    
        // deref & delete backup code list
    
    if (cleanUpCodeLL)      {
        for (counter = 1; counter <= theCodeList->elementNum; counter++)    {
            derefCodeRef((code_ref *) element(&backupCodeList, counter));     }
        deleteLinkedList(&backupCodeList);    }
    
    if (errCode == return_flag)  errCode = passed;    // a return flag is a legitimate excuse for leaving the function
    
    
        // set searchView to point to the return variable, and draw a path to it if necessary (f().q :: ...)
    
    if (errCode == passed)  {
        searchView = returnView;
        GL_Object.type = var_type;
        if (searchView.windowPtr != NULL)  {
            GL_Object.codeList = &(searchView.windowPtr->variable_ptr->codeList);
    }   }
    
    
        // restore the old args variable and clean up
    
    GL_Path.stemMember = NULL;
    
    derefWindow(&(argsView.windowPtr));
    argsView = holdArgsView;
    returnView.windowPtr = NULL;
}


// _built_in_function() runs a built-in Cicada function and returns the appropriate register, if applicable.

void _built_in_function()
{
    view holdBIFArgsView;
    void(*theBuiltInFunction)(void);
    ccInt biFunctionID;
    
    
        // read in the function ID
    
    callNumericFunction(int_type);
    if (errCode != passed)  return;
    
    if ((intRegister < 0) || (intRegister >= bi_commands_num))  {
        setError(nonexistent_built_in_function_err, pcCodePtr);
        return;         }
    
    biFunctionID = intRegister;
    theBuiltInFunction = BuiltInFunctionJumpTable[biFunctionID];
    
    
        // get the argument variable
    
    callBytecodeFunction();
    if (errCode != passed)  return;
    
    holdBIFArgsView = BIF_argsView;
    BIF_argsView = searchView;
    
    
        // run the function and set the return variable
    
    returnView.windowPtr = NULL;
    theBuiltInFunction();
    
    GL_Object.type = BIF_Types[biFunctionID];
    GL_Path.stemMember = NULL;
    
    
        // restore the old args variable
    
    BIF_argsView = holdBIFArgsView;
}


// _def_general() handles all define/define-at/equate operators.
// Most operators are handled with the general procedure, which comes first.  Equate is done separately for speed.
// Unlike forced-equate, def-general requires data types to have compatible types (the same composite structure,
// interconvertible primitive types), which does not imply byte-size equivalence.

void _def_general()
{
    ccInt counter, rtrn, flags, loopArrayDim, sourceType, sourceDataType, arrayDimsToConstruct;
    ccInt *dgCommandPtr, *subjectCommand, *objectCommand, holdCodeNumber, holdMemberID, firstToCustomize = 0;
    unsigned char charRegister;
    ccBool canResizeDestination, errorInConstructor, isVoid, sourceIsScalar, varIsString;
    ccBool wasUnjammed, isUnjammable, doRelink, updateMembers, runConstructor, newTarget, doEquate;
    window fauxStringWindow;
    variable *theNewVariable, fauxStringVariable;
    view sourceView, destView, holdThatView, holdDestView, searchViewToReturn;
    code_ref *loopCodeRef;
    linkedlist *theCodeList = NULL;
    step_params destPath;
    void *toCopy = NULL;
    
    
        // read in the flags
    
    dgCommandPtr = pcCodePtr-1;
    flags = *pcCodePtr;
    pcCodePtr++;
    
    
        // equate resizes arrays if sizes don't match & [*] is present at the end of the destination variable path
    
    canResizeDestination = (*pcCodePtr == step_to_all);
    
    
        // The following block handles all non-equate operators.  (It should do equate too, but would be slow.)
    
    subjectCommand = pcCodePtr;
    if (flags != equ_flags)   {
        
        isUnjammable = getFlag(flags, unjammable);
        doRelink = getFlag(flags, relink_target);
        doEquate = getFlag(flags, post_equate);
        canAddMembers = getFlag(flags, can_add_members);
        updateMembers = getFlag(flags, update_members);
        newTarget = getFlag(flags, new_target);
        runConstructor = getFlag(flags, run_constructor);
        isHiddenMember = getFlag(flags, hidden_member);
        
        searchViewToReturn.windowPtr = NULL;
        
        
            // get the destination variable
        
        callDefineFunction();
        if (errCode != passed)  return;
        if (GL_Object.type != var_type)  {  setError(not_a_variable_err, pcCodePtr-1);  return;  }
        
        
            // get the source variable -- making sure the destination variable doesn't disappear on us
        
        holdDestView = searchView;
        holdCodeNumber = codeNumber;
        if (holdDestView.windowPtr != NULL)  refWindow(holdDestView.windowPtr);
        destPath = GL_Path;        // important -- the last step will not be taken
        if (GL_Path.stemMember != NULL)  holdMemberID = GL_Path.stemMember->memberID;
        else  holdMemberID = 0;
        
        objectCommand = pcCodePtr;       // we can get the pointer to a constant string from this
        compareReadArg(callCodeFunction, &sourceDataType, &sourceIsScalar);
        
        if ((searchView.windowPtr == NULL) || (!searchView.multipleIndices)
                    || (GL_Path.stemView.windowPtr == NULL))  varIsString = ccFalse;
        else  varIsString = (GL_Path.stemView.windowPtr->variable_ptr->type == string_type);
        
        
            // fix the target member if that's broken
        
        if (destPath.stemMember != NULL)  {
            ccBool linkBroken = ccTrue;
            variable *pathStemVar = destPath.stemView.windowPtr->variable_ptr;
            if (destPath.stemMemberNumber <= pathStemVar->mem.members.elementNum)  {
            if ((LL_member(pathStemVar, destPath.stemMemberNumber) == destPath.stemMember)
                        && (destPath.stemMember->memberID == holdMemberID))  {
                linkBroken = ccFalse;
            }}
            
            if (linkBroken)  {
                rtrn = findMemberID(pathStemVar, holdMemberID,
                                &destPath.stemMember, &destPath.stemMemberNumber, canAddMembers, isHiddenMember);
                if (rtrn != passed)  setError(rtrn, subjectCommand);
        }   }
        
        
            // restore the target-variable path info
        
        GL_Path = destPath;
        sourceView = searchView;
        searchView = destPath.stemView;
        if (holdDestView.windowPtr != NULL)  {
            derefWindow(&(holdDestView.windowPtr));
            if (*(searchView.windowPtr->variable_ptr->references) == 0)  return;   }
        
        
            // a couple of error checks
        
        if (errCode == passed)  {
            if (searchView.windowPtr == NULL)  setError(void_member_err, objectCommand);
            else if (((newTarget) || (updateMembers) || (doRelink)) && (GL_Path.stemMember == NULL))
                setError(undefined_member_err, objectCommand);      }
        
        if (errCode != passed)  return;
        
        
            // figure out what type of object is being copied, and the array indices spanned over it
        
        sourceType = sourceDataType = GL_Object.type;
        isVoid = (sourceType == no_type);
        arrayDimsToConstruct = 0;
        if (sourceType == var_type)  {
            
            if (sourceView.windowPtr == NULL)  {
                sourceDataType = no_type;
                isVoid = ccTrue;        }
            
            else  {
                
                    // if our view spans multiple indices of the source object,
                    // then we want to add a term to arrayDimList to span those indices
                
                if ((sourceView.multipleIndices) && (!holdDestView.multipleIndices))  {
                    rtrn = addElements(&(GL_Object.arrayDimList), 1, ccTrue);
                    if (rtrn != passed)  {  setError(rtrn, subjectCommand);  return;  }
                    else  *LL_int(&(GL_Object.arrayDimList), GL_Object.arrayDimList.elementNum) = sourceView.width;
                    if (!newTarget)  arrayDimsToConstruct = 1;          }
                
                
                    // if the source object itself is an array, step through the dimensions of the array
                    // to record the size of each dimension and get to the fundamental object at the end
                
                if (sourceView.windowPtr->variable_ptr->type == array_type)  {
                    
                    variable *sourceVar = sourceView.windowPtr->variable_ptr;
                    ccInt dimCounter = GL_Object.arrayDimList.elementNum+1;
                    
                    rtrn = addElements(&(GL_Object.arrayDimList), sourceVar->arrayDepth, ccFalse);
                    if (rtrn != passed)  {  setError(rtrn, subjectCommand);  return;  }
                    
                    while (sourceVar->type == array_type)  {
                        member *arrayMember = LL_member(sourceVar, 1);
                        *LL_int(&(GL_Object.arrayDimList), dimCounter) = arrayMember->indices;
                        if (arrayMember->memberWindow == NULL)  break;
                        sourceVar = arrayMember->memberWindow->variable_ptr;
                        dimCounter++;
                }   }
                
                sourceDataType = sourceView.windowPtr->variable_ptr->eventualType;
        }   }
        
        else if ((sourceType != no_type) && (doRelink))  {  setError(illegal_target_err, subjectCommand);  return;  }
        
        GL_Object.type = var_type;
        codeNumber = holdCodeNumber;
        
        if (newTarget)  arrayDimsToConstruct = GL_Object.arrayDimList.elementNum;
        
        
            // Catch type-mismatch errors before they make us unlink the whole target.  Check member type, then variable type.
        
        if (GL_Path.stemMember != NULL)   {
            
            ccInt arrayDepth = GL_Object.arrayDimList.elementNum;
            
            rtrn = checkType(&(GL_Path.stemMember->codeList), &(GL_Path.stemMember->type), &(GL_Path.stemMember->eventualType),
                        &(GL_Path.stemMember->arrayDepth), sourceDataType, arrayDepth, &(GL_Path.stemView), ccTrue, updateMembers);
            if (rtrn != passed)  {  setError(rtrn, subjectCommand);  return;  }
            
            if ((!doRelink) && (GL_Path.stemMember->memberWindow != NULL))  {
                
                rtrn = checkType(&(GL_Path.stemMember->memberWindow->variable_ptr->codeList),
                                 &(GL_Path.stemMember->memberWindow->variable_ptr->type),
                                 &(GL_Path.stemMember->memberWindow->variable_ptr->eventualType),
                                 &(GL_Path.stemMember->memberWindow->variable_ptr->arrayDepth),
                                 sourceDataType, arrayDepth, &searchView, ccFalse, newTarget);
                if (rtrn != passed)  {  setError(rtrn, subjectCommand);  return;  }
        }   }
        
        
            // build our each of our array variables, followed by the variable having the given eventualType 
        
        for (loopArrayDim = 1; loopArrayDim <= arrayDimsToConstruct+1; loopArrayDim++)  {
            
            ccInt varType, oneDimSize, arrayDepth = GL_Object.arrayDimList.elementNum - loopArrayDim + 1;
            ccBool expectNewVar = ((loopArrayDim <= arrayDimsToConstruct) || ((newTarget) && (!isVoid)));
            
            if (arrayDepth == 0)  {
                oneDimSize = 0;
                varType = sourceDataType;       }
            else  {
                oneDimSize = *LL_int(&(GL_Object.arrayDimList), loopArrayDim);
                if ((loopArrayDim == 1) && (arrayDimsToConstruct == 1) && (varIsString))  varType = string_type;
                else  varType = array_type;     }
            
            
                // update the type specification of the member
            
            if (updateMembers)  {
                if (GL_Path.indices != GL_Path.stemMember->indices)  {  setError(incomplete_member_err, subjectCommand);  break;  }
                if ((loopArrayDim <= arrayDimsToConstruct) || (!isVoid))  {
                    updateType(&(GL_Path.stemMember->codeList), &(GL_Path.stemMember->type), &(GL_Path.stemMember->eventualType),
                            &(GL_Path.stemMember->arrayDepth), varType, sourceDataType, arrayDepth, ccTrue);
            }   }
            
            
                // add a new variable if there isn't currently one here (i.e. the member is void)
            
            if (expectNewVar)  {
                if (searchView.width != searchView.windowPtr->variable_ptr->instances)
                        {  setError(incomplete_variable_err, subjectCommand);  break;  }
                if ((GL_Path.stemMember->memberWindow == NULL) && ((!doRelink) || (loopArrayDim <= arrayDimsToConstruct)))  {
                    ccInt stepWidth = searchView.width*GL_Path.indices;
                    rtrn = addVariable(&theNewVariable, varType, sourceDataType, arrayDepth,
                            ((varType == composite_type) || ((varType < composite_type) && (stepWidth == 0)) ));
                    if (rtrn == passed)
                        rtrn = addWindow(theNewVariable, 0, stepWidth, &(GL_Path.stemMember->memberWindow),
                                (loopArrayDim <= arrayDimsToConstruct) || (!isUnjammable));
                    if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  break;  }
                    
                    refWindow(GL_Path.stemMember->memberWindow);       // OK, since newWindow is always a member
            }   }
            
            
                // If we're re-aliasing (a.k.a. relinking) a variable, handle that here
            
            if (loopArrayDim > arrayDimsToConstruct)  {
                
                wasUnjammed = ccFalse;
                if (GL_Path.stemMember != NULL)   {
                if (GL_Path.stemMember->memberWindow != NULL)  {
                if (GL_Path.stemMember->memberWindow->jamStatus == unjammed)  {
                    wasUnjammed = ccTrue;
                }}}
                
                if ((doRelink) || (wasUnjammed))  {
                    rtrn = relinkGLStemMember(&sourceView, doRelink && (!isVoid), newTarget, isUnjammable);
                    if (rtrn != passed)  {  setError(rtrn, subjectCommand);  return;  }
            }   }
            
            
                // step into the next variable
            
            if ((expectNewVar) || (doRelink))  {
                stepView(&searchView, GL_Path.stemMember, GL_Path.offset, GL_Path.indices);
                if (errCode == void_member_err)  errCode = passed;
                if (loopArrayDim == 1)  {
                    searchViewToReturn = searchView;
//                    searchViewToReturn.multipleIndices = (searchViewToReturn.multipleIndices | (arrayDimsToConstruct > 0));
            }   }
            
            
                // update the variable type specification
            
            if (expectNewVar)  {
                
                firstToCustomize = searchView.windowPtr->variable_ptr->codeList.elementNum+1;
                
                updateType(&(searchView.windowPtr->variable_ptr->codeList), &(searchView.windowPtr->variable_ptr->type),
                        &(searchView.windowPtr->variable_ptr->eventualType), &(searchView.windowPtr->variable_ptr->arrayDepth),
                                            varType, sourceDataType, arrayDepth, ccFalse);     }
            
            if (loopArrayDim > arrayDimsToConstruct)  break;
            
            
                // if we're building an array variable, make sure it has its single array member
            
            if ((newTarget) || (loopArrayDim <= arrayDimsToConstruct))  {
                if (searchView.windowPtr->variable_ptr->mem.members.elementNum == 0)  {
                    GL_Path.stemMemberNumber = searchView.windowPtr->variable_ptr->mem.members.elementNum + 1;
                    rtrn = addMember(searchView.windowPtr->variable_ptr, GL_Path.stemMemberNumber,
                                oneDimSize, &(GL_Path.stemMember), ccFalse);
                    if (rtrn != passed)  {  setError(rtrn, subjectCommand);  break;  }
                    GL_Path.stemMember->type = no_type;     }
                
                else  {
                    GL_Path.stemMember = LL_member(searchView.windowPtr->variable_ptr, 1);
                    if (oneDimSize != GL_Path.stemMember->indices)  {
                        if (!canAddMembers)  {  setError(mismatched_indices_err, subjectCommand);  break;  }
                        if (doRelink)  GL_Path.stemMember->indices = oneDimSize;
                        else if (newTarget)  {
                            resizeMember(GL_Path.stemMember, searchView.width, oneDimSize);
                            if (errCode != passed)  break;
            }   }   }   }
            
            GL_Path.offset = 0;
            GL_Path.indices = GL_Path.stemMember->indices;
        }
        
        searchView.multipleIndices = (searchView.multipleIndices | (arrayDimsToConstruct > 0));
        
        
            // At last, check/update the variable code list, and run the constructors of the new object, if that is allowed.
        
        errorInConstructor = ccFalse;
        if ((!isVoid) && (errCode == passed) && ((newTarget) || (doRelink)))  {
            
            
                // First customize the codes:  i.e. make them new anchors if we just defined a composite object (function)
            
            if ((errCode == passed) && (newTarget) && (sourceDataType == composite_type))  {
                theCodeList = &(searchView.windowPtr->variable_ptr->codeList);
                for (counter = firstToCustomize; counter <= theCodeList->elementNum; counter++)   {
                    
                    window *pathWindow = GL_Path.stemMember->memberWindow;
                    searchPath *stemPath;
                    
                    loopCodeRef = (code_ref *) element(theCodeList, counter);
                    
                    
                        // if a new code (f :: {}), then link it to pcSearchPath
                    
                    if (loopCodeRef->anchor == NULL)  stemPath = pcSearchPath;
                    else  stemPath = loopCodeRef->anchor->stem;
                    
                    rtrn = drawPath(&(loopCodeRef->anchor), pathWindow, stemPath, GL_Path.indices, pcSearchPath->sourceCode);
                    if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
                    
                    refCodeRef(loopCodeRef);
            }   }
            else if (runConstructor)
                theCodeList = &(searchView.windowPtr->variable_ptr->codeList);
            
            
                // Run all codes.  Don't start at firstToCustomize (some may not have properly initialized the first time
                // (if run in constructor mode, it doesn't know if there have been errors, which are tolerated))
            
            if ((runConstructor) && (errCode == passed) && (sourceDataType == composite_type))  {
            for (counter = 1; counter <= theCodeList->elementNum; counter++)   {
                beginExecution((code_ref *) element(theCodeList, counter), ccTrue, searchView.offset, searchView.width, 0);
                if (errCode == return_flag)  errCode = passed;
                if (errCode != passed)  {  errorInConstructor = ccTrue;  break;  }
        }   }}
        
        if (isVoid)  searchView.windowPtr = NULL;       // if we ended up nowhere, make sure Cicada knows that
        
        if (errCode != passed)  {        // handle any lingering errors (e.g. from construction)
            if (!errorInConstructor)  setError(errCode, dgCommandPtr);
            
            if (searchView.windowPtr != NULL)  relinkGLStemMember(&sourceView, ccFalse, newTarget, isUnjammable);
            searchView.windowPtr = NULL;
            return;      }
        
        if ((GL_Object.type == var_type) && (searchView.windowPtr != NULL))
            GL_Object.codeList = &(searchView.windowPtr->variable_ptr->codeList);
        
        if ((!doEquate) || (searchView.windowPtr == NULL))  {   // if no data copy, exit here
            if (searchViewToReturn.windowPtr != NULL)  {
                searchView = searchViewToReturn;
                resizeLinkedList(&(GL_Object.arrayDimList), 0, ccFalse);        }
            return;     }
        
        
            // OK, we need to do something more elaborate, so we'll fall through to the bottom after the setup for equate
        
        destView = searchView;     }
    
    
        // Everything afterwards is for copying data.
        // The first block is the setup for pure equate, to make up for missing the last block.
    
    else    {       // simple equate (separate for speed)
        
        
            // Get the first (left-hand) argument:  the destination variable
        
        callBytecodeFunction();
        if (errCode != passed)  return;
        if ((GL_Object.type != var_type) || (searchView.windowPtr == NULL))
            {  setError(not_a_variable_err, subjectCommand);  return;  }
        
        
            // store important info; make sure we don't crash if we delete our destination variable from under our feet
        
        destView = searchView;
        destPath = GL_Path;
        holdThatView = thatView;
        holdCodeNumber = codeNumber;
        thatView = searchView;
        
        holdDestView = destView;
        refWindow(holdDestView.windowPtr);
        
        
            // get the source variable coordinate
        
        objectCommand = pcCodePtr;
        compareReadArg(callBytecodeFunction, &sourceDataType, &sourceIsScalar);
        
        
            // restore important variables; derefence the destination window
        
        thatView = holdThatView;
        sourceView = searchView;
        searchView = destView;
        GL_Path = destPath;
        codeNumber = holdCodeNumber;
        
        derefWindow(&(holdDestView.windowPtr));
        if (*(destView.windowPtr->variable_ptr->references) == 0)  return;
        if (errCode != passed)  return;
    }
    
    
        // Copy data.  If it's in a register, make a note of that..
    
    if (sourceIsScalar)  {
        if (sourceDataType == int_type)  toCopy = &intRegister;
        else if (sourceDataType == char_type)  {  charRegister = (unsigned char) intRegister;  toCopy = &charRegister;  }
        else if (sourceDataType == double_type)  toCopy = &doubleRegister;
        else if (sourceDataType == bool_type)  toCopy = &boolRegister;
        else if (sourceDataType == string_type)  {
            fauxStringWindow.variable_ptr = &fauxStringVariable;
            fauxStringWindow.offset = 0;
            fauxStringWindow.width = stringRegister.elementNum;
            fauxStringVariable.mem.data = stringRegister;
            toCopy = &fauxStringWindow;
    }   }
    
    else if (sourceDataType == no_type)  {  setError(not_a_variable_err, subjectCommand);  return;  }
    
    
        // if data is a scalar, copy it using the one-to-many copy routine (also handles one-to-one)
    
    if (sourceIsScalar)  {
        if (destView.windowPtr->variable_ptr->type > string_type)  setError(type_mismatch_err, objectCommand);
        else  copyCompareListToVar(toCopy, sourceDataType, &destView, copyJumpTable);       }
    
    
        // if data is a composite variable, we copy using the variable-to-variable copy routine
    
    else  {
        ccInt sourceWidth = sourceView.width;
        
        
            // test to see if a variable needs resizing
        
        if ((destView.multipleIndices) && (!sourceView.multipleIndices))
            sourceWidth = numMemberIndices(&sourceView);
        
        if ((sourceWidth != destView.width) && ((sourceView.windowPtr->variable_ptr->type != char_type)
                            || (destView.windowPtr->variable_ptr->type != string_type)))  {
            if ((canResizeDestination) && (GL_Path.stemView.width != 0)
                            && (sourceWidth % GL_Path.stemView.width == 0) && (GL_Path.stemMember != NULL))     {
                
                
                if (searchView.width != searchView.windowPtr->variable_ptr->instances)  setError(incomplete_variable_err, subjectCommand);
                
                
                    // if we're reading from the same window we're writing to (for example, myvar[*] = myvar[<2, 5>]),
                    // then just resize the window instead of copying anything -- this avoids conflicts 
                
                if (GL_Path.stemMember->memberWindow == sourceView.windowPtr)  {
                    GL_Path.stemMember->indices = sourceWidth/GL_Path.stemView.width;
                    
                    rtrn = addMemory(sourceView.windowPtr, 0, -sourceView.offset);
                    if (rtrn == passed)
                        rtrn = addMemory(sourceView.windowPtr, sourceView.width, sourceView.width-sourceView.windowPtr->width);
                    if (rtrn != passed)  setError(rtrn, dgCommandPtr);
                    
                    if ((flags != equ_flags) && (searchViewToReturn.windowPtr != NULL))  {
                        searchView = searchViewToReturn;
                        resizeLinkedList(&(GL_Object.arrayDimList), 0, ccFalse);        }
                    
                    return;             }
                
                
                    // otherwise resize the member as usual
                
                else  {
                    resizeMember(GL_Path.stemMember, GL_Path.stemView.width, sourceWidth/GL_Path.stemView.width);
                    if (errCode != passed)  return;
                    destView.width = sourceWidth;
            }   }
            
            else  {  setError(mismatched_indices_err, dgCommandPtr);  return;  }    }
        
        
            // copy the data over
        
        copyCompareMultiView(copyWindowData, &sourceView, &destView);       }
    
    if ((flags != equ_flags) && (searchViewToReturn.windowPtr != NULL))  {
        searchView = searchViewToReturn;
        resizeLinkedList(&(GL_Object.arrayDimList), 0, ccFalse);        }
}


// copyCompareMultiView() is used for commands like a[*] = { 2, 3 } -- we step out of 'a' into a fake dummy variable,
// so that both source and destination variables are one level removed from the data variables and the data copying can work properly.
// (This is also used for comparing, e.g. 'a[*] == { 2, 3 }'.) 

void copyCompareMultiView(void(*ccFunction)(view *, view *), view *view1, view *view2)
{
    window dummyWindow1, dummyWindow2;
    variable dummyVar;
    ccBool multiView1 = ((view1->multipleIndices) && (!(view2->multipleIndices)));
    ccBool multiView2 = ((view2->multipleIndices) && (!(view1->multipleIndices)));
    ccBool hasString = ((view1->windowPtr->variable_ptr->type == string_type) || (view2->windowPtr->variable_ptr->type == string_type));
    ccInt rtrn = passed;
    
    if (multiView1)  {
        rtrn = newLinkedList(&(dummyVar.mem.members), 1, sizeof(member), 0., ccFalse);
        encompassMultiView(view1, &dummyWindow1, &dummyWindow2, &dummyVar, hasString);      }
    else if (multiView2)  {
        rtrn = newLinkedList(&(dummyVar.mem.members), 1, sizeof(member), 0., ccFalse);
        encompassMultiView(view2, &dummyWindow1, &dummyWindow2, &dummyVar, hasString);      }
    if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
    
    ccFunction(view1, view2);
    
    if ((multiView1) || (multiView2))  deleteLinkedList(&(dummyVar.mem.members));
    
    return;
}


// encompassMultiView() makes Cicada pretend that it is in a variable looking at (the original) view->memberWindow->variable_ptr.

void encompassMultiView(view *theView, window *dummyWindow1, window *dummyWindow2, variable *dummyVar, ccBool hasString)
{
    member *dummyMember = LL_member(dummyVar, 1);
    dummyMember->memberWindow = dummyWindow2;
    dummyMember->indices = theView->width / baseView.width;
    dummyMember->ifHidden = ccFalse;
    dummyMember->business = 0;
    
    *dummyWindow2 = *(theView->windowPtr);
    dummyWindow2->offset = theView->offset;
    dummyWindow2->width = theView->width;
    
    dummyWindow1->offset = theView->offset;
    if ((hasString) && (theView->windowPtr->variable_ptr->type == char_type))  {
        dummyVar->type = string_type;
        theView->offset = 0;       }
    else  {
        dummyVar->type = array_type;
        theView->offset = theView->windowPtr->offset;       }
    
    dummyWindow1->jamStatus = cannot_jam;
    dummyWindow1->variable_ptr = dummyVar;
    
    theView->windowPtr = dummyWindow1;
    theView->width = baseView.width;
}


// relinkGLStemMember() sets the member in GL_Path.stemMember to point to a given window.
// Used when aliasing or voiding a member.

ccInt relinkGLStemMember(view *targetView, ccBool doRelink, ccBool newTarget, ccBool isUnjammable)
{
    window *newMemberWindow = NULL;
    ccInt rtrn;
    
    
        // first make sure the aliasing is legal
    
    if (GL_Path.stemMember == NULL)  return undefined_member_err;
    else if (GL_Path.indices != GL_Path.stemMember->indices)  return incomplete_member_err;
    else if (searchView.width != searchView.windowPtr->variable_ptr->instances)  return incomplete_variable_err;
    else if ((doRelink) && (!newTarget))  {
        if ((targetView->width != searchView.width*GL_Path.indices))  return mismatched_indices_err;
        else if (targetView->windowPtr == NULL)  return illegal_target_err;
        else if (*(targetView->windowPtr->variable_ptr->references) == 0)  return target_deleted_err;       }
    
    
        // re-alias the member
    
    if (doRelink)  {   // if we're not voiding it, then relink it
        rtrn = addWindow(targetView->windowPtr->variable_ptr, targetView->offset, targetView->width, &newMemberWindow, !isUnjammable);
        if (rtrn != passed)  return rtrn;
        refWindow(newMemberWindow);        }
    
    derefWindow(&(GL_Path.stemMember->memberWindow));
    
    if (doRelink)  GL_Path.stemMember->memberWindow = newMemberWindow;
    
    return passed;
}


// newStringLL():  reads a constant string into a variable.

void newStringLL(view *destView)
{
    member *newStringMember = LL_member(destView->windowPtr->variable_ptr, destView->offset + 1);
    variable *newStringVar = newStringMember->memberWindow->variable_ptr;
    
    resizeMember(newStringMember, searchView.width, stringRegister.elementNum);
    if (errCode != passed)  return;
    
    copyElements(&stringRegister, 1, &(newStringVar->mem.data), newStringMember->memberWindow->offset+1, stringRegister.elementNum);
    
    return;
}


// checkType() ensures that the type of a member or variable matches, or is compatible with, the given expected type.
// No return value; just throws an error if there is a type mismatch.

ccInt checkType(linkedlist *destLL, ccInt *sourceType, ccInt *eventualSourceType, ccInt *sourceArrayDepth,
            ccInt expectedEventualSourceType, ccInt expectedArrayDepth, view *theView, ccBool isMember, ccBool ifUpdate)
{
    ccInt counter, expectedSourceType = expectedEventualSourceType;
    
    if (expectedArrayDepth > 0)  {
        if (*sourceType == string_type)  expectedSourceType = string_type;
        else  expectedSourceType = array_type;      }
    
    if ((expectedEventualSourceType == no_type) && (!ifUpdate))  return passed;     // we're always allowed to unlink a variable
    
    
        // check: non-void type which explicitly doesn't match the expected type
    
    if ((*sourceType != no_type) && ((expectedSourceType != *sourceType)
                || (expectedEventualSourceType != *eventualSourceType) || (expectedArrayDepth != *sourceArrayDepth)))
        return type_mismatch_err;
    
    
        // make sure we're not copying primitive <-- composite
    
    if (*sourceType < composite_type)  {   // so it's a primitive type
        if (destLL->elementNum != 0)  return type_mismatch_err;
        else  return passed;            }
    
    
        // composite:  we need all the codes currently in our list to also be present in the expected type
    
    if (GL_Object.codeList == NULL)   {
        if (destLL->elementNum != 0)  return type_mismatch_err;
        else  return passed;            }
    
    if (destLL->elementNum > GL_Object.codeList->elementNum)  return type_mismatch_err;
    
    for (counter = 1; counter <= destLL->elementNum; counter++)  {
    if (((code_ref *) element(destLL, counter))->code_ptr != ((code_ref *) element(GL_Object.codeList, counter))->code_ptr)  {
        return type_mismatch_err;
    }}
    
    
        // Make sure that we're changing the whole member and/or variable, then update the type field.
    
    if ((isMember) && ((*sourceType != expectedSourceType) || (expectedSourceType == composite_type)))  {
        if (GL_Path.indices != GL_Path.stemMember->indices)  return incomplete_member_err;
        if (theView->width != theView->windowPtr->variable_ptr->instances)  return incomplete_variable_err;     }
    
    return passed;
}


// updateType() changes the type of a member or variable to match what is given.
// Only partial type-checking:  it is assumed that checkType() was already run.

void updateType(linkedlist *destLL, ccInt *sourceType, ccInt *eventualSourceType, ccInt *arrayDepth,
                ccInt newSourceType, ccInt newEventualSourceType, ccInt newArrayDepth, ccBool isMember)
{
    code_ref *loopCodeRef;
    ccInt counter, copyBottom, rtrn;
    
    *sourceType = newSourceType;
    *eventualSourceType = newEventualSourceType;
    *arrayDepth = newArrayDepth;
    
    
        // If it's composite we have to also update the codes.  (Any changes to the code list are code additions.)
    
    if (newEventualSourceType == composite_type)  {
        
        copyBottom = destLL->elementNum+1;
        rtrn = addElements(destLL, GL_Object.codeList->elementNum-copyBottom+1, ccFalse);
        if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr-1);  return;  }
        copyElements(GL_Object.codeList, copyBottom, destLL, copyBottom, GL_Object.codeList->elementNum-copyBottom+1);
        
        if ((isMember) || (newArrayDepth > 0))  {     // reference the new code refs if we're not going to 'customize' the code later
        for (counter = copyBottom; counter <= destLL->elementNum; counter++)  {
            loopCodeRef = (code_ref *) element(destLL, counter);
            loopCodeRef->anchor = NULL;     // since anchors will be 'customized' later, don't assign anchors just yet
            refCodeRef(loopCodeRef);
    }   }}
}



// _forced_equate() copies data between two variables whose sizes match (or can be made to match by resizing strings
// in the destination variable), irrespective of the types of the variables.

void _forced_equate()
{
    view sourceView, holdThatView, destView;
    step_params destPath;
    void *objectCopy = NULL, *stringPtr;
    ccInt sourceDataSize = 0, destDataSize = 0, sourceStringSize = no_string, destStringSize = no_string;
    ccInt sourceType, holdCodeNumber;
    char byteBackup;
    ccBool canResizeDestination = ccFalse, resizedFromZero = ccFalse;
    
    
        // Get the left-hand argument (the destination variable).
    
    if (*pcCodePtr == step_to_all)  canResizeDestination = ccTrue;
    holdThatView = thatView;
    
    callBytecodeFunction();
    if ((errCode == passed) && (GL_Object.type != var_type))  setError(not_a_variable_err, pcCodePtr-1);
    if ((errCode == passed) && (searchView.windowPtr == NULL))  setError(void_member_err, pcCodePtr-1);
    if (errCode != passed)  return;
    
    if (GL_Path.stemView.width == 0)  canResizeDestination = ccFalse;        // no point in trying
    if (GL_Path.stemView.width != GL_Path.stemView.windowPtr->variable_ptr->instances)  canResizeDestination = ccFalse;

    destView = searchView;
    refWindow(destView.windowPtr);
    
    destPath = GL_Path;
    holdCodeNumber = codeNumber;
    
    
        // Load the right-hand argument.  We do this in different ways depending on the type of the left-hand variable.
    
    thatView = destView;
    callBytecodeFunction();
    
    sourceView = searchView;
    searchView = destView;
    thatView = holdThatView;            // 'that' might have been overwritten
    
    derefWindow(&(destView.windowPtr));
    
    if (errCode != passed)  return;
    
    if (searchView.windowPtr != NULL)  {
    if (*(searchView.windowPtr->references) == 0)  {
        return;
    }}
    
    sourceType = GL_Object.type;
    sourceDataSize = typeSizes[sourceType];
    
    
        // if the data to copy comes from a variable, size it using sizeView()
    
    if (sourceType == var_type)  {
        if (sourceView.windowPtr == NULL)  {  setError(not_a_variable_err, pcCodePtr-1);  return;  }
        
        sizeView(&sourceView, &sourceDataSize, &sourceStringSize);
        if (sourceStringSize != no_string)  sourceDataSize += sourceStringSize;    // we don't care what form the source data is in
        
        objectCopy = (void *) malloc((size_t) sourceDataSize + sizeof(char));      // avoid size-0 malloc
        if (objectCopy == NULL)  {  setError(out_of_memory_err, pcCodePtr-1);  return;  }
        
        stringPtr = objectCopy;     // do this twice since readView() modifies stringPtr
        readView(&sourceView, &stringPtr, &sourceStringSize);
        stringPtr = objectCopy;       }
    
    
        // if the data to copy is a constant (i.e. is stored in one of the registers), make a pointer to that register
        // (we make a copy of the byte stored in intRegister if it's of type 'char')
    
    else if (sourceType <= char_type)  {
        if (sourceType == bool_type)  stringPtr = (void *) &boolRegister;
        else  {  byteBackup = (char) intRegister;  stringPtr = (void *) &byteBackup;  }    }
    else if (sourceType == int_type)  stringPtr = (void *) &intRegister;
    else if (sourceType == double_type)  stringPtr = (void *) &doubleRegister;
    else if (sourceType == string_type)  {
        stringPtr = objectCopy = malloc(stringRegister.elementNum);
        if (objectCopy == NULL)  {  setError(out_of_memory_err, pcCodePtr-1);  return;  }
        getElements(&stringRegister, 1, stringRegister.elementNum, objectCopy);
        sourceDataSize = stringRegister.elementNum;     }
    
    
        // Get the byte size of the destination variable.
    
    sizeView(&searchView, &destDataSize, &destStringSize);
    
    
        // If the source and destination variable sizes don't match, we can try resizing the latter if the destination path ended in [*].
    
    if (((sourceDataSize > destDataSize) && (destStringSize == no_string)) || (sourceDataSize < destDataSize))  {
        
                      // zero indices in source -- hard to deal with, so just throw an error
        
        if (destPath.stemView.width == 0)  setError(unequal_data_size_err, pcCodePtr-1);
        else if (destPath.stemMember == NULL)  setError(undefined_member_err, pcCodePtr-1);
        
        
            // if [*] was present and current top of that list is zero, then size it to 1 so we can measure its byte-size per index
        
        if ((canResizeDestination) && (destPath.stemMember->indices == 0) && (GL_Path.stemView.width > 0) && (errCode == passed))     {        
            resizeMember(destPath.stemMember, destPath.stemView.width, 1);
            if (errCode == passed)  {
                searchView.width = destPath.stemView.width;
                sizeView(&searchView, &destDataSize, &destStringSize);
                resizedFromZero = ccTrue;
        }   }
        
        
            // If sizes don't match and the dest. path ended in a [*], try to resize the destination variable.
        
        if (errCode == passed)  {
        if (((sourceDataSize > destDataSize) && (destStringSize == no_string)) || (sourceDataSize < destDataSize))  {
            if ((destDataSize > 0) && (canResizeDestination) && 
                    ( (destStringSize != no_string) || (sourceDataSize % (destDataSize/destPath.stemMember->indices) == 0) ))  {
                resizeMember(destPath.stemMember, destPath.stemView.width,     // this also rescales seachView.width
                                                        sourceDataSize/(destDataSize/destPath.stemMember->indices));
                destDataSize = sourceDataSize;         }
            
            
                // either we can't resize, or we tried and the dest. variable is still size-0 so there's no hope
            
            else if (resizedFromZero)  resizeMember(destPath.stemMember, destPath.stemView.width, 0);
    }   }}
    
    if (destStringSize != no_string)  {
        destStringSize = sourceDataSize-destDataSize;
        if ((errCode == passed) && (destStringSize < 0))  setError(unequal_data_size_err, pcCodePtr-1);     }
    else if ((errCode == passed) && (sourceDataSize != destDataSize))  setError(unequal_data_size_err, pcCodePtr-1);
    
    
        // write the data from the buffer to the destination variable
    
    if (errCode == passed)  writeView(&searchView, &stringPtr, &destStringSize);
    
    
        // free the memory & clean up
    
    if (objectCopy != NULL)  free(objectCopy);
    
    GL_Path = destPath;
    
    GL_Object.type = var_type;
    GL_Object.codeList = &(searchView.windowPtr->variable_ptr->codeList);
    codeNumber = holdCodeNumber;
}


// *** Navigation commands ***


// _search_member():  looks back up the search path for the member with the given ID (name).
// Throws an error if no such member can be found anywhere along the path tree.
// smStep() does all the actual work.

void _search_member()  {  navigate(&smStep, ccFalse, ccTrue, ccFalse);  }
void _define_search_member()  {  navigate(&smStep, ccTrue, ccFalse, ccFalse);  }
void _object_search_member()  {  navigate(&smStep, ccFalse, ccTrue, ccTrue);  }

void smStep(ccBool allowAddMember)
{
    searchMember((ccInt) *pcCodePtr, &GL_Path.stemMember, &GL_Path.stemMemberNumber, allowAddMember, isHiddenMember);
    pcCodePtr++;
    
    GL_Path.offset = 0;
    GL_Path.indices = 1;
}


// _step_to_memberID():  steps from the current variable into the member with the given ID (name).
// If that can't be found then this returns an error.
// Calls sidStep() to perform the step.

void _step_to_memberID()  {  navigate(&sidStep, ccFalse, ccTrue, ccFalse);  }
void _define_step_to_memberID()  {  navigate(&sidStep, ccTrue, ccFalse, ccFalse);  }
void _object_step_to_memberID()  {  navigate(&sidStep, ccFalse, ccTrue, ccTrue);  }

void sidStep(ccBool allowAddMember)
{
    ccInt *theIDPtr = pcCodePtr, rtrn;
    
    
        // first get the initial steps along the path
    
    pcCodePtr++;
    callSearchPathFunction();
    if (errCode != passed)  return;
    
    
        // look for the member in this variable
    
    rtrn = findMemberID(searchView.windowPtr->variable_ptr, *theIDPtr,
                    &GL_Path.stemMember, &GL_Path.stemMemberNumber, allowAddMember, isHiddenMember);
    if (rtrn != passed)  setError(rtrn, theIDPtr);
    
    GL_Path.offset = 0;
    GL_Path.indices = 1;
}


// step_to_index():  steps from the current variable into the member having the given index.
// The heavy lifting is done by stixStep().

void _step_to_index()  {  navigate(&stixStep, ccFalse, ccTrue, ccFalse);  }
void _define_step_to_index()  {  navigate(&stixStep, ccTrue, ccFalse, ccFalse);  }
void _object_step_to_index()  {  navigate(&stixStep, ccFalse, ccTrue, ccTrue);  }

void stixStep(ccBool allowAddMember)
{
    ccInt soughtIndex, rtrn;
    
    
        // get the initial steps along the path
    
    callSearchPathFunction();
    if (errCode != passed)  return;
    
    
        // get the index to step to
    
    soughtIndex = callIndexFunction();
    if (errCode != passed)  return;
    
    
        // calculate the member number that holds that index
    
    rtrn = findMemberIndex(searchView.windowPtr->variable_ptr, searchView.offset, soughtIndex, &(GL_Path.stemMember),
                                            &(GL_Path.stemMemberNumber), &(GL_Path.offset), allowAddMember);
    if (rtrn != passed)  setError(rtrn, pcCodePtr-1);
    
    GL_Path.indices = 1;
}


// step_to_index():  steps from the current variable into the member having the given index.
// Calls sticsStep().

void _step_to_indices()  {  navigate(&sticsStep, ccFalse, ccTrue, ccFalse);  }
void _define_step_to_indices()  {  navigate(&sticsStep, ccTrue, ccFalse, ccFalse);  }
void _object_step_to_indices()  {  navigate(&sticsStep, ccFalse, ccTrue, ccTrue);  }

void sticsStep(ccBool allowAddMember)
{
    ccInt firstIndex, lastIndex, secondMemberNumber = 0, secondOffset, rtrn;
    
    
        // get the first steps in the path
    
    callSearchPathFunction();
    if (errCode != passed)  return;
    
    
        // get the first index to step into
    
    firstIndex = callIndexFunction();
    if (errCode != passed)  return;
    
    
        // get the last index to step into
    
    lastIndex = callIndexFunction();
    if (errCode != passed)  return;
    
    if (lastIndex < firstIndex-1)  {  setError(index_argument_err, pcCodePtr-1);  return;  }
    
    
    searchView.multipleIndices = ccTrue;
    GL_Path.indices = lastIndex - firstIndex + 1;
    
    
        // special case:  allow us to step into 0 indices of a size-0 array
    
    if ((GL_Path.indices == 0) && (searchView.windowPtr->variable_ptr->type == array_type))  {
        GL_Path.stemMember = LL_member(searchView.windowPtr->variable_ptr, 1);
        if ((firstIndex >= 1) && (lastIndex <= GL_Path.stemMember->indices))  {
            GL_Path.stemMemberNumber = 1;
            GL_Path.offset = firstIndex - 1;
            
            return;
    }   }
    
    
        // otherwise find the first and last indices, and make sure they're in the same member
    
    rtrn = findMemberIndex(searchView.windowPtr->variable_ptr, searchView.offset, firstIndex, &(GL_Path.stemMember),
                                                                &(GL_Path.stemMemberNumber), &(GL_Path.offset), allowAddMember);
    if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
    
    if (lastIndex > firstIndex)  {
        rtrn = findMemberIndex(searchView.windowPtr->variable_ptr, searchView.offset, intRegister, &(GL_Path.stemMember),
                                                    &secondMemberNumber, &secondOffset, allowAddMember);
        if (rtrn != passed)  setError(rtrn, pcCodePtr-1);       }
}


// step_to_all_indices():  steps from the current variable into all indices.
// Only works if there is only one member.
// This operator is what allows an automatic resize of the last dimension of an array.
// Calls staiStep().

void _step_to_all_indices() {  navigate(&staiStep, ccFalse, ccTrue, ccTrue);  }
void _define_step_to_all_indices() {  navigate(&staiStep, ccTrue, ccFalse, ccTrue);  }
void staiStep(ccBool allowAddMember)
{
    
        // get the prior path
    
    GL_Path.indices = 1;         // for subsequent STAIs
    callSearchPathFunction();
    if (errCode != passed)  return;
    
    
        // do more checks
    
    if (searchView.windowPtr->variable_ptr->mem.members.elementNum == 0)  {  setError(no_member_err, pcCodePtr-1);  return;  }
    if (searchView.windowPtr->variable_ptr->mem.members.elementNum != 1)  {  setError(step_multiple_members_err, pcCodePtr-1);  return;  }
    
    searchView.multipleIndices = ccTrue;
    
    
        // set all the path params
    
    GL_Path.stemMember = LL_member(searchView.windowPtr->variable_ptr, 1);
    GL_Path.stemMemberNumber = 1;
    GL_Path.offset = 0;
    GL_Path.indices = GL_Path.stemMember->indices;
}


// _resize() and _resize_start() are both the [^..] operator.  These scale an array by setting the width of its lone member.
// _resize_start() is used when the [^..] 'begins' a sentence (as in, it's the lowest-OoO operation).
// _resize() is used when the expression is embedded in a larger expression:  x = array[^1].
// doResize() does all the actual work.

void _resize()  {  navigate(&doResize, ccFalse, ccTrue, ccFalse);  }
void _define_resize()  {  navigate(&doResize, ccTrue, ccFalse, ccFalse);  }
void _resize_start()  {  navigate(&doResize, ccFalse, ccFalse, ccTrue);  }

void doResize(ccBool allowAddMember)
{
    member *oneMember;
    ccInt loopMember, loopMemberLLIndex, rtrn, oneMemberLLIndex, newTop, oldTopIndex, newTopIndex, firstIndex, memberOffset;
    
    
        // get the beginning of the path
    
    callSearchPathFunction();
    if (errCode != passed)  return;
    
    
        // read in the new top index
    
    newTop = callIndexFunction();
    if (errCode != passed)  return;
    if (newTop < 0)  {  setError(index_argument_err, pcCodePtr-1);  return;  }
    
    
        // assuming everything's OK, resize the member
    
    if (searchView.windowPtr->variable_ptr->type == composite_type)  {
        
        linkedlist *memberLL = &(searchView.windowPtr->variable_ptr->mem.members);
        ccInt oldTop = numMemberIndices(&searchView);
        
        findMemberIndex(searchView.windowPtr->variable_ptr, 0, oldTop, &oneMember, &oldTopIndex, &memberOffset, ccFalse);
        
        if (newTop > oldTop)  {
        for (loopMember = oldTop+1; loopMember <= newTop; loopMember++)  {
            oneMemberLLIndex = searchView.windowPtr->variable_ptr->mem.members.elementNum + 1;
            rtrn = addMember(searchView.windowPtr->variable_ptr, oneMemberLLIndex, 1, &oneMember, ccFalse);
            if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
        }}
        
        else if (newTop < oldTop)  {
            if (newTop == 0)  newTopIndex = 0;
            else  findMemberIndex(searchView.windowPtr->variable_ptr, 0, newTop, &oneMember, &newTopIndex, &memberOffset, ccFalse);
            for (loopMemberLLIndex = memberLL->elementNum; loopMemberLLIndex > newTopIndex; loopMemberLLIndex--)  {
            if (!((member *) element(memberLL, loopMemberLLIndex))->ifHidden)  {
                removeMember(searchView.windowPtr->variable_ptr, loopMemberLLIndex);
        }   }}
        
        if (newTop == 0)  GL_Path.stemMember = NULL;
        else  {
            findMemberIndex(searchView.windowPtr->variable_ptr, 0, 1, &oneMember, &firstIndex, &memberOffset, ccFalse);
            GL_Path.stemMember = (member *) element(memberLL, firstIndex);
    }   }

    else if (searchView.windowPtr->variable_ptr->type == string_type)  {
        GL_Path.stemMember = LL_member(searchView.windowPtr->variable_ptr, searchView.offset + 1);
        if (searchView.width != 1)
            {  setError(multiple_indices_not_allowed_err, pcCodePtr-1);  return;  }
        
        resizeMember(GL_Path.stemMember, searchView.width, intRegister);    }
    
    else    {
        GL_Path.stemMember = LL_member(searchView.windowPtr->variable_ptr, 1);
        if (searchView.width != searchView.windowPtr->variable_ptr->instances)
            {  setError(incomplete_variable_err, pcCodePtr-1);  return;  }
        
        resizeMember(GL_Path.stemMember, searchView.width, intRegister);    }
    
    searchView.multipleIndices = ccTrue;
    
    GL_Path.indices = newTop;
    GL_Path.stemMemberNumber = 1;
    GL_Path.offset = 0;
}


// The next routines correspond to [+ ].  They insert or delete indices from members/variables.
// The _start versions are used when the insertion operation constitutes a complete sentence.

void _insert_index()  {  navigate(&do_insert_index, ccFalse, ccTrue, ccFalse);  }
void _define_insert_index()  {  navigate(&do_insert_index, ccTrue, ccFalse, ccFalse);  }
void _insert_index_start()  {  navigate(&do_insert_index, ccFalse, ccFalse, ccTrue);  }
void _insert_indices()  {  navigate(&do_insert_indices, ccFalse, ccTrue, ccFalse);  }
void _define_insert_indices()  {  navigate(&do_insert_indices, ccTrue, ccFalse, ccFalse);  }
void _insert_indices_start()  {  navigate(&do_insert_indices, ccFalse, ccFalse, ccTrue);  }


// masterInsert() adds indices to a member and the variable it points to.

void do_insert_index(ccBool allowAddMember)  {  masterInsert(ccFalse, allowAddMember);  }
void do_insert_indices(ccBool allowAddMember)  {  masterInsert(ccTrue, allowAddMember);  }

void masterInsert(ccBool multipleIndices, ccBool allowAddMember)
{
    ccInt firstIndex, secondIndex, rtrn;
    
    
        // get the first steps on the search path
    
    callSearchPathFunction();
    if (errCode != passed)  return;
    
    
        // find the first index to insert
    
    firstIndex = callIndexFunction();
    if (errCode != passed)  return;
    
    
        // now read in the second index (there are two for this operator)
    
    if (multipleIndices)  {
        secondIndex = callIndexFunction();
        if (errCode != passed)  return;         }
    
    else  secondIndex = firstIndex;
    
    if (GL_Path.stemMember == NULL)  {  setError(undefined_member_err, pcCodePtr-1);  return;  }
    
    searchView.multipleIndices = multipleIndices;
    if (secondIndex < firstIndex)  secondIndex = firstIndex-1;
    GL_Path.indices = secondIndex - firstIndex + 1;
    
    
        // add members if we're in a composite variable
    
    if (searchView.windowPtr->variable_ptr->type == composite_type)   {
        linkedlist *memberLL = &(searchView.windowPtr->variable_ptr->mem.members);
        ccInt lastMemberNumber, lastMemberOffset, loopMember;
        member *oneMember;
        
        rtrn = findMemberIndex(searchView.windowPtr->variable_ptr, 0, firstIndex,
                                        &oneMember, &lastMemberNumber, &lastMemberOffset, ccFalse);
        if (rtrn != passed)  {
            if (firstIndex == numMemberIndices(&searchView)+1)  lastMemberNumber = memberLL->elementNum+1;
            else  {  setError(rtrn, pcCodePtr-1);  return;  }       }
        
        for (loopMember = firstIndex; loopMember <= secondIndex; loopMember++)  {
            rtrn = addMember(searchView.windowPtr->variable_ptr, loopMember-firstIndex+lastMemberNumber, 1, &oneMember, ccFalse);
            if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }        }
        
        findMemberIndex(searchView.windowPtr->variable_ptr, 0, firstIndex, &(GL_Path.stemMember),
                                                        &(GL_Path.stemMemberNumber), &(GL_Path.offset), ccFalse);
        return;         }
    
    
        // otherwise it's an array or a string -- resize it using resizeIndices()
    
    rtrn = findMemberIndex(searchView.windowPtr->variable_ptr, searchView.offset, firstIndex, &(GL_Path.stemMember),
                                                        &(GL_Path.stemMemberNumber), &(GL_Path.offset), ccFalse);
    if (rtrn != passed)  setError(rtrn, pcCodePtr-1);
    
    
        // 2nd condition on next line:  a([+top(a)+1]) is legal
    
    if ((errCode == invalid_index_err) && (firstIndex == GL_Path.offset))   {
        errCode = passed;
        if (firstIndex == 1)  {      // if all indices have been deleted, fix the error as long as there is a member to add to
            if (searchView.windowPtr->variable_ptr->type == string_type)
                GL_Path.stemMemberNumber = searchView.offset+1;
            else  GL_Path.stemMemberNumber = 1;
            GL_Path.stemMember = LL_member(searchView.windowPtr->variable_ptr, GL_Path.stemMemberNumber);
            GL_Path.offset = 0;     }
        else  {
            rtrn = findMemberIndex(searchView.windowPtr->variable_ptr, searchView.offset, firstIndex-1, &(GL_Path.stemMember),
                                                                        &(GL_Path.stemMemberNumber), &(GL_Path.offset), ccFalse);
            if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
            GL_Path.offset++;       // by default, append to the index just below
    }   }
    
    if (errCode != passed)  return;
    
    if ((searchView.windowPtr->variable_ptr->type != string_type)
                && (searchView.width != searchView.windowPtr->variable_ptr->instances))
        {  setError(incomplete_variable_err, pcCodePtr-1);  return;  }
    
    
        // resizeIndices() takes care of the resize of both the member and the variable
    
    resizeIndices(GL_Path.stemMember, searchView.width, GL_Path.offset, GL_Path.indices);
}


void callSearchPathFunction()
{
    ccInt *commandPtr = pcCodePtr;
    
    callBytecodeFunction();
    
    if (errCode == passed)  {
        if (GL_Object.type != var_type)  setError(not_a_variable_err, commandPtr);
        else if (searchView.windowPtr == NULL)  setError(void_member_err, commandPtr);
        else if (searchView.windowPtr->variable_ptr->type < string_type)  setError(not_composite_err, commandPtr);      }
}


ccInt callIndexFunction()
{
    view holdTopView = topView;
    step_params holdGLPath = GL_Path;
    view holdSearchView = searchView;
    
    topView = searchView;
    
    
        // get the index to step to
    
    callNumericFunction(int_type);
    
    topView = holdTopView;
    GL_Path = holdGLPath;
    searchView = holdSearchView;
    
    return intRegister;
}


// navigate() is the main function for moving up/down a search path.
// This is called by the step/search functions, and in turn relies on their auxiliary functions for the appropriate step info.

void navigate(void (*stepFunction)(ccBool), ccBool defineMode, ccBool takeStep, ccBool allowStepToVoid)
{
    
        // find the member to step into (stored in GL_Path.stemMember)
    
    stepFunction(defineMode && canAddMembers);
    GL_Path.stemView = searchView;
    
    if (errCode != passed)  return;
    
    
        // perform the actual step
    
    if (takeStep)  {
        
        if ((searchView.width > 1) && (GL_Path.indices != GL_Path.stemMember->indices))
            {  setError(incomplete_member_err, pcCodePtr-1);  return;  }
        
        if (searchView.windowPtr->variable_ptr->type == composite_type)  {
            if (GL_Path.indices == 0)  {  setError(no_member_err, pcCodePtr-1);  return;  }
            else if (GL_Path.indices > 1)  {  setError(step_multiple_members_err, pcCodePtr-1);  return;  }     }
        
        stepView(&searchView, GL_Path.stemMember, GL_Path.offset, GL_Path.indices);
    }
    
    
        // set our object info before returning
    
    if ((allowStepToVoid) && (errCode == void_member_err))  {
        errCode = passed;
        GL_Object.type = no_type;   }
    else  GL_Object.type = var_type;
    
    if (searchView.windowPtr != NULL)  GL_Object.codeList = &(searchView.windowPtr->variable_ptr->codeList);
    codeNumber = 1;
}


// resizeMember() adds or subtracts indices to get newTop total indices in a variable.
// Assumes there is one and only one member.  The top is an index, not an offset.

void resizeMember(member *stemMember, ccInt stemWidth, ccInt newTop)
{
    if (newTop > stemMember->indices)  resizeIndices(stemMember, stemWidth, stemMember->indices, newTop-stemMember->indices);
    else if (newTop < stemMember->indices)  resizeIndices(stemMember, stemWidth, newTop, newTop-stemMember->indices);
}


// resizeIndices() adds newIndices indices at the given location in the given member.
// newIndices can be either positive (insertion of new indices) or negative (deletion of existing indices starting at insertionOffset).
// Calls addMemory() once for each index spanned by searchView.width.  GL_Path points to a member in searchView's window.

void resizeIndices(member *stemMember, ccInt stemWidth, ccInt insertionOffset, ccInt newIndices)
{
    member *holdGLMember;
    ccInt counter, count_to_four, c2, cboIndices, rtrn = passed;
    
    
        // First check to see if there are overlaps with existing members before resizing, as this is disallowed.
        // We don't want to have a partial resize if there's an overlap error midway through.
    
    if (stemMember->memberWindow != NULL)  {
        
        for (c2 = 0; c2 < stemWidth; c2++)  {    // in general baseView straddles multiple indices
            if (newIndices > 0)  cboIndices = newIndices;
            else  cboIndices = -newIndices;
            
                // Checking involves a four-step procedure, calling checkMemberOverlap each time.
                // Just run it 4 times and pass the counter.  Need to unflagWindow() after each CBO() call.
            
            for (count_to_four = 1; count_to_four <= 4; count_to_four++)    {
                ccInt onePassRtrn = checkMemberOverlap(stemMember->memberWindow,
                            c2*stemMember->indices + insertionOffset, cboIndices, count_to_four);
                unflagWindow(stemMember->memberWindow, busy_overlap_flag);
                if (onePassRtrn != passed)  {  rtrn = onePassRtrn;  count_to_four = 3;  }     }
            if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }    }
        
        holdGLMember = GL_Path.stemMember;   // delete GLPath.stemMember for safety (AddMem() won't know it stores correct value after resize)
        
        
            // Start from the highest index and work backwards, so we don't delete from under our own feet
            // (element numbers above the deletion point will change when we delete).
        
        for (counter = stemWidth-1; counter >= 0; counter--)  {                            // stemMember should never be 'unjammed'?
            rtrn = addMemory(stemMember->memberWindow, counter*stemMember->indices + insertionOffset, newIndices);
            unflagVariables(stemMember->memberWindow->variable_ptr, busy_add_flag);
            if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }        }
        
        GL_Path.stemMember = holdGLMember;      }
    
    
        // lastly, update the member indices
    
    stemMember->indices += newIndices;
}



// _remove() weeds out a given member or array index/range of indices from an array 

void _remove()
{
    ccInt loopMember;
    
    
        // Get the path to the condemned.
        // Unlike masterInsert(), we don't read the indices separately -- we just step into whatever we want deleted.
    
    callDefineFunction();
    if (errCode != passed)  return;
    
    if ((GL_Object.type != var_type) || (searchView.windowPtr == NULL))
        {  setError(not_a_variable_err, pcCodePtr-1);  return;  }
    if ((searchView.windowPtr != NULL) && (searchView.width != searchView.windowPtr->variable_ptr->instances))
        {  setError(incomplete_variable_err, pcCodePtr-1);  return;  }
    
    if (GL_Path.stemMember == NULL)  {  setError(undefined_member_err, pcCodePtr-1);  return;  }
    
    
        // case 1:  remove a member
    
    if (searchView.windowPtr->variable_ptr->type == composite_type)   {    // we don't take the last step, so SC points to the stem window.
    for (loopMember = 1; loopMember <= GL_Path.indices; loopMember++)  {
//        if (GL_Path.indices != GL_Path.stemMember->indices)  {  setError(incomplete_member_err, pcCodePtr-1);  return;  }
        removeMember(searchView.windowPtr->variable_ptr, GL_Path.stemMemberNumber);    }}
    
    
        // case 2:  delete indices
    
    else if (searchView.windowPtr->variable_ptr->type == array_type)  {
        resizeIndices(GL_Path.stemMember, GL_Path.stemView.width, GL_Path.offset, -GL_Path.indices);  }
    
    GL_Path.stemMember = NULL;          // clean our knives & go
    searchView.windowPtr = NULL;
}




// _if_eq() tests an expression for equality, and stores the result in boolRegister

void _if_eq()
{
    view firstArgView;
    ccInt firstDataType, secondDataType, intRegisterBackup = 0, rtrn;
    ccFloat doubleRegisterBackup = 0.;
    member fauxMember;
    window fauxStringWindow1, fauxStringWindow2, *windowToDeref = NULL;
    variable fauxStringVariable1, fauxStringVariable2;
    ccBool boolRegisterBackup1, boolRegisterBackup2, firstArgIsScalar, secondArgIsScalar;
    void *toCompare1 = NULL, *toCompare2 = NULL;
    
    
        // get the first argument
    
    compareReadArg(callBytecodeFunction, &firstDataType, &firstArgIsScalar);
    if (errCode != passed)  return;
    
    if (firstArgIsScalar)  {
        firstArgView.multipleIndices = ccFalse;
        if (firstDataType == int_type)  {  intRegisterBackup = intRegister;  toCompare1 = &intRegisterBackup;  }
        else if (firstDataType == char_type)  {  boolRegisterBackup1 = (ccBool) intRegister;  toCompare1 = &boolRegisterBackup1;  }
        else if (firstDataType == double_type)  {  doubleRegisterBackup = doubleRegister;  toCompare1 = &doubleRegisterBackup;  }
        else if (firstDataType == bool_type)  {  boolRegisterBackup1 = boolRegister;  toCompare1 = &boolRegisterBackup1;  }
        else if (firstDataType == string_type)  {
            fauxStringWindow1.variable_ptr = &fauxStringVariable1;
            fauxStringWindow1.offset = 0;
            fauxStringWindow1.width = stringRegister.elementNum;
            
            firstArgView = searchView;
            refWindow(firstArgView.windowPtr);
            windowToDeref = firstArgView.windowPtr;
            
            rtrn = newLinkedList(&(fauxStringVariable1.mem.data), stringRegister.elementNum, 1, 0., ccFalse);
            if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
            copyElements(&stringRegister, 1, &(fauxStringVariable1.mem.data), 1, stringRegister.elementNum);
            toCompare1 = &fauxStringWindow1;
    }   }
    
    else  {
        firstArgView = searchView;
        if (firstArgView.windowPtr == NULL)  firstDataType = no_type;
        else  {
            refWindow(firstArgView.windowPtr);
            windowToDeref = firstArgView.windowPtr;
    }   }
    
    compareReadArg(callBytecodeFunction, &secondDataType, &secondArgIsScalar);
    if (errCode == passed)  {
        
        if (secondArgIsScalar)  {
            searchView.multipleIndices = ccFalse;
            if (secondDataType == int_type)  toCompare2 = &intRegister;
            else if (secondDataType == char_type)  {  boolRegisterBackup2 = (ccBool) intRegister;  toCompare2 = &boolRegisterBackup2;  }
            else if (secondDataType == double_type)  toCompare2 = &doubleRegister;
            else if (secondDataType == bool_type)  {  boolRegisterBackup2 = boolRegister;  toCompare2 = &boolRegisterBackup2;  }
            else if (secondDataType == string_type)  {
                fauxStringWindow2.variable_ptr = &fauxStringVariable2;
                fauxStringWindow2.offset = 0;
                fauxStringWindow2.width = stringRegister.elementNum;
                fauxStringVariable2.mem.data = stringRegister;
                toCompare2 = &fauxStringWindow2;
        }   }
        
        boolRegister = ccTrue;      // the comparison functions only set falses upon finding a mismatch
        if (firstArgIsScalar)  {
            if ((secondDataType == var_type) || (secondDataType == no_type)) setError(type_mismatch_err, pcCodePtr-1);
            else if (secondArgIsScalar)  {
                if (secondDataType == string_type)  {  fauxMember.memberWindow = &fauxStringWindow2;  toCompare2 = &fauxMember;  }
                compareJumpTable[5*firstDataType + secondDataType](toCompare1, toCompare2);     }
            else if ((firstDataType == string_type) && (secondDataType == char_type))
                copyCompareMultiView(compareWindowData, &searchView, &firstArgView);
            else  copyCompareListToVar(toCompare1, firstDataType, &searchView, compareJumpTable);   }
        else if (firstDataType == no_type)  {
            if (secondDataType == no_type)  boolRegister = ccTrue;
            else  setError(type_mismatch_err, pcCodePtr-1);   }
        else  {
            if ((firstDataType != var_type) && (secondArgIsScalar))  {
                if ((firstDataType == char_type) && (secondDataType == string_type))
                    copyCompareMultiView(compareWindowData, &searchView, &firstArgView);
                else  copyCompareListToVar(toCompare2, secondDataType, &firstArgView, compareJumpTable);  }
            else if ( ((firstDataType == var_type) || (firstArgView.multipleIndices))
                            == ((secondDataType == var_type) || (searchView.multipleIndices)) )
                copyCompareMultiView(compareWindowData, &searchView, &firstArgView);
            else  setError(type_mismatch_err, pcCodePtr-1);
    }   }
    
    if ((firstArgIsScalar) && (firstDataType == string_type))  deleteLinkedList(&(fauxStringVariable1.mem.data));
    derefWindow(&windowToDeref);
    
    GL_Object.type = bool_type;
}


void _if_ne()
{
    _if_eq();
    boolRegister = !boolRegister;
}


// compareReadArg() reads a bytecode argument and determines its type

void compareReadArg(void(*runBytecodeFunction)(void), ccInt *dataType, ccBool *argIsScalar)
{
    runBytecodeFunction();
    if (errCode != passed)  return;
    
    *dataType = GL_Object.type;
    *argIsScalar = ((*dataType != var_type) && (*dataType != no_type));
    
    if (*dataType == var_type)  {
        if (searchView.windowPtr == NULL)  *dataType = no_type;
        else if ((searchView.windowPtr->variable_ptr->type >= bool_type) && (searchView.windowPtr->variable_ptr->type <= string_type))  {
            *dataType = searchView.windowPtr->variable_ptr->type;
            if ((!searchView.multipleIndices) && (searchView.width == 1))  {        // 2nd condition in case baseView.width /= 1
                *argIsScalar = ccTrue;
                loadRegister(searchView.windowPtr->variable_ptr, searchView.offset);
    }   }   }
}


void _if_eq_at()
{
    view firstView;
    
    
        // get the first argument
    
    eqaOneArg();
    if (errCode != passed)  return;
    firstView = searchView;
    
    eqaOneArg();
    if (errCode != passed)  return;
    
    if (firstView.windowPtr == NULL)  boolRegister = (searchView.windowPtr == NULL);
    else if (searchView.windowPtr == NULL)  boolRegister = ccFalse;
    else boolRegister = ((firstView.windowPtr->variable_ptr == searchView.windowPtr->variable_ptr)
                && (firstView.width == searchView.width) && (firstView.offset == searchView.offset));
    
    GL_Object.type = bool_type;
}


void _if_ne_at()
{
    _if_eq_at();
    boolRegister = !boolRegister;
}


void eqaOneArg()
{
    ccInt *ArgBeginningPtr, *ErrPtr, hold_err_code;
    
    ArgBeginningPtr = pcCodePtr;
    callBytecodeFunction();
    
    if (errCode == passed)  {
        if (GL_Object.type == no_type)  searchView.windowPtr = NULL;
        else if (GL_Object.type != var_type)  setError(not_a_variable_err, pcCodePtr-1);    }
    
    else if (errCode == void_member_err)  {
        
        hold_err_code = errCode;
        errCode = passed;
        ErrPtr = pcCodePtr;
        pcCodePtr = ArgBeginningPtr;
        callSkipFunction();
        
        if (ErrPtr == pcCodePtr)  searchView.windowPtr = NULL;
        else  errCode = hold_err_code;
    }
}



// The next 6 routines compute various mathematical functions of two numeric arguments, returning a numerical value.
// The following 4 routines compute various logical functions of two numeric arguments, returning a logical true or false.
// These use the respective 10 subsequent routines:  Math_..() and mathOperator().

void _if_gt()  {  mathOperator(Math_GT, bool_type);  }
void _if_ge()  {  mathOperator(Math_GE, bool_type);  }
void _if_lt()  {  mathOperator(Math_LT, bool_type);  }
void _if_le()  {  mathOperator(Math_LE, bool_type);  }

void _addf()  {  mathOperator(Math_Add, double_type);  }
void _subf()  {  mathOperator(Math_Sub, double_type);  }
void _mulf()  {  mathOperator(Math_Mul, double_type);  }
void _divf()  {  mathOperator(Math_Div, double_type);  }
void _powerf()  {  mathOperator(Math_Pow, double_type);  }
void _modi()  {  mathOperator(Math_Mod, int_type);  }

void Math_Add(ccFloat firstArgument)  {  doubleRegister += firstArgument;  }
void Math_Sub(ccFloat firstArgument)  {  doubleRegister = firstArgument-doubleRegister;  }
void Math_Mul(ccFloat firstArgument)  {  doubleRegister *= firstArgument;  }
void Math_Div(ccFloat firstArgument)  {
            if (doubleRegister == 0.)  {  setWarning(divide_by_zero_warning, pcCodePtr-1);  }
            doubleRegister = firstArgument/doubleRegister;  }
void Math_Pow(ccFloat firstArgument)  {  doubleRegister = pow(firstArgument, doubleRegister);  }
void Math_Mod(ccFloat firstArgument)  {  
            if (doubleRegister <= 0)  setWarning(out_of_range_err, pcCodePtr-1);
            else  intRegister = ((ccInt) firstArgument) % (ccInt) doubleRegister;  }

void Math_GT(ccFloat firstArgument)  {  boolRegister = (firstArgument > doubleRegister);  }
void Math_GE(ccFloat firstArgument)  {  boolRegister = (firstArgument >= doubleRegister);  }
void Math_LT(ccFloat firstArgument)  {  boolRegister = (firstArgument < doubleRegister);  }
void Math_LE(ccFloat firstArgument)  {  boolRegister = (firstArgument <= doubleRegister);  }


// mathOperator() evaluates two numeric arguments, and then calls some function of these arguments.

void mathOperator(void(*theOp)(ccFloat), ccInt theType)
{
    ccFloat firstArgument;
    
    callNumericFunction(double_type);           // arg 1
    if (errCode != passed)  return;
    firstArgument = doubleRegister;
    
    callNumericFunction(double_type);           // arg 2
    GL_Object.type = theType;
    
    theOp(firstArgument);   // compute the function (return value isn't specified because result is stored in global variables)
}



// The next functions return true or false based on the truth values of their arguments.
// They make use of logicalOperator().

// _if_not() returns the opposite of _if_eq().

void _if_not()
{
    callLogicalFunction();
    boolRegister = !boolRegister;
}

void _if_and()  {  logicalOperator(&logicAnd);  }
void _if_or()  {  logicalOperator(&logicOr);  }
void _if_xor()  {  logicalOperator(&logicXor);  }

void logicAnd(ccBool firstArgument)  {  boolRegister = ((firstArgument) && (boolRegister));  }
void logicOr(ccBool firstArgument)  {  boolRegister = ((firstArgument) || (boolRegister));  }
void logicXor(ccBool firstArgument)  {  boolRegister = (firstArgument != boolRegister);  }


// logicalOperator() evaluates two logical arguments and invokes logicOp() to turn these into one logical value.

void logicalOperator(void(*logicOp)(ccBool))
{
    ccBool firstArgument;
    
    callLogicalFunction();             // get first arg
    if (errCode != passed)  return;
    firstArgument = boolRegister;
    
    callLogicalFunction();             // get second arg
    logicOp(firstArgument);    // and evaluate the conditional
}



// _code_number() changes the code # to run in a function:  e.g. f#0() to run the constructor

void _code_number()
{
    ccInt codeNo;
    
    
        // Get the right-hand argument:  the code # to run.
    
    callNumericFunction(int_type);
    if (errCode != passed)  return;
    codeNo = intRegister;
    
    
        // Get the left-hand argument:  the function
    
    callCodeFunction();
    
    codeNumber = codeNo;
}



// _sub_code() effects a code substitution:  'a << b' returns the code of 'b' and the variable 'a'

void _sub_code()
{
    searchPath **loopPath, *oldPath, *stemPath;
    ccInt counter, holdCodeNumber, numIndices, rtrn;
    linkedlist tempCodeRegister;
    
    
        // get the right-hand argument:  containing the code to substitute
    
    callCodeFunction();
    if (errCode != passed)  return;
    
    getCurrentCodeList(&tempCodeRegister);
    
    holdCodeNumber = codeNumber;
    
    
        // get the left-hand argument:  the variable to substitute into
    
    callBytecodeFunction();
    if (GL_Object.type != var_type)  setError(not_a_variable_err, pcCodePtr-1);
    else if (searchView.windowPtr == NULL)  setError(void_member_err, pcCodePtr-1);
    else if (searchView.windowPtr->variable_ptr->type < composite_type)  setError(not_a_function_err, pcCodePtr-1);
    
    
        // copy our code list into the official code register
    
    derefCodeList(&codeRegister);
    rtrn = resizeLinkedList(&codeRegister, tempCodeRegister.elementNum, ccFalse);
    if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
    copyElements(&tempCodeRegister, 1, &codeRegister, 1, tempCodeRegister.elementNum);
    
    if (errCode != passed)  return;
    
    
        // 'fix' the anchoring paths of each code in the codeRegister:  have their stems drawn
        // from the original stem of the anchors from the function that donated the code
    
    for (counter = 1; counter <= codeRegister.elementNum; counter++)  {
        
        loopPath = &(((code_ref *) element(&codeRegister, counter))->anchor);
        oldPath = *loopPath;
        
        if (oldPath == NULL)  stemPath = PCCodeRef.anchor;
        else  stemPath = oldPath->stem;
        
        numIndices = 1;
        if (stemPath != NULL)  {
            if (searchView.windowPtr->width % stemPath->jamb->width != 0)
                {  setError(mismatched_indices_err, pcCodePtr-1);  return;  }
            numIndices = searchView.windowPtr->width / stemPath->jamb->width;       }
        
        rtrn = drawPath(loopPath, searchView.windowPtr, stemPath, numIndices, PCCodeRef.PLL_index);  // in case GL_P.sM == NULL 
        if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
        
        refPath(*loopPath);
        derefPath(&oldPath);         }
    
    GL_Object.codeList = &codeRegister;
    codeNumber = holdCodeNumber;
}



// _append_code() conjoins two code lists:  a : b.  This is what inheritance means in Cicada.

void _append_code()
{
    linkedlist firstCodeList, secondCodeList;
    ccInt rtrn;
    
    
        // get the first code argument..
    
    callCodeFunction();
    if (errCode != passed)  return;
    
    getCurrentCodeList(&firstCodeList);
    
    
        // get the 2nd argument
    
    callCodeFunction();
    if (errCode != passed)  {  derefCodeList(&firstCodeList);  return;  }
    
    getCurrentCodeList(&secondCodeList);
    
    
        // prepare the codeRegister for holding the concatenated code list
    
    derefCodeList(&codeRegister);
    rtrn = resizeLinkedList(&codeRegister, firstCodeList.elementNum+secondCodeList.elementNum, ccFalse);
    if (rtrn != passed)  {  resizeLinkedList(&codeRegister, 0, ccFalse);  setError(rtrn, pcCodePtr-1);  }
    else  {
        copyElements(&firstCodeList, 1, &codeRegister, 1, firstCodeList.elementNum);
        copyElements(&secondCodeList, 1, &codeRegister, firstCodeList.elementNum+1, secondCodeList.elementNum);   }
    
    
        // clean up and go..
    
    GL_Object.type = composite_type;
    GL_Object.codeList = &codeRegister;
    codeNumber = 1;
}



// getCurrentCodeList() loads the codes stored in a composite- or variable-typed argument into a linked list
// (used by _sub_code(), _append_code())

void getCurrentCodeList(linkedlist *theList)
{
    ccInt numCodes = 0, rtrn;
    
    if (GL_Object.type == composite_type)  numCodes = GL_Object.codeList->elementNum;
    else if ((GL_Object.type == var_type) && (searchView.windowPtr != NULL))  {
    if (searchView.windowPtr->variable_ptr->eventualType == composite_type)  {
        numCodes = GL_Object.codeList->elementNum;
    }}
    else  {  setError(not_a_function_err, pcCodePtr-1);  return;  }
    
    rtrn = newLinkedList(theList, numCodes, sizeof(code_ref), 0., ccFalse);
    if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
    if (numCodes > 0)  copyElements(GL_Object.codeList, 1, theList, 1, numCodes);
    
    refCodeList(theList);
}



// _get_args() returns the args variable

void _get_args()
{
    GL_Path.stemMember = NULL;
    searchView = argsView;
    
    GL_Object.type = var_type;
    if (searchView.windowPtr != NULL)  GL_Object.codeList = &(argsView.windowPtr->variable_ptr->codeList);
    codeNumber = 1;
}


// _this_var() returns 'this'

void _this_var()
{
    GL_Path.stemMember = NULL;
    searchView = baseView;
    
    GL_Object.type = var_type;
    GL_Object.codeList = &(baseView.windowPtr->variable_ptr->codeList);
    codeNumber = 1;
}


// _that_var() returns 'that' -- the variable to the left of the '=' (if there is one; otherwise returns *)

void _that_var()
{
    GL_Path.stemMember = NULL;
    searchView = thatView;
    
    GL_Object.type = var_type;
    if (searchView.windowPtr != NULL)  GL_Object.codeList = &(thatView.windowPtr->variable_ptr->codeList);
    codeNumber = 1;
}



// _parent_var() returns the variable one up the search path from the executing function

void _parent_var()
{
    searchPath *searchPosition = pcSearchPath;
    
    pcCodePtr--;
    searchView.width = baseView.width;          // in case the first stem is NULL
    do  {
        searchView.windowPtr = searchPosition->jamb;
        if (searchPosition->stemIndices == 0)  {
        if (searchPosition->stem != NULL)  {
            searchView.offset = searchView.windowPtr->offset;
            searchView.width = searchPosition->stem->jamb->width;     }}
        else  {
            searchView.offset = (searchView.offset - searchView.windowPtr->offset) / searchPosition->stemIndices;
            if (searchView.width > 1)  searchView.width /= searchPosition->stemIndices;        }
        
        searchPosition = searchPosition->stem;
        if (searchPosition == NULL)  {  setError(no_parent_err, pcCodePtr-1);  return;  }
        pcCodePtr++;
    }  while (*pcCodePtr == parent_variable);
    
    searchView.windowPtr = searchPosition->jamb;
    searchView.multipleIndices = ccFalse;
    GL_Object.type = var_type;
    GL_Object.codeList = &(searchView.windowPtr->variable_ptr->codeList);
}



// _top_var() returns the top of the searchWindow variable

void _top_var()
{
    if (topView.windowPtr == NULL)  {  setError(no_member_err, pcCodePtr-1);  return;  }
    
    GL_Object.type = int_type;
    intRegister = numMemberIndices(&topView);
}



// _no_var() returns the void (* or nothing)

void _no_var()
{
    GL_Object.type = no_type;
}




// _array_cmd() defines an array type

void _array_cmd()
{
    ccInt theIndex, rtrn;
    
    callNumericFunction(int_type);
    if (errCode != passed)  return;
    theIndex = intRegister;
    
    if (theIndex < 0)  {  setError(invalid_index_err, pcCodePtr-1);  return;  }
    
    callCodeFunction();
    if (errCode != passed)  return;
    
    rtrn = insertElements(&(GL_Object.arrayDimList), 1, 1, ccFalse);
    if (rtrn != passed)  setError(rtrn, pcCodePtr-1);
    else  *LL_int(&(GL_Object.arrayDimList), 1) = theIndex;
}



// These routines define various data types.

void _bool_cmd()  {  GL_Object.type = bool_type;  }
void _char_cmd()  {  GL_Object.type = char_type;  }
void _int_cmd()  {  GL_Object.type = int_type;  }
void _double_cmd()  {  GL_Object.type = double_type;  }
void _string_cmd()  {  GL_Object.type = string_type;  }



// _constant_bool() returns the boolean constant embedded in the bytecode.

void _constant_bool()
{
    GL_Object.type = bool_type;
    
    boolRegister = (ccBool) *pcCodePtr;
    
    pcCodePtr++;
}



// _constant_int() returns the given character constant embedded in the bytecode.

void _constant_char()
{
    GL_Object.type = char_type;
    
    intRegister = *pcCodePtr;
    
    pcCodePtr++;
}



// _constant_int() returns the given integer constant embedded in the bytecode.

void _constant_int()
{
    GL_Object.type = int_type;
    
    intRegister = *pcCodePtr;
    
    pcCodePtr++;
}


// _constant_double() returns the given floating point number embedded in the bytecode.

void _constant_double()
{
    GL_Object.type = double_type;
    
    doubleRegister = *(ccFloat *) pcCodePtr;
    
    pcCodePtr += sizeof(ccFloat)/sizeof(ccInt);
}



// _constant_string() loads a pointer to an embedded string constant into GL_Object

void _constant_string()
{
    GL_Object.type = string_type;
    resizeLinkedList(&stringRegister, *pcCodePtr, ccFalse);
    pcCodePtr++;
    
    setElements(&stringRegister, 1, stringRegister.elementNum, (void *) pcCodePtr);
    pcCodePtr += (ccInt) ceil(((ccFloat) stringRegister.elementNum)/sizeof(ccInt));
}



// _code_block() stores a pointer to the code that is encountered in the global GL_Object variable;
// then skips the program counter to the end of the code.

void _code_block()
{
    code_ref *oneCodeRef;
    ccInt rtrn;
    
    derefCodeList(&codeRegister);
    rtrn = resizeLinkedList(&codeRegister, 1, ccFalse);
    if (rtrn != passed)  {  setError(rtrn, pcCodePtr-1);  return;  }
    
    oneCodeRef = (code_ref *) element(&codeRegister, 1);
    oneCodeRef->PLL_index = PCCodeRef.PLL_index;
    oneCodeRef->references = PCCodeRef.references;
    oneCodeRef->code_ptr = pcCodePtr;
    oneCodeRef->anchor = NULL;          // will be 'customized' later
    
    refCodeList(&codeRegister);
    
    runSkipMode(0);     // get over the code without executing any of it
    pcCodePtr++;
    
    GL_Object.type = composite_type;
    GL_Object.codeList = &codeRegister;
    codeNumber = 1;
}



// _illegal() is used in jump tables for forbidden commands -- it just throws an error.

void _illegal()
{
    setError(illegal_command_err, pcCodePtr-1);
}


// refGLObjectCodeLL() references all codes in theCodeLL.

void refCodeList(linkedlist *theCodeLL)
{
    ccInt counter;
    
    for (counter = 1; counter <= theCodeLL->elementNum; counter++)    {
        refCodeRef((code_ref *) element(theCodeLL, counter));   }
}


// derefCodeList() dereferences all code refs in theCodeLL, but leaves the linked list itself intact

void derefCodeList(linkedlist *theCodeLL)
{
    ccInt counter;
    
    for (counter = 1; counter <= theCodeLL->elementNum; counter++)   {
        derefCodeRef((code_ref *) element(theCodeLL, counter));    }
}




// the various numeric adapters are used to read a number from an object.

void intAdapter()  {  numAdapter(int_type);  }      // not used.. need to figure out how to incorporate this for 'mod'
void doubleAdapter()  {  numAdapter(double_type);  }

void numAdapter(ccInt numType)
{
    variable *theVariable;
    void(*loadNum)(variable *, ccInt);
    
    pcCodePtr--;
    callBytecodeFunction();        // read in an argument
    if (errCode != passed)  return;
    
    if ((GL_Object.type != var_type) || (searchView.windowPtr == NULL))
        {  setError(not_a_variable_err, pcCodePtr-1);  return;  }
    
    if (searchView.width != 1)  {  setError(multiple_indices_not_allowed_err, pcCodePtr-1);  return;  }
    theVariable = searchView.windowPtr->variable_ptr;
    if ((theVariable->type < char_type) || (theVariable->type > double_type))
        {  setError(not_a_number_err, pcCodePtr-1);  return;  }    // must be numeric
    
    if (numType == int_type)  loadNum = &loadIntRegister;
    else if (numType == double_type)  loadNum = &loadDoubleRegister;
    else if (theVariable->type == double_type)  loadNum = &loadDoubleRegister;
    else  loadNum = &loadIntRegister;
    
    loadNum(searchView.windowPtr->variable_ptr, searchView.offset);
}


// Next 16 routines:  used for reading & writing numeric types.  Called by the jump tables at the very end.

void loadIntRchar(void *sourcePtr)  {  intRegister = (ccInt) *(unsigned char *) sourcePtr;  }
void loadIntRint(void *sourcePtr)  {  intRegister = *(ccInt *) sourcePtr;  }
void loadIntRdouble(void *sourcePtr)  {
    if (!((*(ccFloat *) sourcePtr <= ccIntMax) && (*(ccFloat *) sourcePtr >= ccIntMin)))  setWarning(out_of_range_err, pcCodePtr-1);
    else  intRegister = (ccInt) *(ccFloat *) sourcePtr;  }

void saveIntRchar(void *destPtr)  {
    if (!((intRegister <= UCHAR_MAX) && (intRegister >= 0)))  setWarning(out_of_range_err, pcCodePtr-1);
    else  *(unsigned char *) destPtr = (unsigned char) intRegister;  }
void saveIntRint(void *destPtr)  {  *(ccInt *) destPtr = (ccInt) intRegister;  }
void saveIntRdouble(void *destPtr)  {  *(ccFloat *) destPtr = (ccFloat) intRegister;  }

void loadDPRchar(void *sourcePtr)  {  doubleRegister = (ccFloat) *(unsigned char *) sourcePtr;  }
void loadDPRint(void *sourcePtr)  {  doubleRegister = (ccFloat) *(ccInt *) sourcePtr;  }
void loadDPRdouble(void *sourcePtr)  {  doubleRegister = *(ccFloat *) sourcePtr;  }

void saveDPRchar(void *destPtr)  {
    if (!((doubleRegister <= UCHAR_MAX) && (doubleRegister >= 0)))  setWarning(out_of_range_err, pcCodePtr-1);
    else  *(unsigned char *) destPtr = (unsigned char) doubleRegister;  }
void saveDPRint(void *destPtr)  {
    if (!((doubleRegister <= ccIntMax) && (doubleRegister >= ccIntMin)))  setWarning(out_of_range_err, pcCodePtr-1);
    else  *(ccInt *) destPtr = (ccInt) doubleRegister;  }
void saveDPRdouble(void *destPtr)  {  *(ccFloat *) destPtr = doubleRegister;  }



// ****************************************
// The next routines are used by the skip jump table when hopping over a chunk of code to get to the end, or to the next code marker.
// They are perhaps similar to the checkBytecode routines, but leave most of the checks out (for speed).
// These are grouped by the kind of arguments/parameters they expect in the bytecode.

void skipNoArgs()  {  }
void skipOneArg()  {  callSkipFunction();  }
void skipTwoArgs()  {  callSkipFunction();  callSkipFunction();  }
void skipThreeArgs() {  callSkipFunction();  callSkipFunction();  callSkipFunction();  }
void skipBackArgs() {  while (*pcCodePtr == parent_variable)  pcCodePtr++;  }

void skipInt()  {  pcCodePtr++;  }
void skipDouble()  {  pcCodePtr += sizeof(ccFloat)/sizeof(ccInt);  }
void skipString()  {  ccInt StrLength = *pcCodePtr;  pcCodePtr += 1 + (ccInt) ceil(1.*StrLength/sizeof(ccInt));  }

void skipIntAndOneArg()  {  pcCodePtr++;  callSkipFunction();  }
void skipIntAndTwoArgs()  {  pcCodePtr++;  callSkipFunction();  callSkipFunction();  }
void skipOneArgAndInt()  {  pcCodePtr++;  callSkipFunction();  }

void skipCode()  {  runSkipMode(0);  pcCodePtr++;  }


// ****************************************

// checkBytecode() tests Cicada code as far as possible before it's loaded.
// This works because the code is static during runtime; the dynamic checks that Cicada performs are a chronic burden.
// 4 checks:  make sure each command is valid and doesn't overrun the end of memory; jumps are within the bytecode block;
// jumps go to start of a sentence; and there is a null statement at the end.

void checkBytecode()
{
    ccInt jumpCounter, sentenceCounter, rtrn;
    
    
        // We keep track of all the goto addresses and sentence beginnings.
    
    errIndex = 1;
    rtrn = newLinkedList(&jumpList, 0, sizeof(ccInt *), 2., ccFalse);
    if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr);  return;  }
    rtrn = newLinkedList(&jumpFromList, 0, sizeof(ccInt *), 2., ccFalse);
    if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr);  return;  }
    rtrn = newLinkedList(&sentenceList, 0, sizeof(ccInt *), 2., ccFalse);
    if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr);  return;  }
    
    
        // Check the outermost layer of bytecode.
    
    checkBytecodeSentences();
    
    
        // Ensure that all gotos point to the start of some sentence.
    
    if (errCode == passed)  {
    for (jumpCounter = 1; jumpCounter <= jumpList.elementNum; jumpCounter++)    {
        for (sentenceCounter = 1; sentenceCounter <= sentenceList.elementNum; sentenceCounter++)   {
            if (*(ccInt **) findElement(&jumpList, jumpCounter) == *(ccInt **) findElement(&sentenceList, sentenceCounter))  break;
            else if (sentenceCounter == sentenceList.elementNum)  {
                pcCodePtr = *(ccInt **) findElement(&jumpFromList, jumpCounter);    // to set errIndex correctly below
                setError(bad_jump_err, pcCodePtr);      // do this manually, since the code isn't yet loaded into memory
                jumpCounter = jumpList.elementNum;
                break;
    }}  }   }
    
    deleteLinkedList(&jumpList);      // clean up
    deleteLinkedList(&sentenceList);
    deleteLinkedList(&jumpFromList);
    
    
        // If we didn't get to the end of the code, we'll set a warning -- at least it won't crash the interpreter.
    
    if ((errCode == passed) && (pcCodePtr != endCodePtr))  setWarning(inaccessible_code_warning, pcCodePtr);
}


// checkBytecodeSentences() evaluates one sentence after another to make sure each command is valid.
// Stops at the null terminating character, or if there's an error.

void checkBytecodeSentences()
{
    ccInt rtrn;
    
    while (*pcCodePtr != end_of_script)   {
        
        
            // keep track of our sentence beginnings for checkBytecode()
            
        rtrn = addElements(&sentenceList, 1, ccFalse);
        if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr);  return;  }
        *(ccInt **) findElement(&sentenceList, sentenceList.elementNum) = pcCodePtr;
        
        
            // run each outermost command
        
        checkBytecodeArg(start_arg);
        if (errCode != passed)  break;         }
        
        
        // add the null terminator as a valid jump point
    
    rtrn = addElements(&sentenceList, 1, ccFalse);
    if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr);  return;  }
    *(ccInt **) findElement(&sentenceList, sentenceList.elementNum) = pcCodePtr;
    
    if (errCode == passed)  pcCodePtr++;   // get past the null terminator
}


// These next few routines skip over Cicada commands having various combinations of arguments/inlined constants.
// They check to make sure the code doesn't spill past endCodePtr, and the jump tables take care of illegal commands.

void checkCommand(ccInt theCmd)      // checks bread-and-butter operators
{
    if (theCmd == define_equate)  pcCodePtr++;
    if ((theCmd == code_number) || (theCmd == substitute_code))  {
        if ((rightArgs[theCmd] != no_arg) && (errCode == passed))  checkBytecodeArg(rightArgs[theCmd]);
        if ((leftArgs[theCmd] != no_arg) && (errCode == passed))  checkBytecodeArg(leftArgs[theCmd]);     }
    else  {
        if ((leftArgs[theCmd] != no_arg) && (errCode == passed))  checkBytecodeArg(leftArgs[theCmd]);
        if ((rightArgs[theCmd] != no_arg) && (errCode == passed))  checkBytecodeArg(rightArgs[theCmd]);     }
}

void checkBackCommand(ccInt theCmd)
{
    while (*pcCodePtr == parent_variable)  {
        pcCodePtr++;
        if (pcCodePtr >= endCodePtr)  {  pcCodePtr = endCodePtr;  setError(code_overflow_err, pcCodePtr);  }        }
}

void checkFunction(ccInt theCmd)      // checks user-defined/built-in functions
{
    if (theCmd == built_in_function)  checkBytecodeArg(data_arg);
    else if (theCmd == user_function)  checkBytecodeArg(var_arg);
    
    if (errCode == passed)  checkBytecodeArg(var_arg);
}

void checkInteger(ccInt theCmd)          // checks commands that have integer constants
{
    pcCodePtr++;
    if (pcCodePtr >= endCodePtr)  {  pcCodePtr = endCodePtr;  setError(code_overflow_err, pcCodePtr);  }
    
    if ((leftArgs[theCmd] != no_arg) && (errCode == passed))  checkBytecodeArg(leftArgs[theCmd]);
    if ((rightArgs[theCmd] != no_arg) && (errCode == passed))  checkBytecodeArg(rightArgs[theCmd]);
}

void checkDouble(ccInt theCmd)    // checks inlined doubles
{
    pcCodePtr += sizeof(ccFloat)/sizeof(ccInt);
    if (pcCodePtr >= endCodePtr)  {  pcCodePtr = endCodePtr;  setError(code_overflow_err, pcCodePtr);  }
}

void checkString(ccInt theCmd)    // checks inlined strings
{
    ccInt sizeofStrings = *pcCodePtr;
    if (sizeofStrings < 0)  {  setError(illegal_command_err, pcCodePtr);  return;  }
    
    sizeofStrings += align(sizeofStrings);
    
    pcCodePtr++;
    
    if (pcCodePtr + (ccInt) ceil(1.*sizeofStrings/sizeof(ccInt)) >= endCodePtr)      // step over the width of the string
        {  pcCodePtr = endCodePtr;  setError(code_overflow_err, pcCodePtr);  return;  }
    pcCodePtr += (ccInt) ceil(1.*sizeofStrings/sizeof(ccInt));
}

void checkCode(ccInt theCmd)      // checks a code block
{
    checkBytecodeSentences();      // each code block contains many sentences
}

void checkJump(ccInt theCmd)     // cheks conditional/unconditional jumps
{
    ccInt rtrn, jumpOffset = *pcCodePtr;
    
    if (pcCodePtr + jumpOffset >= endCodePtr)  {  setError(bad_jump_err, pcCodePtr);  return;  }
    
    
        // Add the jump position to checkBytecode()'s jumpList.
    
    rtrn = addElements(&jumpList, 1, ccFalse);
    if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr);  return;  }
    rtrn = addElements(&jumpFromList, 1, ccFalse);
    if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr);  return;  }
    *(ccInt **) findElement(&jumpList, jumpList.elementNum) = pcCodePtr + jumpOffset;
    *(ccInt **) findElement(&jumpFromList, jumpFromList.elementNum) = pcCodePtr;
    
    pcCodePtr++;
    if (pcCodePtr >= endCodePtr)  {  pcCodePtr = endCodePtr;  setError(code_overflow_err, pcCodePtr);  return;  }
    
    if ((rightArgs[theCmd] != no_arg) && (errCode == passed))  checkBytecodeArg(rightArgs[theCmd]);
}

void checkIndices(ccInt theCmd)     // checks array operators [] of various sorts
{
    if (theCmd == type_array)  checkBytecodeArg(data_arg);
    if ((leftArgs[theCmd] != no_arg) && (errCode == passed))  checkBytecodeArg(leftArgs[theCmd]);
    if ((rightArgs[theCmd] != no_arg) && (errCode == passed))  checkBytecodeArg(rightArgs[theCmd]);
    
    if (theCmd != type_array)  {
        checkBytecodeArg(data_arg);
        if (((theCmd == step_to_indices) || (theCmd == insert_indices)) && (errCode == passed))  {
            checkBytecodeArg(data_arg);     }   }
}

void checkBytecodeArg(ccInt tableID)     // runs a bytecode function via the check jump tables
{
    ccInt theCommand = *pcCodePtr;
    
    if ((theCommand < 0) || (theCommand >= commands_num))  {  setError(illegal_command_err, pcCodePtr);  return;  }
    
    pcCodePtr++;
    if (pcCodePtr >= endCodePtr)  {  pcCodePtr = endCodePtr;  setError(code_overflow_err, pcCodePtr);  return;  }
    
    checkJumpTables[tableID][theCommand](theCommand);
}

void illegalCmd(ccInt theCmd)           // the checker encountered a command that is out of place -- throw an error
{
    setError(illegal_command_err, pcCodePtr);
}




// ****************************************
// The following are the main loops of the Cicada interpreter.
// These routines read in the appropriate commands and call them from the jump tables.


// beginExecution() stores the current program counter & related information on the 'stack',
// then moves it to a new location and starts running a new code.

void beginExecution(code_ref *theCode, ccBool ifStorePC, ccInt newPCOffset, ccInt newPCwidth, ccInt codesToSkip)
{
    code_ref holdPCCodeRef = PCCodeRef;
    searchPath *holdPCpath = pcSearchPath;
    view holdTopView = topView, holdThatView = thatView;
    ccInt *holdPCCodePtr = pcCodePtr, *holdStartCodePtr = startCodePtr, rtrn;
    object_params holdObjectParams = GL_Object;
    linkedlist holdLL;
    step_params holdStepParams = GL_Path;
    void *oldStackTop, *newStackTop;
    
    
        // don't go too many levels deep or we will blow the (real) stack
    
    if (recursionCounter >= glMaxRecursions)  {  
    setError(recursion_depth_err, pcCodePtr);  return;  }
    recursionCounter++;
    
    
        // point to the code we will run
        
    PCCodeRef = *theCode;
    startCodePtr = storedCode(theCode->PLL_index)->bytecode;
    pcCodePtr = PCCodeRef.code_ptr;
    
    
        // store the old state on our makeshift 'program stack'
    
    if (ifStorePC)  {
        rtrn = pushStack(&PCStack, &newStackTop);
        if (rtrn != passed)  {  setError(out_of_memory_err, pcCodePtr);  PCCodeRef = holdPCCodeRef;  return;  }
        *(view *) newStackTop = baseView;
        
        rtrn = pushStack(&PCStack, &newStackTop);
        if (rtrn != passed)    {  setError(out_of_memory_err, pcCodePtr);  PCCodeRef = holdPCCodeRef;  return;  }
        *(view *) newStackTop = searchView;     }
    
    
        // move our memory location to the code's variable
    
    pcSearchPath = theCode->anchor;
    baseView.windowPtr = pcSearchPath->jamb;
    baseView.offset = newPCOffset;
    baseView.width = newPCwidth;
    baseView.multipleIndices = ccFalse;
    topView.windowPtr = NULL;
    
    refCodeRef(&PCCodeRef);
    refPath(pcSearchPath);
    refWindow(baseView.windowPtr);
    
    errCode = passed;
    
    
        // Skip over codes we are not running (i.e. find the Nth 'code' marker), and go.
    
    if (codesToSkip != 0)   {
        runSkipMode(codesToSkip);
        if (*pcCodePtr == code_marker)  {
            pcCodePtr++;
            runBytecode();   }   }
    else  runBytecode();
    
    
        // dereference the old location variables
    
    derefWindow(&(baseView.windowPtr));
    derefPath(&pcSearchPath);
    derefCodeRef(&PCCodeRef);
    
    
        // pull the old location info off the program stack
    
    if (ifStorePC)   {
        popStack(&PCStack, &oldStackTop);
        searchView = *(view *) oldStackTop;
        
        popStack(&PCStack, &oldStackTop);
        baseView = *(view *) oldStackTop;    }
    
    
    
        // restore the old code pointer
    
    pcSearchPath = holdPCpath;
    
    startCodePtr = holdStartCodePtr;
    pcCodePtr = holdPCCodePtr;
    PCCodeRef = holdPCCodeRef;
    GL_Path = holdStepParams;
    thatView = holdThatView;
    topView = holdTopView;
    
    holdLL = GL_Object.arrayDimList;
    GL_Object = holdObjectParams;
    GL_Object.arrayDimList = holdLL;
    
    recursionCounter--;
}


// runBytecode() runs a succession of bytecode sentences, until there is an error or else we hit the end or a 'code' marker.

void runBytecode()
{
    while (*pcCodePtr != end_of_script)   {
        callSentenceStartFunction();
        if (errCode != passed)  return;         }
    
    pcCodePtr++;         // over the 0
}


// runSkipMode() skips over a run of bytecode sentences, stopping at the first null terminator or 'code' marker, or upon error.
// This avoids running the commands by calling a different 'skip' jump table.

void runSkipMode(ccInt codesToSkip)
{
    while (*pcCodePtr != end_of_script)  {
        if ((*pcCodePtr == code_marker) && (codesToSkip != 0))    {
            codesToSkip--;
            if (codesToSkip == 0)  return;    }
        callSkipFunction();
        if (errCode != passed)  return;
    }
}


// These next routines run various functions by calling the function-ID term in the appropriate jump table.
// Commands sometimes behave differently when they begin a sentence; hence the callSentenceStartFunction() function's table
// sometimes has pointers to different functions than the main tables.
// The other jump tables could in principle be put into one, except that the compiler uses them to see what arguments are legal where.

void callSentenceStartFunction()  {  callFunction(sentenceStartJumpTable);  }
void callBytecodeFunction()  {  callFunction(bytecodeJumpTable);  }
void callDefineFunction()  {  callFunction(defineJumpTable);  }

void callLogicalFunction()  {
    
    callFunction(bytecodeJumpTable);
    if (errCode != passed)  return;
    
    if (GL_Object.type == var_type)  {
        if (searchView.windowPtr == NULL)  {  setError(void_member_err, pcCodePtr-1);  return;  }
        if (searchView.width != 1)  {  setError(multiple_indices_not_allowed_err, pcCodePtr-1);  return;  }
        if (searchView.windowPtr->variable_ptr->type != bool_type)  {  setError(type_mismatch_err, pcCodePtr-1);  return;  }
        loadBoolRegister(searchView.windowPtr->variable_ptr, searchView.offset);
        GL_Object.type = bool_type;      }
    
    else if (GL_Object.type != bool_type)  {  setError(type_mismatch_err, pcCodePtr-1);  return;  }
}

void callNumericFunction(ccInt numType)  {
    
    variable *theVariable;
    ccInt *holdPCCodePtr = pcCodePtr;
    void(*loadNum)(variable *, ccInt);
    
    callBytecodeFunction();        // read in an argument
    if (errCode != passed)  return;
    
    
        // if it's stored in the registers, we're done
    
    if (numType == GL_Object.type)  return;
    
        // do a type conversion between registers if necessary
    
    else if ((numType == double_type) && (GL_Object.type == int_type))  {
        doubleRegister = (ccFloat) intRegister;
        return;     }
    else if ((numType == int_type) && (GL_Object.type == double_type))  {
        if (!((doubleRegister <= ccIntMax) && (doubleRegister >= ccIntMin)))  setWarning(out_of_range_err, holdPCCodePtr);
        intRegister = (ccInt) doubleRegister;
        return;     }
    
    
        // the number we want is stored in a variable; if it passes some quick checks then get it out
    
    if (GL_Object.type != var_type)  {  setError(not_a_variable_err, pcCodePtr-1);  return;  }
    if (searchView.windowPtr == NULL)  {  setError(void_member_err, pcCodePtr-1);  return;  }
    if (searchView.width != 1)  {  setError(multiple_indices_not_allowed_err, pcCodePtr-1);  return;  }
    
    theVariable = searchView.windowPtr->variable_ptr;
    if ((theVariable->type < char_type) || (theVariable->type > double_type))  {
        setError(not_a_number_err, pcCodePtr-1);
        return;         }    // must be numeric
    
    if (numType == int_type)  loadNum = loadIntRegister;
    else if (numType == double_type)  loadNum = loadDoubleRegister;
    else if (theVariable->type == double_type)  loadNum = loadDoubleRegister;
    else  loadNum = loadIntRegister;
    
    loadNum(searchView.windowPtr->variable_ptr, searchView.offset);
}

void callSkipFunction()  {  callFunction(skipJumpTable);  }

void callFunction(void(*commandJumpTable[commands_num])(void))
{
    ccInt theFunction = *pcCodePtr;
    
    pcCodePtr++;
    commandJumpTable[theFunction]();
}

void callCodeFunction()
{
    ccInt *commandPtr = pcCodePtr;
    
    callFunction(codeJumpTable);
    
    if ((*commandPtr != type_array) && (*commandPtr != define_equate))  {
        resizeLinkedList(&(GL_Object.arrayDimList), 0, ccFalse);        }
}



// Read/write primitive data types

void saveBoolRegister(variable *destVar, ccInt destOffset)
{
    *LL_Char(&(destVar->mem.data), destOffset + 1) = boolRegister;
}

void loadBoolRegister(variable *sourceVar, ccInt sourceOffset)
{
    boolRegister = *LL_Char(&(sourceVar->mem.data), sourceOffset + 1);
}

void saveIntRegister(variable *destVar, ccInt destOffset)
{
    saveIntRegJumpTable[destVar->type](element(&(destVar->mem.data), destOffset + 1));
}

void loadIntRegister(variable *sourceVar, ccInt sourceOffset)
{
    loadIntRegJumpTable[sourceVar->type](element(&(sourceVar->mem.data), sourceOffset + 1));
}

void saveDoubleRegister(variable *destVar, ccInt destOffset)
{
    saveDPRegJumpTable[destVar->type](element(&(destVar->mem.data), destOffset + 1));
}

void loadDoubleRegister(variable *sourceVar, ccInt sourceOffset)
{
    loadDPRegJumpTable[sourceVar->type](element(&(sourceVar->mem.data), sourceOffset + 1));
}

void loadStringRegister(variable *sourceVar, ccInt sourceOffset)
{
    window *stringCharsWindow = LL_member(sourceVar, sourceOffset + 1)->memberWindow;
    ccInt rtrn = resizeLinkedList(&stringRegister, stringCharsWindow->width, ccFalse);
    if (rtrn != passed)  setError(rtrn, pcCodePtr-1);
    
    copyElements(&(stringCharsWindow->variable_ptr->mem.data), stringCharsWindow->offset+1, &stringRegister, 1, stringRegister.elementNum);
}

void(*loadRegisterFunctions[])(variable *, ccInt) = { loadBoolRegister, loadIntRegister, loadIntRegister, loadDoubleRegister, loadStringRegister };
void loadRegister(variable *sourceVar, ccInt sourceOffset)
{
    loadRegisterFunctions[sourceVar->type](sourceVar, sourceOffset);
}




// leftArgs[] and rightArgs[] determine the type of arguments that each operator can take on the left/right.
// For example, equate takes a variable on the left, and any type of data (variable, constant) on the right.
// The elements of these lists correspond to the various checkJumpTables[]s below.

char leftArgs[commands_num] =
{   no_arg, no_arg, no_arg, no_arg, no_arg,
        no_arg, var_arg, no_arg, var_arg, var_arg,
    no_arg, var_arg, var_arg, var_arg, var_arg,
        var_arg, var_arg, var_arg, no_arg, data_arg,
    data_arg, data_arg, data_arg, data_arg, data_arg,   // 20
        var_arg, var_arg, data_arg, data_arg, data_arg,
    data_arg, data_arg, data_arg, no_arg, data_arg,
        data_arg, data_arg, var_arg, var_arg, code_arg,
    no_arg, no_arg, no_arg, no_arg, no_arg,
        no_arg, no_arg, no_arg, no_arg, no_arg,
    no_arg, no_arg, no_arg, no_arg, no_arg,             // 50
        no_arg, no_arg, no_arg     };

char rightArgs[commands_num] =
{   no_arg, no_arg, data_arg, data_arg, no_arg,
        var_arg, no_arg, no_arg, type_arg, data_arg,
    no_arg, no_arg, no_arg, no_arg, no_arg,
        no_arg, no_arg, no_arg, var_arg, data_arg,
    data_arg, data_arg, data_arg, data_arg, data_arg,   // 20
        var_arg, var_arg, data_arg, data_arg, data_arg,
    data_arg, data_arg, data_arg, data_arg, data_arg,
        data_arg, data_arg, data_arg, code_arg, code_arg,
    no_arg, no_arg, no_arg, no_arg, no_arg,
        no_arg, type_arg, no_arg, no_arg, no_arg,
    no_arg, no_arg, no_arg, no_arg, no_arg,             // 50
        no_arg, no_arg, no_arg       };



// Jump tables/lists for determining whether a given command is allowed, based on the type of command
// expected at the given position (variable, data, etc.).
// The functions contained here are called by checkBytecode(), in bytecd.c(pp), when checking code.
// This table is also used, as a list (i.e. the elements are just compared to &illegalCmd, not run) by cmpile.c(pp).

void(*checkJumpTables[6][commands_num])(ccInt) =  {

{   &illegalCmd, &checkJump, &checkJump, &checkJump, &checkCommand,     // 'starting' commands
        &checkCommand, &checkFunction, &checkFunction, &checkCommand, &checkCommand,
    &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,
        &checkIndices, &checkIndices, &checkIndices, &checkCommand, &illegalCmd,
    &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,
        &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,
    &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,
        &illegalCmd, &illegalCmd, &checkCommand, &checkCommand, &illegalCmd,
    &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,
        &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,
    &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,
        &illegalCmd, &illegalCmd, &illegalCmd      },

{   &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,    // variable
        &illegalCmd, &checkFunction, &checkFunction, &checkCommand, &checkCommand,
    &checkInteger, &checkInteger, &checkIndices, &checkIndices, &checkCommand,
        &checkIndices, &checkIndices, &checkIndices, &illegalCmd, &illegalCmd,
    &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,
        &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,
    &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,
        &illegalCmd, &illegalCmd, &checkCommand, &checkCommand, &illegalCmd,
    &checkCommand, &checkCommand, &checkCommand, &checkBackCommand, &checkCommand,
        &checkCommand, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,
    &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,
        &illegalCmd, &illegalCmd, &illegalCmd         },

{   &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,
        &illegalCmd, &checkFunction, &checkFunction, &checkCommand, &checkCommand,
    &checkInteger, &checkInteger, &checkIndices, &checkIndices, &checkCommand,
        &checkIndices, &checkIndices, &checkIndices, &illegalCmd, &checkCommand,
    &checkCommand, &checkCommand, &checkCommand, &checkCommand, &checkCommand,
        &checkCommand, &checkCommand, &checkCommand, &checkCommand, &checkCommand,
    &checkCommand, &checkCommand, &checkCommand, &checkCommand, &checkCommand,
        &checkCommand, &checkCommand, &checkCommand, &checkCommand, &illegalCmd,
    &checkCommand, &checkCommand, &checkCommand, &checkBackCommand, &checkCommand,
        &checkCommand, &illegalCmd, &checkCommand, &checkCommand, &checkCommand,
    &checkCommand, &checkCommand, &checkInteger, &checkInteger, &checkInteger,
        &checkDouble, &checkString, &illegalCmd         },

{   &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,                // code
        &illegalCmd, &checkFunction, &checkFunction, &checkCommand, &checkCommand,
    &checkInteger, &checkInteger, &checkIndices, &checkIndices, &checkCommand,
        &checkIndices, &checkIndices, &checkIndices, &illegalCmd, &illegalCmd,
    &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,
        &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,
    &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,
        &illegalCmd, &illegalCmd, &checkCommand, &checkCommand, &checkCommand,
    &checkCommand, &checkCommand, &checkCommand, &checkBackCommand, &checkCommand,
        &checkCommand, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,
    &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,
        &illegalCmd, &illegalCmd, &checkCode         },

{   &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd, &illegalCmd,                // type
        &illegalCmd, &checkFunction, &checkFunction, &checkCommand, &checkCommand,
    &checkInteger, &checkInteger, &checkIndices, &checkIndices, &checkCommand,
        &checkIndices, &checkIndices, &checkIndices, &illegalCmd, &checkCommand,
    &checkCommand, &checkCommand, &checkCommand, &checkCommand, &checkCommand,
        &checkCommand, &checkCommand, &checkCommand, &checkCommand, &checkCommand,
    &checkCommand, &checkCommand, &checkCommand, &checkCommand, &checkCommand,
        &checkCommand, &checkCommand, &checkCommand, &checkCommand, &checkCommand,
    &checkCommand, &checkCommand, &checkCommand, &checkBackCommand, &checkCommand,
        &checkCommand, &checkIndices, &checkCommand, &checkCommand, &checkCommand,
    &checkCommand, &checkCommand, &checkInteger, &checkInteger, &checkInteger,
        &checkDouble, &checkString, &checkCode         }  };



// Lastly, the jump tables, which store addresses of the operator-handling functions above in this file, in order of ID number.
// This is much faster than dynamically searching for the appropriate function.

void(*skipJumpTable[commands_num])(void) = {     // for skipping over code
    &_illegal, &skipInt, &skipIntAndOneArg, &skipIntAndOneArg, &skipNoArgs,
        &skipOneArg, &skipTwoArgs, &skipTwoArgs, &skipIntAndTwoArgs, &skipTwoArgs,
    &skipInt, &skipOneArgAndInt, &skipTwoArgs, &skipThreeArgs, &skipOneArg,
        &skipTwoArgs, &skipTwoArgs, &skipThreeArgs, &skipOneArg, &skipTwoArgs,
    &skipTwoArgs, &skipTwoArgs, &skipTwoArgs, &skipTwoArgs, &skipTwoArgs,
        &skipTwoArgs, &skipTwoArgs, &skipTwoArgs, &skipTwoArgs, &skipTwoArgs,
    &skipTwoArgs, &skipTwoArgs, &skipTwoArgs, &skipOneArg, &skipTwoArgs,
        &skipTwoArgs, &skipTwoArgs, &skipTwoArgs, &skipTwoArgs, &skipTwoArgs,
    &skipNoArgs, &skipNoArgs, &skipNoArgs, &skipBackArgs, &skipNoArgs,
        &skipNoArgs, &skipTwoArgs, &skipNoArgs, &skipNoArgs, &skipNoArgs,
    &skipNoArgs, &skipNoArgs, &skipInt, &skipInt, &skipInt,
        &skipDouble, &skipString, &skipCode      };

void(*sentenceStartJumpTable[commands_num])(void) = {    // start-of-sentence:  resize/insert will not step
    &_illegal, &_jump_always, &_jump_if_true, &_jump_if_false, &_code_marker,
        &_func_return, &_user_function, &_built_in_function, &_def_general, &_forced_equate,
    &_search_member, &_step_to_memberID, &_step_to_index, &_step_to_indices, &_step_to_all_indices,
        &_resize_start, &_insert_index_start, &_insert_indices_start, &_remove, &_if_eq,
    &_if_ne, &_if_gt, &_if_ge, &_if_lt, &_if_le,
        &_if_eq_at, &_if_ne_at, &_addf, &_subf, &_mulf,
    &_divf, &_powerf, &_modi, &_if_not, &_if_and,
        &_if_or, &_if_xor, &_code_number, &_sub_code, &_append_code,
    &_get_args, &_this_var, &_that_var, &_parent_var, &_top_var,
        &_no_var, &_illegal, &_bool_cmd, &_char_cmd, &_int_cmd,
    &_double_cmd, &_string_cmd, &_constant_bool, &_constant_char, &_constant_int,
        &_constant_double, &_constant_string, &_code_block   };

void(*numericJumpTable[commands_num])(void) = {     // numeric arguments
    &_illegal, &_illegal, &_illegal, &_illegal, &_illegal,
        &doubleAdapter, &doubleAdapter, &_built_in_function, &doubleAdapter, &doubleAdapter,
    &doubleAdapter, &doubleAdapter, &doubleAdapter, &doubleAdapter, &doubleAdapter,
        &doubleAdapter, &doubleAdapter, &doubleAdapter, &_illegal, &_illegal,
    &_illegal, &_illegal, &_illegal, &_illegal, &_illegal,
        &_illegal, &_illegal, &_addf, &_subf, &_mulf,
    &_divf, &_powerf, &_modi, &_illegal, &_illegal,
        &_illegal, &_illegal, &doubleAdapter, &doubleAdapter, &doubleAdapter,
    &doubleAdapter, &doubleAdapter, &doubleAdapter, &doubleAdapter, &doubleAdapter,
        doubleAdapter, &_illegal, &_illegal, &_illegal, &_illegal,
    &_illegal, &_illegal, &_constant_bool, &_constant_char, &_constant_int,
        &_constant_double, &_illegal, &_illegal    };

void(*codeJumpTable[commands_num])(void) = {     // right argument of define:  something that denotes the new type
    &_illegal, &_illegal, &_illegal, &_illegal, &_illegal,
        &_illegal, &_user_function, &_built_in_function, &_def_general, &_forced_equate,
    &_object_search_member, &_object_step_to_memberID, &_object_step_to_index, &_object_step_to_indices, &_step_to_all_indices,
        &_resize, &_insert_index, &_insert_indices, &_illegal, &_if_eq,
    &_if_ne, &_if_gt, &_if_ge, &_if_lt, &_if_le,
        &_if_eq_at, &_if_ne_at, &_addf, &_subf, &_mulf,
    &_divf, &_powerf, &_modi, &_if_not, &_if_and,
        &_if_or, &_if_xor, &_code_number, &_sub_code, &_append_code,
    &_get_args, &_this_var, &_that_var, &_parent_var, &_top_var,
        &_no_var, &_array_cmd, &_bool_cmd, &_char_cmd, &_int_cmd,
    &_double_cmd, &_string_cmd, &_constant_bool, &_constant_char, &_constant_int,
        &_constant_double, &_constant_string, &_code_block   };

void(*bytecodeJumpTable[commands_num])(void) = {     // generic jump table for mid-sentence operators
    &_illegal, &_jump_always, &_jump_if_true, &_jump_if_false, &_code_marker,
        &_func_return, &_user_function, &_built_in_function, &_def_general, &_forced_equate,
    &_search_member, &_step_to_memberID, &_step_to_index, &_step_to_indices, &_step_to_all_indices,
        &_resize, &_insert_index, &_insert_indices, &_remove, &_if_eq,
    &_if_ne, &_if_gt, &_if_ge, &_if_lt, &_if_le,
        &_if_eq_at, &_if_ne_at, &_addf, &_subf, &_mulf,
    &_divf, &_powerf, &_modi, &_if_not, &_if_and,
        &_if_or, &_if_xor, &_code_number, &_sub_code, &_append_code,
    &_get_args, &_this_var, &_that_var, &_parent_var, &_top_var,
        &_no_var, &_illegal, &_bool_cmd, &_char_cmd, &_int_cmd,
    &_double_cmd, &_string_cmd, &_constant_bool, &_constant_char, &_constant_int,
        &_constant_double, &_constant_string, &_code_block   };

void(*defineJumpTable[commands_num])(void) = {     // jump table for the left side of a define statement
    &_illegal, &_jump_always, &_jump_if_true, &_jump_if_false, &_code_marker,
        &_func_return, &_user_function, &_built_in_function, &_def_general, &_forced_equate,
    &_define_search_member, &_define_step_to_memberID, &_define_step_to_index, &_define_step_to_indices, &_define_step_to_all_indices,
        &_define_resize, &_define_insert_index, &_define_insert_indices, &_remove, &_if_eq,
    &_if_ne, &_if_gt, &_if_ge, &_if_lt, &_if_le,
        &_if_eq_at, &_if_ne_at, &_addf, &_subf, &_mulf,
    &_divf, &_powerf, &_modi, &_if_not, &_if_and,
        &_if_or, &_if_xor, &_code_number, &_sub_code, &_append_code,
    &_get_args, &_this_var, &_that_var, &_parent_var, &_top_var,
        &_no_var, &_illegal, &_bool_cmd, &_char_cmd, &_int_cmd,
    &_double_cmd, &_string_cmd, &_constant_bool, &_constant_char, &_constant_int,
        &_constant_double, &_constant_string, &_code_block   };


ccInt BIF_Types[] = {
    int_type, int_type, string_type, composite_type, string_type, int_type, string_type, no_type, int_type, int_type,
    int_type, no_type, int_type, int_type, double_type, double_type, double_type, double_type, double_type, double_type,
    double_type, double_type, double_type, double_type, double_type, int_type, int_type, int_type, string_type, no_type   };

void(*loadIntRegJumpTable[])(void *) =    // for storing a number in intRegister
        {  NULL, &loadIntRchar, &loadIntRint, &loadIntRdouble  };

void(*saveIntRegJumpTable[])(void *) =    // for getting a number out of intRegister
        {  NULL, &saveIntRchar, &saveIntRint, &saveIntRdouble  };

void(*loadDPRegJumpTable[])(void *) =    // for storing a number in doubleRegister
        {  NULL, &loadDPRchar, &loadDPRint, &loadDPRdouble  };

void(*saveDPRegJumpTable[])(void *) =    // for getting a number out of doubleRegister
        {  NULL, &saveDPRchar, &saveDPRint, &saveDPRdouble  };
