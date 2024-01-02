#include "idensys.h"

const std::string unknownName = "<unknown>", constructerName = "@constructer";
NamespaceInfo *rootNsp;
ClassInfo *voidCls, *basicCls, *int8Cls, *uint8Cls, *int16Cls, *uint16Cls, *int32Cls, *uint32Cls, *int64Cls, *uint64Cls, *float32Cls, *float64Cls, *objectCls, *basicTypeCls[13];

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

bool ExprType::isObject() const { return dimc > 0 || !isBasicCls(cls); }

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

bool ExprType::operator==(const ExprType &other) const {
    if (dimc != other.dimc || cls != other.cls || genericParams.size() != other.genericParams.size()) return false;
    for (size_t i = 0; i < genericParams.size(); i++) if (genericParams[i] != other.genericParams[i]) return false;
    return true;
}

bool ExprType::operator!=(const ExprType &other) const { return !(*this == other); }

std::string ExprType::toString() const
{
    std::string str;
    if (cls != nullptr) str = cls->fullName;
    else str = clsName;
    str += "_DIMC" + std::to_string(dimc);
    return str;
}

std::string ExprType::toDebugString() const {
    std::string str;
    if (cls != nullptr) str = cls->fullName;
    else str = clsName;
    if (genericParams.size() > 0) {
        str.append("<$");
        for (size_t i = 0; i < genericParams.size(); i++) {
            if (i > 0) str.append(", ");
            str.append(genericParams[i].toDebugString());
        }
        str.append("$>");
    }
    for (size_t i = 0; i < dimc; i++) str.append("[]");
    return str;
}

ExprType substitute(const ExprType &target, ClassInfo *cls, const ExprType &clsImpl) {
    if (target.cls == cls) return clsImpl;
    ExprType res = target;
    for (size_t i = 0; i < res.genericParams.size(); i++)
        res.genericParams[i] = substitute(res.genericParams[i], cls, clsImpl);
    return res;
}

