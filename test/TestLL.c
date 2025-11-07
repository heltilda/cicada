#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "lnklst.h"
#include "cmpile.h"
#include "intrpt.h"
#include "bytecd.h"
#include "ccmain.h"


extern ccInt rndm(ccInt);
extern ccInt rseed, start_seed;
extern sublistHeader *findSublist(linkedlist *, ccInt, ccInt *);


typedef struct {
    ccInt data;
    ccInt index;
    ccInt *dataPtr;
    ccInt refs;
} TestPLLType;


void test_LL()
{
    linkedlist TestList, CompareList;
    sublistHeader *TestSublist;
    ccInt counter, loopnum, fill_counter, SublistBottom, LocalSublistIndex, SublistCompareInt, rtrn;
    void *memblock;

    TestList.memory = 0;
    rtrn = newLinkedList(&TestList, rndm(20), sizeof(ccInt), 0.01*rndm(255), false);
    if (rtrn != passed)    {  printf("LL: Failed on NewLL for TestList: %i / %i\n", (int) rtrn, (int) start_seed);    return;  }

    for (fill_counter = 1; fill_counter <= TestList.elementNum; fill_counter++)
        *LL_int(&TestList, fill_counter) = fill_counter;

    for (loopnum = 1; loopnum < 30; loopnum ++)
    {
        ccInt start, end, start_dest, temp1, temp2, compare_num;

        temp1 = (ccInt) floor((ccFloat) (rndm(37)*TestList.elementNum)/36)+1;
        temp2 = rndm(13)+1;
        rtrn = insertElements(&TestList, temp1, temp2, false);
        if (rtrn != passed)    {  printf("LL: rtrn on insertElements: %i (%i %i) / %i\n", (int) rtrn, (int) temp1, (int) temp2,
                                            (int) start_seed);  return;  }
        for (counter = 1; counter <= temp2; counter++)    {
            *LL_int(&TestList, counter+temp1-1) = fill_counter;  fill_counter++;  }

        start = (ccInt) floor((ccFloat) (rndm(11)*TestList.elementNum)/11)+1;
        end = (ccInt) floor((ccFloat) (rndm(29)*TestList.elementNum)/29)+1;
        if (start > end)    {  temp1 = start; start = end; end = temp1;  }
        start_dest = (ccInt) floor((ccFloat) (rndm(29)*(TestList.elementNum-(end-start)))/29)+1;

        rtrn = copyElements(&TestList, start, &TestList, start_dest, end-start+1);
        if (rtrn != passed)    {  printf("LL: Return on copyElements: %i (%i %i %i) / %i\n", (int) rtrn, (int) start, (int) end,
                                        (int) start_dest, (int) start_seed);  return;  }

        compare_num = end-start+1;
        if ((start < start_dest) && (start+compare_num-1 >= start_dest))
            compare_num = start_dest-start;
        else if ((start > start_dest) && (start_dest+compare_num-1 >= start))    {
            compare_num = start-start_dest;
            start_dest += end-start+1-compare_num;
            start += end-start+1-compare_num;    }

        rtrn = compareElements(&TestList, start, &TestList, start_dest, compare_num);
        if (rtrn != true)    {  printf("LL: Return on compareElements (1): %i (%i %i %i %i) / %i\n", (int) rtrn, (int) start,
                                (int) end, (int) start_dest, (int) compare_num, (int) start_seed);  }
        if (rtrn != true)    {  printf("%i %i %i\n", (int) loopnum, (int) *LL_int(&TestList, start), (int) *LL_int(&TestList, start_dest));  }

        rtrn = clearElements(&TestList, start, end);
        if (rtrn != passed)    {  printf("LL: Return on clearElements: %i (%i %i) / %i\n", (int) rtrn, (int) start, (int) end,
                                                    (int) start_seed);  return;  }
        for (counter = start; counter <= end; counter++)    {
            if (counter == start)  {
                TestSublist = findSublist(&TestList, start, &SublistBottom);
                LocalSublistIndex = start-SublistBottom;
                SublistCompareInt = *LL_int(&TestList, start);    }
            else  SublistCompareInt = *(ccInt *) skipElements(&TestList, &TestSublist, &LocalSublistIndex, 1);
            if (SublistCompareInt != 0)  printf("LL Clear: Failed on counter = %i / %i\n", (int) counter, (int) start_seed);
        }

        start = (ccInt) floor((ccFloat) (rndm(97)*TestList.elementNum)/97) + 1;
        end = (ccInt) floor((ccFloat) (rndm(31)*TestList.elementNum)/31) + 1;
        if (start > end)    {temp1 = start; start = end; end = temp1;}
        rtrn = deleteElements(&TestList, start, end);
        if (rtrn != passed)    {  printf("LL: Return on deleteElements: %i (%i %i) / %i\n", (int) rtrn, (int) temp1, (int) temp2,
                                                    (int) start_seed);  return;  }
    }
    
    CompareList.memory = 0;
    rtrn = newLinkedList(&CompareList, TestList.elementNum, sizeof(ccInt), 0.01*rndm(255), false);
    if (rtrn != passed)    {  printf("LL: Failed on NewLL for CompareList: %i / %i\n", (int) rtrn, (int) start_seed);    return;  }
    memblock = (void *) malloc((size_t) TestList.elementNum * sizeof(ccInt));
    if (memblock == 0)    {  printf("LL: Out of memory on malloc: %i / %i\n", (int) rtrn, (int) start_seed);  return;  }

    if (TestList.elementNum >= 1)    {
        rtrn = getElements(&TestList, 1, TestList.elementNum, memblock);
        if (rtrn != passed)    {  printf("LL: Failed on getElements: %i / %i\n", (int) rtrn, (int) start_seed);  return;  }
        rtrn = setElements(&CompareList, 1, TestList.elementNum, memblock);
        if (rtrn != passed)    {  printf("LL: Failed on setElements: %i / %i\n", (int) rtrn, (int) start_seed);  return;  }

        rtrn = compareElements(&TestList, 1, &CompareList, 1, TestList.elementNum);
        if (rtrn != true)    {  printf("LL: Return value on Compare (2): %i / %i\n", (int) rtrn, (int) start_seed);  }
    }

    rtrn = defragmentLinkedList(&TestList);
    if (rtrn != passed)    {  printf("LL: Failed on DefragmentLL: %i / %i\n", (int) rtrn, (int) start_seed);  return;  }

    if (TestList.elementNum >=1)    {
        rtrn = compareElements(&TestList, 1, &CompareList, 1, TestList.elementNum);
        if (rtrn != true)    {  printf("LL: Return value on Compare (3): %i / %i\n", (int) rtrn, (int) start_seed);  }
    }

    deleteLinkedList(&TestList);
    deleteLinkedList(&CompareList);

    free(memblock);

    return;
}

