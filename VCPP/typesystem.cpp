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

IdentifierInfo::~IdentifierInfo() {} 

VariableInfo::VariableInfo() : IdentifierInfo() { offset = 0, type = IdentifierType::Variable; }
VariableInfo::VariableInfo(IdentifierNode *nameNode, IdentifierNode *typeNode) : IdentifierInfo(nameNode) {
    offset = 0;
    type = IdentifierType::Variable;
    name = nameNode->getName();
    fullName = "<unknown>";
    visibility = IdentifierVisibility::Unknown;
    resType = generateExprResType(typeNode);
}

VariableInfo::~VariableInfo() {}

std::string VariableInfo::toString() const {
    std::string res = "VariableInfo: " + name + " " + resType.toString() + " offset = " + std::to_string(offset) + "\n";
    return res;
}

FunctionInfo::FunctionInfo(bool isVarFunction) : IdentifierInfo() {
    offset = 0;
    type = isVarFunction ? IdentifierType::VarFunction : IdentifierType::Function;
    name = fullName = "<unknown>";
    visibility = IdentifierVisibility::Unknown;
}

FunctionInfo::FunctionInfo(FuncDefNode *defNode, bool isVarFunction) : IdentifierInfo(defNode) {
    offset = 0;
    type = isVarFunction ? IdentifierType::VarFunction : IdentifierType::Function;
    name = defNode->getName();
    fullName = "<unknown>";
    visibility = defNode->getVisibility();   
}

FunctionInfo::~FunctionInfo() {
    for (size_t i = 0; i < params.size(); i++) if (params[i] != nullptr) delete params[i];
    for (size_t i = 0; i < genericClasses.size(); i++) if (genericClasses[i] != nullptr) delete genericClasses[i];
}

std::string FunctionInfo::toString() const {
    std::string res = "FunctionInfo: " + name + " " + resType.toString() + " offset = " + std::to_string(offset) + "\n";
    return res;
}

ClassInfo::ClassInfo() : IdentifierInfo() {
    type = IdentifierType::Class;
    name = fullName = "<unknown>";
    visibility = IdentifierVisibility::Unknown;
    parent = nullptr;
    isGenericIdentifier = false;
}

ClassInfo::ClassInfo(ClsDefNode *defNode) : IdentifierInfo(defNode) {
    type = IdentifierType::Class;
    name = defNode->getName();
    fullName = "<unknown>";
    visibility = defNode->getVisibility();
    parent = nullptr;
    isGenericIdentifier = false;
}

ClassInfo::~ClassInfo() {
    for (auto &pir : varMap) if (pir.second != nullptr) delete pir.second;
    for (auto &pir : funcMap)
        for (size_t i = 0; i < pir.second.size(); i++)
            if (pir.second[i] != nullptr) delete pir.second[i];
    for (size_t i = 0; i < genericClasses.size(); i++)
        if (genericClasses[i] != nullptr) delete genericClasses[i];
}

std::string ClassInfo::toString() const {
    std::string res = "ClassInfo: " + name + " " + "->" + fullName + "\n";
    return res;
}

NamespaceInfo::NamespaceInfo() : IdentifierInfo() {
    type = IdentifierType::Namespace;
    name = fullName = "<unknown>";
    visibility = IdentifierVisibility::Public;
}

NamespaceInfo::NamespaceInfo(NspDefNode *defNode) : IdentifierInfo(defNode) {
    type = IdentifierType::Namespace;
    name = defNode->getName();
    fullName = "<unknown>";
    visibility = IdentifierVisibility::Public;
}

NamespaceInfo::~NamespaceInfo() {
    for (auto &pir : varMap) if (pir.second != nullptr) delete pir.second;
    for (auto &pir : funcMap)
        for (size_t i = 0; i < pir.second.size(); i++)
            if (pir.second[i] != nullptr) delete pir.second[i];
    for (auto &pir : clsMap) if (pir.second != nullptr) delete pir.second;
    for (auto &pir : nspMap) if (pir.second != nullptr) delete pir.second;
}

std::string NamespaceInfo::toString() const {
    std::string res = "NamespaceInfo: " + name + " " + "->" + fullName + "\n";
    return res;
}

bool buildTypeSystem(const std::vector<SyntaxNode *> &roots) {
    
    return false;
}
