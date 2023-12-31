#include "idensys.h"

const std::string unknownName = "<unknown>";
NamespaceInfo *rootNsp;
ClassInfo *basicCls, *int8Cls, *uint8Cls, *int16Cls, *uint16Cls, *int32Cls, *uint32Cls, *int64Cls, *uint64Cls, *float32Cls, *float64Cls, *objectCls, *basicTypeCls[12];

#pragma region definition of member functions
ExprType::ExprType() {
    cls = nullptr, clsName = unknownName;
    dimc = 0;
    vtMdf = ValueTypeModifier::t;
}

ExprType::ExprType(const std::string &clsName, int dimc, ValueTypeModifier vtMdf) {
    cls = nullptr, this->dimc = dimc;
    this->clsName = clsName;
    this->vtMdf = vtMdf;
}

ExprType::ExprType(IdentifierNode *node) {
    cls = nullptr, dimc = node->getDimc();
    this->vtMdf = ValueTypeModifier::t;
    this->clsName = node->getName();
    if (node->getGenericArea() != nullptr)
        for (size_t i = 0; i < node->getGenericArea()->getParamCount(); i++)
            genericParams.push_back(ExprType(node->getGenericArea()->getParam(i)));
}

uint64 ExprType::getSize() const {
    if (dimc > 0 || !isBasicCls(cls)) return sizeof(uint64);
    return cls->size;
}

ExprType ExprType::convertToBase() const {
    ExprType res;
    res.cls = cls->baseCls, res.clsName = cls->baseCls->fullName;
    res.dimc = dimc;
    for (size_t i = 0; i < res.cls->genericClasses.size(); i++) {
        ExprType res = cls->genericParams[i];
        // substitute the params in THIS into the param classes of cls
        for (size_t j = 0; j < cls->genericClasses.size(); j++)
            res = substitute(res, cls->genericClasses[i], res.genericParams[i]);
        res.genericParams.emplace_back(res);
    }
    return res;
}

bool ExprType::setCls() {
    cls = findCls(clsName);
    if (cls == nullptr || cls->genericClasses.size() != genericParams.size()) return false;
    bool succ = true;
    for (auto &param : genericParams) succ &= param.setCls();
    return succ;
}

std::string ExprType::toString() const
{
    std::string str;
    if (cls != nullptr) str = cls->fullName;
    else str = clsName;
    str += "_DIMC" + std::to_string(dimc);
    return str;
}

ExprType substitute(const ExprType &target, ClassInfo *cls, const ExprType &clsImpl) {
    if (target.cls == cls) return clsImpl;
    ExprType res = target;
    for (size_t i = 0; i < res.genericParams.size(); i++)
        res.genericParams[i] = substitute(res.genericParams[i], cls, clsImpl);
    return res;
}

VariableInfo::VariableInfo() {
    nameNode = typeNode = nullptr, initNode = nullptr, blgRoot = nullptr;
    blgFunc = nullptr, blgCls = nullptr, blgNsp = nullptr;
    name = fullName = unknownName;
    visibility = IdentifierVisibility::Unknown;
    offset = 0;
}

IdentifierNode *VariableInfo::getNameNode() const { return nameNode; }
void VariableInfo::setNameNode(IdentifierNode *nameNode) {
    this->nameNode = nameNode;
    name = nameNode->getName();
    if (blgFunc != nullptr) fullName = name;
    else if (blgCls != nullptr) fullName = blgCls->fullName + "." + name;
    else if (blgNsp != nullptr) fullName = (blgNsp != rootNsp ? blgNsp->fullName + "." : "") + name;
    else fullName = unknownName;
}
IdentifierNode *VariableInfo::getTypeNode() const { return typeNode; }
void VariableInfo::setTypeNode(IdentifierNode *typeNode) { this->typeNode = typeNode; type = ExprType(typeNode); }

IdentifierRegion VariableInfo::getRegion() const {
    if (blgFunc != nullptr) return IdentifierRegion::Local;
    else if (blgCls != nullptr) return IdentifierRegion::Member;
    else if (blgNsp != nullptr) return IdentifierRegion::Global;
    return IdentifierRegion::Unknown;
}

FunctionInfo::FunctionInfo() {
    blgCls = nullptr, blgNsp = nullptr, blgRoot = nullptr;
    defNode = nullptr;
    name = fullName = nameWithParam = unknownName;
    visibility = IdentifierVisibility::Unknown;
    offset = 0;
}