void test_FLL()
{
    linkedlist TestList, CompareList;
    ccInt counter, loopnum, fill_counter, rtrn;
    void *memblock;

    TestList.memory = 0;
    rtrn = newLinkedList(&TestList, rndm(20), sizeof(ccInt), 0.01*rndm(255), false);
    if (rtrn != passed)    {  printf("LL: Failed on NewLL for TestList: %i / %i\n", (int) rtrn, (int) start_seed);    return;  }

    for (fill_counter = 1; fill_counter <= TestList.elementNum; fill_counter++)
        *LL_int(&TestList, fill_counter) = fill_counter;

    for (loopnum = 1; loopnum < 30; loopnum ++)
    {
        ccInt start, end, start_dest, temp1, temp2, compare_num;

        temp1 = (ccInt) floor((ccFloat) (rndm(37)*TestList.elementNum)/36)+1;
        temp2 = rndm(13)+1;
        rtrn = insertElements(&TestList, temp1, temp2, false);
        if (rtrn != passed)    {  printf("LL: Return on insertElements: %i (%i %i) / %i\n", (int) rtrn, (int) temp1, (int) temp2,
                                            (int) start_seed);  return;  }
        for (counter = 1; counter <= temp2; counter++)    {
            *LL_int(&TestList, counter+temp1-1) = fill_counter;  fill_counter++;  }

        start = (ccInt) floor((ccFloat) (rndm(11)*TestList.elementNum)/11)+1;
        end = (ccInt) floor((ccFloat) (rndm(29)*TestList.elementNum)/29)+1;
        if (start > end)    {temp1 = start; start = end; end = temp1;}
        start_dest = (ccInt) floor((ccFloat) (rndm(29)*(TestList.elementNum-(end-start)))/29)+1;

        copyElements(&TestList, start, &TestList, start_dest, end-start+1);

        compare_num = end-start+1;
        if ((start < start_dest) && (start+compare_num-1 >= start_dest))
            compare_num = start_dest-start;
        else if ((start > start_dest) && (start_dest+compare_num-1 >= start))    {
            compare_num = start-start_dest;
            start_dest += end-start+1-compare_num;
            start += end-start+1-compare_num;    }

        rtrn = compareElements(&TestList, start, &TestList, start_dest, compare_num);
        if (rtrn != true)    {  printf("LL: Return on compareElements (1): %i (%i %i %i %i) / %i\n", (int) rtrn, (int) start,
                                (int) end, (int) start_dest, (int) compare_num, (int) start_seed);  }

        clearElements(&TestList, start, end);
        for (counter = start; counter <= end; counter++)
            if (*LL_int(&TestList, counter) != 0)  printf("LL Clear: Failed on counter = %i / %i\n", (int) counter, (int) start_seed);

        start = (ccInt) floor((ccFloat) (rndm(97)*TestList.elementNum)/97) + 1;
        end = (ccInt) floor((ccFloat) (rndm(31)*TestList.elementNum)/31) + 1;
        if (start > end)    { temp1 = start; start = end; end = temp1;  }
        rtrn = deleteElements(&TestList, start, end);
        if (rtrn != passed)    {  printf("LL: Return on deleteElements: %i (%i %i) / %i\n", (int) rtrn, (int) temp1, (int) temp2,
                                                    (int) start_seed);  return;  }
    }

    CompareList.memory = 0;
    rtrn = newLinkedList(&CompareList, TestList.elementNum, sizeof(ccInt), 0.01*rndm(255), false);
    if (rtrn != passed)    {  printf("LL: Failed on NewLL for CompareList: %i / %i\n", (int) rtrn, (int) start_seed);    return;  }
    memblock = (void *) malloc((size_t) TestList.elementNum * sizeof(ccInt));
    if (memblock == 0)    {  printf("LL: Out of memory on malloc: %i / %i\n", (int) rtrn, (int) start_seed);  return;  }

    if (TestList.elementNum >= 1)    {
        getElements(&TestList, 1, TestList.elementNum, memblock);
        setElements(&CompareList, 1, TestList.elementNum, memblock);

        rtrn = compareElements(&TestList, 1, &CompareList, 1, TestList.elementNum);
        if (rtrn != true)    {  printf("LL: Return value on Compare (2): %i / %i\n", (int) rtrn, (int) start_seed);  }
    }

    defragmentLinkedList(&TestList);

    if (TestList.elementNum >=1)    {
        rtrn = compareElements(&TestList, 1, &CompareList, 1, TestList.elementNum);
        if (rtrn != true)    {  printf("LL: Return value on Compare (3): %i / %i\n", (int) rtrn, (int) start_seed);  }
    }

    deleteLinkedList(&TestList);
    deleteLinkedList(&CompareList);

    free(memblock);

    return;
}

