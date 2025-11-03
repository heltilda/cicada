/*
 *  intrpt.h
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

#ifndef Interpret_h
#define Interpret_h

#include "lnklst.h"
#include "intrpt.h"



// flags used in the "business" field of objects & windows

#define busy_SS_flag (1<<0)
#define busy_source_copy_flag (1<<1)   /* windows */
#define busy_dest_copy_flag (1<<2)     /* windows */
#define busy_comb_flag (1<<3)          /* o & w */
#define busy_add_flag (1<<4)           /* objects */
#define busy_overlap_flag (1<<5)       /* windows */
#define busy_SRW_flag (1<<6)           /* windows */
#define busy_resize_flag (1<<7)        /* windows */

#define isBusy(a,b) ((a->business) & (b))
#define setBusy(a,b) ((a->business) = (unsigned int) ((a->business) | (unsigned int) (b)))
#define clearBusy(a,b) ((a->business) = (unsigned int) ((a->business) & (~(unsigned int) (b))))


// jam status

#define cannot_jam 0
#define can_jam 1
#define unjammed 2


// flags used by checkMemberOverlap()

#define cbo_set_flags 1
#define cbo_check 2
#define cbo_unjam 3
#define cbo_unset_flags 4


// Structures forming Cicada's runtime memory


// variable:  contains either physical storage (primitive variable), or a list of members (composite)

typedef struct {
    ccInt PLL_index;        // element_number in variable PLL
    ccInt *references;      // repeated here for speed; # of windows + variable_ptrs
    
    union {
        linkedlist members;
        linkedlist data;
    } mem;
    
    pinned_LL windows;      // one path list for each instance of this variable
    pinned_LL pathList;
    
    ccInt type;             // for primitive types
    ccInt eventualType;     // if type is array_type, the type of the object that the array ranges over
    ccInt arrayDepth;       // array dimension spanned by this member (not the whole array) 
    linkedlist codeList;    // of type code_ref
    
    ccInt instances;        // how many independent instances of this object are allocated (not counting =@)
                            // for primitive types, = data.elementNum
    int business;
} variable;


// window:  represents a region of a variable accessible by a member in (usually) another variable

typedef struct {
    ccInt PLL_index;        // element_number in window PLL
    ccInt *references;      // repeated here for speed; # of paths + member refs
    
    variable *variable_ptr; // supporting this window
    ccInt offset;           // the offset (starting at 0) for this window in the LL or portion of subsequent LLs used by this var
    ccInt width;            // CUMULATIVE -- #_of of immediate predecessor times #_of of the one before, ...
    
    char jamStatus;         // if this window can jam the object to prevent resizes
    int business;           // to use when checking to see if a window is part of an 'island'
} window;


// member:  points from one variable to another (window)

typedef struct {
    window *memberWindow;
    
    ccInt memberID;         // the name; 0 if cannot be found by step_to_member_ID (i.e. no name in Cicada code)
    ccInt indices;          // 1 unless it's an array member
    
    ccInt type;             // the requirements for this member, which may be met and exceeded in the variable
    ccInt eventualType;     // if type is array_type, the type of the object that the array ranges over
    ccInt arrayDepth;       // array dimension spanned by this member (not the whole array) 
    linkedlist codeList;    // parallels the codeList in the targeted variable, but may be less restrictive
    
    bool ifHidden;        // hidden members don't have indices associated with them
    int business;           // to use when checking to see if a window is part of an 'island'
} member;


// searchPath:  a step on a search path

typedef struct path_temp {
    ccInt PLL_index;        // element_number in path PLL
    ccInt *references;      // repeated here for speed; # of times used as stem or data anchor, or path anchor
    
    variable *variablePtr;  // points to this variable
    window *jamb;           // the window pointer
    
    struct path_temp *stem; // the path that this came from
    ccInt stemIndices;      // the number of the indices of the stem-member that leads here
    
    ccInt sourceCode;       // the global code PLL ID that was being run when this path was created -- not the code_ref entry
} searchPath;


