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
    if (etype.cls == nullptr) return DataTypeModifier::unknown;
    if (etype.cls->isGeneric) {
        if (getCurCls() != nullptr) {
            for (uint8 i = 0; i < getCurCls()->generCls.size(); i++) 
                if (getCurCls()->generCls[i] == etype.cls) 
                    return (DataTypeModifier)(i + (uint8)DataTypeModifier::gv0);
        } else if (getCurFunc() != nullptr) { // this function is a generic function
            for (uint8 i = 0; i < getCurFunc()->generCls.size(); i++)
                if (getCurFunc()->generCls[i] == etype.cls) 
                    return (DataTypeModifier)(i + (uint8)DataTypeModifier::gv0);
        }
        return DataTypeModifier::unknown;
    } else return getDtMdf(etype);
}

uint8 getGTableId(ClassInfo *gCls) {
    if (getCurCls() != nullptr)
        return std::find(getCurCls()->generCls.begin(), getCurCls()->generCls.end(), gCls) - getCurCls()->generCls.begin();
    else if (getCurFunc() != nullptr)
        return std::find(getCurFunc()->generCls.begin(), getCurFunc()->generCls.end(), gCls) - getCurFunc()->generCls.begin();
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

bool buildVCode(SyntaxNode *node);
bool buildExpression(ExpressionNode *node);

bool writePushArg(IdentifierNode *idenNode, uint64 &argCount) {
    if (idenNode == nullptr) return false;
    bool res = true;
    argCount = 0;
    for (size_t i = 0; i < idenNode->getParamCount(); i++)
        if (idenNode->getParam(i) != nullptr) {
            argCount++;
            res &= buildExpression(idenNode->getParam(i));
        }
    auto fInfo = std::get<0>(getFuncCallInfo(idenNode));
    if (fInfo == nullptr) return false;
    for (; argCount < fInfo->params.size(); argCount++)
        writeVCode(Command::i64_push, fInfo->defaultVals[argCount - (fInfo->params.size() - fInfo->defaultVals.size())]->getToken().data);
    return res;
}

bool buildIdentifier(IdentifierNode *idenNode) {
    bool res = true;
    if (idenNode->getName() == "@base") {
        if (getEType(idenNode) != ExprType(basicCls)) {
            auto &info = getFuncCallInfo(idenNode);
            uint64 argCnt = 0;
            res &= writeSetGTable(std::get<2>(info));
            res &= writePushArg(idenNode, argCnt);
            writeVCode(Command::setarg, UnionData(argCnt));
            writeVCode(Command::call, std::get<0>(info)->fullName);
            writeVCode(Command::i64_pop);
        }
        // set the varfunc table
        for (auto &fPir : getCurCls()->funcMap)
            for (auto &func : fPir.second)
                if (func->blgCls == getCurCls() && func->getDefNode()->getType() == SyntaxNodeType::VarFuncDef) {
                    writeVCode(Command::o_pvar, UnionData(0ull));
                    writeVCode(Command::plabel, func->fullName);
                    writeVCode(Command::i64_setmem, UnionData(func->offset));
                }
        return res;
    } else if (idenNode->getName() == "@vasm") {
        writeVCode(idenNode->getParam(0)->getToken().dataStr);
        return true;
    } else if (idenNode->getName() == "@setPause") {
        writeVCode(Command::setpause);
        return true;
    }
    if (idenNode->isFuncCall()) {
        // since that the member function calling which miss "@this" have been handled and add callMem operator
        // the program below just need to handle the global function calling
        auto &info = getFuncCallInfo(idenNode);
        uint64 argCnt = 0;
        res &= writeSetGTable(std::get<2>(info));
        res &= writePushArg(idenNode, argCnt);
        writeVCode(Command::setarg, UnionData(argCnt));
        writeVCode(Command::call, std::get<0>(info)->fullName);
        if (std::get<0>(info)->resType == ExprType(voidCls)) writeVCode(Command::i64_pop);
    } else {
        // the program below just need to handle the global variable calling and local variable calling
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

TCommand getAssignCmd(TCommand bsCmd, TokenType oper) {
    if (oper == TokenType::Assign) return bsCmd;
    else if (oper == TokenType::AddAssign) return getTCommand(std::string("add") + tCommandString[(int)bsCmd].substr(3));
    else if (oper == TokenType::SubAssign) return getTCommand(std::string("sub") + tCommandString[(int)bsCmd].substr(3));
    else if (oper == TokenType::MulAssign) return getTCommand(std::string("mul") + tCommandString[(int)bsCmd].substr(3));
    else if (oper == TokenType::DivAssign) return getTCommand(std::string("div") + tCommandString[(int)bsCmd].substr(3));
    else if (oper == TokenType::ModAssign) return getTCommand(std::string("mod") + tCommandString[(int)bsCmd].substr(3));
    else if (oper == TokenType::AndAssign) return getTCommand(std::string("and") + tCommandString[(int)bsCmd].substr(3));
    else if (oper == TokenType::OrAssign) return getTCommand(std::string("or") + tCommandString[(int)bsCmd].substr(3));
    else if (oper == TokenType::XorAssign) return getTCommand(std::string("xor") + tCommandString[(int)bsCmd].substr(3));
    else if (oper == TokenType::ShlAssign) return getTCommand(std::string("shl") + tCommandString[(int)bsCmd].substr(3));
    else if (oper == TokenType::ShrAssign) return getTCommand(std::string("shr") + tCommandString[(int)bsCmd].substr(3));
    else return TCommand::unknown;
}

bool buildAssignNode(OperatorNode *oper) {
    TCommand tcmdAssign, tcmdGetVal;
    const ExprType &etype = getEType(oper->getLeft());
    VariableInfo *vInfo = nullptr;
    ExpressionNode *pre = nullptr;
    UnionData offset;
    switch (oper->getLeft()->getType()) {
        case SyntaxNodeType::Identifier: {
            const VarCallInfo &info = getVarCallInfo((IdentifierNode *)oper->getLeft());
            vInfo = std::get<0>(info);
            if (vInfo->blgFunc == nullptr)
                tcmdGetVal = TCommand::pglo, tcmdAssign = TCommand::setglo;
            else tcmdGetVal = TCommand::pvar, tcmdAssign = TCommand::setvar;
            offset = std::get<0>(info)->offset;
            break;
        }
        case SyntaxNodeType::Operator: {
            TokenType tk = ((OperatorNode *)oper->getLeft())->getToken().type;
            switch (tk) {
                case TokenType::GetMem: {
                    const VarCallInfo &info = getVarCallInfo((IdentifierNode *)((OperatorNode *)oper->getLeft())->getRight());
                    pre = ((OperatorNode *)oper->getLeft())->getLeft();
                    vInfo = std::get<0>(info);
                    tcmdGetVal = TCommand::pmem, tcmdAssign = TCommand::setmem;
                    offset = std::get<0>(info)->offset;
                    break;
                }
                case TokenType::MBrkL: {
                    pre = oper->getLeft();
                    offset = 0;
                    while (pre->getType() == SyntaxNodeType::Operator && pre->getToken().type == TokenType::MBrkL)
                        pre = ((OperatorNode *)pre)->getLeft(), offset.uint64_v()++;
                    tcmdGetVal = TCommand::parrmem, tcmdAssign = TCommand::setarrmem;
                    break;
                }
            }
            break;
        }
    }
    bool res = true;
    if (pre != nullptr) res = buildExpression(pre);
    if (tcmdAssign == TCommand::setarrmem) {
        std::vector<ExpressionNode *> indexNode;
        ExpressionNode *arr = oper->getLeft();
        while (arr->getType() == SyntaxNodeType::Operator && arr->getToken().type == TokenType::MBrkL) {
            indexNode.push_back(((OperatorNode *)arr)->getRight());
            arr = ((OperatorNode *)arr)->getLeft();
        }
        for (int i = indexNode.size() - 1; i >= 0; i--)
            res &= buildExpression(indexNode[i]);
    }
    res &= buildExpression(oper->getRight());
    if (tcmdAssign != TCommand::setglo)
        writeVCode(wrap(getAssignCmd(tcmdAssign, oper->getToken().type), getRealDtMdf(etype)), offset);
    else writeVCode(wrap(getAssignCmd(tcmdAssign, oper->getToken().type), getRealDtMdf(etype)), vInfo->fullName);
    return res;
}

TCommand getIncDecCommand(TCommand assignCmd, bool isInc, bool isS) {
    std::string tstr = tCommandString[(int)assignCmd];
    return getTCommand(std::string(isS ? "s" : "p") + std::string(isInc ? "inc" : "dec") + tstr.substr(3));
}

bool buildIncDecNode(OperatorNode *oper) {
    bool isInc = oper->getToken().type == TokenType::Inc, isS = oper->getLeft() != nullptr;
    TCommand tcmdAssign;
    ExpressionNode *varNode = isS ? oper->getLeft() : oper->getRight();
    const ExprType &etype = getEType(varNode);
    VariableInfo *vInfo = nullptr;
    ExpressionNode *pre = nullptr;
    UnionData offset;
    switch (varNode->getType()) {
        case SyntaxNodeType::Identifier: {
            const VarCallInfo &info = getVarCallInfo((IdentifierNode *)varNode);
            vInfo = std::get<0>(info);
            tcmdAssign = (vInfo->blgFunc == nullptr ? TCommand::setglo : TCommand::setvar);
            offset = std::get<0>(info)->offset;
            break;
        }
        case SyntaxNodeType::Operator: {
            TokenType tk = ((OperatorNode *)varNode)->getToken().type;
            switch (tk) {
                case TokenType::GetMem: {
                    const VarCallInfo &info = getVarCallInfo((IdentifierNode *)((OperatorNode *)varNode)->getRight());
                    pre = ((OperatorNode *)varNode)->getLeft();
                    vInfo = std::get<0>(info);
                    tcmdAssign = TCommand::setmem;
                    offset = std::get<0>(info)->offset;
                    break;
                }
                case TokenType::MBrkL: {
                    pre = varNode;
                    offset = 0;
                    while (pre->getType() == SyntaxNodeType::Operator && pre->getToken().type == TokenType::MBrkL)
                        pre = ((OperatorNode *)pre)->getLeft(), offset.uint64_v()++;
                    tcmdAssign = TCommand::setarrmem;
                    break;
                }
            }
            break;
        }
    }
    bool res = true;
    if (pre != nullptr) res = buildExpression(pre);
    if (tcmdAssign == TCommand::setarrmem) {
        std::vector<ExpressionNode *> indexNode;
        ExpressionNode *arr = oper->getLeft();
        while (arr->getType() == SyntaxNodeType::Operator && arr->getToken().type == TokenType::MBrkL) {
            indexNode.push_back(((OperatorNode *)arr)->getRight());
            arr = ((OperatorNode *)arr)->getLeft();
        }
        for (int i = indexNode.size() - 1; i >= 0; i--)
            res &= buildExpression(indexNode[i]);
    }
    if (tcmdAssign != TCommand::setglo)
        writeVCode(wrap(getIncDecCommand(tcmdAssign, isInc, isS), getRealDtMdf(etype)), offset);
    else
        writeVCode(wrap(getIncDecCommand(tcmdAssign, isInc, isS), getRealDtMdf(etype)), vInfo->fullName);
    return res;
}

bool buildOperNode(OperatorNode *operNode) {
    if (isAssignFamily(operNode->getToken().type)) return buildAssignNode(operNode);
    else if (operNode->getToken().type == TokenType::Inc || operNode->getToken().type == TokenType::Dec)
        return buildIncDecNode(operNode);
    bool res = true;
    auto operTk = operNode->getToken().type;
    auto buildBinOper = [&operNode, &res, &operTk] () -> bool {
        res &= buildExpression(operNode->getLeft());
        res &= buildExpression(operNode->getRight());
        auto dtMdf = getRealDtMdf(getEType(isCmpOperator(operTk) || isMovOperator(operTk)
                                         ? operNode->getLeft() : operNode));
        auto etypeLeft = getEType(operNode->getLeft());
        auto etypeRight = getEType(operNode->getRight());
        TCommand tcmd = getTCommand(operTk);
        if (tcmd == TCommand::unknown) return false;
        writeVCode(wrap(tcmd, dtMdf));
        return true;
    };
    switch (operTk) {
        case TokenType::LogicAnd: {
            uint64 id = logicCount++;
            std::string fail = "@LOGIC_FAIL" + std::to_string(id),
                        end = "@LOGIC_END" + std::to_string(id);
            auto etypeL = getEType(operNode->getLeft()), etypeR = getEType(operNode->getRight());
            buildExpression(operNode->getLeft());
            writeVCode(Command::jz, fail);
            buildExpression(operNode->getRight());
            writeVCode(Command::jz, fail);
            writeVCode(Command::setflag, UnionData(1ull));
            writeVCode(Command::jmp, end);
            writeVCode("#LABEL", fail);
            writeVCode(Command::setflag, UnionData(0ull));
            writeVCode("#LABEL", end);
            writeVCode(Command::pushflag);
            break;
        }
        case TokenType::LogicOr: {
            uint64 id = logicCount++;
            std::string succ = "@LOGIC_SUCC" + std::to_string(id),
                        end = "@LOGIC_END" + std::to_string(id);
            auto etypeL = getEType(operNode->getLeft()), etypeR = getEType(operNode->getRight());
            buildExpression(operNode->getLeft());
            writeVCode(Command::jp, succ);
            buildExpression(operNode->getRight());
            writeVCode(Command::jp, succ);
            writeVCode(Command::setflag, UnionData(0ull));
            writeVCode(Command::jmp, end);
            writeVCode("#LABEL", succ);
            writeVCode(Command::setflag, UnionData(1ull));
            writeVCode("#LABEL", end);
            writeVCode(Command::pushflag);
            break;
        }
        case TokenType::LogicNot: {
            uint64 id = logicCount++;
            std::string succ = "@LOGIC_SUCC" + std::to_string(id),
                        end = "@LOGIC_END" + std::to_string(id);
            auto etypeC = getEType(operNode->getRight());
            buildExpression(operNode->getRight());
            writeVCode(Command::jp, succ);
            writeVCode(Command::setflag, UnionData(0ull));
            writeVCode(Command::jmp, end);
            writeVCode("#LABEL", succ);
            writeVCode(Command::setflag, UnionData(1ull));
            writeVCode("#LABEL", end);
            writeVCode(Command::pushflag);
            break;
        }
        case TokenType::GetMem: {
            auto etypeL = getEType(operNode->getLeft());
            auto mem = (IdentifierNode *)operNode->getRight();
            buildExpression(operNode->getLeft());
            if (mem->isFuncCall()) {
                auto info = getFuncCallInfo(mem);
                uint64 argCnt = 0;
                writeVCode(Command::o_cpy);
                res &= writePushArg(mem, argCnt);
                writeVCode(Command::setarg, UnionData(argCnt + 1));
                // get the offset of the function
                writeVCode(Command::i64_pmem, UnionData(std::get<0>(info)->offset));
                writeVCode(Command::vcall);
                if (std::get<1>(info) == ExprType(voidCls)) writeVCode(Command::i64_pop);
            } else {
                auto info = getVarCallInfo(mem);
                const ExprType &etype = std::get<1>(info);
                DataTypeModifier dtMdf = getRealDtMdf(etype);
                writeVCode(wrap(TCommand::pmem, dtMdf), UnionData(std::get<0>(info)->offset));
            }
            break;
        }
        case TokenType::MBrkL: {
            std::vector<ExpressionNode *> indexNode;
            ExpressionNode *arr = operNode;
            const ExprType &etype = getEType(arr);
            while (arr->getType() == SyntaxNodeType::Operator && arr->getToken().type == TokenType::MBrkL) {
                indexNode.push_back(((OperatorNode *)arr)->getRight());
                arr = ((OperatorNode *)arr)->getLeft();
            }
            res &= buildExpression(arr);
            for (int i = indexNode.size() - 1; i >= 0; i--)
                res &= buildExpression(indexNode[i]);
            writeVCode(wrap(TCommand::parrmem, getRealDtMdf(etype)), UnionData((uint64)indexNode.size()));
            break;
        }
        case TokenType::NewObj: {
            ExpressionNode *ctNode = operNode->getRight();
            const ExprType &etype = getEType(operNode);
            // the new object is an object...
            if (ctNode->getType() == SyntaxNodeType::Identifier) {
                // build this object
                writeVCode(Command::newobj, etype.cls->fullName);
                // call the constructer
                writeVCode(Command::o_cpy);
                uint64 argCnt = 0;
                res &= writePushArg((IdentifierNode *)ctNode, argCnt);
                writeVCode(Command::setarg, argCnt + 1);
                FuncCallInfo clInfo = getFuncCallInfo((IdentifierNode *)ctNode);
                res &= writeSetGTable(std::get<2>(clInfo));
                writeVCode(Command::call, std::get<0>(clInfo)->fullName);
                writeVCode(Command::i64_pop);
            } else { // the new object is an array...
                std::vector<ExpressionNode *> sizeNode;
                // find the expression of size of each dimension and the type
                while (ctNode->getType() == SyntaxNodeType::Operator)
                    sizeNode.push_back(((OperatorNode *)ctNode)->getRight()),
                    ctNode = ((OperatorNode *)ctNode)->getLeft();
                // get the size of the element of array
                if (etype.cls->isGeneric) {
                    uint8 shift = 0;
                    bool succ = false;
                    if (getCurCls() != nullptr) {
                        shift = getCurCls()->generCls.size();
                        for (uint8 i = 0; i < getCurCls()->generCls.size(); i++) 
                            if (getCurCls()->generCls[i] == etype.cls) {
                                writeVCode(Command::getgtblsz, UnionData((uint64)i));
                                succ = true;
                                break;
                            }
                    }
                    if (!succ) 
                        for (uint8 i = 0; i < getCurFunc()->generCls.size(); i++)
                            if (getCurFunc()->generCls[i] == etype.cls) {
                                writeVCode(Command::getgtblsz, UnionData((uint64)i));
                                succ = true;
                                break;
                            }
                }
                else writeVCode(Command::i64_push, UnionData(std::min(etype.cls->size, 8ull)));
                for (int i = sizeNode.size() - 1; i >= 0; i--)
                    res &= buildExpression(sizeNode[i]);
                writeVCode(Command::newarr, UnionData((uint64)sizeNode.size()));
            }
            break;
        }
        case TokenType::Comma: {
            res &= buildExpression(operNode->getLeft());
            const ExprType &etypeL = getEType(operNode->getLeft());
            if (etypeL != ExprType(voidCls))
                writeVCode(wrap(TCommand::pop, getRealDtMdf(etypeL)));
            res &= buildExpression(operNode->getRight());
            break;
        }
        case TokenType::Convert: {
            const ExprType &etypeL = getEType(operNode->getLeft()),
                            &etypeTrg = getEType(operNode);
            res &= buildExpression(operNode->getLeft());
            if (etypeL != etypeTrg)
                writeVCode(wrap(TCommand::cvt, getRealDtMdf(etypeL), getRealDtMdf(etypeTrg)));
            break;
        }
        case TokenType::TreatAs: res &= buildExpression(operNode->getLeft()); break;
        default:
            return buildBinOper();
    }
    return res;
}

bool buildConstValue(ConstValueNode *node) {
    if (node->getToken().type == TokenType::String) {
        uint64 id = getStrId(node);
        writeVCode(Command::pstr, UnionData(id));
    } else writeVCode(wrap(TCommand::push, getRealDtMdf(getEType(node))), node->getToken().data);
    return true;
}

/// @brief build the vcode of an expression. PS: you need to ensure that this expression has pass the expression 
/// type checking before using this function
/// @param node the root of this expression
/// @return if the vcode of this expression are generated successfully
bool buildExpression(ExpressionNode * node) {
    if (node == nullptr) return true;
    bool res = false;
    switch (node->getType()) {
        case SyntaxNodeType::Expression: return buildExpression(node->getContent());
        case SyntaxNodeType::Identifier: return buildIdentifier((IdentifierNode *)node);
        case SyntaxNodeType::Operator: return buildOperNode((OperatorNode *)node);
        case SyntaxNodeType::ConstValue: return buildConstValue((ConstValueNode *)node);
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
    writeVCode(Command::jz, elseLabel);
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
    bool res = true;
    if (node->getType() == SyntaxNodeType::For) {
        locVarStkPush();
        indentInc();
    }
    loopStk.push(LoopLabel(start, end, contentEnd, locVarStkTop()));
    res &= buildVCode(node->getInitNode());
    auto chkInfo = chkEType(node->getCondNode());
    if (!std::get<0>(chkInfo) || std::get<1>(chkInfo).isObject()) {
        res = false;
        printError(node->getCondNode()->getToken().lineId, "The condition of loop statement must be a boolean expression");
    }
    writeVCode("#LABEL", start);
    if (std::get<0>(chkInfo)) {
        res &= buildExpression(node->getCondNode());
        writeVCode(Command::jz, end);
    }
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
                writeVCode(Command::vret);
            } else {
                if (getCurFunc()->resType != ExprType(voidCls)) {
                    res = false;
                    printError(node->getToken().lineId, "The return value of function is not matched");
                }
                writeVCode(Command::u64_push, UnionData(0ull));
                for (auto frm = locVarStkTop(); frm != nullptr; frm = frm->getPrev())
                    frm->writeCleanVCode();
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
        res &= locVarStkTop()->insertVar(vInfo);
        if (!res) {
            printError(nameNode->getToken().lineId, "redefined variable \"" + nameNode->getName() + "\"");
            continue;
        }
        if (typeNode != nullptr) {
            vInfo->type = ExprType(typeNode);
            res &= vInfo->type.setCls();
            if (initNode != nullptr) {
                auto chkInfo = chkEType(initNode);
                if (!std::get<0>(chkInfo) || std::get<1>(chkInfo) != vInfo->type) {
                    res = false;
                    printError(initNode->getToken().lineId, "The type of the variable is not matched");
                } else {
                    res &= buildExpression(initNode);
                    writeVCode(wrap(TCommand::setvar, getRealDtMdf(vInfo->type)), UnionData(vInfo->offset));
                }
            }
        } else { // get the variable from init expression
            auto chkInfo = chkEType(initNode);
            if (!std::get<0>(chkInfo)) {
                res = false;
                printError(initNode->getToken().lineId, "The type of the variable is not matched");
            } else {
                vInfo->type = std::get<1>(chkInfo), vInfo->type.vtMdf = ValueTypeModifier::Ref;
                res &= buildExpression(initNode);
                writeVCode(wrap(TCommand::setvar, getRealDtMdf(vInfo->type)), UnionData(vInfo->offset));
            }
        }
    }
    return res;
}

bool buildVCode(SyntaxNode *node) {
    if (node == nullptr) return true;
    switch (node->getType()) {
        case SyntaxNodeType::Expression: {
            auto chkRes = chkEType((ExpressionNode *)node);
            if (!std::get<0>(chkRes)) return false;
            buildExpression((ExpressionNode *)node);
            if (std::get<1>(chkRes) != ExprType(voidCls) && std::get<1>(chkRes) != ExprType(basicCls)) 
                writeVCode(wrap(TCommand::pop, getRealDtMdf(std::get<1>(chkRes))));
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

    {
        VariableInfo *thisInfo = new VariableInfo();
        thisInfo->name = thisInfo->fullName = "@this";
        thisInfo->offset = 0;
        thisInfo->type = ExprType(func->blgCls->fullName);
        for (size_t i = 0; i < func->blgCls->generCls.size(); i++)
            thisInfo->type.generParams.push_back(ExprType(func->blgCls->generCls[i]));
        thisInfo->type.cls = func->blgCls;
        thisInfo->blgFunc = func, thisInfo->blgCls = func->blgCls, thisInfo->blgNsp = func->blgNsp, thisInfo->blgRoot = func->blgRoot;
        res &= locVarStkTop()->insertVar(thisInfo);
    }
    // set the param list in the local variable stack
    for (size_t i = 0; i < func->params.size(); i++)
        res &= locVarStkTop()->insertVar(func->params[i]);

    writeVCode(Command::getarg, UnionData((uint64)func->params.size() + 1));

    // set the gtable in @this
    ClassInfo *cls = func->blgCls;
    for (size_t i = 0; i < cls->generCls.size(); i++) 
        writeVCode(Command::o_pvar, UnionData(0ull)), 
        writeVCode(Command::getgtbl, UnionData((uint64)i)),
        writeVCode(Command::b_setmem, UnionData(alignTo(cls->baseCls->size, 8) + i * sizeof(uint8)));
    
    res &= buildVCode(func->getDefNode()->getContent());
    updOperCandy();
    locVarStkPop(true);
    indentDec();

    writeVCode(Command::u64_push, UnionData(0ull));
    writeVCode(Command::ret);
    setCurFunc(nullptr);
    return res;
}
bool buildFunc(FunctionInfo *func) {
    if (func->blgRoot->getType() != SyntaxNodeType::SourceRoot) return true;
    if (func->name == "@constructer") return buildConstructer(func);

    // do not generate empty function
    if (func->getDefNode()->getContent() == nullptr) return true;

    // set the environment
    setCurFunc(func), setCurRoot(func->blgRoot);
    updOperCandy();
    bool isMember = (func->blgCls != nullptr), res = true;

    writeVCode("#LABEL " + func->fullName);
    indentInc();
    locVarStkPush();
    writeVCode(Command::setlocal, UnionData(func->getDefNode()->getLocalVarCount() + isMember));
    // get the param list (if this function is a member of a class, remember to add "@this")
    writeVCode(Command::getarg, UnionData((uint64) (func->params.size() + isMember)));
    // if this is a member function , then generate the vcode to set gtable
    if (func->blgCls != nullptr && func->blgCls->generCls.size() > 0) {
        writeVCode(Command::setclgtbl,
            UnionData(func->blgCls->baseCls->size), 
            UnionData((uint64)func->blgCls->generCls.size()));
    }
    
    // if this function is a member of a class, then there should be a param named "@this"
    if (isMember) {
        VariableInfo *thisInfo = new VariableInfo();
        thisInfo->name = thisInfo->fullName = "@this";
        thisInfo->offset = 0;
        thisInfo->type = ExprType(func->blgCls->fullName);
        for (size_t i = 0; i < func->blgCls->generCls.size(); i++)
            thisInfo->type.generParams.push_back(ExprType(func->blgCls->generCls[i]));
        thisInfo->type.cls = func->blgCls;
        thisInfo->blgFunc = func, thisInfo->blgCls = func->blgCls, thisInfo->blgNsp = func->blgNsp, thisInfo->blgRoot = func->blgRoot;
        res &= locVarStkTop()->insertVar(thisInfo);
    }
    // insert the params into local variable stack
    for (size_t i = 0; i < func->params.size(); i++) 
        locVarStkTop()->insertVar(func->params[i]);

    res &= buildVCode(func->getDefNode()->getContent());

    updOperCandy();
    locVarStkPop(true);
    indentDec();

    writeVCode(Command::u64_push, UnionData(0ull));
    if (func->resType.cls != voidCls) writeVCode(Command::vret);
    else writeVCode(Command::ret);
    setCurFunc(nullptr), setCurRoot(nullptr);

    return res;
}

bool scanCls(ClassInfo *cls) {
    if (isBasicCls(cls) || cls == basicCls || cls->blgRoot->getType() == SyntaxNodeType::SymbolRoot) return true;
    setCurCls(cls), setCurRoot(cls->blgRoot);
    bool res = updOperCandy();
    for (auto &fPair : cls->funcMap)
        for (auto func : fPair.second) if (func->blgCls == cls) res &= buildFunc(func);
    setCurCls(nullptr), setCurRoot(nullptr);
    updOperCandy();
    return res;
}
bool scanNsp(NamespaceInfo *nsp) {
    setCurNsp(nsp);
    updOperCandy();
    bool res = true;
    for (auto &cPair : nsp->clsMap) res &= scanCls(cPair.second);
    for (auto &fPair : nsp->funcMap)
        for (auto func : fPair.second) res &= buildFunc(func);
    setCurNsp(nullptr);
    updOperCandy();
    for (auto &nPair : nsp->nspMap) res &= scanNsp(nPair.second);
    return res;
}
bool generateVCode(const std::string &vasmPath) {
    initOperCandy();

    bool res = setOutputStream(vasmPath);
    if (!res) return false;
    res = scanNsp(rootNsp);
    
    // generate the string list
    const std::vector<std::string> &strList = getStrList();
    for (const auto &str : strList) writeVCode("#STRING", toCodeString(str));
    closeOutputStream();
    return res;
}