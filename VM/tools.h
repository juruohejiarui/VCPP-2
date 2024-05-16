#ifndef __TOOLS_H__
#define __TOOLS_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef unsigned long long uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef long long int64;
typedef int int32;
typedef short int16;
typedef char int8;
typedef float float32;
typedef double float64;
typedef short SBool;

#define memberOffset(s, m) ((size_t)(& ((s*)0)->m ))

#define true 1
#define false 0

#define mallocObj(type) ((type *)malloc(sizeof(type)))
#define mallocArray(type, size) ((type *)malloc(sizeof(type) * (size)))

uint32 calcHash(const char *str, uint32 range);

typedef struct tmpPair {
    size_t keyLength;
    char *key;
    void *value;
} Pair;
/// @brief allocate a pair
/// @param key the key of this pair
/// @param value the value of this pair
/// @return the pair
Pair *makePair(char *key, void *value);

typedef struct tmpPairListElement {
    Pair *content;
    struct tmpPairListElement *prev, *next;
} PairListElement;

/// @brief Initialize a list using the pointers START and END
/// @param start 
/// @param end 
void PairList_init(PairListElement *start, PairListElement *end);
/// @brief Insert the ELE in front of POS
/// @param pos 
/// @param ele 
void PairList_insert(PairListElement *pos, PairListElement *ele);
/// @brief Remove the ELE from the list that ELE currently belongs to (this action will not free the memory of ELE)
/// @param ele 
void PairList_remove(PairListElement *ele);

typedef struct tmpListElement {
    void *content;
    struct tmpListElement *prev, *next;
} ListElement;

/// @brief Initialize a list using the pointers START and END
/// @param start 
/// @param end 
void List_init(ListElement *start, ListElement *end);
/// @brief Insert the ELE in front of POS
/// @param pos 
/// @param ele 
void List_insert(ListElement *pos, ListElement *ele);
/// @brief Remove the ELE from the list that ELE currently belongs to (this action will not free the memory of ELE)
/// @param ele 
void List_remove(ListElement *ele);
/// @brief Clear the list and free the elements of this list, action will not free the memory of content
/// @param start 
/// @param end 
void List_clear(ListElement *start, ListElement *end);


typedef struct tmpHashMap {
    uint32 hashRange;
    PairListElement **listStart, **listEnd;
} HashMap;

/// @brief Initialize MAP using RANGE
/// @param map
/// @param range 
void HashMap_init(HashMap *map, uint32 range);
/// @brief Insert PIR into MAP, if the key exists, the key in PIR and PIR itself will be free
/// @param map 
/// @param pir 
void HashMap_insert(HashMap *map, Pair *pir);
/// @brief Get the pair whose key is STR from MAP, if the key does not exist, then return NULL
/// @param map 
/// @param key 
/// @return 
Pair *HashMap_find(HashMap *map, const char *key);
/// @brief Erase the pair whose key is STR, and free the memory of the key, the pair itself (will not free the value) and the element
/// @param map 
/// @param key
void HashMap_erase(HashMap *map, const char *key);

typedef struct tmpTrieNode {
    void *content;
    struct tmpTrieNode *child[128];
} TrieNode;

/// @brief Initialize the Trie
/// @param root 
void Trie_init(TrieNode *root);
/// @brief Insert STR and the VAL into a trie whose root is ROOT
/// @param root 
/// @param str 
/// @param value 
void Trie_insert(TrieNode *root, const char *str, void *val);
/// @brief get the value where the str is in the trie whose root is ROOT, return NULL if STR does not exist.
/// @param root 
/// @param str 
/// @return 
void *Trie_get(TrieNode *root, const char *str);

typedef struct tmpDArray {
    uint8 *data;
    uint64 len, cap;
    uint64 elemSize;
} DArray;

void DArray_init(DArray *arr, uint64 elemSize);
void DArray_clear(DArray *arr);
void DArray_push(DArray *arr, void *elem);
void DArray_merge(DArray *src, DArray *dst);
void DArray_pop(DArray *arr);
void *DArray_get(DArray *arr, uint64 index);

#define readData(filePtr, dataPtr, dataType) (fread(dataPtr, (sizeof(dataType)), 1, (filePtr)))
#define readArray(filePtr, dataPtr, dataType, arraySize) (fread(dataPtr, sizeof(dataType), (arraySize), filePtr))
#define writeData(filePtr, dataPtr, dataType) (fwrite((dataPtr), (sizeof(dataType)), 1, (filePtr)))

/// @brief read a string from the file, and create a memory block to store the string
/// @param filePtr 
/// @return 
char *readString(FILE *filePtr);
void writeString(FILE *filePtr, const char *str);
void ignoreString(FILE *filePtr);

#define min(a, b) ((a) > (b) ? (b) : (a))
#define max(a, b) ((a) > (b) ? (a) : (b))

enum DataTypeModifier {
    i8, u8, i16, u16, i32, u32, i64, u64, f32, f64, o, gv0, gv1, gv2, gv3, gv4, dtMdfUnknown,
};
enum ValueTypeModifier {
    MemberRef, VarRef, TrueVal, vlMdfUnknown,
};

typedef enum tmpTCommand {
    none      , pstr      , plabel    , pvar      , pglo      , pmem      , parrmem   , push      , setvar    , setglo    , 
    setmem    , setarrmem , pop       , cpy       , pincvar   , pincglo   , pincmem   , pincarrmem, pdecvar   , pdecglo   , 
    pdecmem   , pdecarrmem, sincvar   , sincglo   , sincmem   , sincarrmem, sdecvar   , sdecglo   , sdecmem   , sdecarrmem, 
    addvar    , addglo    , addmem    , addarrmem , subvar    , subglo    , submem    , subarrmem , mulvar    , mulglo    , 
    mulmem    , mularrmem , divvar    , divglo    , divmem    , divarrmem , modvar    , modglo    , modmem    , modarrmem , 
    andvar    , andglo    , andmem    , andarrmem , orvar     , orglo     , ormem     , orarrmem  , xorvar    , xorglo    , 
    xormem    , xorarrmem , shlvar    , shlglo    , shlmem    , shlarrmem , shrvar    , shrglo    , shrmem    , shrarrmem , 
    add       , sub       , mul       , _div      , mod       , _and      , _or       , _xor      , shl       , shr       , 
    _not      , eq        , ne        , gt        , ge        , ls        , le        , cvt       , newobj    , newarr    , 
    call      , vcall     , ret       , vret      , setlocal  , jmp       , jp        , jz        , setarg    , getarg    , 
    setgtbl   , setclgtbl , getgtbl   , getgtblsz , sys       , setflag   , pushflag  , setpause  ,
    unknown,
} TCommand;

extern const int8 argCnt[];

#define AlignRsp \
    uint64 rsp, __isModified = 0; \
    __asm__ __volatile__ ( \
        "movq %%rsp, %0" \
        : "=m"(rsp) \
        :  \
        : "memory" \
    ); \
    if (rsp & 8) \
        __asm__ __volatile__ ( \
            "push %%rax\n\t" \
            "movq $1, %0\n\t" \
            : "=m"(__isModified) \
            : \
            : \
        );
#define cancelAlignRsp \
    if (__isModified) \
        __asm__ __volatile__ ( \
            "pop %%rax" \
            : \
            : \
            : "rax" \
        ); \

#endif