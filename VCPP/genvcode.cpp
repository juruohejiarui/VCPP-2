#include "geninner.h"

bool buildVCode(SyntaxNode *node) {

}

bool buildConstructer(FunctionInfo *func) {
    setCurFunc(func);
    updOperCandy();
    
    writeVCode("#LABEL " + func->fullName);
    indentInc();
    locVarStkPush();
    writeVCode(Command::setlocal, UnionData(func->getDefNode()->getLocalVarCount() + 1));
    locVarStkPop(true);
    indentDec();
    
}
bool buildFunc(FunctionInfo *func) {
    if (func->blgRoot->getType() != SyntaxNodeType::SourceRoot) return true;
    if (func->name == "@constructer") return buildConstructer(func);

    // do not generate empty function
    if (func->getDefNode()->getContent() == nullptr) return true;

    // set the environment
    setCurFunc(func);
    updOperCandy();
    bool isMember = (func->blgCls != nullptr), res = true;

    writeVCode("#LABEL " + func->fullName);
    indentInc();
    locVarStkPush();
    writeVCode(Command::setlocal, UnionData(func->getDefNode()->getLocalVarCount() + isMember));
    locVarStkPop(true);
    indentDec();

    // get the gtable
    uint64 gtableSize = func->generCls.size() + (isMember ? func->blgCls->generCls.size() : 0);
    writeVCode(Command::getgtbl, UnionData(gtableSize));
    // get the param list (if this function is a member of a class, remember to add "@this")
    writeVCode(Command::getarg, UnionData((uint64) (func->params.size() + isMember)));
    
    // insert the params into local variable stack
    for (size_t i = 0; i < func->params.size(); i++) 
        locVarStkTop()->insertVar(func->params[i]), func->params[i]->offset = i + isMember;
    // if this function is a member of a class, then there should be a param named "@this"
    if (isMember) {
        VariableInfo *thisInfo = new VariableInfo();
        thisInfo->name = thisInfo->fullName = "@this";
        thisInfo->offset = 0;
        thisInfo->type = ExprType(func->blgCls->fullName);
        thisInfo->type.setCls();
        thisInfo->blgFunc = func, thisInfo->blgCls = func->blgCls, thisInfo->blgNsp = func->blgNsp, thisInfo->blgRoot = func->blgRoot;
        res &= locVarStkTop()->insertVar(thisInfo);
    }

    res &= buildVCode(func->getDefNode()->getContent());

    setCurFunc(nullptr);
    updOperCandy();
    
    if (func->resType.cls != voidCls) writeVCode(Command::vret);
    else writeVCode(Command::ret);

    return res;
}

bool scanCls(ClassInfo *cls) {
    if (isBasicCls(cls) || cls == basicCls || cls->blgRoot->getType() != SyntaxNodeType::SourceRoot) return true;

}
bool scanNsp(NamespaceInfo *nsp) {
    
}
bool generateVCode(const std::string &vasmPath, const std::vector<std::string> &relyPath) {
    initOperCandy();

    bool res = setOutputStream(vasmPath);
    if (!res) return false;
    res = scanNsp(rootNsp);
    
}