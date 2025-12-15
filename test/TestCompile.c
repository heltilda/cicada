#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "cmpile.h"
#include "bytecd.h"
#include "cclang.h"

#ifndef INFINITY
#define INFINITY (DBL_MAX+DBL_MAX)
#endif
#ifndef NAN
#define NAN (INFINITY-INFINITY)
#endif


void test_lettertype()
{
    char charArray[256];
    ccInt counter;
    bool OK;
    
    for (counter = 0; counter <= 255; counter++)    {
        ((unsigned char *) charArray)[counter] = (unsigned char) counter;        }
        
    for (counter = 0; counter <= 255; counter++)    {
        OK = true;
        if (counter == 0)    {
            if (lettertype(charArray+counter) != a_null) OK = false;        }
        else if (counter == '\n')        {
            if (lettertype(charArray+counter) != a_eol) OK = false;        }
        else if ((counter == ' ') || (counter == '\t'))        {
            if (lettertype(charArray+counter) != a_space) OK = false;        }
        else if ((counter < 32) || (counter >= 127))        {
            if (lettertype(charArray+counter) != unprintable) OK = false;    }
        else if ((counter >= '0') && (counter <= '9'))        {
            if (lettertype(charArray+counter) != a_number) OK = false;    }
        else if (((counter >= 'a') && (counter <= 'z')) || ((counter >= 'A') && (counter <= 'Z')) || (counter == '_'))        {
            if (lettertype(charArray+counter) != a_letter) OK = false;    }
        else        {
            if (lettertype(charArray+counter) != a_symbol) OK = false;    }
        if (!OK)
            printf("lettertype: Failed on counter = %i (%c)\n", (int) counter, (char) counter);    }
    
    return;
}


void test_newCompiler()
{
    commandTokenType tokens1[] = { { "*", 1, "0#9aA", inbytecode "" }, { "+", 2, "", inbytecode "19" },
                        { "*=", 1, "", inbytecode "" }, { "+ =", 2, "", inbytecode " \t" }, { "*\x17  +", 3, "", inbytecode "\t9" },
                        { "f  " type1arg "  in" type2arg, 4, "", inbytecode "7 a1 8 A2  p1 P3 j1 8 J3 " },
                        { "|", 1, "", inbytecode "" }, { "|*", 1, "", inbytecode "" }, { "*|", 1, "", inbytecode "" } };
    commandTokenType tokens2[] = { { type1arg " *", 1, "", inbytecode "a1 19 29 120 P2" },
                { "*", 1, "", inbytecode "p1 P2 j2" }, { "*", 1, "", inbytecode "182" } };
    commandTokenType tokens3[] = { { type1arg " *", 1, "", inbytecode "z2" } };
    commandTokenType tokens4[] = { { type1arg " *", 1, "", inbytecode "-z" } };
    commandTokenType tokens5[] = { { type1arg " *", 1, "", inbytecode "1.3" } };
    commandTokenType tokens6[] = { { type1arg " *", 0, "", inbytecode "1" } };
    commandTokenType tokens7[] = { { type1arg " *" type7arg, 1, "", inbytecode "1 a3" } };
    commandTokenType tokens8[] = { { " *" type7arg, 1, "", inbytecode "1 a2" } };
    commandTokenType tokens9[] = { { " q", 1, "", inbytecode "j1 59 p2" } };
    commandTokenType tokens10[] = { { type1arg "*" commentarg "(" type1arg , 1, "1", inbytecode "a3 a1" } };
    ccInt tokenSize1 = sizeof(tokens1)/sizeof(commandTokenType), tokenSize2 = sizeof(tokens2)/sizeof(commandTokenType);
    ccInt tokenSize3 = sizeof(tokens3)/sizeof(commandTokenType), tokenSize4 = sizeof(tokens4)/sizeof(commandTokenType);
    ccInt tokenSize5 = sizeof(tokens5)/sizeof(commandTokenType), tokenSize6 = sizeof(tokens6)/sizeof(commandTokenType);
    ccInt tokenSize7 = sizeof(tokens7)/sizeof(commandTokenType), tokenSize8 = sizeof(tokens8)/sizeof(commandTokenType);
    ccInt tokenSize9 = sizeof(tokens7)/sizeof(commandTokenType), tokenSize10 = sizeof(tokens10)/sizeof(commandTokenType);
    ccInt OoOdirections1[] = { l_to_r, l_to_r, r_to_l, l_to_r }, OoOsize1 = sizeof(OoOdirections1)/sizeof(ccInt);
    ccInt OoOdirections2[] = { r_to_l, -23, l_to_r }, OoOsize2 = sizeof(OoOdirections2)/sizeof(ccInt);
    ccInt OoOdirections3[] = { l_to_r, r_to_l, l_to_r }, OoOsize3 = sizeof(OoOdirections3)/sizeof(ccInt);
    
    struct {
        commandTokenType *tokens;
        ccInt numTokens;
        ccInt *OoOdirections;
        ccInt numOoOs;
        ccInt rtrn;
        ccInt rtrnPosition;
    } compilerInits[] = {
        { tokens1, tokenSize1, OoOdirections1, OoOsize1, passed, 0 },
        { tokens2, tokenSize2, OoOdirections1, OoOsize1, passed, 0 },
        { tokens3, tokenSize3, OoOdirections1, OoOsize1, read_number_err, 1 },
        { tokens4, tokenSize4, OoOdirections1, OoOsize1, read_number_err, 1 },
        { tokens5, tokenSize5, OoOdirections1, OoOsize1, out_of_range_err, 1 },
        { tokens6, tokenSize6, OoOdirections1, OoOsize1, out_of_range_err, 1 },
        { tokens2, tokenSize2, OoOdirections2, OoOsize2, out_of_range_err, -2 },
        { tokens1, tokenSize1, OoOdirections3, OoOsize3, out_of_range_err, 6 },
        { tokens7, tokenSize7, OoOdirections1, OoOsize1, out_of_range_err, 1 },
        { tokens8, tokenSize8, OoOdirections1, OoOsize1, out_of_range_err, 1 },
        { tokens9, tokenSize9, OoOdirections1, OoOsize1, out_of_range_err, 1 },
        { tokens10, tokenSize10, OoOdirections1, OoOsize1, passed, 0 }
    };
    ccInt cI, rtrn;
    
    for (cI = 0; cI < sizeof(compilerInits)/sizeof(compilerInits[0]); cI++)  {
        errPosition = 0;
        newCompiler(compilerInits[cI].tokens, compilerInits[cI].numTokens, compilerInits[cI].OoOdirections, compilerInits[cI].numOoOs, &rtrn);
        if (rtrn != compilerInits[cI].rtrn)  {
            printf("test_newCompiler():  wrong return value on cI = %i (%i; should be %i)\n", (int) cI, (int) rtrn, (int) compilerInits[cI].rtrn);    }
        if (errPosition != compilerInits[cI].rtrnPosition)  {
            printf("test_newCompiler():  wrong error position on cI = %i (%i; should be %i)\n", (int) cI, (int) errPosition, (int) compilerInits[cI].rtrnPosition);
    }   }
}



