#include <stdlib.h>
#include <stdio.h>
#include "lnklst.h"
#include "cmpile.h"
#include "intrpt.h"
#include "bytecd.h"
#include "ccmain.h"

//Prototypes

void test_ModMem(void);
void test_SetUpArray(ccInt, ccInt, ccInt *);
ccInt test_FillArray(ccInt *);
ccInt test_TestArray(ccInt *, ccInt, bool *);
ccInt test_SwapDelMembers(ccInt);
void test_MM_CountRefs(void);
void test_MMCR_AddRef(linkedlist *, void *);
void test_MMCR_TestRef(linkedlist *, void *, ccInt, char *);
ccInt test_RandomIndex(view *);
void test_RandomWindow(ccInt);
void StepToIndex(ccInt, ccInt);

typedef struct {
    ccInt *referencePtr;
    ccInt man_refs;
    ccInt auto_refs;
} TestVarType;

typedef struct {
    void *ptr;
    ccInt refs;
} TestRefType;

TestVarType DummyTVT;        // in case we can't find the real element (shouldn't happen)
linkedlist CompareRefs;        // used by test_NewDelVar and test_NDV_CountRefs; of type TestVarType
extern ccInt rndm(ccInt);
extern void randomize(ccInt);
extern ccInt rseed, start_seed;

window *dbg2 = NULL;



// *************  Routines to test the referencing in VariableList, MasterCodeList **************


// Tests the memory routines.
// Directly tests:  searchMember(), StepToWindowMember(), Push/PopPCPath(), initCicada(), cleanUp(), addVariable(), Ref/derefWindow(), addWindow(), combVariables(), addMember(), ref/derefPath(), addCodeRef()
// Indirectly tests (routines called by the above):  StepWindow(), FindMemberID(), findMemberIndex(), setError(), addVariable(), Ref/DerefVariable(), CombBranch(), AddFibril(), Ref/Deref_CodeRef(), AddMemory(), AdjustOffsets_And_IW(), AdjustFibrilStemOffsets, UnflagVariables().
// For now, does NOT test DeleteMember(), ModifyMembers(), SplitMember(), MoveWindow(), CheckWindowOverlap().  (Of those, only the last two are likely to survive in Cicada).

void test_ModMem()
{
    ccInt counter, fill_top, MemberIDCounter, NullCode = 0, rtrn;
    const ccInt variable_depth = 4;
    bool passed_array_test;
    
    MemberIDCounter = 1;
    
    rtrn = initCicada();
    if (rtrn == passed)  rtrn = attachStartingCode(&NullCode, 0, NULL, 0, NULL, NULL, 0);
    if (rtrn != passed)  {  printf("ModMem:  Init returned %i  (%i)\n", (int) rtrn, (int) start_seed);  return;  }
    
    baseView.windowPtr = Zero;
    baseView.offset = 0;
    baseView.width = 1;
    
    pcSearchPath = ZeroSuspensor;
    
    refWindow(baseView.windowPtr);
    refPath(pcSearchPath);
    
        // First, set up something like NewWindow[a][b][c][d][e] := int
    
    errCode = passed;
    
    test_SetUpArray(1, variable_depth, &MemberIDCounter);
    
        // Now, fill the array with consecutive integers.
    
    fill_top = 1;
    baseView.offset = 0;
    searchView = baseView;
    
    rtrn = test_FillArray(&fill_top);
    if (rtrn != passed)  return;
    
    counter = 1;
    baseView.offset = 0;
    searchView = baseView;
    
    passed_array_test = true;
    rtrn = test_TestArray(&counter, 0, &passed_array_test);
    if (rtrn != passed)    {  printf("ModMem:  TestArray (1) returned prematurely  (%i)\n", (int) start_seed);  return;  }
    if (counter != fill_top)
            {  printf("ModMem:  TestArray (1) counter = %i; fill_top = %i  (%i)\n", (int) counter, (int) fill_top, (int) start_seed);  return;  }
    
    rtrn = test_SwapDelMembers(variable_depth);
    if (counter != fill_top)  {  printf("ModMem:  SDM returned prematurely  (%i)\n", (int) start_seed);  return;  }
    test_MM_CountRefs();
    
    combVariables();
    test_MM_CountRefs();
    
    cleanUp();
    
    return;
}

