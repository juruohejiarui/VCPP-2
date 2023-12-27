#include "cpltree.h"

NamespaceInfo *curNsp;
ClassInfo *curCls;
FunctionInfo *curFunc;
SyntaxNode *curRoot;

NamespaceInfo *gloNsp;
ClassInfo *basicCls, *objectCls, *
    int8Cls, *uint8Cls, *int16Cls, *uint16Cls, 
    *int32Cls, *uint32Cls, *int64Cls, *uint64Cls, *float32Cls, *float64Cls;

std::vector<NamespaceInfo *> usingList;

void enterNamespace(NamespaceInfo *nsp) { curNsp = nsp; }
void leaveNamespace() { curNsp = nullptr; }
void enterClass(ClassInfo *cls) { curCls = cls; }
void leaveClass() { curCls = nullptr; }
void enterFunction(FunctionInfo *func) { curFunc = func; }
void leaveFunction() { curFunc = nullptr; }

bool buildClass(NamespaceInfo *par, ClsDefNode * defNode) {
    ClassInfo *cls = new ClassInfo(defNode);
    cls->fullName = par->fullName + "." + cls->name;
    cls->visibility = defNode->getVisibility();

    cls->belongRoot = curRoot;

    // 获取泛型符号
    IdentifierNode *nameNode = defNode->getNameNode();
    if (nameNode->childrenCount() > 0) {
        auto genericNode = nameNode->getGenericArea();
        for (size_t i = 0; i < genericNode->childrenCount(); i++) {
            auto paramNode = genericNode->getParam(i);
            ClassInfo *genericCls = new ClassInfo();
            genericCls->name = paramNode->getName();
            genericCls->isGenericIdentifier = true;
            cls->genericClasses.push_back(genericCls);
        }
    }

    if (par->clsMap.count(cls->name)) {
        printError(defNode->getToken().lineId, "class " + cls->name + " is redefined in " + par->fullName);
        return false;
    }
    par->clsMap[cls->name] = cls;
    return true;
}

bool buildClass(NamespaceInfo *par, NspDefNode *defNode) {
    NamespaceInfo *nsp = new NamespaceInfo(defNode);
    nsp->fullName = par->fullName + "." + nsp->name;
    nsp->visibility = IdentifierVisibility::Public;

    nsp->belongRoot = curRoot;

    bool succ = true;
    for (size_t i = 0; i < defNode->childrenCount(); i++) {
        auto child = (*defNode)[i];
        if (child->getType() == SyntaxNodeType::NspDef)
            succ &= buildClass(nsp, (NspDefNode *)child);
        else if (child->getType() == SyntaxNodeType::ClsDef)
            succ &= buildClass(nsp, (ClsDefNode *)child);
    }
    if (par->nspMap.count(nsp->name)) {
        printError(defNode->getToken().lineId, "namespace " + nsp->name + " is redefined in " + par->fullName);
        return false;
    }
    par->nspMap[nsp->name] = nsp;
    return succ;
}

bool addUsing(const UsingNode *usingNode) {
    auto *nsp = gloNsp;
    std::vector<std::string> names;
    stringSplit(usingNode->getPath(), '.', names);
    for (size_t i = 0; i < names.size(); i++) {
        auto ele = nsp->nspMap.find(names[i]);
        if (ele == nsp->nspMap.end()) {
            printError(usingNode->getToken().lineId, "namespace " + names[i] + " is not found");
            return false;
        }
        nsp = ele->second;
    }
    usingList.push_back(nsp);
    return true;
}
void clearUsing() { usingList.clear(); }

bool loadUsing(const SyntaxNode *root) {
    bool succ = true;
    for (size_t i = 0; i < root->childrenCount(); i++) {
        auto child = (*root)[i];
        if (child->getType() == SyntaxNodeType::Using)
            succ &= addUsing((UsingNode *)child);
    }
    return succ;
}


/**
 * @brief Finds a ClassInfo object by name.
 * 
 * @param name The name of the class to find.
 * @return A pointer to the ClassInfo object if found, nullptr otherwise.
 */