ExprType substitute(const ExprType &target, const GenericSubstitutionMap &gsMap) {
    ExprType res = target;
    if (res.cls->isGeneric && gsMap.count(res.cls)) {
        uint32 dimc = res.dimc;
        res = gsMap.find(res.cls)->second;
        return res;
    }
    for (size_t i = 0; i < res.genericParams.size(); i++) res.genericParams[i] = substitute(res.genericParams[i], gsMap);
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
ExpressionNode *VariableInfo::getInitNode() const { return initNode; }
void VariableInfo::setInitNode(ExpressionNode *initNode) { this->initNode = initNode; }

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

std::pair<bool, ExprType> FunctionInfo::satisfy(const GenericSubstitutionMap &gsMap, const std::vector<ExprType> &paramList) {
    if (paramList.size() != this->params.size()) return std::make_pair(false, ExprType());
    auto ngsMap = gsMap;
    auto tryMatch = [&ngsMap](ExprType src, const ExprType &tgr) -> bool {
        auto recursion = [&ngsMap](auto &&self, ExprType src, const ExprType &tgr) -> bool {
            if (src.dimc != tgr.dimc) return false;
            if (tgr.cls->isGeneric) {
                auto iter = ngsMap.find(tgr.cls);
                if (iter == ngsMap.end()) {
                    ngsMap.insert(std::make_pair(tgr.cls, src));
                    ngsMap[tgr.cls].dimc = 0;
                    return true;
                } else {
                    ExprType &gParam = iter->second;
                    gParam.dimc = tgr.dimc;
                    return self(self, src, gParam);
                }
            }
            bool succ = false;
            while (src.cls != basicCls && src.cls != nullptr && !src.cls->isGeneric) {
                if (src.cls == tgr.cls) { succ = true; break; }
                src = src.convertToBase();
            }
            if (!succ) return false;
            if (src.genericParams.size() != tgr.genericParams.size()) return false;
            for (size_t i = 0; i < src.genericParams.size(); i++)
                if (!self(self, src.genericParams[i], tgr.genericParams[i])) return false;
            return true;
        };
        return recursion(recursion, src, tgr);
    };
    for (size_t i = 0; i < paramList.size(); i++) if (!tryMatch(paramList[i], params[i]->type)) return std::make_pair(false, ExprType());
    return std::make_pair(true, substitute(this->resType, ngsMap));
}

ClassInfo::ClassInfo() {
    blgNsp = nullptr, blgRoot = nullptr;
    baseCls = nullptr;
    name = fullName = unknownName;
    visibility = IdentifierVisibility::Unknown;
    size = dep = 0;
}

NamespaceInfo::NamespaceInfo() {
    blgNsp = nullptr;
    name = fullName = unknownName;
}
#pragma endregion

bool isBasicCls(ClassInfo *cls) {
    for (size_t i = 0; i < 10; i++) if (cls == basicTypeCls[i]) return true;
    return false;
}

GenericSubstitutionMap makeSubtitutionMap(const std::vector<ClassInfo *> &gClsList, const std::vector<ExprType> &gParamList) {
    GenericSubstitutionMap gsMap;
    for (size_t i = 0; i < gClsList.size(); i++) gsMap[gClsList[i]] = gParamList[i];
    return gsMap;
}
void mergeSubtitutionMap(GenericSubstitutionMap &dst, const GenericSubstitutionMap &src) { for (auto pir : src) dst[pir.first] = pir.second; }

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
    if (curRoot == node) return true;
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
    voidCls = new ClassInfo(), voidCls->name = voidCls->fullName = "void", voidCls->size = 0;
    int8Cls = new ClassInfo(), int8Cls->name = int8Cls->fullName = "char", int8Cls->size = sizeof(char);
    uint8Cls = new ClassInfo(), uint8Cls->name = uint8Cls->fullName = "byte", uint8Cls->size = sizeof(unsigned char);
    int16Cls = new ClassInfo(), int16Cls->name = int16Cls->fullName = "short", int16Cls->size = sizeof(short);
    uint16Cls = new ClassInfo(), uint16Cls->name = uint16Cls->fullName = "ushort", uint16Cls->size = sizeof(unsigned short);
    int32Cls = new ClassInfo(), int32Cls->name = int32Cls->fullName = "int", int32Cls->size = sizeof(int);
    uint32Cls = new ClassInfo(), uint32Cls->name = uint32Cls->fullName = "uint", uint32Cls->size = sizeof(unsigned int);
    int64Cls = new ClassInfo(), int64Cls->name = int64Cls->fullName = "long", int64Cls->size = sizeof(long long);
    uint64Cls = new ClassInfo(), uint64Cls->name = uint64Cls->fullName = "ulong", uint64Cls->size = sizeof(unsigned long long);
    float32Cls = new ClassInfo(), float32Cls->name = float32Cls->fullName = "float", float32Cls->size = sizeof(float);
    float64Cls = new ClassInfo(), float64Cls->name = float64Cls->fullName = "double", float64Cls->size = sizeof(double);
    objectCls = new ClassInfo(), objectCls->name = objectCls->fullName = "object", objectCls->size = sizeof(uint64);

    basicTypeCls[0] = voidCls;
    basicTypeCls[1] = int8Cls, basicTypeCls[2] = uint8Cls, basicTypeCls[3] = int16Cls, basicTypeCls[4] = uint16Cls;
    basicTypeCls[5] = int32Cls, basicTypeCls[6] = uint32Cls, basicTypeCls[7] = int64Cls, basicTypeCls[8] = uint64Cls;
    basicTypeCls[9] = float32Cls, basicTypeCls[10] = float64Cls, basicTypeCls[11] = objectCls, basicTypeCls[12] = basicCls;

    for (int i = 0; i < 13; i++) 
        rootNsp->clsMap[basicTypeCls[i]->name] = basicTypeCls[i], basicTypeCls[i]->blgNsp = rootNsp;
    for (int i = 0; i < 12; i++) 
        basicTypeCls[i]->baseCls = basicCls, basicCls->derivedClasses.push_back(basicTypeCls[i]),
        basicTypeCls[i]->visibility = IdentifierVisibility::Public;
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
        // set the visibility
        if (clsNode->getVisibility() == IdentifierVisibility::Unknown) cls->visibility = IdentifierVisibility::Private;
        else cls->visibility = clsNode->getVisibility();
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
        ClassInfo *baseCls = nullptr;
        if (cls->defNode->getBaseNode() == nullptr) {
            baseCls = objectCls;
        } else {
            baseCls = (cls->defNode->getBaseNode() == nullptr ? objectCls : findCls(cls->defNode->getBaseNode()->getName()));
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
                    cls->genericParams.push_back(param);
                }
                if (!succ) printError(gArea->getToken().lineId, "Failed to set the generic params for base class");
            }
        }
        
        // build the tree
        baseCls->derivedClasses.push_back(cls);
        cls->baseCls = baseCls;
    }
    if (!succ) return false;
    // build the member structure for each class
    std::queue<ClassInfo *> que;
    que.push(objectCls);
    while (!que.empty()) {
        ClassInfo *bsCls = que.front(); que.pop();
        for (ClassInfo *dCls : bsCls->derivedClasses) {
            dCls->size = bsCls->size + dCls->genericClasses.size() * sizeof(uint8);
            dCls->dep = bsCls->dep + 1;
            // set identifier environment
            setCurNsp(dCls->blgNsp), setCurRoot(dCls->blgRoot), setCurCls(dCls);
            // use memory align to modify the offset of object members
            // inherit the members from base class
            for (auto &fld : bsCls->fieldMap) {
                if (fld.second->visibility == IdentifierVisibility::Private) continue;
                VariableInfo *vInfo = new VariableInfo(*fld.second);
                // substitute the generic class of base cls by the generic params provided by derived class
                for (size_t i = 0; i < bsCls->genericClasses.size(); i++)
                    vInfo->type = substitute(vInfo->type, bsCls->genericClasses[i], dCls->genericParams[i]);
                dCls->fieldMap[vInfo->name] = vInfo;
            }
            for (auto &funcList : bsCls->functionMap) {
                if (funcList.first == "@constructer") continue;
                for (auto func : funcList.second) {
                    FunctionInfo *fInfo = new FunctionInfo(*func);
                    if (fInfo->visibility == IdentifierVisibility::Private) continue;
                    // substitute the generic class of base cls by the generic params provided by derived class
                    for (size_t i = 0; i < bsCls->genericClasses.size(); i++)
                        fInfo->resType = substitute(fInfo->resType, bsCls->genericClasses[i], dCls->genericParams[i]);
                    for (auto &param : fInfo->params) {
                        param = new VariableInfo(*param);
                        for (size_t i = 0; i < bsCls->genericClasses.size(); i++)
                            param->type = substitute(param->type, bsCls->genericClasses[i], dCls->genericParams[i]);
                    }
                }
                dCls->functionMap.insert(funcList);
            }
            // then build the functions and fields of this class
            for (size_t i = 0; i < dCls->defNode->getFieldCount(); i++) {
                auto vDef = dCls->defNode->getFieldDef(i);
                // get the visibility of these variables
                IdentifierVisibility visibility = (vDef->getVisibility() == IdentifierVisibility::Unknown
                                                     ? IdentifierVisibility::Private : vDef->getVisibility());
                for (size_t i = 0; i < vDef->getLocalVarCount(); i++) {
                    VariableInfo *vInfo = new VariableInfo();
                    auto tpl = vDef->getVariable(i);
                    // set the basic information
                    vInfo->blgCls = dCls, vInfo->blgNsp = dCls->blgNsp, vInfo->blgRoot = dCls->blgRoot;
                    vInfo->setNameNode(std::get<0>(tpl));
                    vInfo->setTypeNode(std::get<1>(tpl));
                    vInfo->setInitNode(std::get<2>(tpl));
                    vInfo->visibility = visibility;
                    // generic the type of this variable
                    if (std::get<1>(tpl) == nullptr) {
                        printError(std::get<0>(tpl)->getToken().lineId, "Definition of field \"" + vInfo->name + "\" missed the type hint");
                        succ = false;
                    } else {
                        vInfo->type = ExprType(vInfo->getTypeNode());
                        succ &= vInfo->type.setCls();
                        vInfo->type.vtMdf = ValueTypeModifier::MemberRef;
                        if (succ) {
                            // set the offset of this variable
                            // align the memory offset if needed
                            if (vInfo->type.isObject()) dCls->size = alignTo(dCls->size, sizeof(uint64));
                            vInfo->offset = dCls->size;
                            dCls->size += vInfo->type.getSize();
                        }
                    }
                    dCls->fieldMap.insert(std::make_pair(vInfo->name, vInfo));
                }
            }
            auto gsMap = makeSubtitutionMap(bsCls->genericClasses, dCls->genericParams);
            for (size_t i = 0; i < dCls->defNode->getFuncCount(); i++) {
                auto fDef = dCls->defNode->getFuncDef(i);
                // set basic information
                FunctionInfo *fInfo = new FunctionInfo();
                fInfo->blgCls = dCls, fInfo->blgNsp = dCls->blgNsp, fInfo->blgRoot = dCls->blgRoot;
                fInfo->setDefNode(fDef);
                setCurFunc(fInfo);
                // set classes for all the expression type of this fInfo
                succ &= fInfo->resType.setCls();
                if (!succ) {
                    printError(fDef->getToken().lineId, "Definition of function " + fInfo->fullName + " has error in expression type param and return value checking");
                    delete fInfo;
                    continue;
                }
                std::vector<ExprType> paramTypeList;
                for (size_t i = 0; i < fInfo->params.size(); i++) {
                    succ &= fInfo->params[i]->type.setCls();
                    paramTypeList.push_back(fInfo->params[i]->type);
                }
                if (!succ) {
                    printError(fDef->getToken().lineId, "Definition of function " + fInfo->fullName + " has error in expression type param and return value checking");
                    delete fInfo;
                    continue;
                }
                // insert this function
                // find whether there is a function which is inherited from base and has the same param list
                bool cover = false;
                if (dCls->functionMap.count(fInfo->name)) {
                    auto &fList = dCls->functionMap[fInfo->name];
                    for (size_t i = 0; i < fList.size(); i++) {
                        auto &bsFunc = fList[i];
                        if (bsFunc->blgCls == dCls) continue;
                        // substitute all the generic params into the functions in base class
                        auto chkRes = bsFunc->satisfy(gsMap, paramTypeList);
                        // this function need to cover the function with the same name in the base class
                        if (chkRes.first) {
                            if (bsFunc->getDefNode()->getType() != fDef->getType()) {
                                printError(fDef->getToken().lineId, "The types of functions that have the same name and param list in both base class and derived class should be the same.");
                                succ = false;
                            } else {
                                if (fDef->getType() == SyntaxNodeType::VarFuncDef) {
                                    // if it is a varfunc, then the return value should have the same expression type and visibility
                                    if (chkRes.second != fInfo->resType || fInfo->visibility != bsFunc->visibility) {
                                        printError(fDef->getToken().lineId, "The expression type of return value of function \"" + fInfo->fullName + "\" is invalid.");
                                        succ = false;
                                    } else {
                                        // just cover the function
                                        fInfo->offset = bsFunc->offset;
                                        fList[i] = fInfo;
                                    }
                                    
                                } else fList[i] = fInfo; // simply cover the function
                            }
                            cover = true;
                            break;
                        }
                    }
                    
                }
                if (!cover) { // this is a new function
                    if (fDef->getType() == SyntaxNodeType::VarFuncDef) {
                        // create a variable to store the offset
                        fInfo->offset = dCls->size, dCls->size += sizeof(uint64);
                    }
                    dCls->functionMap[fInfo->name].push_back(fInfo);
                }
            }
            // push dCls into queue
            que.push(dCls);
        }
    }
    return succ;
}

