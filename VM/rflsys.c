#include "rflsys.h"

// the list that used to generate data template 
ListElement clsListStart, clsListEnd;

VariableTypeData *loadTypeData_var(FILE *fPtr) {
    VariableTypeData *var = mallocObj(VariableTypeData);
    var->name = readString(fPtr);
    readData(fPtr, &var->visibility, uint8);
    var->type = readString(fPtr);
    readData(fPtr, &var->offset, uint64);
    return var;
}

FunctionTypeData *loadTypeData_mtd(FILE *fPtr) {
    FunctionTypeData *mtd = mallocObj(FunctionTypeData);
    mtd->name = readString(fPtr);
    mtd->labelName = readString(fPtr);
    readData(fPtr, &mtd->visibility, uint8);
    mtd->resType = readString(fPtr);
    readData(fPtr, &mtd->gtableSize, uint8);
    readData(fPtr, &mtd->offset, uint64);
    readData(fPtr, &mtd->argCount, uint64);
    mtd->argTypes = mallocArray(char *, mtd->argCount);
    for (size_t i = 0; i < mtd->argCount; i++) mtd->argTypes[i] = readString(fPtr);
    return mtd;
}
ClassTypeData *loadTypeData_cls(FILE *fPtr) {
    ClassTypeData *cls = mallocObj(ClassTypeData);
    cls->name = readString(fPtr);
    cls->bsClsName = readString(fPtr);
    readData(fPtr, &cls->visibility, uint8);
    readData(fPtr, &cls->size, uint64);
    readData(fPtr, &cls->gtableSize, uint8);
    readData(fPtr, &cls->gtableOffset, uint64);
    readData(fPtr, &cls->varCount, uint64);
    readData(fPtr, &cls->funcCount, uint64);
    
    // initialize tries
    cls->funcTrie = mallocObj(TrieNode), cls->varTrie = mallocObj(TrieNode);
    Trie_init(cls->funcTrie), Trie_init(cls->varTrie);

    // get member information
    for (size_t i = 0; i < cls->varCount; i++) {
        VariableTypeData *var = loadTypeData_var(fPtr);
        Trie_insert(cls->varTrie, var->name, var);
    }
    for (size_t i = 0; i < cls->funcCount; i++) {
        FunctionTypeData *mtd = loadTypeData_mtd(fPtr);
        Trie_insert(cls->funcTrie, mtd->name, mtd);
    }
    return cls;
}
NamespaceTypeData *loadTypeData_nsp(FILE *fPtr) {
    NamespaceTypeData *nsp = mallocObj(NamespaceTypeData);
    nsp->name = readString(fPtr);

    readData(fPtr, &nsp->varCount, uint64);
    readData(fPtr, &nsp->funcCount, uint64);
    readData(fPtr, &nsp->clsCount, uint64);
    readData(fPtr, &nsp->nspCount, uint64);

    // initialize tries
    nsp->varTrie = mallocObj(TrieNode), nsp->funcTrie = mallocObj(TrieNode);
    nsp->clsTrie = mallocObj(TrieNode), nsp->nspTrie = mallocObj(TrieNode);
    Trie_init(nsp->varTrie), Trie_init(nsp->funcTrie);
    Trie_init(nsp->clsTrie), Trie_init(nsp->nspTrie);

    // get member information
    for (size_t i = 0; i < nsp->varCount; i++) {
        VariableTypeData *var = loadTypeData_var(fPtr);
        Trie_insert(nsp->varTrie, var->name, var);
    }
    for (size_t i = 0; i < nsp->funcCount; i++) {
        FunctionTypeData *mtd = loadTypeData_mtd(fPtr);
        Trie_insert(nsp->funcTrie, mtd->name, mtd);
    }
    for (size_t i = 0; i < nsp->clsCount; i++) {
        ClassTypeData *cls = loadTypeData_cls(fPtr);
        Trie_insert(nsp->clsTrie, cls->name, cls);
		ListElement *ele = mallocObj(ListElement);
		ele->content = cls;
        List_insert(&clsListEnd, ele);
    }
    for (size_t i = 0; i < nsp->nspCount; i++) {
        NamespaceTypeData *subNsp = loadTypeData_nsp(fPtr);
        Trie_insert(nsp->nspTrie, subNsp->name, subNsp);
    }
    return nsp;
}
NamespaceTypeData *loadTypeData(FILE *fPtr) {
    List_init(&clsListStart, &clsListEnd);
    return loadTypeData_nsp(fPtr);
}

uint8 *generateDataTmpl(NamespaceTypeData *root) {
    return NULL;
}

ClassTypeData *findClass(NamespaceTypeData *rootNsp, const char *fullName) {
    size_t len = strlen(fullName);
    for (size_t l = 0; l < len; l++) {
        size_t r = l;
        while (r < len && fullName[r] != '.') r++;
        // the name of the namespace
        if (r < len) {
            TrieNode *nd = rootNsp->nspTrie;
            for (size_t i = l; i < r; i++) {
                if (nd->child[fullName[i]] == NULL) return NULL;
                nd = nd->child[fullName[i]];
            }
            if (nd->content == NULL) return NULL;
            rootNsp = nd->content;
        } else { // the name of the class
            TrieNode *nd = rootNsp->clsTrie;
            for (size_t i = l; i < r; i++) {
                if (nd->child[fullName[i]] == NULL) return NULL;
                nd = nd->child[fullName[i]];
            }
            if (nd->content == NULL) return NULL;
            return nd->content;
        }
    }
    return NULL;
}

FunctionTypeData *findGlobalFunc(NamespaceTypeData *rootNsp, const char *fullName) {
    size_t len = strlen(fullName);
    for (size_t l = 0; l < len; l++) {
        size_t r = l;
        while (r < len && fullName[r] != '.' && fullName[r] != '@') r++;
        // the name of the namespace
        if (r < len && fullName[r] == '.') {
            TrieNode *nd = rootNsp->nspTrie;
            for (size_t i = l; i < r; i++) {
                if (nd->child[fullName[i]] == NULL) return NULL;
                nd = nd->child[fullName[i]];
            }
            if (nd->content == NULL) return NULL;
            rootNsp = nd->content;
        } else { // the name of the function
            TrieNode *nd = rootNsp->funcTrie;
            for (size_t i = l; i < r; i++) {
                if (nd->child[fullName[i]] == NULL) return NULL;
                nd = nd->child[fullName[i]];
            }
            if (nd->content == NULL) return NULL;
            return nd->content;
        }
    }
    return NULL;
}

VariableTypeData *findGlobalVar(NamespaceTypeData *rootNsp, const char *fullName) {
    size_t len = strlen(fullName);
    for (size_t l = 0; l < len; l++) {
        size_t r = l;
        while (r < len && fullName[r] != '.' && fullName[r] != '@') r++;
        // the name of the namespace
        if (r < len && fullName[r] == '.') {
            TrieNode *nd = rootNsp->nspTrie;
            for (size_t i = l; i < r; i++) {
                if (nd->child[fullName[i]] == NULL) return NULL;
                nd = nd->child[fullName[i]];
            }
            if (nd->content == NULL) return NULL;
            rootNsp = nd->content;
        } else { // the name of the variable
            TrieNode *nd = rootNsp->varTrie;
            for (size_t i = l; i < r; i++) {
                if (nd->child[fullName[i]] == NULL) return NULL;
                nd = nd->child[fullName[i]];
            }
            if (nd->content == NULL) return NULL;
            return nd->content;
        }
    }
    return NULL;
}