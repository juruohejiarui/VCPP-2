#include "geninner.h"

uint64 ifCount, loopCount, logicCount;

struct LoopLabel {
    std::string start, end, contentEnd;
    LocalVarFrame *blgFrm;
    LoopLabel(const std::string &start, const std::string &end, const std::string &contentEnd, LocalVarFrame *blgFrm) 
        : start(start), end(end), contentEnd(contentEnd), blgFrm(blgFrm) {}
};
std::stack<LoopLabel> loopStk;

DataTypeModifier getRealDtMdf(const ExprType &etype) {
    if (etype.dimc > 0) return DataTypeModifier::o;
    return getDtMdf(etype);
}

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

bool writeSetGTable(const GTableData &gtbl) {
    if (gtbl.size == 0) return true;
    for (uint8 i = 0; i < gtbl.size; i++) {
        if (gtbl[i].dimc > 0) writeVCode(Command::i64_push, UnionData(DataTypeModifier::o));
        else if (gtbl[i].cls->isGeneric) {
            uint8 id = getGTableId(gtbl[i].cls);
            if (id == 5) return false;
            writeVCode(Command::getgtbl, UnionData((uint64)id));
        }
        else writeVCode(Command::i64_push, UnionData((int64)getDtMdf(gtbl[i])));
    }
    writeVCode(Command::setgtbl, UnionData(gtbl.size));
    return true;
}

bool writeGvl(const ExprType &etype) {
    if (etype.vtMdf == ValueTypeModifier::TrueValue) return ;
    writeVCode(wrap(TCommand::gvl, getRealDtMdf(etype), etype.vtMdf));
    return true;
}

bool writePushArg(IdentifierNode *idenNode, uint64 &argCount) {
    bool res = true;
    argCount = 0;
    for (size_t i = 0; i < idenNode->getParamCount(); i++)
        if (idenNode->getParam(i) != nullptr) {
            argCount++;
            res &= buildExpression(idenNode->getParam(i));
            const ExprType &etype = getEType(idenNode->getParam(i));
            res &= writeGvl(etype);
        }
    return res;
}

