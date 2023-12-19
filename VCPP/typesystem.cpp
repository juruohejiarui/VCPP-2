#include "cpltree.h"

const std::string identifierTypeString[] = {
    "Variable", "Function", "VarFunction", "Class", "Namespace", "Unknown",
};
const int identifierTypeNumber = 5;

IdentifierType getIdentifierType(const std::string &name) {
    for (int i = 0; i < identifierTypeNumber; i++)
        if (name == identifierTypeString[i]) return (IdentifierType)i;
    return IdentifierType::Unknown;
}

EResultType::EResultType() {
    dimc = 0;
    cls = nullptr;
    clsName = "<unknown>";
    isConst = false;
    valueType = ValueTypeModifier::Unknown;
}

EResultType::EResultType(ClassInfo *cls, int dimc, ValueTypeModifier valueType, bool isConst) {
    this->cls = cls;
	this->dimc = dimc;
	this->valueType = valueType;
	this->isConst = isConst;
	this->clsName = cls->name;
}

EResultType EResultType::convertToParent() const {
	if (cls == nullptr) return nullptr;
	if (cls->parent == nullptr) return nullptr;
    EResultType result = *this;
    result.cls = cls->parent;
    result.clsName = cls->parent->name;
    result.dimc = 
}