void test_readNum()
{
    #define ReadNum_NumTests 34
    
    char *testStrings[ReadNum_NumTests] = {
            "52 ", "5 3t", " 46", "5\x00o", "53t",
                "4.5 ", "5.2.7", "3..2", "4.k", "5. ",
            "0xA3 ", "0x5F1 ", "0xG1 ", "A5 ", "0x9.4 ",
                "-5 ", "-0xA ", "0x-A ", "-5.02 ", "--2 ",
            "4E2 ", "4e23456789 ", "4e-23456789 ", "0x-.CH2 ", "0x03h- ",
                "5e\x00 ", "5\x01q ", "-\x00 ", "-4e-\x00 ", "3e-03\x03k ",
            "-e6 ", "inf ", "-INFINITY ", "nan "      };
    
    char testEndChars[ReadNum_NumTests] = {
            ' ', ' ', '\0', '\x00', 't',
                ' ', '.', '.', 'k', ' ',
            ' ', ' ', 'x', 'A', ' ',
                ' ', ' ', 'x', ' ', '-',
            ' ', ' ', ' ', 'x', 'h',
                'e', '\x01', '-', 'e', '\x03',
            '-', ' ', ' ', ' '     };
    
    ccFloat testNums[ReadNum_NumTests] = {
            52, 5, 46, 5, 53,
                4.5, 5.2, 3, 4, 5,
            163, 1280+240+1, 0, 0, 9.25,
                -5, -10, 0, -5.02, 0,
            400, 0, 0, 0, 3,
                5, 5, 0, -4, .003,
            0, INFINITY, -INFINITY, NAN       };
    
    char testErrors[ReadNum_NumTests] = {
            passed, passed, passed, passed, passed,
                passed, passed, passed, passed, passed,
            passed, passed, passed, read_number_err, passed,
                passed, passed, passed, passed, read_number_err,
            passed, overflow_err, underflow_err, passed, passed,
                passed, passed, read_number_err, passed, passed,
            read_number_err, passed, passed, passed        };
    
    ccInt counter, read_err;
    const char *thePtr;
    ccFloat theNum;
    bool dummy;
    
    for (counter = 0; counter < ReadNum_NumTests; counter++)    {
        thePtr = testStrings[counter];
        read_err = readNum(&thePtr, &theNum, &dummy);
        if (*thePtr != testEndChars[counter])
            printf("readNum:  Failed on counter = %i; end chars: '%i'; should be '%c'\n", (int) counter, (int) *thePtr, testEndChars[counter]);
        if ((theNum != testNums[counter]) && (!((theNum != theNum) && (testNums[counter] != testNums[counter]))) && (read_err == passed))
            printf("readNum:  Failed on counter = %i; numbers: '%f'; should be '%f'\n", (int) counter, theNum, testNums[counter]);
        if (read_err != testErrors[counter])
            printf("readNum:  Failed on counter = %i; errors: '%i'; should be '%i'\n", (int) counter, (int) read_err, (int) testErrors[counter]);        }
    
    return;
}


void test_nextChar()
{
    char *testStrings[] = {
            "k\2\4to", "763", "\0t2", "y\0z", ".\n,",
            "q\t5", "o54", "\n53", "\6\3,2", "\6- ",
            "\n5", "\0dd " };
    
    char testAnswers[] = {
            't', '6', '\0', '\0', '\n',
            '\t', '5', '5', ',', '-',
            '5', '\0' };
    
    ccInt counter;
    const char *charPtr;
    
    for (counter = 0; counter < sizeof(testStrings)/sizeof(char *); counter++)    {
        charPtr = testStrings[counter];
        nextChar(&charPtr);
        if (*charPtr != testAnswers[counter])
            printf("ncclib_char:  Failed on counter = %i; answers: '%c' / '%c'\n", (int) counter, *charPtr, testAnswers[counter]);        }
    
    return;
}



void test_readTextString()
{
    #define readTextString_NumTests 12
    
    char *testStrings[readTextString_NumTests] = {
                        "\"Brian\"\1", "\" me \"\1", "\"8* 87\"\1", "\"]v|\2\3\0|-v1%*\"\1", "\"\n\"\1",
                        "\"\"\1", "\"Me\\\\\"\1", "\"Me\\\"\"\1", "\"Me\\n\"\1", "\"Me\\t\"\1",
                        "\"\\6\"\1", "\"\\\0\"\1"       };
    
    char *compareStrings[readTextString_NumTests] = {
                        "Brian", " me ", "8* 87", "]v|\2\3", "\n",
                        "", "Me\\", "Me\42", "Me\n", "Me\t",
                        "", ""      };
    
    
    const ccInt error_size = -2*((ccInt) sizeof(ccInt)) + 1;
    
    ccInt StringLengths[readTextString_NumTests] = {5, 4, 5, error_size, 1,
                        0, 3, 3, 3, 3,
                        error_size, error_size  };
    
    ccInt errCodes[readTextString_NumTests] = {passed, passed, passed, string_read_err, passed,
                    passed, passed, passed, passed, passed,
                    string_read_err, string_read_err};
    
    ccInt *tempMem, rtrn, testCounter, charCounter, numInts;
    const char *wPtr, *tempPtr1, *tempPtr2;
    
    for (testCounter = 0; testCounter < readTextString_NumTests; testCounter++)    {
        
        wPtr = testStrings[testCounter]+1;
        rtrn = readTextString(&wPtr, "\"", &tempMem, &numInts, false);
        if (errCodes[testCounter] != rtrn)  {
            printf("readTextString: (%i)  errs %i / %i\n", (int) testCounter, (int) rtrn, (int) errCodes[testCounter]);    }
        
        if (rtrn == passed)  {
            
            if ((StringLengths[testCounter]+sizeof(ccInt)-1)/sizeof(ccInt) + 1 != numInts)
                printf("readTextString: (%i)  elnum (%i %i)\n", (int) testCounter, (int) StringLengths[testCounter], (int) numInts);
            
            if (*tempMem != StringLengths[testCounter])
                printf("readTextString: (%i)  LLcharsnum (%i %i)\n", (int) testCounter, (int) *tempMem, (int) StringLengths[testCounter]);
            if (*(wPtr+1) != '\1')    printf("readTextString: (%i)  wPtr '%c' (%i)\n", (int) testCounter, *wPtr, (int) *wPtr);
            
            tempPtr1 = compareStrings[testCounter];
            for (charCounter = 1; charCounter <= StringLengths[testCounter]; charCounter++)    {
                tempPtr2 = (char *) (tempMem + 1 + (charCounter-1)/sizeof(ccInt));    // get past first element, header
                tempPtr2 += (charCounter-1) - ((charCounter-1)/sizeof(ccInt))*sizeof(ccInt);
                if (*tempPtr1 != *tempPtr2)
                    printf("readTextString: (%i, %i) chars '%c' (%i) / should be '%c' (%i)\n", (int) testCounter,(int) charCounter,
                                    *tempPtr2,(int) *tempPtr2,*tempPtr1,(int) *tempPtr1);
                do tempPtr1++; while (lettertype(tempPtr1) == unprintable);        }
            
            free((void *) tempMem);
    }   }
    
    return;
}