FuncDefNode *FunctionInfo::getDefNode() const { return defNode; }
void FunctionInfo::setDefNode(FuncDefNode *defNode) {
    this->defNode = defNode;
    name = nameWithParam = defNode->getNameNode()->getName();
    // get the generic params
    genericClasses.clear();
    if (defNode->getNameNode()->getGenericArea() != nullptr) {
        for (size_t i = 0; i < defNode->getNameNode()->getGenericArea()->getParamCount(); i++) {
            IdentifierNode *gClsNode = defNode->getNameNode()->getGenericArea()->getParam(i);
            ClassInfo *gCls = new ClassInfo();
            gCls->isGeneric = true;
            gCls->fullName = gCls->name = gClsNode->getName();
            gCls->blgNsp = blgNsp, gCls->blgRoot = blgRoot;
            genericClasses.push_back(gCls);
        }
    }
    // get the param list
    params.clear();
    for (size_t i = 0; i < defNode->getParamCount(); i++) {
        auto pir = defNode->getParam(i);
        auto param = new VariableInfo();
        param->blgFunc = this, param->blgCls = blgCls, param->blgNsp = blgNsp, param->blgRoot = blgRoot;
        param->setNameNode(pir.first), param->setTypeNode(pir.second);
        params.push_back(param);
        nameWithParam.push_back('@');
        nameWithParam.append(param->type.toString());
    }
    resType = ExprType(defNode->getResNode());
    if (blgCls != nullptr) fullName = blgCls->fullName + "." + nameWithParam;
    else if (blgNsp != nullptr) fullName = (blgNsp != rootNsp ? blgNsp->fullName + "." : "") + nameWithParam;
    else fullName = unknownName;
}

IdentifierRegion FunctionInfo::getRegion() const {
    if (blgCls != nullptr) return IdentifierRegion::Member;
    else if (blgNsp != nullptr) return IdentifierRegion::Global;
    return IdentifierRegion::Unknown;
}
std::pair<bool, ExprType> FunctionInfo::satisfy(const std::vector<ExprType> paramList) {
    return std::pair<bool, ExprType>();
}
#pragma endregion

NamespaceInfo *rootNsp;
std::vector<ClassInfo *> clsList;

FunctionInfo *curFunc;
ClassInfo *curCls;
NamespaceInfo *curNsp;
RootNode *curRoot;

std::vector<NamespaceInfo *> usingList;

FunctionInfo *getCurFunc() { return curFunc; }
void setCurFunc(FunctionInfo *func) { curFunc = func; }
ClassInfo *getCurCls() { return curCls; }
void setCurCls(ClassInfo *cls) { curCls = cls; }
NamespaceInfo *getCurNsp() { return curNsp; }
void setCurNsp(NamespaceInfo *nsp) { curNsp = nsp; }
RootNode *getCurRoot() { return curRoot; }
bool setCurRoot(RootNode *node) {
    if (curRoot == node) return ;
    usingList.clear();
    bool succ = true;
    for (size_t i = 0; i < node->getUsingCount(); i++) {
        std::vector<std::string> path;
        stringSplit(node->getUsing(i)->getPath(), '.', path);
        NamespaceInfo *nsp = rootNsp;
        for (const auto &pth : path) {
            auto iter = nsp->nspMap.find(pth);
            if (iter == nsp->nspMap.end()) {
                printError(node->getToken().lineId, "Can't find namespace " + node->getUsing(i)->getPath());
                succ = false, nsp = nullptr;
                break;
            }
            nsp = iter->second;
        }
        usingList.push_back(nsp);
    }
    curRoot = node;
    return succ;
}

ClassInfo *findCls(const std::string &path) {
    std::vector<std::string> prts;
    stringSplit(path, '.', prts);
    if (prts.size() == 1) {
        // it can be a generic class
        if (curFunc != nullptr) {
            for (size_t i = 0; i < curFunc->genericClasses.size(); i++)
                if (curFunc->genericClasses[i] != nullptr && curFunc->genericClasses[i]->name == prts[0])
                    return curFunc->genericClasses[i];
        }
        if (curCls != nullptr) {
            for (size_t i = 0; i < curCls->genericClasses.size(); i++)
                if (curCls->genericClasses[i] != nullptr && curCls->genericClasses[i]->name == prts[0])
                    return curCls->genericClasses[i];
        }
        // or a class in curNsp
        if (curNsp != nullptr && curNsp->clsMap.count(prts[0])) return curNsp->clsMap[prts[0]];
        // or a class in using list
        for (auto nsp : usingList)
            if (nsp != nullptr && nsp->clsMap.count(prts[0]) && nsp->clsMap[prts[0]]->visibility == IdentifierVisibility::Public)
                return nsp->clsMap[prts[0]];
        // or a class in root namespace
        if (rootNsp->clsMap.count(prts[0]) && rootNsp->clsMap[prts[0]]->visibility == IdentifierVisibility::Public)
            return rootNsp->clsMap[prts[0]];
        return nullptr;
    } else {
        NamespaceInfo *st = rootNsp;
        if (curNsp->nspMap.count(prts[0])) st = curNsp;
        else {
            for (auto nsp : usingList)
                if (nsp != nullptr && nsp->nspMap.count(prts[0])) {
                    st = nsp;
                    break;
                }
        }
        for (size_t i = 0; i < prts.size() - 1; i++) {
            if (st->nspMap.count(prts[0])) st = st->nspMap[prts[0]];
            else return nullptr;
        }
        if (st->clsMap.count(prts.back()) && st->clsMap[prts.back()]->visibility == IdentifierVisibility::Public)
            return st->clsMap[prts.back()];
        else return nullptr;
    }
}

