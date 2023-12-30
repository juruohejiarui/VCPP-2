#include "idensys.h"

const std::string unknownName = "<unknown>";
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

std::string ExprType::toString() const {
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
    else if (blgNsp != nullptr) fullName = blgNsp->fullName + "." + name;
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
    else if (blgNsp != nullptr) fullName = blgNsp->fullName + "." + nameWithParam;
    else fullName = unknownName;
}

IdentifierRegion FunctionInfo::getRegion() const {
    if (blgCls != nullptr) return IdentifierRegion::Member;
    else if (blgNsp != nullptr) return IdentifierRegion::Global;
    return IdentifierRegion::Unknown;
}

bool buildIdenSystem(const RootList &roots) {
    for (auto root : roots) {
        
    }
}
