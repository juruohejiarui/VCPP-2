#include "geninner.h"

static std::ofstream oStream;

void generateTypeData_Var(VariableInfo *var, int dep = 0) {
    if (var->blgRoot->getType() == SyntaxNodeType::SymbolRoot) return ;
    oStream << getIndent(dep) << "V " << var->name;
    oStream << " v " << idenVisibilityStr[(int)var->visibility];
    oStream << " : " << var->type.toVtdString();
    oStream << " " << toString(var->offset, 16) << "\n";
}

std::string getFuncName(FunctionInfo *func) {
    std::string name;
    if (func->blgCls != nullptr) name = func->fullName.substr(func->blgCls->fullName.size() + 1);
    else {
        if (func->blgNsp != rootNsp) name = func->fullName.substr(func->blgNsp->fullName.size() + 1);
        else name = func->fullName;
    }
    return name;
}
void generateTypeData_Func(FunctionInfo *func, int dep = 0) {
    if (func->blgRoot->getType() == SyntaxNodeType::SymbolRoot) return ;
    oStream << getIndent(dep) << "F " + func->nameWithParam
            << " F " + func->fullName
            << " v " + idenVisibilityStr[(int)func->visibility]
            << " " + toString(func->generCls.size(), 16)
            << " : " + func->resType.toVtdString()
            << " {";
    for (const auto &param : func->params)
        oStream << " : " + param->type.toVtdString();
    oStream << "}\n";
    if (func->getDefNode()->getType() == SyntaxNodeType::VarFuncDef) {
        oStream << getIndent(dep) << "V " + func->nameWithParam
                << " v " + idenVisibilityStr[(int)func->visibility]
                << " : ulong"
                << " " + toString(func->offset, 16)
                << "\n"; 
    }
}

void generateTypeData_Cls(ClassInfo *cls, int dep = 0) {
    if (isBasicCls(cls) || cls == basicCls || cls == objectCls || cls->blgRoot->getType() == SyntaxNodeType::SymbolRoot) return ;
    oStream << getIndent(dep) << "C " << cls->name
            << " C " << cls->baseCls->fullName
            << " v " << idenVisibilityStr[(int)cls->visibility]
            << " " + toString(cls->size, 16)
            << " " + toString(cls->generCls.size(), 16)
            << " " + toString(cls->baseCls->size, 16)
            << "{\n";
    for (auto vPair : cls->fieldMap) generateTypeData_Var(vPair.second, dep + 1);
    for (auto fPair : cls->funcMap)
        for (auto func : fPair.second) if (func->blgCls == cls)
            generateTypeData_Func(func, dep + 1);
    oStream << getIndent(dep) << "}" << std::endl;
}

void generateTypeData_Nsp(NamespaceInfo *nsp, int dep = 0) {
    oStream << getIndent(dep);
    if (nsp != rootNsp) oStream << "N " << nsp->name;
    oStream << "{" << std::endl;
    for (auto &vPair : nsp->varMap)
        generateTypeData_Var(vPair.second, dep + 1);
    for (auto &vPair : nsp->funcMap)
        for (auto func : vPair.second)
            generateTypeData_Func(func, dep + 1);
    for (auto &vPair : nsp->clsMap)
        generateTypeData_Cls(vPair.second, dep + 1);
    for (auto &vPair : nsp->nspMap)
        generateTypeData_Nsp(vPair.second, dep + 1);
    oStream << getIndent(dep) << "}" << std::endl;
}
bool generateTypeData(const std::string &path) {
    oStream = std::ofstream(path, std::ios::out);
    if (!oStream.good()) {
        printError(0, "Fail to write file " + path);
        return false;
    }
    generateTypeData_Nsp(rootNsp);
    oStream.close();
    return true;
}

std::string toCode(const std::string &vcodeName) {
    std::vector<std::string> prt;
    std::string res;
    stringSplit(vcodeName, '.', prt);
    for (size_t i = 0; i < prt.size(); i++) {
        if (i > 0) res.append("::");
        res.append(prt[i]);
    }
    return res;
}

