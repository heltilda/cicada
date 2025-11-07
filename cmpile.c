/*
 *  cmpile.c(pp) - converts script into bytecode
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

#define CicadaCompileFile

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <float.h>
#include "cmpile.h"



// global variables -- should be changed by the user if the default int/float types are changed


// The main part of the compiler begins here

cc_compile_global_struct cc_compile_globals;


#define tokenSpec(a) ((tokenSpecType *) element(&(compiler->tokenSpecs), a))
#define scriptToken(a) ((scriptTokenType *) element(&(compiler->scriptTokens), a))

#define constDataArgType 100

// newCompiler() sets up the memory for a compiler, based on the set of token strings that the user passes.
// The token strings are stored in a series of 'jump tables' (filled with table IDs, not addresses).
// Each table stores the first 128 ASCII possibilities for matching the next character, given a certain set of prior chars.
// For example, if the only token was "*=" then table 1 would store a 2 (signifying table #2) at position '*';
// table 2 would store a 3 (meaning table #3) at position '='; and table 3 would store a 1 (signifying token #1) at position 0.

#define errOut(a,b) {  freeCompiler(compiler);  *rtrn = a;  errPosition = b;  return NULL;  }

compiler_type *newCompiler(commandTokenType *commandTokens, ccInt numCommands, ccInt *OoOdirections, ccInt numOoOs, ccInt *rtrn)
{
    compiler_type *compiler = (compiler_type *) malloc(sizeof(compiler_type));
    ccInt loopList, loopCmd, loopTokenString, loopOoO, loopArgCmd, loopArgType, loopRtrnType, loopToken = 3;
    ccInt jumpTo[9], jumpMarkers[9], loopJump, loopStartingChar;
    const char *oneChar;
    ccFloat theNum;
    bool ifFloat;
    
    *rtrn = passed;
    
    if (compiler == NULL)  {  *rtrn = out_of_memory_err;  errPosition = 0;  return compiler;  }
    
    compiler->tokenSpecs.memory = NULL;         // zero our pointers first so we know what we've allocated
    compiler->allCodeWords.memory = NULL;       // (in case we have to bail out before we've finished building arrays)
    compiler->scriptTokens.memory = NULL;
    compiler->bytecode.memory = NULL;
    compiler->opCharNum.memory = NULL;
    compiler->varNames.memory = NULL;
    compiler->numVariables = compiler->anonymousMemberNum = 0;
    
    for (loopArgType = 0; loopArgType < 11; loopArgType++)  {
    for (loopRtrnType = 0; loopRtrnType < 11; loopRtrnType++)  {
        compiler->adapters[loopArgType][loopRtrnType] = 0;
    }}
    
    for (loopTokenString = 0; loopTokenString < 128; loopTokenString++)  {
        compiler->tokens[loopTokenString].memory = NULL;        }
    
    
        // construct the jump tables from the language tokens
    
    compiler->OoOdirections = (ccInt *) malloc((numOoOs+2)*sizeof(ccInt));
    if ((newLinkedList(&(compiler->tokenSpecs), 3, sizeof(tokenSpecType), 1., true) != passed)
                || (newLinkedList(&(compiler->allCodeWords), 0, sizeof(ccInt), 1., false) != passed)
                || (compiler->OoOdirections == NULL)
                || (newLinkedList(&(compiler->scriptTokens), 0, sizeof(scriptTokenType), 1., false) != passed)
                || (newLinkedList(&(compiler->bytecode), 0, sizeof(ccInt), 1., false) != passed)
                || (newLinkedList(&(compiler->opCharNum), 0, sizeof(ccInt), 1., false) != passed)
                || (newLinkedList(&(compiler->varNames), 0, sizeof(varNameType), 1., false) != passed))
        errOut(out_of_memory_err, 0)
    
    for (loopStartingChar = 0; loopStartingChar < 128; loopStartingChar++)  {
        compiler->numLanguageOps[loopStartingChar] = 0;             }
    
    
        // read in miscellaneous information about the language; initialize memory
    
    if ((numCommands < 0) || (numOoOs < 0))  errOut(out_of_range_err, 0)
    
    for (loopOoO = 0; loopOoO < numOoOs; loopOoO++)  {
        compiler->OoOdirections[loopOoO] = OoOdirections[loopOoO];
        if ((OoOdirections[loopOoO] != l_to_r) && (OoOdirections[loopOoO] != r_to_l)) {
            errOut(out_of_range_err, -loopOoO-1)
    }   }
    compiler->OoOdirections[numOoOs] = l_to_r;
    compiler->OoOdirections[numOoOs+1] = l_to_r;
    
    compiler->specialTokens[0] = compiler->specialTokens[1] = compiler->specialTokens[2] = -1;
    
    for (loopList = 0; loopList < 128; loopList++)  {
    if (newLinkedList(compiler->tokens + loopList, 0, sizeof(tokenStringType), 1., false) != passed)  {
        errOut(out_of_memory_err, 0)
    }}
    
    
        // we start with a null token for comments, constant data and macro args, and the typeXadapter placeholder token
    
        // constant data:  we add an explicit token for this, so that, for example, strings won't feel that there's a
        // missing argument between the two quotation 'operators'
    
    tokenSpec(1)->leftArgType = tokenSpec(1)->rightArgType = -1;
    tokenSpec(1)->precedence = numOoOs+2;
    tokenSpec(1)->firstCodeWord = 1;
    
    
        // typeX adapter
    
    tokenSpec(2)->leftArgType = tokenSpec(2)->rightArgType = -1;
    tokenSpec(2)->precedence = numOoOs+2;
    tokenSpec(2)->firstCodeWord = 1;
    tokenSpec(2)->rtrnTypes[10] = true;
    compiler->adapters[10][10] = 2;
    
    
        // macro argument -- the idea here is that the code writes the compiler command to load an argument (0, 1, arg #)
        // into the result of the macro 'bytecode'
    
    tokenSpec(3)->leftArgType = -1;
    tokenSpec(3)->rightArgType = constDataArgType;
    tokenSpec(3)->precedence = numOoOs+1;
    tokenSpec(3)->firstCodeWord = 1;
    tokenSpec(3)->numCodeWords = 6;
    for (loopRtrnType = 0; loopRtrnType < 10; loopRtrnType++)
        tokenSpec(3)->rtrnTypes[loopRtrnType] = true;
    if (addElements(&(compiler->allCodeWords), 6, true) != passed)  errOut(out_of_memory_err, 1)
    *LL_int(&(compiler->allCodeWords), compiler->allCodeWords.elementNum-3) = 1;
    *LL_int(&(compiler->allCodeWords), compiler->allCodeWords.elementNum-1) = 1;
    *LL_int(&(compiler->allCodeWords), compiler->allCodeWords.elementNum) = 1;
    
    
        // read in each token along with its bytecode expansion
    
    for (loopCmd = 0; loopCmd < numCommands; loopCmd++)  {
        
        const char *tokenChar = (char *) commandTokens[loopCmd].cmdString;
        ccInt firstToken = loopToken+1, LHarg = -1, RHarg = -1, firstRtrnType = -1, numJumps = 0, aOrtrn, numArgs = 0;
        bool expectLHarg = false, optionalLHarg;
        bool tobeRemoved = ( (*commandTokens[loopCmd].rtrnTypeString == *removedexpression)
                            || (*commandTokens[loopCmd].translation == *removedexpression) );
        
        
            // first we read the text description of an operation (usually a string interspersed with
            // 'typeNarg's, but may be something like 'int_constant', in which case we just mark the token number)
            
            // case 1:  it's a pre-defined object:  a constant, user-defined variable or argument adapter
        
        if ((*tokenChar >= *type0arg_adapter) && (*tokenChar <= *variable_name))  {
            
            loopToken++;
            if (addElements(&(compiler->tokenSpecs), 1, true) != passed)  errOut(out_of_memory_err, loopCmd+1)
            
            if (*tokenChar >= *int_constant)  {
                compiler->specialTokens[(ccInt) ((*tokenChar)-(*int_constant))] = loopToken;
                nextChar(&tokenChar);        }
            
            numArgs = 1;
            tokenSpec(loopToken)->leftArgType = -1;
            if (*tokenChar <= *noarg_adapter)  tokenSpec(loopToken)->rightArgType = -1;
            else  tokenSpec(loopToken)->rightArgType = constDataArgType;
            tokenSpec(loopToken)->precedence = numOoOs+1;        }
        
        
            // case 2:  it's an ordinary series of tokens and arguments -- read those until we run out
        
        else  {
            
                // first read in the left-hand argument type, if there is one
            
            if ((*tokenChar >= *type0arg) && (*tokenChar <= *type9arg))  {
                LHarg = ((ccInt) *tokenChar) - ((ccInt) *type0arg);
                if (LHarg == (ccInt) (*chararg - *type0arg))  errOut(char_read_err, loopCmd+1)
                if (LHarg == (ccInt) (*stringarg - *type0arg))  errOut(string_read_err, loopCmd+1)
                numArgs = 1;
                expectLHarg = true;
                do  {  tokenChar++;  }
                while ((lettertype(tokenChar) == unprintable) || (lettertype(tokenChar) == a_space));  }
            
            
                // removable expressions (comments) can happen irrespective of whether there is a left-hand argument
            
            if (tobeRemoved)  optionalLHarg = true;
            else  optionalLHarg = false;
            
            
                // now read in each token/argument type
            
            while (*tokenChar != 0)  {
                
                bool isOptional = (*tokenChar == *optionalargs);
                
                while ((lettertype(tokenChar) == a_space) || (lettertype(tokenChar) == unprintable))  tokenChar++;
                if (*tokenChar == 0)  break;
                
                loopToken++;
                if (addElements(&(compiler->tokenSpecs), 1, true) != passed)  errOut(out_of_memory_err, loopCmd+1)
                
                aOrtrn = addTokenSpec(compiler, &tokenChar, &(tokenSpec(loopToken)->name), loopToken, false,
                                        expectLHarg, optionalLHarg, loopToken-firstToken, 0);
                if (aOrtrn != passed)  errOut(aOrtrn, loopCmd+1)
                
                
                    // if there's a right-hand argument (or an argument in between tokens), mark its type
                
                if ((*tokenChar >= *type0arg) && (*tokenChar <= *commentarg))  {
                    RHarg = ((ccInt) *tokenChar) - ((ccInt) *type0arg);
                    numArgs++;
                    tokenChar++;        }
                else  RHarg = -1;
                
                
                    // store the token information and move on
                
                tokenSpec(loopToken)->isOptional = isOptional;
                tokenSpec(loopToken)->tobeRemoved = tobeRemoved;
                tokenSpec(loopToken)->leftArgType = LHarg;
                tokenSpec(loopToken)->rightArgType = RHarg;
                if (loopToken != firstToken)  {
                    tokenSpec(loopToken)->prevToken = loopToken-1;
                    tokenSpec(loopToken-1)->nextToken = loopToken;   }
                
                LHarg = RHarg;
            }
            
            
                // now store info about the command as a whole -- we use the first token for storing this information
                // first store the precedence level of the command
            
            if ((tokenSpec(firstToken)->leftArgType == -1) && (tokenSpec(loopToken)->rightArgType == -1))  {
                tokenSpec(firstToken)->precedence = numOoOs+2;        }
            else  {
                tokenSpec(firstToken)->precedence = commandTokens[loopCmd].precedence;
                
                if ((commandTokens[loopCmd].precedence < 1) || (commandTokens[loopCmd].precedence > numOoOs))  {
                    errOut(out_of_range_err, loopCmd+1)
        }   }   }
        
        
            // a character or string has to be flanked by opening and closing tokens
        
        if (RHarg == (ccInt) (*chararg - *type0arg))  errOut(char_read_err, loopCmd+1)
        if (RHarg == (ccInt) (*stringarg - *type0arg))  errOut(string_read_err, loopCmd+1)
        
        
            // read in the return types of the command
        
        oneChar = (char *) commandTokens[loopCmd].rtrnTypeString;
        
        if (*oneChar == *argXtype)  {
            tokenSpec(firstToken)->rtrnTypes[10] = true;       }
        else  {
        while (*oneChar != 0)  {
            if ((*oneChar >= '0') && (*oneChar <= '9'))  {
                tokenSpec(firstToken)->rtrnTypes[(*oneChar)-'0'] = true;
                if (firstRtrnType == -1)  firstRtrnType = (*oneChar)-'0';
                if ((*tokenChar >= *type0arg_adapter) && (*tokenChar <= *noarg_adapter))  {
                    compiler->adapters[(*tokenChar)-(*type0arg_adapter)][(*oneChar)-'0'] = firstToken;
            }   }
            
            oneChar++;
        }}
        
        
            // parse the string that stores the command's bytecode
        
        tokenSpec(firstToken)->firstCodeWord = compiler->allCodeWords.elementNum+1;
        
        
        for (loopJump = 0; loopJump < 9; loopJump++)  {  jumpMarkers[loopJump] = jumpTo[loopJump] = 0;  }
        
        
            // case #1:  the string is raw bytecode (i.e. it has an 'inbytecode' character at the beginning)
        
        oneChar = (char *) commandTokens[loopCmd].translation;
        if (*oneChar == *inbytecode)  {
            oneChar++;
            while (*oneChar != 0)  {
                while ( (*oneChar == ' ') || (*oneChar == '\t') ||
                        ((lettertype(oneChar) == unprintable) && (*oneChar != anonymousmember[1])) )  {
                    oneChar++;      }
                
                if (*oneChar != 0)  {
                    
                    ccInt intNum = 0, wordsToAdd = 1, wordPosition, wordType = -1;          // an ordinary command word
                    const char *holdNumPos = NULL;
                    
                    if (*oneChar == anonymousmember[1])  {
                        oneChar++;
                        wordType = 4;
                        wordsToAdd = 2;         }
                    
                    else  {
                        if ((*oneChar == 'A') || (*oneChar == 'a'))  wordType = 1;        // an argument
                        else if ((*oneChar == 'P') || (*oneChar == 'p'))  wordType = 2;   // a code position marker
                        else if ((*oneChar == 'J') || (*oneChar == 'j'))  wordType = 3;   // a jump offset
                        
                        if (wordType != -1)  {
                            oneChar++;
                            wordsToAdd = 3;     }
                        
                        holdNumPos = oneChar;
                        if (readNum(&oneChar, &theNum, &ifFloat) != passed)
                            errOut(read_number_err, loopCmd+1)
                        else if ((ifFloat) || ((wordType != -1) && ((theNum < 1) || (theNum > 9))))
                            errOut(out_of_range_err, loopCmd+1)
                        intNum = (ccInt) theNum;
                        
                        if ((wordType == 1) && (intNum > numArgs))
                            errOut(out_of_range_err, loopCmd+1)       }
                    
                    if ((wordType == -1) && (intNum == 0))  {  wordType = 0;  wordsToAdd = 2;  }
                    if (wordType == 2)  {
                        numJumps++;
                        jumpMarkers[intNum-1] = 1;
                        if (numJumps > 9)  {
                            errOut(out_of_range_err, loopCmd+1)
                    }   }
                    
                    if (wordType == 3)  jumpTo[intNum-1] = 1;
                    
                    wordPosition = compiler->allCodeWords.elementNum+1;
                    if (addElements(&(compiler->allCodeWords), wordsToAdd, false) != passed)  errOut(out_of_memory_err, loopCmd+1)
                    
                    if (wordType == -1)  {
                        *LL_int(&(compiler->allCodeWords), wordPosition) = intNum;
                        if (intNum == 0)  *LL_int(&(compiler->allCodeWords), wordPosition+1) = 0;      }
                    else  {
                        *LL_int(&(compiler->allCodeWords), wordPosition) = 0;
                        *LL_int(&(compiler->allCodeWords), wordPosition+1) = wordType;
                        if (wordsToAdd == 3)  {
                            *LL_int(&(compiler->allCodeWords), wordPosition+2) = intNum;
        }   }  }   }   }
        
        
            // case #2:  the string is a macro command -- compile it using the commands we've already encountered
        
        else if (!tobeRemoved)  {
            tokenSpec(firstToken)->numCodeWords = 0;        // temporary patch, to ensure we can compile the macro string
            
            for (loopArgCmd = *arg1; loopArgCmd <= *arg9; loopArgCmd++)  lettertypeArray[loopArgCmd] = a_symbol;  
            *rtrn = tokenize(compiler, oneChar, firstRtrnType);
            for (loopArgCmd = *arg1; loopArgCmd <= *arg9; loopArgCmd++)  lettertypeArray[loopArgCmd] = unprintable;  
            
            if (*rtrn == passed)  *rtrn = reorderTokens(compiler, firstRtrnType, false);
            if (*rtrn == passed)  *rtrn = addElements(&(compiler->allCodeWords), compiler->bytecode.elementNum, false);
            if (*rtrn != passed)  errOut(*rtrn, loopCmd+1)
            
            copyElements(&(compiler->bytecode), 1,
                    &(compiler->allCodeWords), tokenSpec(firstToken)->firstCodeWord, compiler->bytecode.elementNum);
        }
        
        for (loopJump = 0; loopJump < 9; loopJump++)  {
        if ((jumpMarkers[loopJump] == 0) && (jumpTo[loopJump] != 0))  {
            errOut(out_of_range_err, loopCmd+1)
        }}
        
        tokenSpec(firstToken)->numCodeWords = compiler->allCodeWords.elementNum - tokenSpec(firstToken)->firstCodeWord + 1;
    }
    
    
        // defragment all the linked lists to make the parsing faster, and count number of operators
    
    if ((defragmentLinkedList(&(compiler->allCodeWords)) != passed)
                || (defragmentLinkedList(&(compiler->tokenSpecs)) != passed))  errOut(out_of_memory_err, 0)
    
    for (loopList = 0; loopList < 128; loopList++)  {
    if (defragmentLinkedList(compiler->tokens + loopList) != passed)  {
        errOut(out_of_memory_err, 0)
    }}
    
    return compiler;
}

#undef errOut



// addTokenSpec() adds a token string or variable name to the compiler's list of known strings,
// and makes a copy of the string to store

ccInt addTokenSpec(compiler_type *compiler, const char **tokenCharPtr, char **newName, ccInt tokenID, bool isVariableName,
        bool expectLHarg, bool optionalLHarg, ccInt nthToken, ccInt varID)
{
    linkedlist *theList = compiler->tokens + ((ccInt) **tokenCharPtr);
    ccInt whichElement, ifFound, LLrtrn, numChars = 1;
    char *tokenStringCopy = NULL, *tokenStringCopy2;
    const char *tokenChar = *tokenCharPtr;
    tokenStringType *oneTokenString;
    
    
        // calculate the length of the token string, so we know how much memory to allocate
    
    while ( (*tokenChar != 0) && ((*tokenChar < *type0arg) || (*tokenChar > *commentarg)) )  {
        if ((isVariableName) && (lettertype(tokenChar) != a_letter) && (lettertype(tokenChar) != a_number))  break;
        if (lettertype(tokenChar) != unprintable)  numChars++;
        tokenChar++;    }
    
    
        // allocate memory for our copy of the string
    
    tokenStringCopy = (char *) malloc(numChars);
    if (tokenStringCopy == NULL)  {
        freeCompiler(compiler);
        return out_of_memory_err;        }
    
    
        // finally, copy the string into our own memory
    
    tokenChar = *tokenCharPtr;
    tokenStringCopy2 = tokenStringCopy;
    while ( (*tokenChar != 0) && ((*tokenChar < *type0arg) || (*tokenChar > *commentarg)) )  {
        if ((isVariableName) && (lettertype(tokenChar) != a_letter) && (lettertype(tokenChar) != a_number))  break;
        if (lettertype(tokenChar) != unprintable)  {
            *tokenStringCopy2 = *tokenChar;
            tokenStringCopy2++;     }
        tokenChar++;     }
    *tokenStringCopy2 = 0;     // append a null even if there's an argument command
    
    
        // now we need to add the pointer to the appropriate tokens list
        // findToken() returns the alphabetical location in the list where we need to insert this pointer
    
    tokenStringCopy2 = tokenStringCopy;
    ifFound = findToken(compiler, theList, (const char **) &tokenStringCopy2, &whichElement, true, isVariableName);
    
    if (!ifFound)  {
        if (!isVariableName)  compiler->numLanguageOps[(ccInt) **tokenCharPtr]++;
        LLrtrn = insertElements(theList, whichElement, 1, true);
        if (LLrtrn != passed)  {
            freeCompiler(compiler);
            return LLrtrn;
    }   }
    
    oneTokenString = (tokenStringType *) element(theList, whichElement);
    
    if (!ifFound)  {
        oneTokenString->name = tokenStringCopy;
        if (isVariableName)  {
            varNameType *newVarNameEntry;
            LLrtrn = addElements(&(compiler->varNames), 1, false);
            if (LLrtrn != passed)  {  freeCompiler(compiler);  return LLrtrn;  }
            
            newVarNameEntry = (varNameType *) element(&(compiler->varNames), compiler->varNames.elementNum);
            newVarNameEntry->theName = tokenStringCopy;
            newVarNameEntry->nameLength = numChars-1;
    }   }
    else  free(tokenStringCopy);       // if we already have it, then never mind
    
    if (nthToken == 0)  {
        if ((!expectLHarg) || (optionalLHarg))  oneTokenString->tokenID_noLHarg = tokenID;
        if ((expectLHarg) || (optionalLHarg))  oneTokenString->tokenID_LHarg = tokenID;        }
    
    oneTokenString->varID = varID;
    
    *tokenCharPtr = tokenChar;
    
    if (newName != NULL)  *newName = oneTokenString->name;
    
    return passed;
}


// findToken() finds the matching token in a linked list of token symbols
// *whichElement is the matching index; if no match was found it is the index where the token string can be inserted
//
// use 2 different search strategies
// * operators -- we're not sure how long of an token to look for (e.g. first token of ': = myvar' could be ':' or ': =')
//    so we exhaustively search the list from the beginning until we are alphabetically past where the token can be
// * words -- we know where the end of the word is (it's a non-letter), so can use a binary search

ccInt findToken(compiler_type *compiler, linkedlist *tokenStringList, const char **scriptStringPtr, ccInt *whichElement,
        bool endWithNull, bool maybeVariableName)
{
    ccInt loopElement, searchBottom = 1, searchTop = tokenStringList->elementNum;
    ccInt numLanguageOps = compiler->numLanguageOps[(ccInt) **scriptStringPtr], directionToAdjust;
    const char *scriptChar, *storedChar, *scriptStringStart = *scriptStringPtr;
    bool isWord = false, comparingLanguageOp = true, maybeFinishedScriptToken, finishedScriptToken, finishedStoredToken;
    
    if (searchTop == 0)  {  *whichElement = 1;  return false;  }
    
    
        // loop over all tokens in the current list
    
    *whichElement = 0;
    do  {
        scriptChar = scriptStringStart;
        
        if (searchBottom > numLanguageOps)  {
            if ((comparingLanguageOp) && (*whichElement > 0))  break;       // interpret it as a language op if possible
            comparingLanguageOp = false;      }
        
        if (comparingLanguageOp)  loopElement = searchBottom;               // first do an enumerative search over all built-in operators,
        else  loopElement = (searchBottom+searchTop)/2;                     // then a binary search over the stored variable names
        
        
            // try to match all of the characters of this particular token
        
        storedChar = ((tokenStringType *) element(tokenStringList, loopElement))->name;
        while (*storedChar != 0)  {
            
            isWord = false;
            if (*storedChar != ' ')  {
                if ((lettertype(scriptChar) == a_letter) || (lettertype(scriptChar) == a_number))  isWord = true;
                if (*scriptChar != *storedChar)  break;
                
                nextChar(&scriptChar);
                storedChar++;       }
            
            else  {                         // spaces get special treatment
                if ((*(storedChar+1) == ' ') && (*scriptChar != *storedChar))  break;
                else  {
                    while (*storedChar == ' ')  storedChar++;
                    while ((*scriptChar == ' ') || (*scriptChar == '\t') || (lettertype(scriptChar) == unprintable))  {
                        scriptChar++;
        }   }   }   }
        
        
            // figure out if we have to move up (directionToAdjust = 1), down (directionToAdjust = -1), or if we have a match
        
        if ( ((isWord) && ((lettertype(scriptChar) == a_letter) || (lettertype(scriptChar) == a_number)))
                    || ((endWithNull) && (lettertype(scriptChar) != a_null)) )  {
            finishedScriptToken = maybeFinishedScriptToken = false;       }
        else  {
            finishedScriptToken = (!comparingLanguageOp);
            maybeFinishedScriptToken = true;            }
        finishedStoredToken = (*storedChar == 0);
        
        if ((maybeFinishedScriptToken) && (finishedStoredToken))  directionToAdjust = 0;
        else if (finishedScriptToken)  directionToAdjust = -1;
        else if (finishedStoredToken) directionToAdjust = 1;
        else  {
            while (lettertype(storedChar) == a_space)  nextChar(&storedChar);       // operator spaces confuse the alphabetization
            while (lettertype(scriptChar) == a_space)  scriptChar++;
            
            if (*scriptChar < *storedChar)  directionToAdjust = -1;
            else  directionToAdjust = 1;            }
        
        
            // adjust searchBottom/searchTop accordingly
        
        if (directionToAdjust == -1)  {
            if (!comparingLanguageOp)  searchTop = loopElement-1;
            else if ((!maybeVariableName) || (!(*whichElement == 0)))  break;
            else  searchBottom = numLanguageOps+1;      }
        
        else if (directionToAdjust == 0)  {
            *whichElement = loopElement;
            *scriptStringPtr = scriptChar;
            if (!comparingLanguageOp)  break;
            else  searchBottom++;           }
        
        else  searchBottom = loopElement+1;
        
    }  while (searchBottom <= searchTop);
    
    
        // if the string wasn't found, return the position where a new entry can be inserted
    
    if (*whichElement == 0)  {
        *whichElement = searchBottom;
        return  false;       }
    
    else  return true;
}


// freeCompiler() frees the memory allocated for a compiler

void freeCompiler(compiler_type *compiler)
{
    ccInt loopList, loopTokenString;
    
    deleteLinkedList(&(compiler->tokenSpecs));
    deleteLinkedList(&(compiler->allCodeWords));
    
    free((void *) compiler->OoOdirections);
    
    for (loopList = 0; loopList < 128; loopList++)  {
        linkedlist *theList = compiler->tokens + loopList;
        if (theList->memory != NULL)  {
        for (loopTokenString = 1; loopTokenString <= theList->elementNum; loopTokenString++)  {
            free((void *) (((tokenStringType *) element(theList, loopTokenString))->name));
        }}
        deleteLinkedList(theList);       }
    
    freeBytecode(compiler);
    
    deleteLinkedList(&(compiler->scriptTokens));
    deleteLinkedList(&(compiler->bytecode));
    deleteLinkedList(&(compiler->opCharNum));
    deleteLinkedList(&(compiler->varNames));
    free((void *) compiler);
}


void freeBytecode(compiler_type *compiler)
{
    ccInt loopOldToken;
    
    for (loopOldToken = 1; loopOldToken <= compiler->scriptTokens.elementNum; loopOldToken++)  {
    if (scriptToken(loopOldToken)->constData != NULL)  {
        free((void *) scriptToken(loopOldToken)->constData);
    }}
    
    resizeLinkedList(&(compiler->scriptTokens), 0, false);
    resizeLinkedList(&(compiler->bytecode), 0, false);
    resizeLinkedList(&(compiler->opCharNum), 0, false);
}



// Compile():  translates a script (text) into bytecode.  Expects a null character at the end (eof).
// This routine calls all the subsequent functions in this file.

ccInt compile(compiler_type *compiler, const char *programString)
{
    ccInt rtrn;
    
    freeBytecode(compiler);
    compilerWarning = 0;
    
    rtrn = tokenize(compiler, programString, 0);
    if (rtrn == passed)  rtrn = reorderTokens(compiler, 0, true);
    
    return rtrn;
}


// tokenize() converts ASCII text into tokens (these may include variable names and inlined constants)

#define errOut(b) {  errPosition = tokenErrPosition;  deleteLinkedList(&nextTokenStack);  return b;  }
#define tokenStackElement(a) ((tokenStackElementType *) element(&nextTokenStack, a))

ccInt lastScriptChar;

ccInt tokenize(compiler_type *compiler, const char *charPtr, ccInt scriptType)
{
    linkedlist nextTokenStack;
    ccInt prevTokenType = 0, theTokenType = 0, tokenErrPosition = 1, prevErrPosition, rtrn;
    ccInt nextRightArgType = scriptType, lastRightArgType = scriptType, loopSkippedToken, expectedArgType;
    ccFloat theNum;
    const char *tokenCharStartPtr = charPtr, *scriptStartPtr = charPtr;
    bool expectLHarg = false, ifFloat;
    tokenSpecType *theTokenSpec = NULL;
    
    typedef struct {
        char *name;
        ccInt tokenID;
        ccInt prevScriptToken;    } tokenStackElementType;
    
    resizeLinkedList(&(compiler->scriptTokens), 0, false);
    
    while (lettertype(charPtr) == unprintable)  charPtr++;
    
    rtrn = newLinkedList(&nextTokenStack, 0, sizeof(tokenStackElementType), 1., false);
    if (rtrn != passed)  return rtrn;
    
    while (*charPtr != 0)  {
        
        ccInt constDataSize = 0, *constData = NULL, otherTokenType = 0, varID = 0;
        bool isNumber = false;
        
        prevTokenType = theTokenType;
        prevErrPosition = tokenErrPosition;
        
        theTokenType = 0;
        
            // read to the end of the longest token that matches our starting point
            // theTokenType > 0 means it's a known operator; theTokenType < 0 means that it's a user-defined variable
            // if the token wasn't found, then theTokenType == 0
        
        while ((lettertype(charPtr) == a_space) || (lettertype(charPtr) == unprintable))  charPtr++;
        if (*charPtr == 0)  break;
        
        tokenCharStartPtr = charPtr;
        tokenErrPosition = (ccInt) (tokenCharStartPtr-scriptStartPtr+1);
        
        
            // if it's a numeric constant, read it and store the number
        
        if ( ((*charPtr >= '0') && (*charPtr <= '9')) || (*charPtr == '-') || (*charPtr == '.')) {
            
            rtrn = readNum(&charPtr, &theNum, &ifFloat);
            if ((rtrn == passed) || (rtrn == overflow_err) || (rtrn == underflow_err))  {
                if (rtrn != passed)  {  compilerWarning = rtrn;  errPosition = tokenErrPosition;  }
                
                isNumber = true;
                
                if (expectLHarg)  charPtr = tokenCharStartPtr;
                else  {
                    if ((!ifFloat) && (compiler->specialTokens[0] == -1))  ifFloat = true;
                    
                    if ((!ifFloat) && (compiler->specialTokens[0] > -1))  {
                        theTokenType = compiler->specialTokens[0];
                        constDataSize = 1;
                        constData = (ccInt *) malloc(constDataSize*sizeof(ccInt));
                        if (constData == NULL)  errOut(out_of_memory_err)
                        *constData = (ccInt) theNum;          }
                    
                    else if ((ifFloat) && (compiler->specialTokens[1] > -1))  {
                        theTokenType = compiler->specialTokens[1];
                        constDataSize = (ccInt) ceil(1.*sizeof(ccFloat)/sizeof(ccInt));
                        constData = (ccInt *) malloc(constDataSize*sizeof(ccInt));
                        if (constData == NULL)  errOut(out_of_memory_err)
                        *((ccFloat *) constData) = theNum;
        }   }  }    }
        
        
            // if we're reading a macro-command and it's an argument, store the arg-command and argument number
        
        else if ((*charPtr >= *arg1) && (*charPtr <= *arg9))  {
            
            theTokenType = 3;
            
            constData = (ccInt *) malloc(sizeof(ccInt));
            if (constData == NULL)  errOut(out_of_memory_err)
            *((ccInt *) constData) = (*charPtr) - (*arg1) + 1;
            constDataSize = 1;
            
            charPtr++;          }
        
        
            // otherwise we assume it's an operator and use the token tables to find out which one it is
        
        if (theTokenType == 0)  {
            
            linkedlist *theList = compiler->tokens + ((ccInt) *charPtr);
            ccInt tokenListElement, ifFound;
            tokenStringType *theTokenString = NULL;
            bool followingToken = false;
            
            ifFound = findToken(compiler, theList, &charPtr, &tokenListElement, false, true);
            if (ifFound)  {
                
                ccInt loopNextToken, nextTokenID = 0;
                
                theTokenString = (tokenStringType *) element(theList, tokenListElement);
                if ((theTokenString->tokenID_LHarg == 0) && (theTokenString->tokenID_noLHarg == 0))  followingToken = true;
                varID = theTokenString->varID;
                
                
                    // it might be a 'next' token we are expecting ('in' after 'for' for example)...
                    
                    // if we skipped an optional argument or two, mark their nextScriptToken fields as 0
                
                for (loopNextToken = nextTokenStack.elementNum; loopNextToken >= 1; loopNextToken--)  {
                    if ( (theTokenString->name == tokenStackElement(loopNextToken)->name) && ((nextTokenID == 0)
                                    || (tokenSpec(tokenStackElement(loopNextToken)->tokenID)->isOptional == false)) )  {
                        nextTokenID = loopNextToken;
                        for (loopSkippedToken = loopNextToken+1; loopSkippedToken <= nextTokenStack.elementNum; loopSkippedToken++)  {
                            ccInt pST = tokenStackElement(loopSkippedToken)->prevScriptToken;
                            if (pST > 0)  scriptToken(pST)->nextScriptToken = 0;
                    }   }
                    if (tokenSpec(tokenStackElement(loopNextToken)->tokenID)->isOptional == false)  break;         }
                
                if (nextTokenID != 0)  {
                    theTokenType = tokenStackElement(nextTokenID)->tokenID;
                    deleteElements(&nextTokenStack, nextTokenID, nextTokenStack.elementNum);      }
                
                
                    // otherwise, it's probably a stored token with or without a left-hand argument
                
                else if (!expectLHarg)  {
                    theTokenType = theTokenString->tokenID_noLHarg;   }
                else  {
                    theTokenType = theTokenString->tokenID_LHarg;   }
                
                if (theTokenType > 0)  theTokenSpec = tokenSpec(theTokenType);
                
                
                    // if we expect a token without, and only have a token with, a left argument,
                    // then try to fix this problem with a nothing-adapter
                    
                    // case 1:  we hit the end of a block and expect an argument (such as '(a +)' or '{x =}')
                
                if ((!expectLHarg) && (nextTokenID != 0) && (theTokenSpec->leftArgType <= (*typeXarg-*type0arg)))  {
                    
                    expectedArgType = theTokenSpec->leftArgType;
                    if (prevTokenType > 0)  {
                    if (tokenSpec(prevTokenType)->precedence > theTokenSpec->precedence)  {
                        expectedArgType = tokenSpec(prevTokenType)->rightArgType;
                    }}
                    
                    if (expectedArgType != constDataArgType)  {
                    if (compiler->adapters[10][expectedArgType] != 0)  {
                        rtrn = addScriptToken(compiler, compiler->adapters[10][expectedArgType], tokenErrPosition, NULL, 0);
                        if (rtrn != passed)  errOut(rtrn)
                }}  }
                
                    // case 2:  we aren't expecting a left-hand argument and we only have an operator that takes one (e.g. '+2')
                
                else if ((!expectLHarg) && (theTokenType == 0) && (!followingToken))  {
                    
                    otherTokenType = theTokenString->tokenID_LHarg;
                    expectedArgType = tokenSpec(otherTokenType)->leftArgType;
                    if (prevTokenType > 0)  {
                    if (tokenSpec(prevTokenType)->precedence > tokenSpec(otherTokenType)->precedence)  {
                        expectedArgType = tokenSpec(prevTokenType)->rightArgType;
                    }}
                    
                    if (expectedArgType != constDataArgType)  {
                    if (compiler->adapters[10][expectedArgType] != 0)  {
                        rtrn = addScriptToken(compiler, compiler->adapters[10][expectedArgType], tokenErrPosition, NULL, 0);
                        if (rtrn != passed)  errOut(rtrn)
                        theTokenType = otherTokenType;
                        theTokenSpec = tokenSpec(theTokenType);
                        expectLHarg = !expectLHarg;
            }   }   }}
            
            
            if (theTokenType > 0)  {
            if (theTokenSpec->nextToken != 0)  {
                rtrn = addElements(&nextTokenStack, 1, false);
                if (rtrn != passed)  errOut(rtrn)
                tokenStackElement(nextTokenStack.elementNum)->name = tokenSpec(theTokenSpec->nextToken)->name;
                tokenStackElement(nextTokenStack.elementNum)->tokenID = theTokenSpec->nextToken;
                if (theTokenSpec->tobeRemoved)  {
                    tokenStackElement(nextTokenStack.elementNum)->prevScriptToken = 0;  }
                else  {
                    tokenStackElement(nextTokenStack.elementNum)->prevScriptToken = compiler->scriptTokens.elementNum+1;
            }}  }
            
            
                // alternatively, add a new variable name if it's a word that we haven't seen before
            
            else  {
                
                if ((!ifFound) && (lettertype(tokenCharStartPtr) == a_letter) && (compiler->specialTokens[2] > -1))  {
                    
                    compiler->numVariables++;
                    varID = compiler->numVariables;
                    
                    charPtr = tokenCharStartPtr;
                    rtrn = addTokenSpec(compiler, &charPtr, NULL, compiler->specialTokens[2], true, expectLHarg, false, 0, varID);
                    if (rtrn != passed)  errOut(rtrn)
                    
                    theTokenType = compiler->specialTokens[2];
                    theTokenSpec = tokenSpec(theTokenType);             }
                
                
                    // if it's none of the above throw some sort of error
                
                else  {
                    if (isNumber)  errOut(no_left_arg_allowed_err)
                    else if (!ifFound)  errOut(unknown_command_err)
                    else if (followingToken)  errOut(unexpected_token_err)
                    
                    if ((prevTokenType > 0) && (otherTokenType != 0))  {
                    if (tokenSpec(prevTokenType)->precedence > tokenSpec(otherTokenType)->precedence)  {
                        tokenErrPosition = prevErrPosition;
                        if (expectLHarg)  errOut(no_right_arg_allowed_err)
                        else  errOut(right_arg_expected_err);           }}
                    
                    if (expectLHarg)  errOut(no_left_arg_allowed_err)
                    else  errOut(left_arg_expected_err);
        }   }   }
        
        else  theTokenSpec = tokenSpec(theTokenType);
        
        if (nextRightArgType != (*commentarg - *type0arg))  lastRightArgType = nextRightArgType;
        nextRightArgType = theTokenSpec->rightArgType;
        
        
            // if it's a variable, make room for the variable ID
        
        if (varID > 0)  {
            constDataSize = 1;
            constData = (ccInt *) malloc(constDataSize*sizeof(ccInt));
            if (constData == NULL)  errOut(out_of_memory_err)
            *((ccInt *) constData) = varID;         }
        
        
            // if it's a string constant, read in the string
        
        else if ((nextRightArgType == (ccInt) (*chararg - *type0arg)) || (nextRightArgType == (ccInt) (*stringarg - *type0arg)))  {
            
            char *closingStr = tokenSpec(theTokenSpec->nextToken)->name;
            
            rtrn = readTextString(&charPtr, closingStr, &constData, &constDataSize, nextRightArgType == (ccInt) (*chararg - *type0arg));
            if (rtrn != passed)  errOut(rtrn)        }
        
        
            // store the token if it's a known symbol, or a known variable name ending at the last allowable name character,
            // and expand constants as separate tokens (as they appear as arguments in the bytecode string)
        
        if (!theTokenSpec->tobeRemoved)  {
            rtrn = addScriptToken(compiler, theTokenType, tokenErrPosition, NULL, 0);
            if (rtrn != passed)  errOut(rtrn)
            if (constData != NULL)  {
                rtrn = addScriptToken(compiler, 1, tokenErrPosition, constData, constDataSize);
                if (rtrn != passed)  errOut(rtrn)
        }   }
        
        
            // deal with the right-hand argument of the token, if it has one;
            // if it's a comment argument, skip over it
        
        if (nextRightArgType == (ccInt) (*commentarg - *type0arg))  {
            
            const char *holdCharPtr;
            tokenSpecType *LHtokenSpec = theTokenSpec;
            
            
                // if this comment is contributing to the bytecode (i.e. via its flanking arguments),
                // represent the comment literal with a blank token
            
            if (!theTokenSpec->tobeRemoved)  {
                rtrn = addScriptToken(compiler, 1, tokenErrPosition, NULL, 0);
                if (rtrn != passed)  errOut(rtrn)       }
            
            theTokenType = theTokenSpec->nextToken;
            theTokenSpec = tokenSpec(theTokenType);
            
            holdCharPtr = charPtr;
            if (theTokenType >= 0)  charPtr = strstr(charPtr, theTokenSpec->name);
            if (charPtr == NULL)  {
                charPtr = holdCharPtr;
                while (*charPtr != 0)  charPtr++;   }
            
            
                // we don't expect a left-hand argument if this comment has flanking arguments (think 'type0arg | ... \n type0arg')
            
            else if ((LHtokenSpec->leftArgType != -1) || (theTokenSpec->rightArgType != -1))  {
                expectLHarg = false;
        }   }
        
        
            // move on to the next token; don't update expectLHarg if we're passing over a comment
        
        else if (theTokenSpec->leftArgType != (ccInt) (*commentarg - *type0arg))  {
            if ((nextRightArgType == -1) || (nextRightArgType == constDataArgType))  expectLHarg = true;
            else  expectLHarg = false;        }
        
        else  nextRightArgType = lastRightArgType;                  // in case this is the last thing we read
    }
    
    lastScriptChar = (ccInt) (charPtr-scriptStartPtr+1);
    
    
        // we should not be missing an argument at the end of the code block
    
    for (loopSkippedToken = nextTokenStack.elementNum; loopSkippedToken >= 1; loopSkippedToken--)  {
    if (tokenSpec(tokenStackElement(loopSkippedToken)->tokenID)->isOptional == true)  {
        ccInt pST = tokenStackElement(loopSkippedToken)->prevScriptToken;
        if (pST > 0)  scriptToken(pST)->nextScriptToken = 0;
        deleteElement(&nextTokenStack, loopSkippedToken);
    }}
    
    if ((!expectLHarg) || (nextTokenStack.elementNum > 0))  {
        
        if (compiler->scriptTokens.elementNum == 0)  rtrn = arg_expected_err;
        else if (!expectLHarg)  rtrn = right_arg_expected_err;
        else  rtrn = token_expected_err;
        
        
            // if the last operator was a comment, look back at what the previous operator wanted
        
        if (nextRightArgType != *commentarg - *type0arg)  expectedArgType = nextRightArgType;
        else  expectedArgType = tokenSpec(prevTokenType)->rightArgType;
        
        if (expectedArgType != constDataArgType)  {
        if (compiler->adapters[10][expectedArgType] != 0)  {
            rtrn = addScriptToken(compiler, compiler->adapters[10][expectedArgType], tokenErrPosition, NULL, 0);   }}
        
        if (rtrn != passed)  {
            if (rtrn == token_expected_err)  {
                tokenErrPosition = lastScriptChar;
                expectedTokenName = tokenStackElement(nextTokenStack.elementNum)->name;        }
            errOut(rtrn)
    }   }
    
    deleteLinkedList(&nextTokenStack);
    
    return passed;
}

#undef errOut
#undef tokenStackElement



// addScriptToken() adds one token to the compiler's scriptTokens linked list

ccInt addScriptToken(compiler_type *compiler, ccInt tokenID, ccInt tokenCharNum, ccInt *constData, ccInt constDataSize)
{
    scriptTokenType *oneScriptToken;
    
    ccInt rtrn = addElements(&(compiler->scriptTokens), 1, true);
    if (rtrn != passed)  return rtrn;
    
    oneScriptToken = scriptToken(compiler->scriptTokens.elementNum);
    
    oneScriptToken->tokenID = tokenID;
    oneScriptToken->rtrnTypes = tokenSpec(tokenID)->rtrnTypes;
    oneScriptToken->tokenCharNum = tokenCharNum;
    oneScriptToken->constData = constData;
    oneScriptToken->constDataSize = constDataSize;
    if (tokenSpec(tokenID)->nextToken != 0)  oneScriptToken->nextScriptToken = -1;
    else  oneScriptToken->nextScriptToken = 0;
    if (tokenID != 2)  oneScriptToken->adapterTokenToFix = 0;
    else  oneScriptToken->adapterTokenToFix = compiler->scriptTokens.elementNum;
    
    return passed;
}



// readNum() reads in a number (type ccFloat, but if_float is set if it could be read as an integer).

const ccInt numBufferSize = 999;
char numBuffer[numBufferSize+1];

ccInt readNum(const char **charPtr, ccFloat *returnedNum, bool *ifFloat)
{
    ccInt intnum, intnumsread, doublenumsread, cc;
    int intcharsread, doublecharsread;
    ccFloat doublenum;
    const char *newCharPtr, *numStart;
    
    while ((lettertype(*charPtr) == a_space) || (lettertype(*charPtr) == unprintable))  (*charPtr)++;
    numStart = *charPtr;
    
    for (cc = 0; cc < numBufferSize; cc++)  {       // super stupid -- sscanf() will copy the whole string up to \x00 into a file stream,
        const char *oneChar = numStart + cc;              // so let's make sure it only copies the number we're reading
        if ((lettertype(oneChar) == a_space) || (lettertype(oneChar) == a_eol) || (lettertype(oneChar) == a_null) || (*oneChar == ','))  break;
        numBuffer[cc] = *oneChar;       }
    
    if (cc < numBufferSize)  {
        numBuffer[cc] = '\x00';
        numStart = numBuffer;       }
    
    doublenumsread = sscanf(numStart, readFloatFormatString, &doublenum, &doublecharsread);
    if (doublenumsread == 1)  {
        
        if (ifFloat != NULL)  {
            intnumsread = sscanf(numStart, readIntFormatString, &intnum, &intcharsread);
            *ifFloat = ((intnumsread != 1) || (intcharsread != doublecharsread) || (doublenum != (ccFloat) intnum));    }
        
        errno = 0;
        doublenum = strtod(numStart, (char **) &newCharPtr);      // reread to check for over/underflow
        *returnedNum = doublenum;
        *charPtr += doublecharsread;
        
        if (errno == ERANGE)  {
            if ((doublenum > 1) || (doublenum < -1))  return overflow_err;
            else  return underflow_err;     }
        
        return passed;      }
    
    return read_number_err;
}


// readTextString() compiles a string enclosed in quotes (or, if CodeLLPtr == NULL, just skips over it).
// Doesn't write the constant_string code word -- that is done by GetOperation().

ccInt readTextString(const char **scanPtr, char *closingString, ccInt **stringBuffer, ccInt *stringSizeInInts, bool isChar)
{
    ccInt chars_num = 0, numPriorSlashes;
    const char *startPtr = *scanPtr, *scanBackPtr, *endPtr;
    char *charPtr;
    
    
        // First, count the number of characters in the string.
    
    endPtr = startPtr-1;
    do  {
        endPtr = strstr(endPtr+1, closingString);
        if (endPtr == NULL)  {
            errPosition = 1;
            return string_read_err;     }
        
        numPriorSlashes = 0;
        scanBackPtr = endPtr-1;
        while (*scanBackPtr == '\\')  {  numPriorSlashes++;  scanBackPtr--;  }
    }  while (numPriorSlashes % 2 == 1);
    
    while (*scanPtr < endPtr)   {
        if (**scanPtr == '\\')  {      // deal with special characters
            nextChar(scanPtr);
            if ((lettertype(*scanPtr) == a_number) ||
                        ((**scanPtr >= 'A') && (**scanPtr <= 'F')) || ((**scanPtr >= 'a') && (**scanPtr <= 'f')))   {
                nextChar(scanPtr);
                nextChar(scanPtr);    }
            else if ((**scanPtr == '\\') || (**scanPtr == 'r') || (**scanPtr == 'n') || (**scanPtr == 't')
                                            || (**scanPtr == closingString[0]))
                nextChar(scanPtr);
            else    {
                errPosition = (ccInt) (*scanPtr-startPtr) + 1;
                return string_read_err;   }    }
        else    {
            nextChar(scanPtr);    }
        chars_num++;    }
    
    if ((isChar) && (chars_num != 1))  return char_read_err;
    
    
        // get a buffer to store the string in
    
    *stringSizeInInts = ceil(1.*chars_num/sizeof(ccInt));    // align along a ccInt boundary
    if (!isChar)  (*stringSizeInInts)++;
    *stringBuffer = (ccInt *) malloc((size_t) (*stringSizeInInts)*sizeof(ccInt));
    if (*stringBuffer == NULL)   {
        errPosition = (ccInt) (*scanPtr-startPtr) + 1;
        return out_of_memory_err;     }
    
    if (!isChar)  **stringBuffer = chars_num;
    (*stringBuffer)[(*stringSizeInInts)-1] = 0;         // in case the final int isn't completely overwritten
    
    charPtr = ((char *) *stringBuffer);
    if (!isChar)  charPtr += sizeof(ccInt);
    *scanPtr = startPtr;
    
    
        // Now re-read the string, this time storing the characters in the buffer as we go.
    
    while (*scanPtr < endPtr)   {
        if (**scanPtr == '\\')  {
            nextChar(scanPtr);
            
                // general special character:  \xy where x, y are hexadecimal
            
            if ((lettertype(*scanPtr) == a_number) ||
                        ((**scanPtr >= 'A') && (**scanPtr <= 'F')) || ((**scanPtr >= 'a') && (**scanPtr <= 'f')))   {
                
                if (lettertype(*scanPtr) == a_number)  *charPtr = **scanPtr - '0';
                else if ((**scanPtr >= 'A') && (**scanPtr <= 'F'))  *charPtr = **scanPtr - 'A' + 10;
                else if ((**scanPtr >= 'a') && (**scanPtr <= 'f'))  *charPtr = **scanPtr - 'a' + 10;
                else  {
                    errPosition = (ccInt) (*scanPtr - startPtr) + 1;
                    free((void *) *stringBuffer);
                    return string_read_err;         }
                
                *charPtr *= 16;
                nextChar(scanPtr);
                
                if (lettertype(*scanPtr) == a_number)  *charPtr += **scanPtr - '0';
                else if ((**scanPtr >= 'A') && (**scanPtr <= 'F'))  *charPtr += **scanPtr - 'A' + 10;
                else if ((**scanPtr >= 'a') && (**scanPtr <= 'f'))  *charPtr += **scanPtr - 'a' + 10;
                else  {
                    errPosition = (ccInt) (*scanPtr - startPtr) + 1;
                    free((void *) *stringBuffer);
                    return string_read_err;     }     }
            
            else if (**scanPtr == '\\')     // backslash
                    *charPtr = '\\';
            else if (**scanPtr == 'n')      // end-of-line
                    *charPtr = '\n';
            else if (**scanPtr == 'r')      // carriage return
                    *charPtr = '\r';
            else if (**scanPtr == 't')      // tab
                    *charPtr = '\t';
            else if (**scanPtr == closingString[0])      // quote
                    *charPtr = closingString[0];        }
        
        else    {
            *charPtr = **scanPtr;    }       // if it's a normal character, just copy it over
        
        nextChar(scanPtr);
        charPtr++;    }
    
    
        // characters are just stored as ints -- do the conversion
    
    if (isChar)  (*stringBuffer)[0] = (ccInt) *(charPtr-1);
    
    return passed;
}


// nextChar() skips to the next character; ignores unprintable ones.
// Used for parsing strings when we don't have to worry about comments.

void nextChar(const char **charPtr)
{
    if (lettertype(*charPtr) != a_null) do  {
        (*charPtr)++;
    } while (lettertype(*charPtr) == unprintable);
}


// reorderTokens() puts the tokens into expressions by writing the loosest-bound tokens first

ccInt reorderTokens(compiler_type *compiler, ccInt expressionType, bool replaceCommandWords)
{
    ccInt firstCmd, *lastOrderedToken = &firstCmd, firstToken = 1, rtrn;
    ccInt dummyLastTokenToFix;
    bool dummyRtrnTypes[11], *dummyRtrnTypePtr = &(dummyRtrnTypes[0]);       // this extra step prevents a warning message
    
    rtrn = defragmentLinkedList(&(compiler->scriptTokens));
    if (rtrn != passed)  return rtrn;
    
    resizeLinkedList(&(compiler->bytecode), 0, false);
    resizeLinkedList(&(compiler->opCharNum), 0, false);
    
    rtrn = relinkExpression(compiler, &firstToken, compiler->scriptTokens.elementNum, 0, expressionType, &lastOrderedToken,
        &dummyRtrnTypePtr, &dummyLastTokenToFix);
    if (rtrn != passed)  return rtrn;
    *lastOrderedToken = 0;         // so the reorderer knows it's finished here
    
    rtrn = writeTokenOps(compiler, firstCmd, replaceCommandWords);
    if (rtrn == passed)  rtrn = defragmentLinkedList(&(compiler->bytecode));
    
    return rtrn;
}


// relinkExpression() reorders the tokens in one 'phrase'
// This routine makes a list of the tokens in a given 'block', where operator precedence has to be used to figure out their order
// The actual reordering is done by relinkBestToken()

ccInt relinkExpression(compiler_type *compiler, ccInt *loopToken, ccInt topToken, ccInt endTokenType, ccInt expectedType,
        ccInt **lastOrderedToken, bool **lastRtrnTypes, ccInt *lastTokenToFix)
{
    ccInt rtrn, oneTokenType = 0, *enclosedArgs;
    tokenSpecType *oneSpec;
    linkedlist OoOblock;
    
    rtrn = newLinkedList(&OoOblock, 0, sizeof(ccInt), 2., false);
    if (rtrn != passed)  return rtrn;
    
    
        // loop over all tokens in our current block
    
    while (*loopToken <= topToken)  {
        
        scriptTokenType *oneScriptToken = scriptToken(*loopToken);
        oneTokenType = oneScriptToken->tokenID;
        if (oneTokenType == endTokenType)  break;
        
        
            // decide whether we've hit the end of a sentence or sentence block
        
        oneSpec = tokenSpec(oneTokenType);
        if (oneSpec->prevToken != 0)  {
            errPosition = oneScriptToken->tokenCharNum;
            return unexpected_token_err;       }
        
        
            // jot down the token for future reordering
        
        rtrn = addElements(&OoOblock, 1, false);
        if (rtrn != passed)  return rtrn;
        *LL_int(&OoOblock, OoOblock.elementNum) = *loopToken;
        
        
            // if the token expects a nextToken, recursively reorder everything from here to nextToken.
            // Multiple nextTokens are chained together here via their nextExpression fields
        
        enclosedArgs = &(oneScriptToken->enclosedArgs);
        
        while (scriptToken(*loopToken)->nextScriptToken != 0)  {
            
            ccInt previousScriptToken = *loopToken;
            
            (*loopToken)++;
            
            rtrn = relinkExpression(compiler, loopToken, topToken, oneSpec->nextToken, oneSpec->rightArgType,
                        &enclosedArgs, &(oneScriptToken->rtrnTypes), &(oneScriptToken->adapterTokenToFix));
            if (rtrn != passed)  {
                if ((rtrn == arg_expected_err) && (errPosition == 0))  errPosition = scriptToken(*loopToken)->tokenCharNum;
                return rtrn;        }
            
            scriptToken(previousScriptToken)->nextScriptToken = *loopToken;
            
            oneTokenType = scriptToken(*loopToken)->tokenID;
            oneSpec = tokenSpec(oneTokenType);        }
        
        
            // advance to the next token
        
        (*loopToken)++;
    }
    
    
        // if there's an ending token we're expecting, we'd better have ended with it
    
    if ((endTokenType != 0) && (oneTokenType != endTokenType))  {
        errPosition = lastScriptChar;
        expectedTokenName = tokenSpec(endTokenType)->name;
        return token_expected_err;       }
    
    
        // after we've recorded all the tokens in this expression, we do the reordering
    
    rtrn = relinkBestToken(compiler, &OoOblock, 1, OoOblock.elementNum, expectedType, lastOrderedToken, lastRtrnTypes, lastTokenToFix);
    
    deleteLinkedList(&OoOblock);
    
    return rtrn;
}


// relinkBestToken() finds the highest-precedence token in a 'block', and links that to the previous/next operations
// The OoOblock is a list of the token indexes in an expression that are being considered for reordering
// (i.e. this list skips over tokens that are in sub-expressions flanked by operators contained in OoOblock).
// Called recursively for the sub-blocks before and after the highest-precedence token

ccInt relinkBestToken(compiler_type *compiler, linkedlist *OoOblock, ccInt firstToken, ccInt lastToken, ccInt expectedType,
        ccInt **lastOrderedToken, bool **lastRtrnTypes, ccInt *lastTokenToFix)
{
    ccInt lowOoO = 0, bestToken = 0, lowOoOpos = 0, blockOpCounter, tokenNum = 0, rtrn;       // inits to suppress warning
    ccInt innerToken, adapter = 0, tokenToFix, *argPtr, loopArgType;
    bool **rtrnTypes = NULL;
    tokenSpecType *oneSpec, *bestSpec = NULL;
    scriptTokenType *oneScriptToken;
    
    
        // find the lowest-precedence token
    
    if (lastToken < firstToken)  {
        errPosition = 0;                        // temporary -- we will update errPosition later
        return arg_expected_err;        }
    
    for (blockOpCounter = firstToken; blockOpCounter <= lastToken; blockOpCounter++)  {
        
        tokenNum = *LL_int(OoOblock, blockOpCounter);
        oneScriptToken = scriptToken(tokenNum);
        oneSpec = tokenSpec(oneScriptToken->tokenID);
        
        if ((blockOpCounter == firstToken) || (oneSpec->precedence <= lowOoO))  {
            if ((blockOpCounter == firstToken) || ((oneSpec->leftArgType != -1)
                        && ((blockOpCounter == lastToken) || (oneSpec->rightArgType != -1))
                        && ((compiler->OoOdirections[oneSpec->precedence-1] == l_to_r) || (oneSpec->precedence < lowOoO))) )  {
                bestToken = tokenNum;
                bestSpec = oneSpec;
                rtrnTypes = &(oneScriptToken->rtrnTypes);
                lowOoO = oneSpec->precedence;
                lowOoOpos = blockOpCounter;         }
            
            else if (lowOoO == 1)  break;     // speeds up sentence-parsing (\n is highest-precedence)
    }   }
    
    
        // In rare cases we may have a type-X no-argument adapter whose type needs setting (if possible)
        // (it might be either this operator or else an enclosed operator)
        // 
        // 2 possibilities:  either we know what type we are expecting, in which case we 'fix' the adapter
        // type (or throw an error if we don't have an adapter of the appropriate type);
        // or else we don't know because it is an enclosed argument, so we feed the token # that needs fixing
        // back to the enclosing operator.
    
    tokenToFix = scriptToken(bestToken)->adapterTokenToFix;
    if (tokenToFix != 0)  {
        if (expectedType == 10)  *lastTokenToFix = tokenToFix;
        else if (compiler->adapters[10][expectedType] != 0)  {
            scriptToken(tokenToFix)->adapterTokenToFix = compiler->adapters[10][expectedType];
            (*rtrnTypes)[expectedType] = true;     }
        else  {
            errPosition = scriptToken(tokenToFix)->tokenCharNum;
            return arg_expected_err;
    }   }
    
    
        // Check to see if the best token is not of a type that we expected;
        // if possible, add an adapter to the argument.
        //
        // Note:  the adapters are all added at the END of the unreordered bytecode
        // so as not to disrupt the positions of earlier tokens which may already be linked.
    
    if (expectedType < (ccInt) (*typeXarg - *type0arg))  {
    if (!((*rtrnTypes)[expectedType]))  {
        
        for (loopArgType = 0; loopArgType < 10; loopArgType++)  {
        if (((*rtrnTypes)[loopArgType]) && (compiler->adapters[loopArgType][expectedType] != 0))  {
            
            rtrn = addScriptToken(compiler, compiler->adapters[loopArgType][expectedType], scriptToken(bestToken)->tokenCharNum, NULL, 0);
            if (rtrn != passed)  return rtrn;
            
            adapter = compiler->scriptTokens.elementNum;
            **lastOrderedToken = adapter;
            *lastOrderedToken = &(scriptToken(adapter)->firstArg);
            
            break;
        }}
        
            // otherwise throw a type-mismatch error
        
        if (adapter == 0)  {
            errPosition = scriptToken(bestToken)->tokenCharNum;
            return type_mismatch_err;
    }}  }
    
    
        // build the link to bestToken from some previous token
        // (lastOrderedToken is either the firstArg or nextExpression field of that prior token)
    
    **lastOrderedToken = bestToken;
    
    
        // now set bestToken's firstArg field to point to the lowest-OoO operator of its first argument
        // 
        // this low-OoO token's nextExpression field will point to the second argument of bestToken;
        // then that token's nextExpression will point to the third argument; etc.
        // 
        // the last argument's nextExpression field (or firstArg, if there are no arguments) is set to zero
    
    argPtr = &(scriptToken(bestToken)->firstArg);
    *lastOrderedToken = argPtr;
    **lastOrderedToken = 0;
    
    
        // link the left-hand argument
    
    if (bestSpec->leftArgType != -1)  {
        rtrn = relinkBestToken(compiler, OoOblock, firstToken, lowOoOpos-1, bestSpec->leftArgType, lastOrderedToken,
            rtrnTypes, &(scriptToken(bestToken)->adapterTokenToFix));
        if (rtrn != passed)  {
            if ((rtrn == arg_expected_err) && (errPosition == 0))  {
                rtrn = left_arg_expected_err;
                errPosition = scriptToken(bestToken)->tokenCharNum;     }
            return rtrn;
    }   }
    else if (lowOoOpos != firstToken)  {
        errPosition = scriptToken(bestToken)->tokenCharNum;
        return no_left_arg_allowed_err;     }
    
    
        // link any enclosed arguments -- these have already been chained via their nextExpression fields
    
    if (scriptToken(bestToken)->nextScriptToken != 0)  {
        
        ccInt tokenInExpression = bestToken;
        
        innerToken = scriptToken(bestToken)->enclosedArgs;
        **lastOrderedToken = innerToken;
        while (scriptToken(innerToken)->nextExpression != 0)  innerToken = scriptToken(innerToken)->nextExpression;
        *lastOrderedToken = &(scriptToken(innerToken)->nextExpression);
        
        
            // update bestSpec to get our final right-hand argument
        
        while (scriptToken(tokenInExpression)->nextScriptToken != 0)  {
            tokenInExpression = scriptToken(tokenInExpression)->nextScriptToken;
            bestSpec = tokenSpec(bestSpec->nextToken);
    }   }
    
    
        // link the right-hand argument
    
    if (bestSpec->rightArgType != -1)  {
        rtrn = relinkBestToken(compiler, OoOblock, lowOoOpos+1, lastToken, bestSpec->rightArgType, lastOrderedToken,
            rtrnTypes, &(scriptToken(bestToken)->adapterTokenToFix));
        if (rtrn != passed)  {
            if ((rtrn == arg_expected_err) && (errPosition == 0))  {
                rtrn = right_arg_expected_err;
                errPosition = scriptToken(bestToken)->tokenCharNum;     }
            return rtrn;
    }   }
    else if (lowOoOpos != lastToken)  {
        errPosition = scriptToken(bestToken)->tokenCharNum;
        return no_right_arg_allowed_err;     }
    
    
        // for C function calls, replace varIDs with Cfunction IDs
    
    if (expectedType == (ccInt) (*Cfunctionarg - *type0arg))  {
        scriptTokenType *dataToken = scriptToken(*argPtr);
        varNameType *tokenName = ((varNameType *) element(&(compiler->varNames), *dataToken->constData));
        const char *tokenNameStr = tokenName->theName, *compareStr;
        ccInt cf, cc;
        
        for (cf = 0; cf < numCfunctions; cf++)  {
            if (cf < numIBCfunctions)  compareStr = inbuiltCFs[cf].functionName;
            else  compareStr = userCFs[cf-numIBCfunctions].functionName;
            for (cc = 0; cc < tokenName->nameLength; cc++)  {
                if (tokenNameStr[cc] != compareStr[cc])  break;    }
            if (cc == tokenName->nameLength)  {
            if ((compareStr[cc] == ':') || (compareStr[cc] == 0))  {
                if (cf < numIBCfunctions)  *dataToken->constData = -(cf+1);
                else  *dataToken->constData = (cf-numIBCfunctions+1);
                break;
    }   }   }}
    
    
        // finally, prepare to link to the token of the next argument bestToken
        // if this token is an argument of another token, then this will be the next argument of that token
        // the linking is done one level up, so all we have to do is set the pointer here
    
    if (adapter == 0)  argPtr = &(scriptToken(bestToken)->nextExpression);
    else  argPtr = &(scriptToken(adapter)->nextExpression);
    
    *lastOrderedToken = argPtr;
    **lastOrderedToken = 0;
    
    
        // if the enclosing operator is, say, a ( .. ), feed the return type of this operator on back
    
    if (((*lastRtrnTypes)[10]) && (expectedType == 10))  {
        *lastRtrnTypes = scriptToken(bestToken)->rtrnTypes;       }
    
    return passed;
}


// writeTokenOps() writes the bytecode operators for a given token,
// including its arguments (so it calls itself recursively)

ccInt writeTokenOps(compiler_type *compiler, ccInt theToken, bool replaceCommandWords)
{
    ccInt nextOpPosition, loopToken, loopJump, rtrn;
    ccInt tokenCharNum = scriptToken(theToken)->tokenCharNum;
    ccInt jumpMarkers[9], jumps[9], jumpTo[9], numJumps = 0;
    ccInt theTokenID = scriptToken(theToken)->tokenID;
    tokenSpecType *theTokenSpec = tokenSpec(theTokenID);
    
    
        // transcribe the bytecode words of our operator
    
    loopToken = 1;
    while (loopToken <= theTokenSpec->numCodeWords)  {
        
        ccInt *toWrite = LL_int(&(compiler->allCodeWords), theTokenSpec->firstCodeWord + loopToken-1);
        ccInt loopWrite, numTimesToWrite = 1;
        bool regularCommand = true;
        
        nextOpPosition = compiler->bytecode.elementNum + 1;
        
        
            // handle special bytecode commands in the code word sequence
        
        if (toWrite[0] == 0)  {
        if ((replaceCommandWords) || (toWrite[1] <= 1))  {
            
            regularCommand = false;
            
            
                // maybe we just wanted to write command #0 -- this is represented in bytecode by (0, 0)
            
            if (toWrite[1] == 0)  {
                
                if ((!replaceCommandWords) && (theTokenID != 3))  numTimesToWrite = 2;
                
                loopToken++;
                regularCommand = true;        }
            
            
                // if we are to write an argument of the operator, we call this function for the n'th argument
                // command is (0, 1, arg #)
            
            else if (toWrite[1] == 1)  {
                
                ccInt theArg, numArgsAhead;
                
                loopToken += 3;
                
                theArg = scriptToken(theToken)->firstArg;
                numArgsAhead = toWrite[2]-1;
                
                while (numArgsAhead > 0)  {
                    theArg = scriptToken(theArg)->nextExpression;
                    if (theArg == 0)  break;
                    numArgsAhead--;     }
                
                if (theArg != 0)  {
                    rtrn = writeTokenOps(compiler, theArg, replaceCommandWords);
                    if (rtrn != passed)  return rtrn;
            }   }
            
                // the code sequence for a jump-position marker is (0, 2, marker #)
            
            else if (toWrite[1] == 2)  {
                
                jumpMarkers[toWrite[2]-1] = nextOpPosition;
                
                loopToken += 3;            }
            
            
                // a jump command is represented by (0, 3, marker #)
            
            else if (toWrite[1] == 3)  {
                
                rtrn = addElements(&(compiler->bytecode), 1, false);
                if (rtrn == passed)  rtrn = addElements(&(compiler->opCharNum), 1, false);
                if (rtrn != passed)  return rtrn;
                
                *LL_int(&(compiler->opCharNum), nextOpPosition) = tokenCharNum;
                
                jumpTo[numJumps] = toWrite[2];
                jumps[numJumps] = nextOpPosition;
                numJumps++;
                
                loopToken += 3;     }
            
            
                // a new hidden variable is represented by (0, 4)
            
            else if (toWrite[1] == 4)  {
                
                compiler->anonymousMemberNum--;
                toWrite = &(compiler->anonymousMemberNum);
                regularCommand = true;
                
                loopToken++;
        }}  }
        
        
            // if it's a regular bytecode command, just copy it into the bytecode array
        
        if (regularCommand)  {
            loopToken++;
            for (loopWrite = 0; loopWrite < numTimesToWrite; loopWrite++)  {
                rtrn = addElements(&(compiler->bytecode), 1, false);
                if (rtrn == passed)  rtrn = addElements(&(compiler->opCharNum), 1, false);
                if (rtrn != passed)  return rtrn;
                
                *LL_int(&(compiler->bytecode), nextOpPosition+loopWrite) = *toWrite;
                *LL_int(&(compiler->opCharNum), nextOpPosition+loopWrite) = tokenCharNum;
    }   }   }
    
    
        // fill in all the jump offsets after the bytecode has been transcribed
    
    for (loopJump = 0; loopJump < numJumps; loopJump++)  {
        *LL_int(&(compiler->bytecode), jumps[loopJump]) = jumpMarkers[jumpTo[loopJump]-1] - jumps[loopJump];    }
    
    
        // next, copy any constant data into the bytecode
    
    if (scriptToken(theToken)->constData != NULL)  {
        
        ccInt loopInt, numInts = scriptToken(theToken)->constDataSize;
        
        nextOpPosition = compiler->bytecode.elementNum + 1;
        
        rtrn = addElements(&(compiler->bytecode), numInts, false);
        if (rtrn != passed)  return rtrn;
        
        setElements(&(compiler->bytecode), nextOpPosition, nextOpPosition+numInts-1,
                        (void *) scriptToken(theToken)->constData);
        
        if (!replaceCommandWords)  {
        for (loopInt = 0; loopInt < numInts; loopInt++)  {
        if (*LL_int(&(compiler->bytecode), nextOpPosition+loopInt) == 0)  {
            rtrn = insertElements(&(compiler->bytecode), nextOpPosition+loopInt+1, numInts, true);
            if (rtrn != passed)  return rtrn;
            numInts++;
            loopInt++;      // get past the new 0
        }}}
        
        rtrn = addElements(&(compiler->opCharNum), numInts, false);
        if (rtrn != passed)  return rtrn;
        
        for (loopInt = 1; loopInt <= numInts; loopInt++)  {
            *LL_int(&(compiler->opCharNum), nextOpPosition+loopInt-1) = tokenCharNum;
    }   }
    
    return passed;
}

#undef tokenSpec
#undef scriptToken

#undef constDataArgType


ccInt lettertypeArray[256] = {
    a_null, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable,
    unprintable, a_space, a_eol, unprintable, unprintable, unprintable, unprintable, unprintable,
    unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, //16
    unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable,
    a_space, a_symbol, a_symbol, a_symbol, a_symbol, a_symbol, a_symbol, a_symbol, //32
    a_symbol, a_symbol, a_symbol, a_symbol, a_symbol, a_symbol, a_symbol, a_symbol,
    a_number, a_number, a_number, a_number, a_number, a_number, a_number, a_number, //48
    a_number, a_number, a_symbol, a_symbol, a_symbol, a_symbol, a_symbol, a_symbol,
    a_symbol, a_letter, a_letter, a_letter, a_letter, a_letter, a_letter, a_letter, //64
    a_letter, a_letter, a_letter, a_letter, a_letter, a_letter, a_letter, a_letter,
    a_letter, a_letter, a_letter, a_letter, a_letter, a_letter, a_letter, a_letter, //80
    a_letter, a_letter, a_letter, a_symbol, a_symbol, a_symbol, a_symbol, a_letter,
    a_symbol, a_letter, a_letter, a_letter, a_letter, a_letter, a_letter, a_letter, //96
    a_letter, a_letter, a_letter, a_letter, a_letter, a_letter, a_letter, a_letter,
    a_letter, a_letter, a_letter, a_letter, a_letter, a_letter, a_letter, a_letter, //112
    a_letter, a_letter, a_letter, a_symbol, a_symbol, a_symbol, a_symbol, unprintable,
    unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, //128
    unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable,
    unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable,
    unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable,
    unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable,
    unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable,
    unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable,
    unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable,
    unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable,
    unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable,
    unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable,
    unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable,
    unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable,
    unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable,
    unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable,
    unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable, unprintable      };