ClassInfo *findClass(const std::string &name) {
    std::vector<std::string> names;
    stringSplit(name, '.', names);
    // this name may be a generic class name
    if (names.size() == 1 && curCls != nullptr) {
        for (size_t i = 0; i < curCls->genericClasses.size(); i++)
            if (curCls->genericClasses[i]->name == name) return curCls->genericClasses[i];
    }
    // search in the current namespace
    if (curNsp->clsMap.count(names[0])) return curNsp->clsMap[names[0]];
    if (curNsp->nspMap.count(names[0])) {
        NamespaceInfo *nsp = curNsp;
        for (size_t i = 0; i < names.size() - 2; i++) {
            auto ele = nsp->nspMap.find(names[i]);
            if (ele == nsp->nspMap.end()) { nsp = nullptr; break;}
            nsp = ele->second;
        }
        if (nsp != nullptr && nsp->clsMap.count(names[names.size() - 1]))
            return nsp->clsMap[names[names.size() - 1]];
    }
    // search in the using list
    for (size_t i = 0; i < usingList.size(); i++) {
        NamespaceInfo *nsp = usingList[i];
        for (size_t j = 0; j < names.size() - 2; j++) {
            auto ele = nsp->nspMap.find(names[j]);
            if (ele == nsp->nspMap.end()) { nsp = nullptr; break;}
            nsp = ele->second;
        }
        if (nsp != nullptr && nsp->clsMap.count(names[names.size() - 1]))
            return nsp->clsMap[names[names.size() - 1]];
    }
    // start from the global namespace
    NamespaceInfo *nsp = gloNsp;
    for (size_t i = 0; i < names.size() - 2; i++) {
        auto ele = nsp->nspMap.find(names[i]);
        if (ele == nsp->nspMap.end()) { nsp = nullptr; break;}
        nsp = ele->second;
    }
    if (nsp != nullptr && nsp->clsMap.count(names[names.size() - 1]))
        return nsp->clsMap[names[names.size() - 1]];
    return nullptr;
}

NamespaceInfo *findNamespace(const std::string &name) {
    std::vector<std::string> names;
    stringSplit(name, '.', names);
    // start from the global namespace
    NamespaceInfo *nsp = gloNsp;
    for (size_t i = 0; i < names.size() - 1; i++) {
        auto ele = nsp->nspMap.find(names[i]);
        if (ele == nsp->nspMap.end()) { nsp = nullptr; break;}
        nsp = ele->second;
    }
    return nsp;
}

bool buildClsTree(ClassInfo *cls) {
    if (cls->getDefNode()->getBaseClassNode() == nullptr) cls->parent = objectCls;
    else {
        cls->parent = findClass(cls->getDefNode()->getBaseClassNode()->getName());
        if (cls->parent == nullptr) {
            printError(cls->getDefNode()->getToken().lineId, "class " + cls->getDefNode()->getBaseClassNode()->getName() + " is not found");
            return false;
        }
    }
    cls->derives.push_back(cls);
    return true;
}

void buildRootTypeInfo() {
    gloNsp = new NamespaceInfo();
    objectCls = new ClassInfo();
    basicCls = new ClassInfo();
    int8Cls = new ClassInfo();
    uint8Cls = new ClassInfo();
    int16Cls = new ClassInfo();
    uint16Cls = new ClassInfo();
    int32Cls = new ClassInfo();
    uint32Cls = new ClassInfo();
    int64Cls = new ClassInfo();
    uint64Cls = new ClassInfo();
    float32Cls = new ClassInfo();
    float64Cls = new ClassInfo();

    basicCls->name = basicCls->fullName = "basic", basicCls->size = 0;
    objectCls->name = objectCls->fullName = "object", objectCls->size = sizeof(uint64), objectCls->parent = basicCls;
    int8Cls->name = int8Cls->fullName = "char", int8Cls->size = 1, int8Cls->parent = basicCls;
    uint8Cls->name = uint8Cls->fullName = "uchar", uint8Cls->size = 1, uint8Cls->parent = basicCls;
    int16Cls->name = int16Cls->fullName = "short", int16Cls->size = 2, int16Cls->parent = basicCls;
    uint16Cls->name = uint16Cls->fullName = "ushort", uint16Cls->size = 2, uint16Cls->parent = basicCls;
    int32Cls->name = int32Cls->fullName = "int", int32Cls->size = 4, int32Cls->parent = basicCls;
    uint32Cls->name = uint32Cls->fullName = "uint", uint32Cls->size = 4, uint32Cls->parent = basicCls;
    int64Cls->name = int64Cls->fullName = "long", int64Cls->size = 8, int64Cls->parent = basicCls;
    uint64Cls->name = uint64Cls->fullName = "ulong", uint64Cls->size = 8, uint64Cls->parent = basicCls;
    float32Cls->name = float32Cls->fullName = "float", float32Cls->size = 4, float32Cls->parent = basicCls;
    float64Cls->name = float64Cls->fullName = "double", float64Cls->size = 8, float64Cls->parent = basicCls;

    gloNsp->clsMap["object"] = objectCls;
    gloNsp->clsMap["basic"] = basicCls;
    gloNsp->clsMap["char"] = int8Cls;
    gloNsp->clsMap["uchar"] = uint8Cls;
    gloNsp->clsMap["short"] = int16Cls;
    gloNsp->clsMap["ushort"] = uint16Cls;
    gloNsp->clsMap["int"] = int32Cls;
    gloNsp->clsMap["uint"] = uint32Cls;
    gloNsp->clsMap["long"] = int64Cls;
    gloNsp->clsMap["ulong"] = uint64Cls;
    gloNsp->clsMap["float"] = float32Cls;
    gloNsp->clsMap["double"] = float64Cls;
}