bool buildCls(const RootList &roots) {
    bool succ = true;
    // build the symbols of classes
    succ &= buildClsSymbol(roots);
    // build inheritation tree
    succ &= buildClsTree(roots);
    return succ;
}

bool buildGlo(const RootList &roots) {
    std::map<RootNode *, uint64> vOffset;
    auto buildGloVar = [&](VarDefNode *varDef, NamespaceInfo *blgNsp, RootNode *blgRoot) -> bool {
        IdentifierVisibility visibility = (varDef->getVisibility() == IdentifierVisibility::Unknown ?
                                            IdentifierVisibility::Private : varDef->getVisibility());
        bool succ = true;
        setCurNsp(blgNsp);
        for (size_t i = 0; i < varDef->getLocalVarCount(); i++) {
            auto tpl = varDef->getVariable(i);
            VariableInfo *vInfo = new VariableInfo();
            vInfo->blgNsp = blgNsp, vInfo->blgRoot = blgRoot;
            vInfo->setNameNode(std::get<0>(tpl));
            vInfo->setTypeNode(std::get<1>(tpl));
            vInfo->setInitNode(std::get<2>(tpl));
            if (blgNsp->varMap.count(vInfo->name)) {
                printError(vInfo->getNameNode()->getToken().lineId, "Redefinition of variable " + vInfo->fullName);
                delete vInfo;
                succ = false;
                continue;
            }
            if (vOffset.count(blgRoot))
                vInfo->offset = vOffset[blgRoot], vOffset[blgRoot] += sizeof(vInfo->type.getSize());
            vInfo->visibility = visibility;
        }
        return succ;
    };
    auto buildGloFunc = [&](FuncDefNode *funcDef, NamespaceInfo *blgNsp, RootNode *blgRoot) -> bool {
        FunctionInfo *fInfo = new FunctionInfo();
        fInfo->blgNsp = blgNsp, fInfo->blgRoot = blgRoot;
        fInfo->setDefNode(funcDef);
        setCurFunc(fInfo);
        setCurNsp(blgNsp);
        // build the expression type of params and return value, then check if conflict
        bool succ = true;
        std::vector<ExprType> paramTypeList;
        for (size_t i = 0; i < fInfo->params.size(); i++) {
            succ &= fInfo->params[i]->type.setCls();
            paramTypeList.push_back(fInfo->params[i]->type);
        }
        succ &= fInfo->resType.setCls();
        if (!succ) {
            printError(fInfo->defNode->getToken().lineId, "Expression Type error occured in function \"" + fInfo->fullName + "\"");
            delete fInfo;
            return false;
        }
        // check whether this function conflicts with the existed ones.
        if (blgNsp->funcMap.count(fInfo->name)) {
            auto &fList = blgNsp->funcMap[fInfo->name];
            for (size_t i = 0; i < fList.size(); i++) {
                auto *otherFunc = fList[i];
                auto chkRes = otherFunc->satisfy(GenericSubstitutionMap(), paramTypeList);
                if (chkRes.first) {
                    printError(funcDef->getToken().lineId, "Definition of function \"" + fInfo->fullName + "\" conflicts with function \"" + otherFunc->fullName + "\"");
                    delete fInfo;
                    return false;
                } 
            }
        }
        blgNsp->funcMap[fInfo->name].push_back(fInfo);
        return true;
    };
    auto scanNsp = [&](NspDefNode *nspDef, RootNode *blgRoot) -> bool {
        NamespaceInfo *nsp = rootNsp;
        std::vector<std::string> path;
        stringSplit(nspDef->getNameNode()->getName(), '.', path);
        for (size_t i = 0; i < path.size(); i++) nsp = nsp->nspMap[path[i]];
        bool succ = true;
        for (size_t i = 0; i < nspDef->getVarCount(); i++) succ &= buildGloVar(nspDef->getVarDef(i), nsp, blgRoot);
        for (size_t i = 0; i < nspDef->getFuncCount(); i++) succ &= buildGloFunc(nspDef->getFuncDef(i), nsp, blgRoot);
        return succ;
    };
    bool succ = true;
    for (auto &root : roots) {
        setCurRoot(root);
        if (root->getType() == SyntaxNodeType::SourceRoot)
            vOffset.insert(std::make_pair(root, 0));
        for (size_t i = 0; i < root->getDefCount(); i++) {
            switch (root->getDef(i)->getType()) {
                case SyntaxNodeType::VarDef:
                    succ &= buildGloVar((VarDefNode *)root->getDef(i), rootNsp, root);
                    break;
                case SyntaxNodeType::FuncDef:
                    succ &= buildGloFunc((FuncDefNode *)root->getDef(i), rootNsp, root);
                    break;
                case SyntaxNodeType::NspDef:
                    succ &= scanNsp((NspDefNode *)root->getDef(i), root);
                    break;
            }
        }
    }
    return succ;
}

