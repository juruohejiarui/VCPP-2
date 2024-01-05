#include "gen.h"

static std::ofstream oStream;

void generateVar(VariableInfo *var, int dep = 0) {
    oStream << getIndent(dep) << "V " << var->name;
    oStream << " v " << idenVisibilityStr[(int)var->visibility] << " " << toString(var->offset, 16);
    oStream << " : " << var->type.toVtdString() << std::endl;
}

void generateFunc(FunctionInfo *func, int dep = 0) {
    oStream << getIndent(dep) << "F " << func->nameWithParam;
    oStream << " v " << idenVisibilityStr[(int)func->visibility] << " : " <<  func->resType.toVtdString();
    oStream << "{";
    for (size_t i = 0; i < func->params.size(); i++)
        oStream << " : " << func->params[i]->type.toVtdString();
    oStream << "}" << std::endl;
    // generate the variable for this function
    if (func->getDefNode()->getType() == SyntaxNodeType::VarFuncDef) {
        oStream << getIndent(dep) << "V " << func->nameWithParam;
        oStream << " v " << idenVisibilityStr[(int)func->visibility];
        oStream << " " << toString(func->offset, 16);
        oStream << " : " << func->resType.toVtdString() << std::endl;
    }
}

void generateCls(ClassInfo *cls, int dep = 0) {
    if (isBasicCls(cls) || cls == basicCls || cls == objectCls) return ;
    oStream << getIndent(dep) << "C " << cls->name;
    oStream << " v " << idenVisibilityStr[(int)cls->visibility];
    oStream << " " << toString(cls->size, 16) << " {" << std::endl;
    for (auto vPair : cls->fieldMap) generateVar(vPair.second, dep + 1);
    for (auto fPair : cls->funcMap)
        for (auto func : fPair.second)
            generateFunc(func, dep + 1);
    // write the generic map of members
    std::set<uint64> fOffsetSet;
    // write the table of varfunc
    for (auto tCls = cls; tCls != objectCls; tCls = tCls->baseCls) {
        for (auto fPair : tCls->funcMap)
            for (auto func : fPair.second) {
                if (func->getDefNode()->getType() != SyntaxNodeType::VarFuncDef || fOffsetSet.count(func->offset)) continue;
                fOffsetSet.insert(func->offset);
                oStream << getIndent(dep + 1) << toString(func->offset, 16) << " " << "F " << func->fullName << std::endl;
            }
    }
    // write the table of size of generic class for base class
    {
        auto tCls = cls;
        auto gParam = cls->generParams;
        for (; tCls->baseCls != objectCls; tCls = tCls->baseCls) {
            ClassInfo *bsCls = tCls->baseCls;
            GenerSubstMap gsMap;
            for (size_t i = 0; i < gParam.size(); i++)
                oStream << getIndent(dep + 1) << toString(bsCls->size + i * sizeof(uint64), 16) << " " << toString(gParam[i].getSize(), 16) << std::endl,
                gsMap.insert(std::make_pair(bsCls->generCls[i], gParam[i]));
            auto newParam = bsCls->generParams;
            for (size_t i = 0; i < newParam.size(); i++) newParam[i] = subst(newParam[i], gsMap);
            gParam = newParam;
        }
    }
    
    oStream << getIndent(dep) << "}" << std::endl;
}

void generateNsp(NamespaceInfo *nsp, int dep = 0) {
    oStream << getIndent(dep);
    if (nsp != rootNsp) oStream << "N " << nsp->name;
    oStream << "{" << std::endl;
    for (auto &vPair : nsp->varMap)
        generateVar(vPair.second, dep + 1);
    for (auto &vPair : nsp->funcMap)
        for (auto func : vPair.second)
            generateFunc(func, dep + 1);
    for (auto &vPair : nsp->clsMap)
        printf("%s\n", vPair.first.c_str()),
        generateCls(vPair.second, dep + 1);
    for (auto &vPair : nsp->nspMap)
        generateNsp(vPair.second, dep + 1);
    oStream << getIndent(dep) << "}" << std::endl;
}
bool generateTypeData(const std::string &path) {
    oStream = std::ofstream(path, std::ios::out);
    if (!oStream.good()) {
        printError(0, "Fail to write file " + path);
        return false;
    }
    generateNsp(rootNsp);
    oStream.close();
    return true;
}

bool generateSymbol(const std::string &vdtPath, const std::string &defPath) {
    return generateTypeData(vdtPath);
}