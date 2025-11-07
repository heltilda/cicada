#include <stdio.h>
#include "lnklst.h"

extern bool debug_on;
extern void test_MM_CountRefs(void);


#include <time.h>


extern void TestCicada(void);

// TestLL prototypes
extern void test_LL(void);
extern void test_FLL(void);

// TestCompile prototypes
extern void test_Compile(void);
extern void test_ParseExpression(void);
extern void test_GetOperation(void);
extern void test_ReadVarName(void);
extern void test_ReadString(void);
extern void test_CompareString(void);
extern void test_ReadNum(void);
extern void test_ncclib_char(void);
extern void test_get_ncclib_word(void);
extern void test_lettertype(void);
extern void test_newCompiler(void);
extern void test_readNum(void);
extern void test_nextChar(void);
extern void test_readTextString(void);
extern void test_tokenize(void);
extern void test_reorderTokens(void);

// TestInterpret prototypes
extern void test_NewDelVar(void);
extern void test_ModMem(void);
extern void test_checkBytecode(void);
extern void test_PLL(void);
extern void test_Stack(void);

// TestBytecode prototypes
extern void test_CheckBytecode(void);
extern void test_YHErrors(void);
extern void test_scripts(void);


#define main_retrn unsigned char
#define initialize_err 150        /*  init_err is taken by LL.h  */

extern ccInt rndm(ccInt);
extern void randomize(ccInt);

bool debug_on = false;

ccInt rseed, start_seed;

void TestCicada()
{
    ccInt counter;
    
    start_seed = (ccInt) time(0);
//    start_seed = 1467425017;
    printf("Random seed: %i\n\n", (int) start_seed);
        
    for (counter = 1; counter <= 100; counter++)  {
        rseed = start_seed;
        test_LL();
        test_FLL();
        start_seed = rseed+1;            }
    
    test_lettertype();
    test_newCompiler();
    test_readNum();
    test_nextChar();
    test_readTextString();
    test_tokenize();
    test_reorderTokens();
    test_ParseExpression();
    test_Compile();
    
    test_Stack();
    test_PLL();
    test_ModMem();
    test_checkBytecode();
    test_YHErrors();
    
    test_scripts();
}

ccInt rndm(ccInt max_number)
{
    rseed++;
    return (rseed % max_number);
}

// In loops, calling, say, rndm(5) will yield 1, 2, 3, 4, 0, 1, ... -- call randomize with a different number than 5.
void randomize(ccInt max_number)
{
    rseed += rndm(max_number);
}