void test_tokenize()
{
    commandTokenType commands[] = { { "*", 1, "0#9aA", inbytecode "" }, { type1arg "+" type1arg, 2, "", inbytecode "19" },
                        { "*=" type1arg, 1, "", inbytecode "" }, { type1arg "+ =" type1arg, 2, "", inbytecode " \t" },
                        { type2arg " *\x01  + " type1arg, 3, "8", inbytecode "\t9" },
                        { "f  " type1arg "  in" type2arg, 4, "", inbytecode "7 a1 8 A2  p2 P1 j1 8 J2 " },
                        { "|" commentarg "\n", 1, removedexpression, inbytecode "" },
                        { "|*" commentarg optionalargs "*|", 1, removedexpression, inbytecode "" },
                        { variable_name, 0, "1", inbytecode "a1" }, { type0arg "\n" type0arg, 1, "", inbytecode "a1 a2" },
                        { "= @", 0, "1", inbytecode "" }, { "==", 0, "2", inbytecode "" } };
    ccInt nCrtrn, OoOdirections[] = { l_to_r, l_to_r, r_to_l, l_to_r }, numOoOs = sizeof(OoOdirections)/sizeof(ccInt);
    
    char *tokens[] = { "\n", "*", "*  + ", "*=", "*|", "+", "+ =", "==", "= @", "f  ", "in", "|", "|*", "" };
    ccInt tokenID_LHarg[] = { 16, 0, 8, 0, 0, 5, 7, 0, 0, 0, 0, 11, 13 };
    ccInt tokenID_noLHarg[] = { 0, 4, 0, 6, 0, 0, 0, 18, 17, 9, 0, 11, 13 };
    
    ccInt loopList, loopOp, loopTestString;
    ccInt tokenOoOlevels[] = { 6, 6, 5, 6, 2, 1, 2, 3, 4, 0, 6, 0, 6, 0, 5, 1, 6, 6 };
    ccInt tokenLeftArgTypes[] = { -1, -1, -1, -1, 1, -1, 1, 2, -1, 1, -1, 14, -1, 14, -1, 0, -1, -1 };
    ccInt tokenRightArgTypes[] = { -1, -1, 100, -1, 1, 1, 1, 1, 1, 2, 14, -1, 14, -1, 100, 0, -1, -1 };
    ccInt tokenRtrnTypes[][11] = { { 0,0,0,0,0,0,0,0,0,0,0 }, { 0,0,0,0,0,0,0,0,0,0,1 }, { 1,1,1,1,1,1,1,1,1,1,0 },
                { 1,0,0,0,0,0,0,0,0,1,0 }, { 0,0,0,0,0,0,0,0,0,0,0 }, { 0,0,0,0,0,0,0,0,0,0,0 }, { 0,0,0,0,0,0,0,0,0,0,0 },
                { 0,0,0,0,0,0,0,0,1,0,0 }, { 0,0,0,0,0,0,0,0,0,0,0 }, { 0,0,0,0,0,0,0,0,0,0,0 }, { 0,0,0,0,0,0,0,0,0,0,0 },
                { 0,0,0,0,0,0,0,0,0,0,0 }, { 0,0,0,0,0,0,0,0,0,0,0 }, { 0,0,0,0,0,0,0,0,0,0,0 }, { 0,1,0,0,0,0,0,0,0,0,0 },
                { 0,0,0,0,0,0,0,0,0,0,0 }, { 0,1,0,0,0,0,0,0,0,0,0 }, { 0,0,1,0,0,0,0,0,0,0,0 }       };
    ccInt nextTokens[] = { 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 12, 0, 14, 0, 0, 0, 0, 0 };
    ccInt allCodeWords[] = { 0, 0, 1, 0, 1, 1, 19, 9, 
                7, 0, 1, 1, 8, 0, 1, 2, 0, 2, 2, 0, 2, 1, 0, 3, 1, 8, 0, 3, 2,
                0, 1, 1, 0, 1, 1, 0, 1, 2 };
    ccInt firstCodeWord[] = { 1, 1, 1, 7, 7, 8, 8, 8, 9, 0, 30, 0, 30, 0, 30, 33, 39, 39 };
    ccInt numCodeWords[] = { 0, 0, 6, 0, 1, 0, 0, 1, 21, 0, 0, 0, 0, 0, 3, 6, 0, 0 };
    
    ccInt c1, c2;
    ccInt numCommands = sizeof(tokenOoOlevels) / sizeof(ccInt);
    ccInt numBytecodeWords = sizeof(allCodeWords)/sizeof(ccInt);
    char *scriptStrings[] = { " aa+ |oeut \na+ =|**| |**|*+=a * +*= ** +a +*+ * |\n+aba +a+ a |* q*| +aba |* 5",
                            " *+ b+ a + \x7f *\tin ab + ~ *", " *+ a+ a + \x7f ~ *", "\x08  ab% b" };
    ccInt scriptTokens[] = { 15, 1, 5, 15, 1, 7, 4, 7, 15, 1, 8, 6, 4, 8, 15, 1, 5, 4, 5,      // 1-25 (or 0-24)
                            4, 5, 15, 1, 5, 15, 1, 5, 15, 1, 5, 15, 1 };
    ccInt scriptTokensNum = sizeof(scriptTokens)/sizeof(ccInt), rtrn;
    ccInt scriptErrors[] = { 0, unexpected_token_err, unknown_command_err, unknown_command_err };
    char scriptErrChars[] = { ' ', 'i', '~', '%' };
    
    compiler_type *theCompiler = newCompiler(commands, sizeof(commands)/sizeof(commandTokenType),
                    OoOdirections, numOoOs, &nCrtrn);
    if (theCompiler == NULL)  {  printf("test_tokenize():  initialization gave %i\n", (int) nCrtrn);  return;  }
    
    
        // check the tokens first
    
    loopTestString = 0;
    for (loopList = 0; loopList < 128; loopList++)  {
    for (loopOp = 1; loopOp <= theCompiler->tokens[loopList].elementNum; loopOp++)  {
        if (loopList != *(tokens[loopTestString]))
            printf("test_tokenize():  wrong token list (%i; was supposed to be %i)\n", (int) loopList, (int) *(tokens[loopTestString]));
        if (*tokens[loopTestString] == 0)
            printf("test_tokenize():  extra string %s\n", (char *) element(theCompiler->tokens + loopList, loopOp));
        else  {
            if (strcmp(((tokenStringType *) element(theCompiler->tokens + loopList, loopOp))->name, tokens[loopTestString]) != 0)
                printf("test_tokenize():  strings didn't compare (list %i op %i = %s; should be tokens[%i] = %s)\n", (int) loopList, (int) loopOp,
                            ((tokenStringType *) element(theCompiler->tokens + loopList, loopOp))->name, (int) loopTestString,
                            tokens[loopTestString]);
            if (((tokenStringType *) element(theCompiler->tokens + loopList, loopOp))->tokenID_LHarg != tokenID_LHarg[loopTestString])
                printf("test_tokenize():  LHarg didn't compare on token %i (%i; should be %i)\n", (int) loopTestString,
                            (int) ((tokenStringType *) element(theCompiler->tokens + loopList, loopOp))->tokenID_LHarg, (int) tokenID_LHarg[loopTestString]);
            if (((tokenStringType *) element(theCompiler->tokens + loopList, loopOp))->tokenID_noLHarg != tokenID_noLHarg[loopTestString])
                printf("test_tokenize():  noLHarg didn't compare on token %i (%i; should be %i)\n", (int) loopTestString,
                            (int) ((tokenStringType *) element(theCompiler->tokens + loopList, loopOp))->tokenID_noLHarg, (int) tokenID_noLHarg[loopTestString]);
            loopTestString++;
    }}  }
    if (*tokens[loopTestString] != 0)
        printf("test_tokenize():  missing elements in the token list (%i strings found)\n", (int) loopTestString);
    
    
        // test the token specs
    
    if (numCommands != theCompiler->tokenSpecs.elementNum)
        printf("test_tokenize():  wrong number of commands (%i; should be %i)\n", (int) theCompiler->tokenSpecs.elementNum, (int) numCommands);
    if (numBytecodeWords != theCompiler->allCodeWords.elementNum)
        printf("test_tokenize():  wrong number of code words (%i; should be %i)\n", (int) theCompiler->allCodeWords.elementNum, (int) numBytecodeWords);
    
    for (c1 = 0; c1 < numCommands; c1++)  {
        tokenSpecType *oneTT = (tokenSpecType *) element(&(theCompiler->tokenSpecs), c1+1);
        if (oneTT->precedence != tokenOoOlevels[c1])  {
            printf("test_tokenize():  wrong token level[%i]:  was %i, should be %i\n", (int) c1, (int) oneTT->precedence, (int) tokenOoOlevels[c1]);  }
        if (oneTT->leftArgType != tokenLeftArgTypes[c1])  {
            printf("test_tokenize():  wrong left arg type [%i]:  was %i, should be %i\n", (int) c1, (int) oneTT->leftArgType, (int) tokenLeftArgTypes[c1]);  }
        if (oneTT->rightArgType != tokenRightArgTypes[c1])  {
            printf("test_tokenize():  wrong right arg type [%i]:  was %i, should be %i\n", (int) c1, (int) oneTT->rightArgType, (int) tokenRightArgTypes[c1]);  }
        if (oneTT->nextToken != nextTokens[c1])  {
            printf("test_tokenize():  wrong next token[%i]:  was %i, should be %i\n", (int) c1, (int) oneTT->nextToken, (int) nextTokens[c1]);  }
        if (oneTT->firstCodeWord != firstCodeWord[c1])  {
            printf("test_tokenize():  wrong first code word [%i]:  was %i, should be %i\n", (int) c1, (int) oneTT->firstCodeWord, (int) firstCodeWord[c1]);  }
        if (oneTT->numCodeWords != numCodeWords[c1])  {
            printf("test_tokenize():  wrong num code words [%i]:  was %i, should be %i\n", (int) c1, (int) oneTT->numCodeWords, (int) numCodeWords[c1]);  }
        for (c2 = 0; c2 < 11; c2++)  {
        if (oneTT->rtrnTypes[c2] != tokenRtrnTypes[c1][c2])  {
            printf("test_tokenize():  wrong token rtrn[%i][%i]:  was %i, should be %i\n", (int) c1, (int) c2, (int) oneTT->rtrnTypes[c2], (int) tokenRtrnTypes[c1][c2]);
    }   }}
    
    if (theCompiler->allCodeWords.elementNum != numBytecodeWords)  printf("test_tokenize():  wrong number of bytecode words\n");
    else  for (c1 = 0; c1 < numBytecodeWords; c1++)  {
    if (*LL_int(&(theCompiler->allCodeWords), c1+1) != allCodeWords[c1])  {
        printf("test_tokenize():  wrong code word [%i]:  was %i, should be %i\n", (int) c1,
                        (int) *LL_int(&(theCompiler->allCodeWords), c1+1), (int) allCodeWords[c1]);
    }}
    
    for (c1 = 0; c1 < numOoOs; c1++)  {
    if (theCompiler->OoOdirections[c1] != OoOdirections[c1])  {
        printf("test_tokenize():  wrong OoO direction [%i]:  was %i, should be %i\n", (int) c1,
                        (int) theCompiler->OoOdirections[c1], (int) OoOdirections[c1]);
    }}
    
    
        // check the tokenize() function
    
    for (c1 = 0; c1 < sizeof(scriptStrings)/sizeof(char *); c1++)  {
        
        errPosition = 1;
        rtrn = tokenize(theCompiler, scriptStrings[c1], 1);
        
        if (rtrn != scriptErrors[c1])
            printf("test_tokenize():  error code on counter = %i was %i; should be %i\n", (int) c1, (int) rtrn, (int) scriptErrors[c1]);
        if (scriptStrings[c1][errPosition-1] != scriptErrChars[c1])
            printf("test_tokenize():  error character %i on counter = %i was '%c'; should be '%c'\n",
                                    (int) errPosition, (int) c1, scriptStrings[c1][errPosition-1], scriptErrChars[c1]);
        
        if ((c1 == 0) && (rtrn == passed))  {
            
            if (scriptTokensNum != theCompiler->scriptTokens.elementNum)
                printf("test_tokenize():  wrong numbers of tokens (%i; should be %i)\n", (int) theCompiler->scriptTokens.elementNum, (int) scriptTokensNum);
            
            else for (c2 = 0; c2 < scriptTokensNum; c2++)  {
            if (((scriptTokenType *) element(&(theCompiler->scriptTokens), c2+1))->tokenID != scriptTokens[c2])  {
                printf("test_tokenize():  token %i was %i (should be %i)\n", (int) c2,
                            (int) ((scriptTokenType *) element(&(theCompiler->scriptTokens), c2+1))->tokenID, (int) scriptTokens[c2]);
            }}
    }   }
    
    
        // clean up
    
    freeCompiler(theCompiler);
    
    return;
}