// view:  a subset of a window (for example, the current search path)

typedef struct {
    window *windowPtr;      // window that the view is taken from
    ccInt offset;           // additional offset (starting at 0) on top of the window offset
    ccInt width;            // number of the window's elements being viewed
    bool multipleIndices; // true if and only if a [*] or [<..>] operator was used (even if width == 1)
} view;


// code_ref:  an element in a variable or member code list

typedef struct {
    ccInt PLL_index;        // element_number in code PLL
    ccInt *references;      // repeated here for speed; # of times used by a variable
    ccInt *code_ptr;        // the first code word of this script (which may be in the middle of a compiled code block)
    searchPath *anchor;     // the first step on the search path from this code
} code_ref;


typedef struct {
    char *fileName;
    char *sourceCode;
    ccInt *bytecode;
    ccInt *opCharNum;
    ccInt compilerID;
} storedCodeType;



// Globals

typedef struct {
    pinned_LL _MasterCodeList;   // pointers to blocks of bytecode that store code
    pinned_LL _VariableList;     // master list containing all variables
    stack _PCStack;              // for storing the old baseView, searchView during function calls
} cc_interpret_global_struct;

extern cc_interpret_global_struct cc_interpret_globals;

#define MasterCodeList cc_interpret_globals._MasterCodeList
#define VariableList cc_interpret_globals._VariableList
#define PCStack cc_interpret_globals._PCStack


#define LL_member(a,b) ((member *) element(&(a->mem.members),b))
#define storedCode(b) ((storedCodeType *) element(&(cc_interpret_globals._MasterCodeList.data),b))



// Prototypes