void buildRootInfo() {
    rootNsp = new NamespaceInfo();
    basicCls = new ClassInfo(), basicCls->size = 0;
    int8Cls = new ClassInfo(), int8Cls->name = int8Cls->fullName = "char", int8Cls->size = sizeof(char);
    uint8Cls = new ClassInfo(), uint8Cls->name = uint8Cls->fullName = "byte", uint8Cls->size = sizeof(unsigned char);
    int16Cls = new ClassInfo(), int16Cls->name = int16Cls->fullName = "short", int16Cls->size = sizeof(short);
    uint16Cls = new ClassInfo(), uint16Cls->name = uint16Cls->fullName = "ushort", uint16Cls->size = sizeof(unsigned short);
    int32Cls = new ClassInfo(), int32Cls->name = int32Cls->fullName = "int", int32Cls->size = sizeof(short);
    uint32Cls = new ClassInfo(), uint32Cls->name = uint32Cls->fullName = "uint", uint32Cls->size = sizeof(unsigned short);
    int64Cls = new ClassInfo(), int64Cls->name = int64Cls->fullName = "long", int64Cls->size = sizeof(short);
    uint64Cls = new ClassInfo(), uint64Cls->name = uint64Cls->fullName = "ulong", uint64Cls->size = sizeof(unsigned short);
    objectCls = new ClassInfo(), objectCls->name = objectCls->fullName = "object", objectCls->size = sizeof(uint64);

    basicTypeCls[0] = int8Cls, basicTypeCls[1] = uint8Cls, basicTypeCls[2] = int16Cls, basicTypeCls[3] = uint16Cls;
    basicTypeCls[4] = int32Cls, basicTypeCls[5] = uint32Cls, basicTypeCls[6] = int64Cls, basicTypeCls[7] = uint64Cls;
    basicTypeCls[8] = float32Cls, basicTypeCls[9] = float64Cls, basicTypeCls[10] = objectCls, basicTypeCls[11] = basicCls;

    for (int i = 0; i < 12; i++) rootNsp->clsMap[basicTypeCls[i]->name] = basicTypeCls[i], basicTypeCls[i]->blgNsp = rootNsp;
    for (int i = 0; i < 11; i++) basicTypeCls[i]->baseCls = basicCls, basicCls->derivedClasses.push_back(basicTypeCls[i]);
}