void test_reorderTokens()
{
    commandTokenType tokens[] = { { "pi", 5, "13", inbytecode "11" }, { "I", 5, "23", inbytecode "4" },
                { type3arg "+" type3arg, 2, "123", inbytecode "3 a1 a2" }, { type3arg "-" type3arg, 2, "123", inbytecode "10 a1 a2" },
                { type3arg "*" type3arg, 3, "123", inbytecode "2 a1 a2" },
                { "inv" type2arg, 4, "23", inbytecode "1 a1" }, { type2arg "^T", 4, "23", inbytecode "1 a1" },
                { type2arg "dot" type2arg, 3, "13", inbytecode "12 a1 a2" }, { type0arg "and" type0arg, 1, "3", inbytecode "a1 a2" },
                { "calc" type3arg, 1, "3", inbytecode "a1" }, { type3arg "calculated", 1, "3", inbytecode "a1" },
                { "do" type3arg "forever", 1, "3", inbytecode "p1 a1 100 j1" },     // 11
                { "skip" type1arg "here", 1, "3", inbytecode "100 j4 a1 p4" },
                { "disp" type8arg, 1, "3", inbytecode "20 a1" }, { variable_name, 0, "13", inbytecode "40 a1" },
                { int_constant, 0, "13", inbytecode "50 a1" }, { double_constant, 0, "13", inbytecode "51 a1" },
                { "'" stringarg "'", 5, "8", inbytecode "0 a1" }, { noarg_adapter, 0, "0", inbytecode "" },
                { type3arg_adapter, 0, "8", inbytecode "8021 a1" }, { type2arg_adapter, 0, "1", inbytecode "-10 a1" },
                { type3arg_adapter, 0, "0", inbytecode "a1" }, { type8arg_adapter, 0, "0", inbytecode "a1 -1" },
                { "cat" type3arg "and" stringarg "to" stringarg "." type2arg, 5, "0", inbytecode "a2 a1 5 a3" },
                { type6arg "++" type6arg, 2, "37", inbytecode "99 a1 a2" }, { type7arg "**" type7arg, 3, "36", inbytecode "199 a1 a2" },
                { "t6", 0, "67", inbytecode "14" }, { noarg_adapter, 0, "6", inbytecode "15" }, { noarg_adapter, 0, "7", inbytecode "16" },
                { type3arg "/*" commentarg optionalargs "*/+" type3arg, 2, "123", inbytecode "3 a1 a3" },
                { "|*" commentarg optionalargs "*|", 5, "", removedexpression },
                { type0arg "&*" commentarg optionalargs "*&" type0arg, 5, "0", inbytecode "a1 a2" },
                { "<" chararg ">", 0, "3", inbytecode "5 a1" } };
    ccInt OoOdirections[] = { l_to_r, l_to_r, r_to_l, r_to_l, l_to_r }, rtrn, cS, cW;
    
    char *scriptStrings[] = {
        "pi  -  I dot I", "- pi", "I +", "inv pi", "I^T calculated T",
            "I calc inv I", "", " and ", "do calc I dot I forever", " skip pi here",
        " 1+3.14 calculated", "disp '\\'hello\\'*\"_'", "disp pi", "I dot I and skip I here and 'q '", "and pi and cat pi and*to/.I and I",
            "t6 ++ ** t6", "t6**++t6", "pi /* stuff */+ pi", "pi and |*", "pi and /*",
        "pi and &* hi!", " <\\70>", "<p> ", " < p>", "<>"     };
    
    ccInt scriptErrorCodes[] = {
        passed, left_arg_expected_err, right_arg_expected_err, type_mismatch_err, no_right_arg_allowed_err,
            no_left_arg_allowed_err, arg_expected_err, passed, passed, passed,
        passed, passed, passed, passed, passed,
            passed, passed, passed, passed, left_arg_expected_err,
        passed, passed, passed, char_read_err, char_read_err      };
    
    char scriptErrorChars[] = {
        0, '-', '+', 'p', 'I', 'c', 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, '/',
        0, 0, 0, '<', '<'      };
    
    ccInt script1[] = { 10, 11, 12, 4, 4 };
    ccInt script2[] = {};
    ccInt script3[] = { 12, 4, 4, 100, -4 };
    ccInt script4[] = { 100, 2, 11 };
    ccInt script5[] = { 3, 50, 1, 51, 0, 0, 0, 0, 0, 0, 0, 0 };  ccFloat script5_pi = 3.14;
    ccInt script6[] = { 20, 0, 10, 0, 0, 0 };  char *script6_str = "'hello'*\"_";
    ccInt script7[] = { 20, 8021, 11 };
    ccInt script8[] = { 12, 4, 4, 100, 3, -10, 4, 0, 2, 0, -1 };  char *script8_str = "q ";
    ccInt script9[] = { 11, 1, 0, 11, 5, 1, 0, 4 };
    ccInt script10[] = { 99, 14, 199, 16, 14 };
    ccInt script11[] = { 99, 199, 14, 16, 14 };
    ccInt script12[] = { 3, 11, 11 };
    ccInt script13[] = { 11 };
    ccInt script14[] = { 5, 112 };
    ccInt *compiledScripts[] = {
        script1, NULL, NULL, NULL, NULL, NULL, script2, script2, script3, script4,
        script5, script6, script7, script8, script9, script10, script11, script12, script13, NULL,
        script13, script14, script14, NULL, NULL   };
    
    ccInt compiledScriptLengths[] = {
        5, 0, 0, 0, 0, 0, 0, 0, 5, 3,
        4+sizeof(ccFloat)/sizeof(ccInt), 3+ceil(1.*10/sizeof(ccInt)), 3, 11, 8, 5, 5, 3, 1, 0, 1, 2, 2, 0, 0 };
    
    for (cW = 0; cW < sizeof(ccFloat)/sizeof(ccInt); cW++)  *(script5+4+cW) = *(((ccInt *) &script5_pi) + cW);
    for (cW = 0; cW < 100; cW++)  {  *(((char *) script6) + cW + 3*sizeof(ccInt)) = script6_str[cW];  if(script6_str[cW] == 0)  break;  }
    for (cW = 0; cW < 100; cW++)  {  *(((char *) script8) + cW + 9*sizeof(ccInt)) = script8_str[cW];  if(script8_str[cW] == 0)  break;  }
    *(char *) (script9+2) = '*';  *(char *) (script9+6) = '/';
    
    compiler_type *theCompiler = newCompiler(tokens, sizeof(tokens)/sizeof(commandTokenType),
                    OoOdirections, sizeof(OoOdirections)/sizeof(ccInt), &rtrn);
    if (theCompiler == NULL)  {  printf("test_reorderTokens():  initialization gave %i\n", (int) rtrn);  return;  }
    
    for (cS = 0; cS < sizeof(scriptStrings)/sizeof(char *); cS++)  {
        
        ccInt scriptTokenLevel = 3;
        if (cS == 20)  scriptTokenLevel = 0;
        
        rtrn = tokenize(theCompiler, scriptStrings[cS], scriptTokenLevel);
        
        if (rtrn != passed)  {
            if (rtrn != scriptErrorCodes[cS])
                printf("test_reorderTokens():  tokenization on script %i caused error %i; should be %i\n", (int) cS, (int) rtrn, (int) scriptErrorCodes[cS]);
            if (scriptStrings[cS][errPosition-1] != scriptErrorChars[cS])
                printf("test_reorderTokens():  script %i tokenization gave error character %c; should be %c\n", (int) cS,
                            scriptStrings[cS][errPosition-1], scriptErrorChars[cS]);  }
        
        else  {
            rtrn = reorderTokens(theCompiler, 3, true);
            if (rtrn != scriptErrorCodes[cS])
                printf("test_reorderTokens():  script %i gave error %i; should be %i\n", (int) cS, (int) rtrn, (int) scriptErrorCodes[cS]);
            
            if (rtrn != passed)  {
            if (scriptStrings[cS][errPosition-1] != scriptErrorChars[cS])  {
                printf("test_reorderTokens():  script %i gave error character %c at position %i; should be %c\n", (int) cS,
                            scriptStrings[cS][errPosition-1], (int) errPosition, scriptErrorChars[cS]);
            }}
            
            if (theCompiler->bytecode.elementNum != compiledScriptLengths[cS])
                printf("test_reorderTokens():  script %i had length %i; should be %i\n", (int) cS, (int) theCompiler->bytecode.elementNum, (int) compiledScriptLengths[cS]);
            else  {
            for (cW = 0; cW < theCompiler->bytecode.elementNum; cW++)  {
            if (*LL_int(&(theCompiler->bytecode), cW+1) != compiledScripts[cS][cW])  {
                printf("test_reorderTokens():  script %i, word %i was %i (should be %i)\n", (int) cS, (int) cW,
                            (int) *LL_int(&(theCompiler->bytecode), cW+1), (int) compiledScripts[cS][cW]);
            }}}
    }   }
    
    freeCompiler(theCompiler);
}