bool buildClsMem(ClassInfo *cls) {
    if (cls->isGenericIdentifier) return true;
    if (cls->belongRoot != curRoot)
        clearUsing(), loadUsing(cls->belongRoot), curRoot = cls->belongRoot;
    // Inherit the members of the parent class
    if (cls->parent != nullptr) {
        cls->size = cls->parent->size;
        for (auto &pir : cls->parent->varMap) 
            if (pir.second->visibility != IdentifierVisibility::Private)
                cls->varMap.insert(pir);
        for (auto &pir : cls->parent->funcMap) {
            cls->funcMap[pir.first].clear();
            for (auto func : pir.second)
                if (func->visibility != IdentifierVisibility::Private)
                    cls->funcMap[pir.first].push_back(func);
        }
    }
    cls->size += sizeof(uint64) * cls->genericClasses.size();
    // Add the members of the current class
    for (size_t i = 0; i < cls->getDefNode()->childrenCount(); i++) {
        auto child = (*cls->getDefNode())[i];
        switch (child->getType()) {
            case SyntaxNodeType::VarDef: {
                auto varDef = (VarDefNode *)child;
                for (size_t i = 0; i < varDef->getVarCount(); i++) {
                    const auto &var = varDef->getVar(i);
                    VariableInfo *vInfo = new VariableInfo(std::get<0>(var), std::get<1>(var));
                    vInfo->offset = cls->size;
                    cls->size += vInfo->resType.getSize();

                    cls->varMap[vInfo->name] = vInfo;

                    vInfo->belongRoot = cls->belongRoot;
                }
                break;
            }
            case SyntaxNodeType::FuncDef: {
                auto funcDef = (FuncDefNode *)child;
                FunctionInfo *fInfo = new FunctionInfo(funcDef, false);
            }
        }
    }
    return true;
}

bool buildTypeSystem(const std::vector<SyntaxNode *> &roots) {
    bool succ = true;
    // build the struct of namespace and class
    for (auto root : roots) {
        curRoot = root;
        for (size_t i = 0; i < root->childrenCount(); i++) {
            auto child = (*root)[i];
            if (child->getType() == SyntaxNodeType::NspDef)
                succ &= buildClass(gloNsp, (NspDefNode *)child);
            else if (child->getType() == SyntaxNodeType::ClsDef)
                succ &= buildClass(gloNsp, (ClsDefNode *)child);
        }
    }
    // build the content of class
    for (auto root : roots) {
        succ &= loadUsing(root);
        auto recursion = [&](auto &&self, NamespaceInfo *nsp) -> bool {
            bool succ = true;
            for (auto &ele : nsp->clsMap)
                succ &= buildClsTree(ele.second);
            for (auto &ele : nsp->nspMap)
                succ &= self(self, ele.second);
            return succ;
        };
        for (size_t i = 0; i < root->childrenCount(); i++) {
            auto child = (*root)[i];
            if (child->getType() == SyntaxNodeType::NspDef)
                succ &= recursion(recursion, findNamespace(((NspDefNode *)child)->getName()));
            else if (child->getType() == SyntaxNodeType::ClsDef)
                succ &= buildClsTree(findClass(((ClsDefNode *)child)->getName()));
        }
        clearUsing();
    }

    // build the members of classes in the class tree
    succ &= buildClsMem(basicCls);
    if (!succ) return false;
    return false;
}