// The windows actually go to a depth of MaxDepth+1; we only link new members up to MaxDepth (the last row of windows belong to primitive vars).

void test_SetUpArray(ccInt Depth, ccInt MaxDepth, ccInt *MemberIDCounter)
{
    variable *NewVar;
    window *NewWindow;
    view holdBaseView;
    member *NewMember, *SoughtMember;
    ccInt NewIndices, dummy_member, MemberEntry, EntryOffset, rtrn;
    
    randomize(13);
    NewIndices = rndm(7)+1;
    if (Depth == 1)  NewIndices = 1;
    
    if (Depth < MaxDepth)    {
        rtrn = addVariable(&NewVar, array_type, int_type, MaxDepth-Depth, true);
        if (rtrn != passed)  {  printf("ModMem:  AddCV returned %i  (%i)\n", (int) rtrn, (int) start_seed);  return;  }        }
    else    {
        rtrn = addVariable(&NewVar, int_type, int_type, 0, true);
        if (rtrn != passed)  {  printf("ModMem:  AddCV returned %i  (%i)\n", (int) rtrn, (int) start_seed);  return;  }        }
    
    dummy_member = baseView.windowPtr->variable_ptr->mem.members.elementNum + 1;
    rtrn = addMember(baseView.windowPtr->variable_ptr, dummy_member, NewIndices, &NewMember, false, 1, true);
    if (rtrn != passed)  {  printf("ModMem:  addMember returned %i  (%i)\n", (int) rtrn, (int) start_seed);  return;  }
    rtrn = addWindow(NewVar, NewVar->instances, NewIndices*baseView.width, &NewWindow, true);
    if (rtrn != passed)  {  printf("ModMem:  addWindow returned %i  (%i)\n", (int) rtrn, (int) start_seed);  return;  }
    refWindow(NewWindow);
    (LL_member(baseView.windowPtr->variable_ptr, baseView.windowPtr->variable_ptr->mem.members.elementNum))->memberWindow = NewWindow;
    
    searchView = baseView;
    holdBaseView = baseView;
    
    rtrn = findMemberIndex(searchView.windowPtr->variable_ptr, 0, 1, &SoughtMember, &MemberEntry, &EntryOffset, false);
    if (rtrn != passed)  {  setError(rtrn, pcCodePtr);  return;  }
    
    stepView(&searchView, SoughtMember, EntryOffset, NewIndices);
    
    if (errCode != passed)  {  printf("SetUpArray:  stepView returned %i  (%i)\n", (int) errCode, (int) start_seed);  return;  }
    derefWindow(&(baseView.windowPtr));
    
    baseView = searchView;
    refWindow(baseView.windowPtr);
    
    rtrn = addCodeRef(&(baseView.windowPtr->variable_ptr->codeList), pcSearchPath, storedCode(1)->bytecode, 1);
    if (rtrn != passed)  {  printf("ModMem:  addCodeRef returned %i  (%i)\n", (int) rtrn, (int) start_seed);  return;  }
    
    if (Depth < MaxDepth)  test_SetUpArray(Depth+1, MaxDepth, MemberIDCounter);
    
    derefWindow(&(baseView.windowPtr));
    
    baseView = holdBaseView;
    
    refWindow(baseView.windowPtr);
    
    NewMember->memberID = *MemberIDCounter;
    (*MemberIDCounter)++;
    
    if (Depth < MaxDepth)    NewMember->type = array_type;
    else    NewMember->type = int_type;
    
    return;
}