bool buildIdenSystem(const RootList &roots) {
    bool succ = true;
    buildRootInfo();
    succ &= buildCls(roots);
    succ &= buildGlo(roots);
    return succ;
}

void debugPrintClsStruct(ClassInfo *cls, int dep = 0) {
    if (cls == nullptr) return ;
    std::cout << getIndent(dep) << cls->name << " " << cls->fullName;
    if (cls->genericClasses.size() > 0) {
        std::cout << "<$";
        for (size_t i = 0; i < cls->genericClasses.size(); i++) {
            if (i > 0) std::cout << ", ";
            std::cout << cls->genericClasses[i]->name;
        }
        std::cout << "$>";
    }
    if (cls->baseCls != nullptr) {
        std::cout << getIndent(dep) << " : " << cls->baseCls->fullName;
        if (cls->genericParams.size() > 0) {
            std::cout << "<$";
            for (size_t i = 0; i < cls->genericParams.size(); i++) {
                if (i > 0) std::cout << ", ";
                std::cout << cls->genericParams[i].toDebugString();
            }
            std::cout << "$>";
        }
    }
    std::cout << std::endl;
    std::cout << getIndent(dep + 1) << "<size> = " << cls->size << std::endl;
    for (auto &varPair : cls->fieldMap) {
        std::cout << getIndent(dep + 1) << identifierVisibilityString[(int)varPair.second->visibility] << " " << varPair.first
         << "->" << varPair.second->fullName << " " << varPair.second->type.toDebugString() << " offset=" << varPair.second->offset << std::endl;
    }
    for (auto &funcPair : cls->functionMap) {
        std::cout << getIndent(dep + 1) << funcPair.first
         << "->" << std::endl;
        for (auto &func : funcPair.second) {
            std::cout << getIndent(dep + 2) << identifierVisibilityString[(int)func->visibility] << " " << func->name << " " << func->fullName;
            if (func->genericClasses.size() > 0) {
                std::cout << "<$";
                for (size_t i = 0; i < func->genericClasses.size(); i++) {
                    if (i > 0) std::cout << ", ";
                    std::cout << func->genericClasses[i]->name;
                }
                std::cout << "$>";
            }
            std::cout << "(";
            for (size_t i = 0; i < func->params.size(); i++) {
                if (i > 0) std::cout << ", ";
                std::cout << func->params[i]->name << ":" << func->params[i]->type.toDebugString();
            }
            std::cout << ") : " << func->resType.toDebugString();
            std::cout << std::endl;
            if (func->defNode->getType() == SyntaxNodeType::VarFuncDef)
                std::cout << getIndent(dep + 2) << "<offset> = " << func->offset << std::endl;
        }
    }
}

