#include "geninner.h"

uint64 ifCount, loopCount;

struct LoopLabel {
    std::string start, end, contentEnd;
    LocalVarFrame *blgFrm;
    LoopLabel(const std::string &start, const std::string &end, const std::string &contentEnd, LocalVarFrame *blgFrm) 
        : start(start), end(end), contentEnd(contentEnd), blgFrm(blgFrm) {}
};
std::stack<LoopLabel> loopStk;

/// @brief build the vcode of an expression. PS: you need to ensure that this expression has pass the expression type checking before using this function
/// @param node the root of this expression
/// @return if the vcode of this expression are generated successfully
bool buildExpression(ExpressionNode * node) {
    if (node == nullptr) return true;
    switch (node->getType()) {
        case SyntaxNodeType::Expression: return buildExpression(node->getContent());
        case SyntaxNodeType::Identifier: {
            auto idenNode = (IdentifierNode *)node;
            if (idenNode->isFuncCall()) {
                
            }
        }
    }
    return true;
}

bool buildIf(IfNode *node) {
    return true;
}

bool buildLoop(LoopNode *node) {
    return true;
}

bool buildControl(ControlNode *node) {
    return true;
}

bool buildBlock(BlockNode *node) {
    return true;
}

bool buildVarDef(VarDefNode *node) {
    return true;
}

bool buildVCode(SyntaxNode *node) {
    if (node == nullptr) return true;
    switch (node->getType()) {
        case SyntaxNodeType::Expression: {
            auto chkRes = chkEType((ExpressionNode *)node);
            if (std::get<0>(chkRes)) return false;
            buildExpression((ExpressionNode *)node);
            if (std::get<1>(chkRes) != ExprType(voidCls)) 
                writeVCode(wrap(TCommand::pop, getDtMdf(std::get<1>(chkRes))));
            break;
        }
        case SyntaxNodeType::If: return buildIf((IfNode *)node);
        case SyntaxNodeType::While:
        case SyntaxNodeType::For:
            return buildLoop((LoopNode *)node);
        case SyntaxNodeType::Return:
        case SyntaxNodeType::Break:
        case SyntaxNodeType::Continue:
            return buildControl((ControlNode *)node);
        case SyntaxNodeType::Block:
            return buildBlock((BlockNode *)node);
        case SyntaxNodeType::VarDef:
            return buildVarDef((VarDefNode *)node);
    }
    return true;
}

bool buildConstructer(FunctionInfo *func) {
    setCurFunc(func);
    bool res = updOperCandy();
    
    writeVCode("#LABEL " + func->fullName);
    indentInc();
    locVarStkPush();
    writeVCode(Command::setlocal, UnionData(func->getDefNode()->getLocalVarCount() + 1));

    // set the param list in the local variable stack
    locVarStkPop(true);
    for (size_t i = 0; i < func->params.size(); i++)
        func->params[i]->offset = i + 1, res &= locVarStkTop()->insertVar(func->params[i]);
    {
        VariableInfo *thisInfo = new VariableInfo();
        thisInfo->name = thisInfo->fullName = "@this";
        thisInfo->offset = 0;
        thisInfo->type = ExprType(func->blgCls->fullName);
        thisInfo->type.setCls();
        thisInfo->blgFunc = func, thisInfo->blgCls = func->blgCls, thisInfo->blgNsp = func->blgNsp, thisInfo->blgRoot = func->blgRoot;
        res &= locVarStkTop()->insertVar(thisInfo);
    }

    writeVCode(Command::setarg, UnionData((uint64)func->params.size() + 1));

    // set the gtable in @this
    ClassInfo *cls = func->blgCls;
    for (size_t i = 0; i < cls->generCls.size(); i++) 
        writeVCode(Command::o_pvar, UnionData(0ull)), 
        writeVCode(Command::i64_r_mem, UnionData(cls->baseCls->size + i * sizeof(uint64))),
        writeVCode(Command::getgtbl, UnionData((uint64)i)),
        writeVCode(Command::i64_mr_t_mov);

    res &= buildVCode(func->getDefNode()->getContent());
    indentDec();
    
    setCurFunc(nullptr);
    updOperCandy();
    locVarStkPop(true);
    
    if (func->resType.cls != voidCls) writeVCode(Command::vret);
    else writeVCode(Command::ret);

    return res;
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
    indentDec();

    // get the gtable
    uint64 gtableSize = func->generCls.size() + (isMember ? func->blgCls->generCls.size() : 0);
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
    locVarStkPop(true);
    
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