// Fills an array (composite variables of composite variables of ... of primitive ints) with consecutive integers.  (No recursive links allowed).
// Array must be in this[1].
ccInt test_FillArray(ccInt *fill_counter)
{
    window *holdSVwindow;
    member *loopMember;
    ccInt member_counter, counter, index_bottom, holdSVoffset, rtrn;
    
    index_bottom = 0;
    
    if (searchView.windowPtr->variable_ptr->type == int_type)    {
        *LL_int(&(searchView.windowPtr->variable_ptr->mem.data), searchView.offset+1) = *fill_counter;
        (*fill_counter)++;
        return passed;        }
    
    for (member_counter = 1; member_counter <= searchView.windowPtr->variable_ptr->mem.members.elementNum; member_counter++)    {
        loopMember = LL_member(searchView.windowPtr->variable_ptr, member_counter);
        for (counter = 1; counter <= loopMember->indices; counter++)    {
            
            holdSVwindow = searchView.windowPtr;
            holdSVoffset = searchView.offset;
            StepToIndex(counter+index_bottom, 1);
            if (errCode != passed)
                {  printf("FillArray:  StepToIndex returned %i (%i)\n", (int) errCode, (int) start_seed);  return errCode;  }
            
            rtrn = test_FillArray(fill_counter);
            if (rtrn != passed)  return rtrn;
            
            searchView.windowPtr = holdSVwindow;
            searchView.offset = holdSVoffset;
        }
        index_bottom += loopMember->indices;
    }
    
    return passed;
}

// Adds the baseView to the PCStack stack.  Refs the new baseView and pcSearchPath for convenience, since presumably they were just set.
ccInt PushPCPath(window *NewPCWindow, searchPath *NewPCsearchPath, ccInt newOffset)
{
    ccInt rtrn;
    void *NewStackTop;
    
    rtrn = pushStack(&PCStack, &NewStackTop);
    if (rtrn != passed)  return rtrn;
    *(view *) NewStackTop = baseView;
    
    baseView.windowPtr = NewPCWindow;
    baseView.offset = newOffset;
    pcSearchPath = NewPCsearchPath;
    
    refWindow(baseView.windowPtr);        // the code doesn't have to be referenced, since pcSearchPath references the code
    refPath(pcSearchPath);
    
    return passed;
}

// Adds the window in baseView to the PCStack stack.
// Loads the previous baseView, etc. -- never blows the stack since we don't bother popping PC_0
// in the same way when we close out.  Dereferences the old baseView and pcSearchPath.
void PopPCPath(searchPath *oldSearchPath)
{
    void *OldStackTop;
    
    derefWindow(&(baseView.windowPtr));        // the code doesn't have to be referenced, since pcSearchPath references the code
    derefPath(&pcSearchPath);
    
    popStack(&PCStack, &OldStackTop);
    baseView = *(view *) OldStackTop;
    
    pcSearchPath = oldSearchPath;
    pcSearchPath->variablePtr = baseView.windowPtr->variable_ptr;
    pcSearchPath->stemIndices = baseView.width;
    
    return;
}

// Tests to make sure the numbers in the array are still in order (they have been somewhat convoluted by splitting members).
ccInt test_TestArray(ccInt *fill_counter, ccInt ThisMemberID, bool *passed_test)
{
    variable *baseVar = baseView.windowPtr->variable_ptr;
    member *loopMember, *NewMember;
    ccInt member_counter, counter, index_bottom, NewMemberID, NewMemberNumber, *HoldPCCodePtr, rtrn;

    index_bottom = 0;
    
    if (baseVar->type == int_type)    {
        if (*LL_int(&(baseVar->mem.data), searchView.offset+1) != *fill_counter)    {
            *passed_test = false;
            printf("TestArray:  Comparison Failed at %i -- read %i  (%i)\n", (int) *fill_counter,
                                    (int) *LL_int(&(baseVar->mem.data), baseView.offset+1), (int) start_seed);
            return passed;        }        // not a contradiction -- no error code
        (*fill_counter)++;
        return passed;        }
    
    for (member_counter = 1; member_counter <= baseVar->mem.members.elementNum; member_counter++)    {
        loopMember = LL_member(baseVar, member_counter);
        NewMemberID = loopMember->memberID;
        for (counter = 1; counter <= loopMember->indices; counter++)    {
            searchPath newSearchPath, *oldSearchPath = pcSearchPath;
            ccInt dummyRefs;
            searchMember(((member *) findElement(&(baseVar->mem.members), member_counter))->memberID,
                                                                                        &NewMember, &NewMemberNumber, false, false);
            if (errCode != passed)
                    {  printf("TestArray:  searchMember returned %i (%i)\n", (int) errCode, (int) start_seed);  return errCode;  }
            stepView(&searchView, NewMember, counter-1, 1);
            
            newSearchPath.variablePtr = searchView.windowPtr->variable_ptr;
            newSearchPath.stemIndices = searchView.width;
            newSearchPath.references = &dummyRefs;
            newSearchPath.stem = NULL;
            newSearchPath.sourceCode = 1;
            newSearchPath.jamb = NewMember->memberWindow;
            
            rtrn = PushPCPath(NewMember->memberWindow, &newSearchPath, searchView.offset);
            if (rtrn != passed)  {  printf("TestArray:  PPPE returned %i (%i)\n", (int) errCode, (int) start_seed);  return errCode;  }
            HoldPCCodePtr = pcCodePtr;
            
            if (errCode != passed)
                    {  printf("TestArray:  StepToIndex returned %i (%i)\n", (int) errCode, (int) start_seed);  return errCode;  }
            
            rtrn = test_TestArray(fill_counter, NewMemberID, passed_test);
            if (rtrn != passed)  return rtrn;
            if (!(*passed_test))  return passed;
            
            pcCodePtr = HoldPCCodePtr;
            PopPCPath(oldSearchPath);
        }
        index_bottom += loopMember->indices;
    }
    
    return passed;
}