// *************  Routine to test pinned linked lists **************

void test_PLL()
{
    pinned_LL TestPLL;
    linkedlist AuxLL;
    ccInt oldtop, counter, c2, temp1, temp2, data_counter = 1, maxLLelements = 0, NewIndex, *PLL_elPtr, rtrn;
    void *NewPtr;

    rtrn = newPLL(&TestPLL, 0, sizeof(ccInt), 1.);
    if (rtrn != passed)  {  printf("PLL: newPLL returned %i (%i)\n", (int) rtrn, (int) start_seed);  return;  }
    rtrn = newLinkedList(&AuxLL, 0, sizeof(TestPLLType), 1., false);
    if (rtrn != passed)  {  printf("PLL: newLinkedList returned %i (%i)\n", (int) rtrn, (int) start_seed);  return;  }

    for (counter = 1; counter <= 30; counter++)    {
        oldtop = AuxLL.elementNum;

        temp1 = rndm(5)+1;

        for (c2 = 1; c2 <= temp1; c2++)    {            // add some data
            rtrn = addElements(&AuxLL, 1, false);
            if (rtrn != passed)  {  printf("PLL: addElements returned %i (%i)\n", (int) rtrn, (int) start_seed);  return;  }
            rtrn = addPLLElement(&TestPLL, &NewPtr, &NewIndex);
            if (rtrn != passed)  {  printf("PLL: addPLLElement returned %i (%i)\n", (int) rtrn, (int) start_seed);  return;  }

            temp2 = rndm(3)+1;
            *LL_int(&(TestPLL.data), NewIndex) = data_counter;
            *LL_int(&(TestPLL.references), NewIndex) = temp2;
            ((TestPLLType *) findElement(&AuxLL, c2+oldtop))->data = data_counter;
            ((TestPLLType *) findElement(&AuxLL, c2+oldtop))->index = NewIndex;
            ((TestPLLType *) findElement(&AuxLL, c2+oldtop))->dataPtr = NewPtr;
            ((TestPLLType *) findElement(&AuxLL, c2+oldtop))->refs = temp2;
            data_counter++;        }

        if (AuxLL.elementNum > maxLLelements)  maxLLelements = AuxLL.elementNum;

        for (c2 = 1; c2 <= AuxLL.elementNum; c2++)    {
            if (rndm(11) == 0)    {
                refPLLElement(&TestPLL, ((TestPLLType *) findElement(&AuxLL, c2))->index);
                (((TestPLLType *) findElement(&AuxLL, c2))->refs)++;    }    }
                    
        for (c2 = 1; c2 <= AuxLL.elementNum; c2++)    {
            if (rndm(31) == 0)    {
                RefPLLPtr(LL_int(&(TestPLL.references), ((TestPLLType *) findElement(&AuxLL, c2))->index));
                (((TestPLLType *) findElement(&AuxLL, c2))->refs)++;    }    }
                    
        for (c2 = 1; c2 <= AuxLL.elementNum; c2++)    {
            if (rndm(7) == 0)    {
                derefPLLElement(&TestPLL, ((TestPLLType *) findElement(&AuxLL, c2))->index);
                (((TestPLLType *) findElement(&AuxLL, c2))->refs)--;
                if (((TestPLLType *) findElement(&AuxLL, c2))->refs == 0)    {
                    deleteElement(&AuxLL, c2);
        }    }    }

        for (c2 = 1; c2 <= AuxLL.elementNum; c2++)    {
            if (rndm(13) == 0)    {
                derefPLLPtr(&TestPLL, ((TestPLLType *) findElement(&AuxLL, c2))->index,
                        LL_int(&(TestPLL.references), ((TestPLLType *) findElement(&AuxLL, c2))->index));
                (((TestPLLType *) findElement(&AuxLL, c2))->refs)--;
                if (((TestPLLType *) findElement(&AuxLL, c2))->refs == 0)    {
                    deleteElement(&AuxLL, c2);
        }   }   }

        if ((maxLLelements != TestPLL.data.elementNum) || (maxLLelements != TestPLL.references.elementNum))
            {  printf("PLL: MaxEls are %i %i %i (%i)\n", (int) maxLLelements, (int) TestPLL.data.elementNum,
                            (int) TestPLL.references.elementNum, (int) start_seed);  return;  }
        
        for (c2 = 1; c2 <= AuxLL.elementNum; c2++)    {
            PLL_elPtr = LL_int(&(TestPLL.data), ((TestPLLType *) findElement(&AuxLL, c2))->index);
            if (((TestPLLType *) findElement(&AuxLL, c2))->dataPtr != PLL_elPtr)
                {  printf("PLL: El/data mismatch -- %i %p %p (%i)\n", (int) c2, ((TestPLLType *) findElement(&AuxLL, c2))->dataPtr,
                        (void *) PLL_elPtr, (int) start_seed);  }
            else if (((TestPLLType *) findElement(&AuxLL, c2))->data != *PLL_elPtr)
                {  printf("PLL: El/data mismatch -- %i %i %p (%i)\n", (int) c2, (int) ((TestPLLType *) findElement(&AuxLL, c2))->data,
                        (void *) PLL_elPtr, (int) start_seed);  }
        }
    }

    deletePLL(&TestPLL);
    deleteLinkedList(&AuxLL);
}