bool generateDefData_EType(const ExprType &etype) {
    oStream << toCode(etype.cls->fullName);
    if (etype.generParams.size() > 0) {
        oStream << "<$";
        for (size_t i = 0; i < etype.generParams.size(); i++) {
            if (i > 0) oStream << ", ";
            generateDefData_EType(etype.generParams[i]);
        }
        oStream << "$>";
    }
    for (size_t i = 0; i < etype.dimc; i++) oStream << "[]";
    return true;
}

bool generateDefData_Var(VariableInfo *var, int dep) {
    if (var->blgRoot->getType() == SyntaxNodeType::SymbolRoot) return true;
    oStream << getIndent(dep) << idenVisibilityStr[(int)var->visibility] << " var " << var->name << " : ";
    generateDefData_EType(var->type);
    oStream << ";\n";
    return true;
}
bool generateDefData_GClassList(const std::vector<ClassInfo *> &gClsList) {
    if (gClsList.size() > 0) {
        oStream << "<$";
        for (size_t i = 0; i < gClsList.size(); i++) {
            if (i > 0) oStream << ", ";
            oStream << gClsList[i]->name;
        }
        oStream << "$>";
    }
    return true;
}

bool generateDefData_Func(FunctionInfo *func, int dep) {
    if (func->blgRoot->getType() == SyntaxNodeType::SymbolRoot) return true;
    oStream << getIndent(dep) << idenVisibilityStr[(int)func->visibility];
    if (func->getDefNode()->getType() == SyntaxNodeType::VarFuncDef) oStream << " varfunc ";
    else oStream << " func ";
    oStream << func->name;
    generateDefData_GClassList(func->generCls);
    oStream << "(";
    for (size_t i = 0; i < func->params.size(); i++) {
        if (i > 0) oStream << ", ";
        oStream << func->params[i]->name << " : ";
        generateDefData_EType(func->params[i]->type);
    }
    oStream << ") : ";
    generateDefData_EType(func->resType);
    oStream << ";\n";
    return true;
}

bool generateDefData_Cls(ClassInfo *cls, int dep) {
    if (isBasicCls(cls)) return true;
    if (cls->blgRoot->getType() == SyntaxNodeType::SymbolRoot) return true;
    oStream << getIndent(dep) << idenVisibilityStr[(int)cls->visibility] << " class " << cls->name;
    generateDefData_GClassList(cls->generCls);
    oStream << " : " << toCode(cls->baseCls->fullName);
    if (cls->generParams.size() > 0) {
        oStream << "<$";
        for (size_t i = 0; i < cls->generParams.size(); i++) {
            if (i > 0) oStream << ", ";
            generateDefData_EType(cls->generParams[i]);
        }
        oStream << "$>";
    }
    oStream << "{\n";
    for (auto &vPair : cls->fieldMap) if (vPair.second->blgCls == cls) generateDefData_Var(vPair.second, dep + 1);
    for (auto &fPair : cls->funcMap)
        for (auto func : fPair.second) if (func->blgCls == cls) generateDefData_Func(func, dep + 1);
    oStream << getIndent(dep) << "}\n";
    return true;
}
bool generateDefData_Nsp(NamespaceInfo *nsp) {
    int nxtDep = 0;
    if (nsp != rootNsp) oStream << "namespace " << toCode(nsp->fullName) << "{\n", nxtDep = 1;
    for (auto &vPair : nsp->varMap) generateDefData_Var(vPair.second, nxtDep);
    for (auto &fPair : nsp->funcMap)
        for (auto func : fPair.second) generateDefData_Func(func, nxtDep);
    for (auto &cPair : nsp->clsMap) generateDefData_Cls(cPair.second, nxtDep);
    if (nsp != rootNsp) oStream << "}\n";
    for (auto &nPair : nsp->nspMap) generateDefData_Nsp(nPair.second);
    return true;
}
bool generateDefData(const std::string &path) {
    oStream = std::ofstream(path, std::ios::out);
    if (!oStream.good()) {
        printError(0, "Fail to write file " + path);
        return false;
    }
    generateDefData_Nsp(rootNsp);
    oStream.close();
    return true;
}

bool generateSymbol(const std::string &vdtPath, const std::string &defPath) {
    bool res1 = generateTypeData(vdtPath), res2 = generateDefData(defPath);
    return res1 && res2;
}