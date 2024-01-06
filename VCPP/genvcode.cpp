#include "geninner.h"

uint64 ifCount, loopCount;

struct LoopLabel {
    std::string start, end, contentEnd;
    LocalVarFrame *blgFrm;
    LoopLabel(const std::string &start, const std::string &end, const std::string &contentEnd, LocalVarFrame *blgFrm) 
        : start(start), end(end), contentEnd(contentEnd), blgFrm(blgFrm) {}
};
std::stack<LoopLabel> loopStk;

uint8 getGTableId(ClassInfo *gCls) {
    uint8 shift = 0;
    if (getCurCls() != nullptr) {
        shift = getCurCls()->generCls.size();
        for (uint8 i = 0; i < getCurCls()->generCls.size(); i++) 
            if (getCurCls()->generCls[i] == gCls) return i;
    }
    for (uint8 i = 0; i < getCurFunc()->generCls.size(); i++)
        if (getCurFunc()->generCls[i] == gCls) return i + shift;
    return 5;
}

bool buildIdentifier(IdentifierNode *idenNode) {
    bool res = true;
    if (idenNode->isFuncCall()) {
        // since that the member function calling which miss "@this" have been handled and add callMem operator
        // the program below just need to handle the global function calling
        auto &info = getFuncCallInfo(idenNode);
        const GTableData &gtbl = std::get<2>(info);
        if (gtbl.size > 0) {
            for (uint8 i = 0; i < gtbl.size; i++) {
                if (gtbl[i].dimc > 0) writeVCode(Command::i64_push, UnionData(DataTypeModifier::o));
                else if (gtbl[i].cls->isGeneric)
                    writeVCode(Command::getgtbl, UnionData(getGTableId(gtbl[i].cls)));
                else writeVCode(Command::i64_push, UnionData(getDtMdf(gtbl[i])));
            }
            writeVCode(Command::setgtbl, UnionData(gtbl.size));
        }
        for (size_t i = 0; i < idenNode->getParamCount(); i++)
            res &= buildExpression(idenNode->getParam(i));
        writeVCode(Command::call, std::get<0>(info)->fullName);
    } else {
        // the program belpow just need to handle the global variable calling and local variable calling
        auto &info = getVarCallInfo(idenNode);
        VariableInfo *vInfo = std::get<0>(info);
        DataTypeModifier dtMdf = (std::get<1>(info).dimc > 0 ? DataTypeModifier::o : getDtMdf(std::get<1>(info)));
        // this variable is a local variable
        if (vInfo->blgFunc != nullptr) writeVCode(wrap(TCommand::pvar, dtMdf), UnionData(vInfo->offset));
        // this variable is a global variable
        else writeVCode(wrap(TCommand::pglo, dtMdf), vInfo->fullName);
    }
    return res;
}

bool buildOperNode(OperatorNode *operNode) {
    bool res = true;
    auto operTk = operNode->getToken().type;
    switch (operTk) {
        case TokenType::Add:
    }
    return res;
}

/// @brief build the vcode of an expression. PS: you need to ensure that this expression has pass the expression type checking before using this function
/// @param node the root of this expression
/// @return if the vcode of this expression are generated successfully
bool buildExpression(ExpressionNode * node) {
    if (node == nullptr) return true;
    bool res = false;
    switch (node->getType()) {
        case SyntaxNodeType::Expression: return buildExpression(node->getContent());
        case SyntaxNodeType::Identifier: return buildIdentifier((IdentifierNode *)node);
        case SyntaxNodeType::Operator: return buildOperNode((OperatorNode *)node);
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