// Randomly sets the members of some variables to other random variables (unlinking the former targets), and also simply deletes members.
// These must be proxies for this to work.
ccInt test_SwapDelMembers(ccInt InitialDepth)
{
    window *holdWindowPtr;
    member *MemberToLink;
    ccInt counter, MaxLoops = rndm(20), WindowToLink, HoldSearchOffset;

    for (counter = 1; counter <= MaxLoops; counter++)    {
        test_RandomWindow(InitialDepth);
        if (errCode != passed)    {  printf("SwapDelMembers:  TRB bailed out (%i)\n", (int) start_seed);  return errCode;  }
        test_RandomWindow(InitialDepth-2);
        if (errCode != passed)    {  printf("SwapDelMembers:  TRB (2) bailed out (%i)\n", (int) start_seed);  return errCode;  }
        holdWindowPtr = searchView.windowPtr;
        
        if (holdWindowPtr->variable_ptr->type == array_type)    {        // a composite variable that points, hopefully, to a proxy
            WindowToLink = rndm(searchView.windowPtr->variable_ptr->mem.members.elementNum)+1;
            MemberToLink = LL_member(searchView.windowPtr->variable_ptr, WindowToLink);
            HoldSearchOffset = searchView.offset;
            stepView(&searchView, MemberToLink, rndm(MemberToLink->indices)+1, 1);
            
            randomize(7);
            searchView.offset = HoldSearchOffset;
            searchView.windowPtr = holdWindowPtr;
    }   }
        
    return passed;
}

