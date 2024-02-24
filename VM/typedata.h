#ifndef __TYPEDATA_H__
#define __TYPEDATA_H__

#include "tools.h"

typedef struct tmpVarTypeData {
    char *name, *fullName, *type;
    uint64 offset;
} VariableTypeData;

typedef struct tmpMethodTypeData {
    char *name, *fullName, *resType, *argType;
    uint64 argCount;
    uint64 offset;
} MethodTypeData;

typedef struct tmpClassTypeData {
    TrieNode *mtdTrie, *fldTrie;
    char *name, *fullName;
    const uint8 *dataTmpl;
    uint64 size, offset;
} ClassTypeData;

typedef struct tmpNamespaceTypeData {
    TrieNode *nspTrie, *clsTrie, *mtdTrie, *varTrie;
    char *name;
    uint64 clsSize;
} NamespaceTypeData;

NamespaceTypeData *readTypeData(FILE *fPtr);

/// @brief Generate a memory block for the class template
/// @param root the root of type data
/// @return the memory block
const uint8 *generateTypeData(NamespaceTypeData *root);
#endif