#ifdef __cplusplus
extern "C" {
#endif

extern const ccFloat LLFreeSpace;

extern void copyWindowData(view *, view *);
extern void copyData(linkedlist *, ccInt, linkedlist *, ccInt, ccInt);
extern void copyString(window *, member *);
extern void copyBoolToBool(void *, void *);
extern void copyCharToChar(void *, void *);
extern void copyCharToInt(void *, void *);
extern void copyCharToDouble(void *, void *);
extern void copyIntToChar(void *, void *);
extern void copyIntToInt(void *, void *);
extern void copyIntToDouble(void *, void *);
extern void copyDoubleToChar(void *, void *);
extern void copyDoubleToInt(void *, void *);
extern void copyDoubleToDouble(void *, void *);
extern void copyStringToString(void *, void *);
extern void(*copyJumpTable[])(void *, void *);
extern void compareWindowData(view *, view *);
extern void compareData(linkedlist *, ccInt, linkedlist *, ccInt, ccInt);
extern void compareString(window *, member *);
extern void compareBoolToBool(void *, void *);
extern void compareCharToChar(void *, void *);
extern void compareCharToInt(void *, void *);
extern void compareCharToDouble(void *, void *);
extern void compareIntToChar(void *, void *);
extern void compareIntToInt(void *, void *);
extern void compareIntToDouble(void *, void *);
extern void compareDoubleToChar(void *, void *);
extern void compareDoubleToInt(void *, void *);
extern void compareDoubleToDouble(void *, void *);
extern void compareStringToString(void *, void *);
extern void setTMerror(void *, void *);
extern void(*compareJumpTable[])(void *, void *);
extern void doCopyCompare(view *, view *,
    void(*)(view *, view *),
    void(*)(linkedlist *, ccInt, linkedlist *, ccInt, ccInt),
    void(*)(window *, member *),
    void(**)(void *, void *));
extern void findNextVisibleMember(variable *, member **, ccInt *);
extern void copyCompareListToVar(void *, ccInt, view *, void(**)(void *, void *));

extern void sizeView(view *, void *, void *);
extern void sizeData(view *, void *, void *);
extern void sizeString(view *, ccInt, void *, void *);
extern void storageSizeView(view *, void *, void *);
extern void storageSizeData(view *, void *, void *);
extern void storageSize_String(view *, ccInt, void *, void *);
extern void writeView(view *, void *, void *);
extern void writeData(view *, void *, void *);
extern void writeString(view *, ccInt, void *, void *);
extern void readView(view *, void *, void *);
extern void readData(view *, void *, void *);
extern void readString(view *, ccInt, void *, void *);
extern void printViewString(view *, void *, void *);
extern void printDataString(view *, void *, void *);
extern void sizeViewString(view *, void *, void *);
extern void sizeDataString(view *, void *, void *);
extern void readViewString(view *, void *, void *);
extern void readDataString(view *, void *, void *);
extern void readStringFromString(view *, ccInt, void *, void *);
extern void countDataLists(view *, void *, void *);
extern void countDataView(view *, void *, void *);
extern void countStringView(view *, ccInt, void *, void *);
extern void argvFillHandles(view *, void *, void *);
extern void passDataView(view *, void *, void *);
extern void passString(view *, ccInt, void *, void *);
extern void argvFixStrings(view *, void *, void *);
extern void fixNothing(view *, void *, void *);
extern void fixString(view *, ccInt, void *, void *);
extern void printView(view *, void *, void *);
extern void printData(view *, void *, void *);
extern void printString(view *, ccInt, void *, void *);
extern char hexDigit(unsigned char);
extern void doReadWrite(view *, void *, void *, bool, bool, bool,
    void(*)(view *, void *, void *),
    void(*)(view *, void *, void *),
    void(*)(view *, ccInt, void *, void *));
extern void printNumber(char *, const ccFloat, ccInt *, const ccInt, const ccInt);

extern void searchMember(ccInt, member **, ccInt *, bool, bool);
extern ccInt findMemberID(variable *, ccInt, member **, ccInt *, bool, bool);
extern ccInt findMemberIndex(variable *, ccInt, ccInt, member **, ccInt *, ccInt *, bool);
extern ccInt numMemberIndices(view *);
extern void stepView(view *, member *, ccInt, ccInt);

extern void setError(ccInt, ccInt *);
extern void setWarning(ccInt, ccInt *);
extern void setErrIndex(ccInt *, ccInt, code_ref *, ccInt *, ccInt *, code_ref *);

extern ccInt addVariable(variable **, ccInt, ccInt, ccInt, bool);
extern void refVariable(variable *);
extern void derefVariable(variable *);
extern ccInt addWindow(variable *, ccInt, ccInt, window **, bool);
extern void refWindow(window *);
extern void derefWindow(window **);
extern void combVariables(void);
extern void combBranch(variable *, bool);
extern void unlinkWindow(variable *, window **, ccInt);
extern ccInt addMember(variable *, ccInt, ccInt, member **, bool, const ccInt, const bool);
extern void removeMember(variable *, ccInt);
extern void refPath(searchPath *);
extern void derefPath(searchPath **);
extern ccInt drawPath(searchPath **, window *, searchPath *, ccInt, ccInt);
extern ccInt addCode(ccInt *, ccInt **, ccInt *, ccInt, const char *, ccInt, const char *, ccInt *, ccInt, const ccInt);
extern void refCode(ccInt *);
extern void derefCode(ccInt *, ccInt);
extern ccInt addCodeRef(linkedlist *, searchPath *, ccInt *, ccInt);
extern void refCodeRef(code_ref *);
extern void derefCodeRef(code_ref *);
extern ccInt checkMemberOverlap(window *, ccInt, ccInt, ccInt);
extern ccInt addMemory(window *, ccInt, ccInt);
extern void adjustOffsetAndIW(bool, ccInt *, ccInt *, ccInt, ccInt);
extern void unflagVariables(variable *, unsigned char);
extern void unflagWindow(window *, unsigned char);

extern ccInt align(ccInt);

extern const ccInt typeSizes[];

#ifdef __cplusplus
}
#endif

#endif
