#ifndef __RFLSYS_H__
#define __RFLSYS_H__
#include "tools.h"

typedef enum IdenVisibility {
    Private, Protected, Public
};
typedef struct tmpVariableTypeData {
    char *name, *type;
    uint32 visbility;
    uint64 offset;
} VariableTypeData;
typedef struct tmpMethodTypeData {
    char *name, *resType, **argType;
    uint32 visbility;
    uint64 argCount, offset;
} MethodTypeData;
typedef struct tmpClassTypeData {
    TrieNode *methods, *fields;
    uint32 visbility;
    char *name;
    uint64 size, offset;
    uint8 *dataTemplate;
} ClassTypeData;
typedef struct tmpNamespaceTypeData {
    TrieNode *children, *classes, *methods, *variables;
    char *name;
    uint64 dataTemplateSize;
} NamespaceTypeData;

typedef struct tmpMethodList {
    char *name;
    uint64 listSize;
    MethodTypeData **funcList;
} MethodList;

/// @brief Load the type data from the file and return the root of type data
/// @param fPtr the pointer of the file
/// @return the root of type data
NamespaceTypeData *loadTypeData(FILE *fPtr);
/// @brief Generate the data template using the type data whose root is ROOT, this function will create a memory block
/// @param root the root of type data
/// @return the data template
uint8 *generateDataTemplate(NamespaceTypeData *root);

ClassTypeData *findClass(const char *fullName);
MethodTypeData *findMethod(const char *fullName);
VariableTypeData *findVariable(const char *fullName);

/// @brief find the class named NAME in NSP, if cannot find this class, then return NULL 
/// @param nsp the namespace
/// @param name the name of the class (not the full name)
/// @return the type data of the class
ClassTypeData *findClass_nsp(NamespaceTypeData *nsp, const char *name);
MethodTypeData *findMethod_nsp(NamespaceTypeData *nsp, const char *fullName);
MethodList *findMethods_nsp(NamespaceTypeData *nsp, const char *name);
MethodList *findMethods_cls(ClassTypeData *nsp, const char *name);

#endif