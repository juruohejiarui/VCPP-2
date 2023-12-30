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
void VariableInfo::setTypeNode(IdentifierNode *typeNode) {
    this->typeNode = typeNode;
    type = ExprType(typeNode);
    
}