void debugPrintNspStruct(NamespaceInfo *nsp, int dep) {
    if (nsp == nullptr) return ;
    std::cout << getIndent(dep) << nsp->name << " " << nsp->fullName << std::endl;
    for (auto &pir : nsp->nspMap) debugPrintNspStruct(pir.second, dep + 1);
    for (auto &pir : nsp->clsMap) debugPrintClsStruct(pir.second, dep + 1);
    for (auto &pir : nsp->varMap)
        std::cout << getIndent(dep + 1) << identifierVisibilityString[(int)pir.second->visibility] << " "
             << pir.first << " "
             << pir.second->fullName << " : " << pir.second->type.toDebugString() << " <offset>=" << pir.second->offset << std::endl;
    for (auto &pir : nsp->funcMap) {
        std::cout << getIndent(dep + 1) << pir.first << "->" << std::endl;
        for (auto &func : pir.second) {
            std::cout << getIndent(dep + 2) << identifierVisibilityString[(int)func->visibility] << func->fullName;
            if (func->genericClasses.size() > 0) {
                std::cout << "<$";
                for (size_t i = 0; i < func->genericClasses.size(); i++) {
                    if (i > 0) std::cout << ", ";
                    std::cout << func->genericClasses[i]->name;
                }
                std::cout << "$>";
            }
            std::cout << "(";
            for (size_t i = 0; i < func->params.size(); i++) {
                if (i > 0) std::cout << ", ";
                std::cout << func->params[i]->name << ":" << func->params[i]->type.toDebugString();
            }
            std::cout << ") : " << func->resType.toDebugString() << std::endl;
        }
    }
}