void test_MM_CountRefs()
{
    linkedlist RefsList;
    ccInt counter, c2, c3, rtrn;
    variable *theVariable;
    window *theWindow;
    searchPath *theFibril;
    member *theMember;
    code_ref *theCodeRef;
    
    rtrn = newLinkedList(&RefsList, 0, sizeof(TestRefType), 2.55, false);
    if (rtrn != passed)  {  printf("MM_CountRefs:  NewLL returned %i (%i)\n", (int) rtrn, (int) start_seed);  return;  }
    
        // First, set up the refs list
    
    for (counter = 1; counter <= VariableList.data.elementNum; counter++)    {
    if (*LL_int(&(VariableList.references), counter) != 0)    {
        theVariable = (variable *) findElement(&(VariableList.data), counter);
        for (c2 = 1; c2 <= theVariable->windows.data.elementNum; c2++)    {
        if (*LL_int(&(theVariable->windows.references), c2) != 0)    {
            theWindow = (window *) findElement(&(theVariable->windows.data), c2);
            test_MMCR_AddRef(&RefsList, (void *) theWindow->variable_ptr);
        }}
        
        for (c2 = 1; c2 <= theVariable->pathList.data.elementNum; c2++)    {
        if (*LL_int(&(theVariable->pathList.references), c2) != 0)    {
            theFibril = (searchPath *) findElement(&(theVariable->pathList.data), c2);
            test_MMCR_AddRef(&RefsList, (void *) storedCode(theFibril->sourceCode)->bytecode);
            test_MMCR_AddRef(&RefsList, (void *) theFibril->stem);
            test_MMCR_AddRef(&RefsList, (void *) theFibril->jamb);
        }}
        
        if ((theVariable->type == composite_type) || (theVariable->type == array_type) || (theVariable->type == string_type))    {
        for (c2 = 1; c2 <= theVariable->mem.members.elementNum; c2++)    {
            theMember = (member *) findElement(&(theVariable->mem.members), c2);
            test_MMCR_AddRef(&RefsList, (void *) theMember->memberWindow);
            for (c3 = 1; c3 <= theMember->codeList.elementNum; c3 ++)    {
                test_MMCR_AddRef(&RefsList, (void *) storedCode(((code_ref *) findElement(&(theMember->codeList), c3))->PLL_index)->bytecode);
        }}  }
        
        for (c2 = 1; c2 <= theVariable->codeList.elementNum; c2++)    {
            theCodeRef = (code_ref *) findElement(&(theVariable->codeList), c2);
            test_MMCR_AddRef(&RefsList, (void *) storedCode(theCodeRef->PLL_index)->bytecode);
            test_MMCR_AddRef(&RefsList, (void *) theCodeRef->anchor);
    }}  }
    
    for (counter = 1; counter <= codeRegister.elementNum; counter++)    {
        theCodeRef = (code_ref *) findElement(&codeRegister, counter);
        test_MMCR_AddRef(&RefsList, (void *) storedCode(theCodeRef->PLL_index)->bytecode);
        test_MMCR_AddRef(&RefsList, (void *) theCodeRef->anchor);    }
    
    test_MMCR_AddRef(&RefsList, (void *) Zero);
    test_MMCR_AddRef(&RefsList, (void *) ZeroSuspensor);
    test_MMCR_AddRef(&RefsList, (void *) baseView.windowPtr);
    test_MMCR_AddRef(&RefsList, (void *) pcSearchPath);
    
    for (counter = 1; counter <= PCStack.top; counter++)        {
        test_MMCR_AddRef(&RefsList, (void *) ((view *) element(&(PCStack.data), counter))->windowPtr);        }
    
    if (errScript.code_ptr != 0)  {
        test_MMCR_AddRef(&RefsList, errScript.anchor);
        test_MMCR_AddRef(&RefsList, storedCode(errScript.PLL_index)->bytecode);    }
    
    if (warningScript.code_ptr != 0)  {
        test_MMCR_AddRef(&RefsList, warningScript.anchor);
        test_MMCR_AddRef(&RefsList, storedCode(warningScript.PLL_index)->bytecode);    }
    
    
        // Now, check the numbers of references
    
    for (counter = 1; counter <= VariableList.data.elementNum; counter++)    {
    if (*LL_int(&(VariableList.references), counter) != 0)    {
        theVariable = (variable *) findElement(&(VariableList.data), counter);
        test_MMCR_TestRef(&RefsList, (void *) theVariable, *(theVariable->references), "variable");
        
        for (c2 = 1; c2 <= theVariable->windows.data.elementNum; c2++)    {
        if (*LL_int(&(theVariable->windows.references), c2) != 0)    {
            theWindow = (window *) findElement(&(theVariable->windows.data), c2);
            test_MMCR_TestRef(&RefsList, (void *) theWindow, *(theWindow->references), "window");
        }}
        
        for (c2 = 1; c2 <= theVariable->pathList.data.elementNum; c2++)    {
        if (*LL_int(&(theVariable->pathList.references), c2) != 0)    {
            theFibril = (searchPath *) findElement(&(theVariable->pathList.data), c2);
            test_MMCR_TestRef(&RefsList, (void *) theFibril, *(theFibril->references), "path");
    }}  }}
    
    for (counter = 1; counter <= MasterCodeList.data.elementNum; counter++)    {
    if (*LL_int(&(MasterCodeList.references), counter) != 0)  {
        test_MMCR_TestRef(&RefsList, (void *) storedCode(counter)->bytecode, *LL_int(&(MasterCodeList.references), counter), "code list");
    }}
    
    for (counter = 1; counter <= RefsList.elementNum; counter++)  {       // flag all the pointers whose objects we couldn't find
        test_MMCR_TestRef(&RefsList, (void *) ((TestRefType *) element(&RefsList, counter))->ptr, 0, "unknown");      }
    
    deleteLinkedList(&RefsList);
    
    return;
}