// test_ParseExpression() attempts to compile expressions, and checks the error codes/error positions, etc.

void test_ParseExpression()
{
    char *testStrings[] = {
            "x+y", "a*b", "n^m+q\t\t,", "\1", "a mod c",
              "a modb ", "Ab * (d- c)", "x -2 = y", "a*()+2", "a+(-4)",
            "((\1a\t))", "Brian +-David", "a:( b)", "a+(\n\tB", "0xA3.0\t+ !e",    //10
              "------a", "(-  (( -(a)+b)))", "a=a*-3\n\n", ")=5\n", "a=(%)+2",
            "(A+.04]}", ".2e7--1", "a*(b/%6)", "-%'/n", "b+|*e\n*|8%",        //20
              "aQ[5]", "both[<q,5>]", "three[<5,b,9>]", "[\0]", "[]",
            "b.c[<53+2,>]", "a[x:*],", "[7)", "c[a==b]", "a[why?]",            //30
              "c.b[where?]}", "{a:=b, a=b }", "{a+9", "{ a }", "{ a(5)+ 1",
            
            "{q = p in< p}", "if 8-3>3 then, ,,\t\n, ",        //40
            "if q> 5.5 then a, else if .4 > 3 then, b:=9, else,, a = 3",
            "else if q,", "then", "if then else q = b", "if return then", "if 8 -3",
            "if 8==4 then ( a=b, if(8==2) then q=c\n else if 1==b then()\n else (q=d) ) else(u=4), v=12", "if a==b",
            
            "for i in <a,b> q=rc",        //50
            "for in", "for i in <", "for x in <2>  ", "for ab  in <1 ",
            "for q in <a , 8>", "for k in <a b>", "for i in <a, *> ",    //55
            "for j in <x,y> ()", "for c in <q,b>(", "for b in <q, r> (, ())",
            
            "while a+b>c do q=.5, ", "while a do ()", "while a > 3 do ", "while a > 4 do (a=3)",    //61
            "while b==2 do (), b=7", "while a==2 do)",
            
            "{loop( a=b) until ((c==d))}", "loop() until a>b", " loop a=d) until 9>8", "loop( a=1) until",    //67
            "{loop (a=b)until, e==y)", "loop( a=b) until q ", "{loop a=9 until 9>2, until 9>1]}", "a[b[c+1]+1]",
            
            "if a == b then if c == d then a = b else c = d", "f(if a<b then c=d)", "((rtrn =@ *) = @output)",
            "a=b+1 |comment\n if a < b then ()", "if a<2 then b=2, c=4, else d=5" };
    
    ccInt lowOoOtype[] = {
            5, 5, 1, 1, 5,
                5, 5, 1, 5, 5,
            3, 5, 7, 5, 5,          // 10
                5, 5, 1, 1, 1,
            5, 5, 5, 5, 5,          // 20
                3, 3, 3, 3, 3,
            5, 3, 5, 5, 5,          // 30
                3, 1, 7, 1, 7,
            7, 1, 1, 1, 1,          // 40
                1, 1, 1, 1, 7,
            1, 1, 1, 1, 1,          // 50
                1, 1, 1, 1, 1,
            1, 1, 1, 1, 1,          // 60
                1, 1, 7, 1, 1,
            1, 7, 1, 7, 7,
                1, 1, 3, 1, 1 };
    
    ccInt FirstOps[] = {
            add_num, multiply_num, define_equate, 0, mod_int,
              search_member, multiply_num, define_equate, add_num, add_num,
            search_member, add_num, append_code, add_num, add_num,
              multiply_num, multiply_num, define_equate, 0, define_equate,
            0, subtract_num, multiply_num, 0, add_num,
              step_to_index, step_to_indices, search_member, 0, 0,
            step_to_member_ID, search_member, 0, search_member, 0,
              step_to_member_ID, define_equate, 0, define_equate, 0,
        
            0, jump_if_false, jump_if_false, 0, 0, 0, 0,                // 40
            0, jump_if_false, 0,
        
            define_equate, 0, 0, define_equate, 0, define_equate, 0,          // 50
            0, define_equate, 0, define_equate,
        
            jump_if_false, jump_if_false, jump_if_false, jump_if_false, jump_if_false, 0,       // 61
        
            code_block, jump_if_false, 0, 0, 0, define_equate, 0, step_to_index,
            
            jump_if_false, define_equate, define_equate, define_equate, jump_if_false         };
    
    char TestErrors[] = {
            passed, passed, passed, passed, passed,
              no_right_arg_allowed_err, passed, type_mismatch_err, arg_expected_err, passed,
            passed, passed, passed, token_expected_err, unknown_command_err,
              passed, passed, passed, unexpected_token_err, unknown_command_err,
            unexpected_token_err, passed, unknown_command_err, unknown_command_err, unknown_command_err,
              passed, passed, type_mismatch_err, right_arg_expected_err, right_arg_expected_err,
            arg_expected_err, type_mismatch_err, unexpected_token_err, type_mismatch_err, unknown_command_err,
              unknown_command_err, passed, token_expected_err, passed, token_expected_err,
        
            unexpected_token_err, passed, passed, unexpected_token_err, unexpected_token_err, arg_expected_err,     //40
            type_mismatch_err, token_expected_err, passed, token_expected_err,
        
            passed, token_expected_err, right_arg_expected_err, right_arg_expected_err, token_expected_err,    //50
            passed, no_left_arg_allowed_err, type_mismatch_err, passed,
            token_expected_err, passed,
        
            passed, passed, passed, passed, passed, unexpected_token_err,    //61
        
            passed, passed, unexpected_token_err, right_arg_expected_err, unexpected_token_err, passed,        //67
                                                                                            unexpected_token_err, passed,
            
            passed, passed, passed, passed, passed      };
    
    
    char TestErrPositions[] = {
            '#', '#', '#', '#', '#',
              'a', '#', '-', ')', '#',
            '#', '#', '#', '\0', '!',
              '#', '#', '#', ')', '%',
            ']', '#', '%', '%', '%',
              '#', '#', ',', '[', '[',
            '>', ':', ')', '=', '?',            // 30
              '?', '#', '\0', '#', '\0',
            'i', '#', '#', 'e', 't', 't', 'r', '\0', '#', '\0',
            '#', '\0', 'i', '>', '\0', '#', 'b', '*', '#', '\0', '#',     // 50
            '#', '#', '#', '#', '#', ')',       // 61
            '#', '#', ')', 'u', ')', '#', 'u', '#',
            '#', '#', '#', '#', '#'    };
    
    ccInt the_op, counter, rtrn;
    
    compiler_type *theCompiler = newCompiler(cicadaLanguage, cicadaLanguageNumCommands,
                    cicadaLanguageAssociativity, cicadaNumPrecedenceLevels, &rtrn);
    if (theCompiler == NULL)  {  printf("test_ParseExpression():  initialization gave %i on command %i\n", (int) rtrn, (int) errPosition);  return;  }
    
    for (counter = 0; counter < sizeof(testStrings)/sizeof(char *); counter++)    {
        
        rtrn = tokenize(theCompiler, testStrings[counter], lowOoOtype[counter]);
        if (rtrn == passed)  rtrn = reorderTokens(theCompiler, lowOoOtype[counter], true);
        
        if (rtrn != TestErrors[counter])
            printf("ParseExpression:  Return on counter = %i:  %i; should be %i\n", (int) counter, (int) rtrn, (int) TestErrors[counter]);
        
        if (rtrn != passed)  {
        if (testStrings[counter][errPosition-1] != TestErrPositions[counter])  {
            printf("ParseExpression:  errPosition on counter = %i:  '%c%c%c%c' (%i); should be %c (%i)\n", (int) counter,
                testStrings[counter][errPosition-1], testStrings[counter][errPosition], testStrings[counter][errPosition+1],
                            testStrings[counter][errPosition+2], (int) errPosition, TestErrPositions[counter], (int) TestErrPositions[counter]);
        }}
        
        if ((rtrn == passed) && (theCompiler->bytecode.elementNum > 0))  {
            the_op = *LL_int(&(theCompiler->bytecode), 1);
            if (the_op != FirstOps[counter])
                printf("ParseExpression:  Operation on counter = %i:  %i; should be %i\n", (int) counter, (int) the_op, (int) FirstOps[counter]);
    }   }
    
    return;
}



