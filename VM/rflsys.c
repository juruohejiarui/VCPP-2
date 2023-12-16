#include "rflsys.h"

// the list that used to generate data template 
ListElement clsListStart, clsListEnd;

VariableTypeData *loadTypeData_var(FILE *fPtr) {
    VariableTypeData *var = mallocObj(VariableTypeData);
    var->name = readString(fPtr);
    readData(fPtr, &var->visbility, uint32);
    readData(fPtr, &var->offset, uint64);
    var->type = readString(fPtr);
    return var;
}

MethodTypeData *loadTypeData_mtd(FILE *fPtr) {
    MethodTypeData *mtd = mallocObj(MethodTypeData);
    mtd->name = readString(fPtr);
    readData(fPtr, &mtd->visbility, uint32);
    readData(fPtr, &mtd->offset, uint64);
    mtd->resType = readString(fPtr);
    readData(fPtr, &mtd->argCount, uint64);
    mtd->argType = mallocArray(char *, mtd->argCount);
    for (int i = 0; i < mtd->argCount; i++) mtd->argType[i] = readString(fPtr);
    return mtd;
}
ClassTypeData *loadTypeData_cls(FILE *fPtr) {
    ClassTypeData *cls = mallocObj(ClassTypeData);

    cls->name = readString(fPtr);
    readData(fPtr, &cls->visbility, uint32);
    readData(fPtr, &cls->size, uint64);
    readData(fPtr, &cls->offset, uint64);
    cls->dataTemplate = mallocArray(uint8, cls->size);
    readArray(fPtr, cls->dataTemplate, uint8, cls->size);
    uint64 mCnt, fCnt;
    readData(fPtr, &mCnt, uint64), readData(fPtr, &fCnt, uint64);
    while (mCnt--) {
        MethodTypeData *mtd = loadTypeData_mtd(fPtr);
        Trie_insert(cls->methods, mtd->name, mtd);
    }
    while (fCnt--) {
        VariableTypeData *var = loadTypeData_var(fPtr);
        Trie_insert(cls->fields, var->name, var);
    }

    // insert this class into the class list
    ListElement *ele = mallocObj(ListElement);
    ele->content = cls;
    List_insert(&clsListEnd, ele);
    return cls;
}
NamespaceTypeData *loadTypeData_nsp(FILE *fPtr) {
    NamespaceTypeData *nsp = mallocObj(NamespaceTypeData);
    nsp->name = readString(fPtr);
    readData(fPtr, &nsp->dataTemplateSize, uint64);
    nsp->children = mallocObj(TrieNode),    nsp->classes = mallocObj(TrieNode);
    nsp->methods = mallocObj(TrieNode),     nsp->variables = mallocObj(TrieNode);
    Trie_init(nsp->children),   Trie_init(nsp->classes);
    Trie_init(nsp->methods),    Trie_init(nsp->variables);
    uint64 nCnt, cCnt, mCnt, vCnt;
    readData(fPtr, &nCnt, uint64), readData(fPtr, &cCnt, uint64);
    readData(fPtr, &mCnt, uint64), readData(fPtr, &vCnt, uint64);
    
    while (nCnt--) {
        NamespaceTypeData *child = loadTypeData_nsp(fPtr);
        Trie_insert(nsp->children, child->name, child);
    }
    while (cCnt--) {
        ClassTypeData *cls = loadTypeData_cls(fPtr);
        Trie_insert(nsp->classes, cls->name, cls);
    }
    while (mCnt--) {
        MethodTypeData *mtd = loadTypeData_mtd(fPtr);
        Trie_insert(nsp->methods, mtd->name, mtd);
    }
    while (vCnt--) {
        VariableTypeData *var = loadTypeData_var(fPtr);
        Trie_insert(nsp->variables, var->name, var);
    }
    return nsp;
}
NamespaceTypeData *loadTypeData(FILE *fPtr) {
    List_init(&clsListStart, &clsListEnd);
    return loadTypeData_nsp(fPtr);
}

uint8 *generateDataTemplate(NamespaceTypeData *root) {
    uint8 *dtTemplate = mallocArray(uint8, root->dataTemplateSize);
    // copy the date template into dtTemplate
    for (ListElement *ele = clsListStart.next; ele != &clsListEnd; ele = ele->next) {
        ClassTypeData *cls = ele->content;
        memcpy(dtTemplate + cls->offset, cls->dataTemplate, sizeof(uint8) * cls->size);
    }
    List_clear(&clsListStart, &clsListEnd);
    return dtTemplate;
}
