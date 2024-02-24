#ifndef __RFLSYS_H__
#define __RFLSYS_H__
#include "tools.h"

typedef enum tmpIdenVisibility {
    Private, Protected, Public
} IdenVisibility;
typedef struct tmpVariableTypeData {
    char *name;
    uint8 visibility;
    char *type;
    uint64 offset;
} VariableTypeData;
typedef struct tmpFunctionTypeData {
    char *name;
    char *labelName;
    uint8 visibility;
    char *resType;
    uint8 gtableSize;
    uint64 offset;
    uint64 argCount;
    char **argTypes;
} FunctionTypeData;
typedef struct tmpClassTypeData {
    char *name;
    char *bsClsName;
    uint8 visibility;
    uint64 size;
    uint8 gtableSize;
    uint64 gtableOffset;
    uint64 varCount;
    uint64 funcCount;
    TrieNode *varTrie, *funcTrie;
} ClassTypeData;
typedef struct tmpNamespaceTypeData {
    char *name;
    uint64 varCount, funcCount, clsCount, nspCount;
    TrieNode *varTrie, *funcTrie, *clsTrie, *nspTrie;
} NamespaceTypeData;

/// @brief Load the type data from the file and return the root of type data
/// @param fPtr the pointer of the file
/// @return the root of type data
NamespaceTypeData *loadTypeData(FILE *fPtr);
/// @brief Generate the data template using the type data whose root is ROOT, this function will create a memory block
/// @param root the root of type data
/// @return the data template
uint8 *generateDataTmpl(NamespaceTypeData *root);

ClassTypeData *findClass(NamespaceTypeData *rootNsp, const char *fullName);
FunctionTypeData *findGlobalFunc(NamespaceTypeData *rootNsp, const char *fullName);
VariableTypeData *findGlobalVar(NamespaceTypeData *rootNsp, const char *fullName);

#endif