// test_Compile() tries to compile a ~20-line script, and checks the bytecode output

void test_Compile()
{
    
    #define user_f_arg_flags ((1<<2)+(1<<3)+(1<<5)+(1<<6)+(1<<7))
//    #define user_f_arg_flags ((1<<2)+(1<<3)+(1<<5)+(1<<6)+(1<<7)+(1<<8))
    
//    #define for_pos_A (13-102)        /* (one plus the number assigned in the CodeCompare list -- since C starts arrays at 0)  */
/*    #define for_pos_B (42-91)
    #define do_pos (22-23)
    #define endf_pos_A (103-14)
    #define endf_pos_B (92-43)
    #define endif_posA (156-125)
    #define endif_posB (156-155)
    #define endw_pos (154-145)
    #define while_pos (144-153)*/
    #define for_pos_A (12-96)        /* (one plus the number assigned in the CodeCompare list -- since C starts arrays at 0)  */
    #define for_pos_B (40-86)
    #define do_pos (21-22)
    #define endf_pos_A (97-13)
    #define endf_pos_B (87-41)
    #define endif_posA (146-116)
    #define endif_posB (146-145)
    #define endw_pos (144-135)
    #define while_pos (134-143)
    
    
        // counter - 1        (Var IDs)
        // int - 2
        // a-d -- 3-6
        // A -- 7
    
    char *programString =
        "counter @:: my_var\n"
        "for counter in <0, a.b>("
        "    loop\n "
        "    until (c > d) or(c <= d)\n"

        "    for a in <1 ,2> (\n"
        "        b:=  @a, c=! A(a, 3),"
        "    )\n"
        ")\n"
        "counter = ((c*(-d/-5300-52)))\n\n\n"
        
        "if (a < b) and (0>=b) xor (a ==b)\t\1 then (\n"
        "    while ( (( (not a /=b) ))\t\1)do(,"
        "    )\t,"
        " ), "
        
        " this, a=@b, b::((a:d))\n"
        "a = (A<<(b:b))(),"
        "a[a+2] :: *, b:=args, return b, "
        "a[<2 , 3 > ] = b[1]+4 mod 5,"
        
        "a *:: (this:this:{a, this=2} ), return," "{a},"
                "a[+5][+4] = b[+ < 5,6>][+<8 ,9> ]," "a=a[b[c+1]+1]," "(return)";
    
    ccInt CodeCompare[] =  {
        define_equate, vdf_flags, search_member, 2, search_member, 3,    // 0  (off from markers (after #def's above); C arrays start at 0)
        define_equate, equ_flags, search_member, 2, constant_int, 0,        // 6
                            jump_if_true, endf_pos_A, if_greater, search_member, 2, step_to_member_ID, 5, search_member, 4,
                jump_if_false, do_pos, if_or, if_greater, search_member, 6, search_member, 7, if_less_or_equal, search_member, 6, search_member, 7,     // 21

                define_equate, equ_flags, search_member, 4, constant_int, 1, jump_if_true, endf_pos_B,    // 34
                                                    if_greater, search_member, 4, constant_int, 2,
                        define_equate, dqa_flags, search_member, 5, search_member, 4, forced_equate, search_member, 6, user_function,     // 47
                                    search_member, 8, define_equate, user_f_arg_flags, search_member, -1, code_block,
                    define_equate, dqa_flags-2+128, search_member, -2, search_member, 4, define_equate, deq_flags-2+128, search_member,
                                                                                                    -3, constant_int, 3, end_of_script,
                define_equate, equ_flags, search_member, 4, add_num, that_variable, constant_int, 1, jump_always, for_pos_B,            // 77
        define_equate, equ_flags, search_member, 2, add_num, that_variable, constant_int, 1, jump_always, for_pos_A,        // 87
        define_equate, equ_flags, search_member, 2, multiply_num, search_member, 6, subtract_num, divide_num, multiply_num, constant_int, -1, search_member, 7,    // 97
                                                constant_int, -5300, constant_int, 52,
        
        jump_if_false, endif_posA, if_xor, if_and, if_less, search_member, 4, search_member, 5, if_greater_or_equal, constant_int, 0, search_member, 5, if_equal,    // 115
                                                                                                search_member, 4, search_member, 5,
                jump_if_false, endw_pos, if_not, if_not_equal, search_member, 4, search_member, 5,                    // 134
                jump_always, while_pos, jump_always, endif_posB,                                            // 142
        
        define_equate, dqa_flags-2+128, search_member, -4, this_variable, define_equate, eqa_flags, search_member, 4, search_member, 5, // 146
                                                define_equate, def_flags, search_member, 5, append_code, search_member, 4, search_member, 7,
        define_equate, equ_flags, search_member, 4, user_function, substitute_code, append_code, search_member, 5, search_member, 5,              // 166
                    search_member, 8, define_equate, user_f_arg_flags, search_member, -5, code_block, end_of_script,
        define_equate, def_flags, step_to_index, search_member, 4, add_num, search_member, 4, constant_int, 2, no_variable, define_equate,  // 185
                                                                deq_flags, search_member, 5, args_variable, function_return, search_member, 5,
        define_equate, equ_flags, step_to_indices, search_member, 4, constant_int, 2, constant_int, 3, add_num, step_to_index,    // 204
                                                        search_member, 5, constant_int, 1, mod_int, constant_int, 4, constant_int, 5,
        
        define_equate, mdf_flags, search_member, 4, append_code, append_code, this_variable, this_variable, code_block, define_equate, dqa_flags+128-2, // 224
                    search_member, -6, search_member, 4, define_equate, equ_flags, this_variable, constant_int, 2, end_of_script, function_return, no_variable,
                            define_equate, def_flags-2+128, search_member, -7, code_block, define_equate, dqa_flags-2+128,         //  247
                                                                        search_member, -8, search_member, 4, end_of_script,
        define_equate, equ_flags, insert_index, insert_index, search_member, 4, constant_int, 5, constant_int, 4,   // 259
                insert_indices, insert_indices, search_member, 5, constant_int, 5, constant_int, 6, constant_int, 8, constant_int, 9,
                                define_equate, equ_flags, search_member, 4, step_to_index, search_member, 4,
                        add_num, step_to_index, search_member, 5, add_num, search_member, 6, constant_int, 1, constant_int, 1,
        function_return, no_variable, end_of_script         // 299
    };
    
    ccInt counter, rtrn, dummy_refs;
    
    compiler_type *theCompiler = newCompiler(cicadaLanguage, cicadaLanguageNumCommands,
                    cicadaLanguageAssociativity, cicadaNumPrecedenceLevels, &rtrn);
    if (theCompiler == NULL)  {  printf("test_Compile():  initialization gave %i on command %i\n", (int) rtrn, (int) errPosition);  return;  }
    
    rtrn = tokenize(theCompiler, programString, 0);
    if (rtrn != passed)  {  printf("test_Compile():  tokenization gave %i at %c%c%c%c%c%c%c%c%c%c\n", (int) rtrn, programString[errPosition-1],
            programString[errPosition], programString[errPosition+1], programString[errPosition+2], programString[errPosition+3],
            programString[errPosition+4], programString[errPosition+5], programString[errPosition+6], programString[errPosition+7],
            programString[errPosition+8]);  return;  }
    
    rtrn = reorderTokens(theCompiler, 0, true);
    if (rtrn != passed)  {  printf("test_Compile():  token reordering gave %i at word %c%c%c%c%c%c%c%c%c%c\n", (int) rtrn, programString[errPosition-1],
            programString[errPosition], programString[errPosition+1], programString[errPosition+2], programString[errPosition+3],
            programString[errPosition+4], programString[errPosition+5], programString[errPosition+6], programString[errPosition+7],
            programString[errPosition+8]);  return;  }
    
    if (theCompiler->bytecode.elementNum != sizeof(CodeCompare)/sizeof(ccInt))
        printf("test_Compile: Number of code words was %i; should be %i\n", (int) theCompiler->bytecode.elementNum, (int) (sizeof(CodeCompare)/sizeof(ccInt)));
    
    for (counter = 0; counter < theCompiler->bytecode.elementNum; counter++)    {
        if (counter >= sizeof(CodeCompare)/sizeof(ccInt))  {
            printf("test_Compile: Code overflow on counter = %i\n", counter);
            break;
        }
        if (*(ccInt *) findElement(&(theCompiler->bytecode), counter+1) != CodeCompare[counter])  {
            printf("test_Compile: Code mismatch on counter = %i:  compiled %i vs expected %i\n", 
                (int) counter, (int) *(ccInt *) findElement(&(theCompiler->bytecode), counter+1), (int) CodeCompare[counter]);
    }   }
    
    startCodePtr = LL_int(&(theCompiler->bytecode), 1);
    endCodePtr = LL_int(&(theCompiler->bytecode), theCompiler->bytecode.elementNum) + 1;
    
    pcCodePtr = startCodePtr;
    dummy_refs = 10;
    PCCodeRef.references = &dummy_refs;
    PCCodeRef.code_ptr = startCodePtr;
    PCCodeRef.anchor = 0;
    PCCodeRef.PLL_index = 0;
    
    errScript.references = &dummy_refs;
    warningScript.references = &dummy_refs;
    
    checkBytecode();
    if (errCode != passed)  printf("Compile: checkBytecode gave %i at %i\n", (int) errCode, (int) errIndex);
    else if (warningCode != passed)  printf("Compile: checkBytecode gave %i at %i\n", (int) warningCode, (int) errIndex);
    
    freeCompiler(theCompiler);
    
    return;
}