void test_MMCR_AddRef(linkedlist *theList, void *thePtr)
{
    ccInt counter, rtrn;
    
    if (thePtr == 0)  return;
    
    for (counter = 1; counter <= theList->elementNum; counter++)    {
    if (((TestRefType *) findElement(theList, counter))->ptr == thePtr)    {
        (((TestRefType *) findElement(theList, counter))->refs)++;
        return;
    }}
    
    rtrn = addElements(theList, 1, false);
    if (rtrn != passed)  {  printf("MMCR_AddRef:  AddEl returned %i (%i)\n", (int) rtrn, (int) start_seed);  return;  }
    
    ((TestRefType *) findElement(theList, theList->elementNum))->ptr = thePtr;
    ((TestRefType *) findElement(theList, theList->elementNum))->refs = 1;
    return;
}

void test_MMCR_TestRef(linkedlist *theList, void *thePtr, ccInt refsnum, char *pointerType)
{
    ccInt counter;

    for (counter = 1; counter <= theList->elementNum; counter++)    {
        if (((TestRefType *) findElement(theList, counter))->ptr == thePtr)    {
            if (((TestRefType *) findElement(theList, counter))->refs != refsnum)    {
                printf("MMCR_TestRef:  %s ptr %p: Counted %i refs; refs = %i (%i)\n", pointerType, thePtr,
                        (int) ((TestRefType *) findElement(theList, counter))->refs, (int) refsnum, (int) start_seed);    }
            deleteElement(theList, counter);
            return;
    }    }
    
    printf("MMCR_TestRef:  %s ptr %p: Counted 0 refs; refs = %i (%i)\n", pointerType, thePtr, (int) refsnum, (int) start_seed);
    
    return;
}

ccInt test_RandomIndex(view *CurrentView)
{
    ccInt counter, TotalIndices = 0;
    
    for (counter = 1; counter <= CurrentView->windowPtr->variable_ptr->mem.members.elementNum; counter++)
        TotalIndices += (LL_member(CurrentView->windowPtr->variable_ptr, counter))->indices;
    
    if (TotalIndices == 0)  return 0;
    else  return rndm(TotalIndices)+1;
}

void test_RandomWindow(ccInt MaxDepth)
{
    member *RandomMember;
    ccInt loopDepth, RandomMemberNumber, IndexToStepInto, Depth = rndm(MaxDepth)+1;
    
    searchMember((LL_member(Zero->variable_ptr, 1))->memberID, &RandomMember, &RandomMemberNumber, false, false);
    if (errCode != passed)
        {  printf("TRV:  searchMember returned %i  (%i)\n", (int) errCode, (int) start_seed);  return;  }

    stepView(&searchView, RandomMember, 0, 1);
    
    for (loopDepth = 1; loopDepth <= Depth; loopDepth++)    {            // go to a random 'depth', at least
    if (searchView.windowPtr->variable_ptr->type == array_type)    {
        IndexToStepInto = test_RandomIndex(&searchView);
        StepToIndex(IndexToStepInto, 1);
        if (errCode != passed)
            {  printf("TRV:  StepToIndex returned %i  (%i)\n", (int) errCode, (int) start_seed);  return;  }
    }}
    
    return;
}



// Steps to a specified index of a composite variable; updates searchView.windowPtr, searchView.width, and searchView.offset.
// (Those must be reset when it's time to come back).  SoughtIndex is the index number in searchView.
void StepToIndex(ccInt SoughtIndex, ccInt IndexWidth)
{
    member *SoughtMember;
    ccInt MemberEntry, EntryOffset, rtrn;
    
    rtrn = findMemberIndex(searchView.windowPtr->variable_ptr, 0, SoughtIndex, &SoughtMember, &MemberEntry, &EntryOffset, false);
    if (rtrn != passed)  {  setError(rtrn, pcCodePtr);  return;  }

    stepView(&searchView, SoughtMember, EntryOffset, IndexWidth);
    
    return;
}