void test_Stack()
{
    ccInt counter, *StackElPtr, rtrn;
    stack theStack;

    rtrn = newStack(&theStack, sizeof(ccInt), 5, 1.);
    if (rtrn != passed)  {  printf("Stack: newStack returned %i (%i)\n", (int) rtrn, (int) start_seed); return;  }

    for (counter = 1; counter <= 30; counter++)    {
        if (counter % 3 != 0)    {
            rtrn = pushStack(&theStack, (void **) &StackElPtr);
            if (rtrn != passed)  {  printf("Stack: pushStack returned %i on %i (%i)\n", (int) rtrn, (int) counter, (int) start_seed); return;  }
            *StackElPtr = counter;    }
        else    {
            popStack(&theStack, (void **) &StackElPtr);
            if (*StackElPtr != counter-1)  {  printf("Stack: popStack gave %i on %i (%i)\n", (int) *StackElPtr, (int) counter, (int) start_seed); return;  }
    }    }

    for (counter = 30; counter >= 1; counter--)    {
        if (counter % 3 == 0)    {
            rtrn = pushStack(&theStack, (void **) &StackElPtr);
            if (rtrn != passed)  {  printf("Stack: pushStack (2) returned %i on %i (%i)\n", (int) rtrn, (int) counter, (int) start_seed); return;  }
            *StackElPtr = counter-1;    }
        else    {
            popStack(&theStack, (void **) &StackElPtr);
            if (*StackElPtr != counter)  {  printf("Stack: popStack (2) gave %i on %i (%i)\n", (int) *StackElPtr, (int) counter, (int) start_seed); return;  }    }    }

    deleteStack(&theStack);

    return;
}
