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

ExprResType::ExprResType() {
    dimc = 0;
    cls = nullptr;
    clsName = "<unknown>";
    valueType = ValueTypeModifier::Unknown;
}

ExprResType::ExprResType(ClassInfo *cls, int dimc, ValueTypeModifier valueType, bool isConst) {
    this->cls = cls;
	this->dimc = dimc;
	this->valueType = valueType;
	this->clsName = cls->name;
}

void SubsititueGeneric(ExprResType &eRes, ClassInfo *genericCls, ExprResType impl) {
    if (eRes.isGeneric()) {
        if (eRes.cls == genericCls) {
            size_t tmp = eRes.dimc;
            eRes = impl, eRes.dimc += tmp;
        }
        return ;
    }
    for (size_t i = 0; i < eRes.genericParams.size(); i++)
        SubsititueGeneric(eRes.genericParams[i], genericCls, impl);
}
ExprResType ExprResType::convertToParent() const {
	if (cls == nullptr) return nullptr;
	if (cls->parent == nullptr) return nullptr;
    ExprResType result = *this;
    // copy the basic info
    result.cls = cls->parent;
    result.clsName = cls->parent->name;
    result.dimc = dimc;
    result.valueType = valueType;
    // substitute the generic parameters
    result.genericParams.clear();
    for (size_t i = 0; i < cls->genericParams.size(); i++) {
        ExprResType impl = cls->genericParams[i];
        for (size_t j = 0; j < cls->genericClasses.size(); j++)
            SubsititueGeneric(impl, cls->genericClasses[j], genericParams[j]);
        result.genericParams.push_back(impl);
    }
    return result;
}

std::string ExprResType::toString() const {
    std::string res;
    if (cls != nullptr) res += cls->name;
    else res += clsName;
    if (dimc > 0) res += std::string(dimc, '*');
    if (isRef()) res += "&";
    if (genericParams.size() > 0) {
        res += "<$";
        for (size_t i = 0; i < genericParams.size(); i++) {
            if (i > 0) res += ", ";
            res += genericParams[i].toString();
        }
        res += "$>";
    }
    return res;
}

bool ExprResType::isGeneric() const { return cls != nullptr && cls->isGenericIdentifier; }
bool ExprResType::isRef() const {  return valueType == ValueTypeModifier::r || valueType == ValueTypeModifier::mr; }

bool ExprResType::equalTo(const ExprResType &other) const {
    bool genericEqual = true;
    if (genericParams.size() != other.genericParams.size()) genericEqual = false;
    else for (size_t i = 0; i < genericParams.size(); i++)
        if (!genericParams[i].equalTo(other.genericParams[i])) genericEqual = false;
    if (cls == other.cls && dimc == other.dimc && genericEqual) return true;
    if (cls == nullptr || other.cls == nullptr) return false;
    return convertToParent().equalTo(other);
}

/**
 * Generates the expression result type based on the given type node.
 *
 * @param typeNode The identifier node representing the type.
 * @return The expression result type.
 */
ExprResType generateExprResType(IdentifierNode *typeNode) {
    ExprResType result;
    result.clsName = typeNode->getName();
    result.dimc = typeNode->getDimc();
    result.valueType = ValueTypeModifier::t;
    if (typeNode->childrenCount() > 0) {
        auto genericNode = typeNode->getGenericArea();
        for (size_t i = 0; i < genericNode->childrenCount(); i++)
            result.genericParams.push_back(generateExprResType(genericNode->getParam(i)));
    }
    return result;
}

IdentifierInfo::IdentifierInfo() {
    type = IdentifierType::Unknown;
    name = fullName = "<unknown>";
    defNode = nullptr;
    visibility = IdentifierVisibility::Unknown;
}

IdentifierInfo::IdentifierInfo(SyntaxNode *defNode) {
    type = IdentifierType::Unknown;
    name = fullName = "<unknown>";
    this->defNode = defNode;
    visibility = IdentifierVisibility::Unknown;
}

VariableInfo::VariableInfo() : IdentifierInfo() { type = IdentifierType::Variable; }
VariableInfo::VariableInfo(IdentifierNode *nameNode, IdentifierNode *typeNode) : IdentifierInfo(nameNode) {
    type = IdentifierType::Variable;
    name = nameNode->getName();
    fullName = "<unknown>";
    visibility = IdentifierVisibility::Unknown;
    resType = generateExprResType(typeNode);
}

FunctionInfo::FunctionInfo(bool isVarFunction) : IdentifierInfo() {
    type = isVarFunction ? IdentifierType::VarFunction : IdentifierType::Function;
    name = fullName = "<unknown>";
    visibility = IdentifierVisibility::Unknown;
}

FunctionInfo::FunctionInfo(FuncDefNode *defNode, bool isVarFunction) : IdentifierInfo(defNode) {
    type = isVarFunction ? IdentifierType::VarFunction : IdentifierType::Function;
    name = defNode->getName();
    fullName = "<unknown>";
    visibility = defNode->getVisibility();
    
}