bool buildIdentifier(IdentifierNode *idenNode) {
    bool res = true;
    if (idenNode->isFuncCall()) {
        // since that the member function calling which miss "@this" have been handled and add callMem operator
        // the program below just need to handle the global function calling
        auto &info = getFuncCallInfo(idenNode);
        uint64 argCnt = 0;
        res &= writeSetGTable(std::get<2>(info));
        res &= writePushArg(idenNode, argCnt);
        writeVCode(Command::setarg, UnionData(argCnt));
        writeVCode(Command::call, std::get<0>(info)->fullName);
    } else {
        // the program belpow just need to handle the global variable calling and local variable calling
        auto &info = getVarCallInfo(idenNode);
        VariableInfo *vInfo = std::get<0>(info);
        DataTypeModifier dtMdf = getRealDtMdf(std::get<1>(info));
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
    auto buildBinOper = [&] () -> bool {
        auto dtMdf = getDtMdf(getEType(operNode));
        auto etypeLeft = getEType(operNode->getLeft());
        auto etypeRight = getEType(operNode->getRight());
        TCommand tcmd = getTCommand(operTk);
        if (tcmd == TCommand::unknown) return false;
        writeVCode(wrap(tcmd, dtMdf, etypeLeft.vtMdf, etypeRight.vtMdf));
        return true;
    };
    switch (operTk) {
        case TokenType::LogicAnd: {
            uint64 id = logicCount++;
            std::string fail = "@LOGIC_FAIL" + std::to_string(id),
                        end = "@LOGIC_END" + std::to_string(id);
            auto etypeL = getEType(operNode->getLeft()), etypeR = getEType(operNode->getRight());
            buildExpression(operNode->getLeft());
            writeVCode(wrap(TCommand::jz, getRealDtMdf(etypeL), etypeL.vtMdf), fail);
            buildExpression(operNode->getRight());
            writeVCode(wrap(TCommand::jz, getRealDtMdf(etypeR), etypeR.vtMdf), fail);
            writeVCode(Command::i64_push, UnionData(1ull));
            writeVCode(Command::jmp, end);
            writeVCode("#LABEL", fail);
            writeVCode(Command::i64_push, UnionData(0ull));
            writeVCode("#LABEL", end);
            break;
        }
        case TokenType::LogicOr: {
            uint64 id = logicCount++;
            std::string succ = "@LOGIC_SUCC" + std::to_string(id),
                        end = "@LOGIC_END" + std::to_string(id);
            auto etypeL = getEType(operNode->getLeft()), etypeR = getEType(operNode->getRight());
            buildExpression(operNode->getLeft());
            writeVCode(wrap(TCommand::jp, getRealDtMdf(etypeL), etypeL.vtMdf), succ);
            buildExpression(operNode->getRight());
            writeVCode(wrap(TCommand::jp, getRealDtMdf(etypeR), etypeR.vtMdf), succ);
            writeVCode(Command::i64_push, UnionData(0ull));
            writeVCode(Command::jmp, end);
            writeVCode("#LABEL", succ);
            writeVCode(Command::i64_push, UnionData(1ull));
            writeVCode("#LABEL", end);
            break;
        }
        case TokenType::LogicNot: {
            uint64 id = logicCount++;
            std::string succ = "@LOGIC_SUCC" + std::to_string(id),
                        end = "@LOGIC_END" + std::to_string(id);
            auto etypeC = getEType(operNode->getRight());
            buildExpression(operNode->getRight());
            writeVCode(wrap(TCommand::jp, getRealDtMdf(etypeC), etypeC.vtMdf), succ);
            writeVCode(Command::i64_push, UnionData(0ull));
            writeVCode(Command::jmp, end);
            writeVCode("#LABEL", succ);
            writeVCode(Command::i64_push, UnionData(1ull));
            writeVCode("#LABEL", end);
            break;
        }
        case TokenType::GetMem: {
            auto etypeL = getEType(operNode->getLeft());
            auto mem = (IdentifierNode *)operNode->getRight();
            buildExpression(operNode->getLeft());
            if (mem->isFuncCall()) {
                if (etypeL.vtMdf != ValueTypeModifier::TrueValue)
                    writeVCode(wrap(TCommand::gvl, DataTypeModifier::o, etypeL.vtMdf));
                auto info = getFuncCallInfo(mem);
                uint64 argCnt = 0;
                writeVCode(Command::o_cpy);
                res &= writePushArg(mem, argCnt);
                writeVCode(Command::setarg, UnionData(argCnt + 1));
                res &= writeSetGTable(std::get<2>(info));
                // get the offset of the function
                writeVCode(Command::i64_t_mem, UnionData(std::get<0>(info)->offset));
                // set the gtable
                writeVCode(Command::mr_vcall);
            } else {
                auto info = getVarCallInfo(mem);
                const ExprType &etype = std::get<1>(info);
                DataTypeModifier dtMdf = (etype.dimc > 0 ? DataTypeModifier::o : getDtMdf(etype));
                writeVCode(wrap(TCommand::mem, dtMdf, etypeL.vtMdf));
            }
            break;
        }
        case TokenType::MBrkL: {
            std::vector<ExpressionNode *> indexNode;
            ExpressionNode *arr = operNode;
            while (arr->getType() == SyntaxNodeType::Operator) {
                indexNode.push_back(((OperatorNode *)arr)->getRight());
                arr = ((OperatorNode *)arr)->getLeft();
            }
            res &= buildExpression(arr);
            const ExprType &etype = getEType(arr);
            for (int i = indexNode.size() - 1; i >= 0; i--)
                res &= buildExpression(indexNode[i]) && writeGvl(getEType(indexNode[i]));
            writeVCode(wrap(TCommand::arrmem, getDtMdf(etype), etype.vtMdf), UnionData((uint64)indexNode.size()));
            break;
        }
        case TokenType::NewObj: {
            ExpressionNode *ctNode = operNode->getRight();
            const ExprType &etype = getEType(operNode);
            // the new object is an object...
            if (ctNode->getType() == SyntaxNodeType::Identifier) {
                // build this object
                writeVCode(Command::_new, etype.cls->fullName);
                // call the constructer
                writeVCode(Command::o_cpy);
                uint64 argCnt = 0;
                res &= writePushArg((IdentifierNode *)ctNode, argCnt);
                writeVCode(Command::setarg, argCnt + 1);
                res &= writeSetGTable(std::get<2>(getFuncCallInfo((IdentifierNode *)ctNode)));
            } else { // the new object is an array...
                std::vector<ExpressionNode *> sizeNode;
                // find the expression of size of each dimension and the type
                while (ctNode->getType() == SyntaxNodeType::Operator)
                    sizeNode.push_back(((OperatorNode *)ctNode)->getRight()),
                    ctNode = ((OperatorNode *)ctNode)->getLeft();
                writeVCode(Command::i64_push, UnionData(etype.cls->size));
                for (int i = sizeNode.size() - 1; i >= 0; i--)
                    res &= buildExpression(sizeNode[i]) && writeGvl(getEType(sizeNode[i]));
                writeVCode(Command::arrnew, UnionData((uint64)sizeNode.size()));
            }
            break;
        }
        case TokenType::Inc: {
            TCommand tcmd = TCommand::unknown;
            ExpressionNode *subExpr = nullptr;
            if (operNode->getLeft() != nullptr)
                subExpr = operNode->getLeft(), tcmd = TCommand::pinc;
            else if (operNode->getRight() != nullptr)
                subExpr = operNode->getRight(), tcmd = TCommand::sinc;
            else return false;
            res &= buildExpression(subExpr);
            const ExprType &etype = getEType(subExpr);
            DataTypeModifier dtMdf = getDtMdf(etype);
            writeVCode(wrap(tcmd, dtMdf, etype.vtMdf));
            break;
        }
        case TokenType::Dec: {
            TCommand tcmd = TCommand::unknown;
            ExpressionNode *subExpr = nullptr;
            if (operNode->getLeft() != nullptr)
                subExpr = operNode->getLeft(), tcmd = TCommand::pdec;
            else if (operNode->getRight() != nullptr)
                subExpr = operNode->getRight(), tcmd = TCommand::sdec;
            else return false;
            res &= buildExpression(subExpr);
            const ExprType &etype = getEType(subExpr);
            DataTypeModifier dtMdf = getDtMdf(etype);
            writeVCode(wrap(tcmd, dtMdf, etype.vtMdf));
            break;
        }
        case TokenType::Comma: {
            res &= buildExpression(operNode->getLeft());
            const ExprType &etypeL = getEType(operNode->getLeft());
            if (getEType(operNode->getLeft()) != ExprType(voidCls))
                writeVCode(wrap(TCommand::pop, getRealDtMdf(etypeL), etypeL.vtMdf));
            res &= buildExpression(operNode->getRight());
            break;
        }
        case TokenType::Convert: {
            const ExprType &etypeL = getEType(operNode->getLeft()),
                            &etypeTrg = getEType(operNode);
            if (!etypeL.isObject() && !etypeTrg.isObject())
                writeVCode(wrap(TCommand::cvt, getRealDtMdf(etypeL), getRealDtMdf(etypeTrg), etypeL.vtMdf));
            break;
        }
        default:
            return buildBinOper();
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
    uint64 id = ifCount++;
    std::string end = "@IF_END" + std::to_string(id),
                elseLabel = "@IF_ELSE" + std::to_string(id);
    auto chkInfo = chkEType(node->getCondNode());
    bool res = true;
    if (!std::get<0>(chkInfo) || std::get<1>(chkInfo).isObject()) {
        res = false;
        printError(node->getCondNode()->getToken().lineId, "The condition of if statement must be a boolean expression");
    }
    res &= buildExpression(node->getCondNode());
    writeVCode(wrap(TCommand::jz, getDtMdf(std::get<1>(chkInfo)), std::get<1>(chkInfo).vtMdf), elseLabel);
    indentInc();
    res &= buildVCode(node->getSuccNode());
    indentDec();
    writeVCode(Command::jmp, end);
    writeVCode("#LABEL", elseLabel);
    indentInc();
    res &= buildVCode(node->getFailNode());
    indentDec();
    writeVCode("#LABEL", end);
    return res;
}

bool buildLoop(LoopNode *node) {
    uint64 id = loopCount++;
    std::string start = "@LOOP_START" + std::to_string(id),
                end = "@LOOP_END" + std::to_string(id),
                contentEnd = "@LOOP_CONTENT_END" + std::to_string(id);
    loopStk.push(LoopLabel(start, end, contentEnd, locVarStkTop()));
    auto chkInfo = chkEType(node->getCondNode());
    bool res = true;
    if (!std::get<0>(chkInfo) || std::get<1>(chkInfo).isObject()) {
        res = false;
        printError(node->getCondNode()->getToken().lineId, "The condition of loop statement must be a boolean expression");
    }
    if (node->getType() == SyntaxNodeType::For) {
        locVarStkPush();
        indentInc();
    }
    res &= buildVCode(node->getInitNode());
    writeVCode("#LABEL", start);
    res &= buildExpression(node->getCondNode());
    writeVCode(wrap(TCommand::jz, getDtMdf(std::get<1>(chkInfo)), std::get<1>(chkInfo).vtMdf), end);
    indentInc();
    res &= buildVCode(node->getContent());
    indentDec();
    writeVCode("#LABEL", contentEnd);
    res &= buildVCode(node->getStepNode());
    writeVCode(Command::jmp, start);
    writeVCode("#LABEL", end);
    if (node->getType() == SyntaxNodeType::For) {
        indentDec();
        locVarStkPop(true);
    }
    return res;
}

bool buildControl(ControlNode *node) {
    bool res = true;
    switch (node->getType()) {
        case SyntaxNodeType::Return: {
            if (node->getContent() != nullptr) {
                auto chkInfo = chkEType(node->getContent());
                if (!std::get<0>(chkInfo) || std::get<1>(chkInfo) != getCurFunc()->resType) {
                    res = false;
                    printError(node->getToken().lineId, "The return value of function is not matched");
                }
                res &= buildExpression(node->getContent());
                // clear the local variable stack
                for (auto frm = locVarStkTop(); frm != nullptr; frm = frm->getPrev())
                    frm->writeCleanVCode();
                // return the value
                res &= writeGvl(std::get<1>(chkInfo));
                writeVCode(wrap(TCommand::vret, getDtMdf(std::get<1>(chkInfo)), std::get<1>(chkInfo).vtMdf));
            } else {
                if (getCurFunc()->resType != ExprType(voidCls)) {
                    res = false;
                    printError(node->getToken().lineId, "The return value of function is not matched");
                }
                writeVCode(Command::ret);
            }
            break;
        }
        case SyntaxNodeType::Break: {
            if (loopStk.empty()) {
                res = false;
                printError(node->getToken().lineId, "The break statement must be in a loop");
            }
            // clear the local variable stack
            for (auto frm = locVarStkTop(); frm != loopStk.top().blgFrm; frm = frm->getPrev())
                frm->writeCleanVCode();
            writeVCode(Command::jmp, loopStk.top().end);
            break;
        }
        case SyntaxNodeType::Continue: {
            if (loopStk.empty()) {
                res = false;
                printError(node->getToken().lineId, "The continue statement must be in a loop");
            }
            // clear the local variable stack
            for (auto frm = locVarStkTop(); frm != loopStk.top().blgFrm; frm = frm->getPrev())
                frm->writeCleanVCode();
            writeVCode(Command::jmp, loopStk.top().contentEnd);
            break;
        }
    }
    return res;
}

bool buildBlock(BlockNode *node) {
    locVarStkPush();
    bool res = true;
    indentInc();
    for (size_t i = 0; i < node->getChildrenCount(); i++)
        res &= buildVCode(node->get(i));
    locVarStkPop(true);
    indentDec();
    return res;
}

bool buildVarDef(VarDefNode *node) {
    bool res = true;
    for (size_t i = 0; i < node->getLocalVarCount(); i++) {
        auto info = node->getVariable(i);
        IdentifierNode *nameNode = std::get<0>(info), *typeNode = std::get<1>(info);
        ExpressionNode *initNode = std::get<2>(info);
        VariableInfo *vInfo = new VariableInfo();
        vInfo->name = vInfo->fullName = nameNode->getName();
        vInfo->blgCls = getCurCls(), vInfo->blgFunc = getCurFunc();
        vInfo->blgNsp = getCurNsp(), vInfo->blgRoot = getCurRoot();
        if (typeNode != nullptr) {
            vInfo->type = ExprType(typeNode);
            res &= vInfo->type.setCls();
            if (initNode != nullptr) {
                auto chkInfo = chkEType(initNode);
                if (!std::get<0>(chkInfo) || std::get<1>(chkInfo) != vInfo->type) {
                    res = false;
                    printError(initNode->getToken().lineId, "The type of the variable is not matched");
                } else res &= buildExpression(initNode);
                writeVCode(wrap(TCommand::pvar, getRealDtMdf(vInfo->type)), UnionData(vInfo->offset));
                writeVCode(wrap(TCommand::mov, getRealDtMdf(vInfo->type), vInfo->type.vtMdf));
            }
        } else { // get the variable from init expression
            auto chkInfo = chkEType(initNode);
            if (!std::get<0>(chkInfo)) {
                res = false;
                printError(initNode->getToken().lineId, "The type of the variable is not matched");
            } else vInfo->type = std::get<1>(chkInfo);
            res &= buildExpression(initNode);
            writeVCode(wrap(TCommand::pvar, getRealDtMdf(vInfo->type)), UnionData(vInfo->offset));
            writeVCode(wrap(TCommand::mov, getRealDtMdf(vInfo->type), vInfo->type.vtMdf));
        }
        res &= locVarStkTop()->insertVar(vInfo);
    }
    return res;
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
    setCurNsp(cls->blgNsp), setCurCls(cls), setCurRoot(cls->blgRoot);
    bool res = updOperCandy();
    if (isBasicCls(cls) || cls == basicCls || cls->blgRoot->getType() != SyntaxNodeType::SourceRoot) return true;
    for (auto &fPair : cls->funcMap)
        for (auto func : fPair.second) res &= buildFunc(func);
    setCurCls(nullptr), setCurNsp(nullptr), setCurRoot(nullptr);
    return res;
}
bool scanNsp(NamespaceInfo *nsp) {
    bool res = true;
    for (auto &cPair : nsp->clsMap) res &= scanCls(cPair.second);
    for (auto &nPair : nsp->nspMap) res &= scanNsp(nPair.second);
    for (auto &fPair : nsp->funcMap)
        for (auto func : fPair.second) res &= buildFunc(func);
    return res;
}
bool generateVCode(const std::string &vasmPath, const std::vector<std::string> &relyPath) {
    initOperCandy();

    bool res = setOutputStream(vasmPath);
    if (!res) return false;
    res = scanNsp(rootNsp);
    
}