/// @brief This function can scan and build the namespace structure and build the symbols of classes, generate the generic classes of each class.
/// PS: This function will clean the clsList and store all the classes in roots into clsList
/// @param roots the roots
/// @return if it is successful to build the structure.
bool buildClsSymbol(const RootList &roots) {
    auto buildCls = [&](RootNode *root, NamespaceInfo *blgNsp, ClsDefNode *clsNode) -> bool {
        ClassInfo *cls = new ClassInfo();
        cls->blgNsp = blgNsp;
        cls->blgRoot = root;
        cls->defNode = clsNode;
        // set class name and full name
        cls->name = clsNode->getNameNode()->getName();
        if (blgNsp != rootNsp) cls->fullName = blgNsp->fullName + "." + cls->name;
        else cls->fullName = cls->name;

        if (blgNsp->clsMap.count(cls->name)) {
            printError(clsNode->getToken().lineId, "Redefine class : \"" + cls->name + "\"");
            delete cls;
            return false;
        }

        cls->isGeneric = false;
        // build the generic classes
        auto gArea = clsNode->getNameNode()->getGenericArea();
        if (gArea != nullptr) {
            for (size_t i = 0; i < gArea->getParamCount(); i++) {
                ClassInfo *gCls = new ClassInfo();
                gCls->name = gCls->fullName = gArea->getParam(i)->getName();
                gCls->isGeneric = true;
                gCls->blgNsp = blgNsp, gCls->blgRoot = root;
                cls->genericClasses.push_back(gCls);
            }
        }
        // Install this class
        blgNsp->clsMap[cls->name] = cls;
        clsList.push_back(cls);
        return true;
    };
    auto scanNsp = [&](RootNode *root, NspDefNode *nsp) -> bool {
        auto curNsp = rootNsp;
        std::vector<std::string> path;
        stringSplit(nsp->getNameNode()->getName(), '.', path);
        for (const auto &prt : path) {
            auto iter = curNsp->nspMap.find(prt);
            if (iter == curNsp->nspMap.end()) {
                auto newNsp = new NamespaceInfo();
                // set name and full name
                newNsp->name = prt;
                if (curNsp != rootNsp) newNsp->fullName = curNsp->fullName + "." + prt;
                else newNsp->fullName = prt;
                // install this namespace
                curNsp->nspMap.insert(std::make_pair(prt, newNsp));
                curNsp = newNsp;
            } else curNsp = iter->second;
        }
        curNsp->defList.emplace_back(nsp, root);
        bool succ = true;
        for (size_t i = 0; i < nsp->getClsCount(); i++) {
            auto clsDef = nsp->getClsDef(i);
            succ &= buildCls(root, curNsp, clsDef);
        }
        return succ;
    };
    clsList.clear();
    bool succ = true;
    for (auto root : roots) {
        for (size_t i = 0; i < root->getDefCount(); i++) {
            auto node = root->getDef(i);
            switch (node->getType()) {
                case SyntaxNodeType::ClsDef:
                    succ &= buildCls(root, rootNsp, (ClsDefNode *)node);
                    break;
                case SyntaxNodeType::NspDef:
                    succ &= scanNsp(root, (NspDefNode *)node);
                    break;
            }
        }
    }
    return succ;
}

/// @brief This function will build the inheritance structure of classes and build the member structure of each class.
/// @param roots 
/// @return 
bool buildClsTree(const RootList &roots) {
    bool succ = true;
    // build the inheritance structure
    for (auto cls : clsList) {
        // set the environment
        setCurRoot(cls->blgRoot);
        setCurNsp(cls->blgNsp);
        // get the base class
        ClassInfo *baseCls = (cls->defNode->getBaseNode() == nullptr ? objectCls : findCls(cls->defNode->getBaseNode()->getName()));
        if (baseCls == nullptr) {
            printError(cls->defNode->getBaseNode()->getToken().lineId, "Could not find base class " + cls->defNode->getBaseNode()->getName());
            succ = false;
            continue;
        }
        // check the genericDefinition
        if ((cls->defNode->getBaseNode()->getGenericArea() == nullptr && baseCls->genericClasses.size() > 0)
            || (cls->defNode->getBaseNode()->getGenericArea()->getParamCount() != baseCls->genericClasses.size())) {
            printError(cls->defNode->getBaseNode()->getToken().lineId, "Miss the definition of generic class(es) for base class " + baseCls->fullName);
            succ = false;
            continue;
        }
        if (cls->defNode->getBaseNode()->getGenericArea() != nullptr && baseCls->genericClasses.size() == 0) {
            printError(cls->defNode->getBaseNode()->getToken().lineId, "Base class " + baseCls->fullName + " has no generic class");
            succ = false;
            continue;
        }
        // generate the expression type for each generic param
        setCurCls(cls);
        {
            auto gArea = cls->defNode->getBaseNode()->getGenericArea();
            for (size_t i = 0; i < gArea->getParamCount(); i++) {
                auto param = ExprType(gArea->getParam(i));
                succ &= param.setCls();
            }
            if (!succ) printError(gArea->getToken().lineId, "Failed to set the generic params for base class");
        }
        // build the tree
        baseCls->derivedClasses.push_back(cls);
        cls->baseCls = baseCls;
    }
    if (!succ) return false;
    // build the member structure for each class
    std::queue<ClassInfo *> que;
    que.push(basicCls);
    while (!que.empty()) {
        auto bsCls = que.front(); que.pop();
        for (auto dCls : bsCls->derivedClasses) {
            if (dCls->size > 0) {
                printError(0, "Ring inheritance!!");
                return false;
            }
            dCls->size = bsCls->size + dCls->genericClasses.size() * sizeof(uint8);
            // use memory align to modify the offset of object members

        }
    }
    return true;
}

bool buildCls(const RootList &roots) {
    bool succ = true;
    // build the symbols of classes
    succ &= buildClsSymbol(roots);
    // build inheritation tree
    succ &= buildClsTree(roots);
    return succ;
}

bool buildIdenSystem(const RootList &roots) {
    bool succ = true;
    buildRootInfo();
    succ &= buildCls(roots);
}
