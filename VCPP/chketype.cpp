#include "geninner.h"

std::map<ExpressionNode *, ExprType> eTypeMap;
std::map<IdentifierNode *, FuncCallInfo> funcCallMap;
std::map<IdentifierNode *, VarCallInfo> varCallMap;

inline bool isInteger(const ExprType &eType) { return eType.dimc == 0 && isIntegerCls(eType.cls); }
inline bool isFloat(const ExprType &eType) { return eType.dimc == 0 && isFloatCls(eType.cls); }
inline bool isNumberEType(const ExprType &eType) { return isInteger(eType) || isFloat(eType); }

const ExprType &getEType(ExpressionNode *node) { return eTypeMap[node]; }
const FuncCallInfo &getFuncCallInfo(IdentifierNode *node) { return funcCallMap[node]; }
const VarCallInfo &getVarCallInfo(IdentifierNode *node) { return varCallMap[node]; }

std::tuple<bool, ExprType> chkEType(ExpressionNode *node);

// this function will find the result type of the basic operator
ExprType getExprType(const ExprType &etypeL, const ExprType &etypeR) {
    if (etypeL == etypeR) return etypeL;
    // if one of them is float, the result will be float
    if (isFloat(etypeL) && isFloat(etypeR))
        return etypeL.cls->size > etypeR.cls->size ? etypeL : etypeR;
    else if (isFloat(etypeL) || isFloat(etypeR))
        return isFloat(etypeL) ? etypeL : etypeR;
    // if both is integer, the result will be the bigger one
    if (isInteger(etypeL) && isInteger(etypeR)) {
        // if the size of these two is the same, then the result will be the one with the unsigned modifier
        if (etypeL.cls->size == etypeR.cls->size)
            return etypeL.cls->name[0] == 'u' ? etypeL : etypeR;
        else return etypeL.cls->size > etypeR.cls->size ? etypeL : etypeR;
    }
    return ExprType();
}

OperatorNode *buildOperCandy(VariableInfo *vInfo, ExpressionNode *exprL, ExpressionNode *exprR) {
    // the structure of the candy is "operVar.calculate(left expression, right expression)"
    OperatorNode *operCandy = new OperatorNode();
    IdentifierNode *vIden = new IdentifierNode();
    IdentifierNode *fIden = new IdentifierNode();
    vIden->getToken().type = TokenType::Identifier;
    vIden->getToken().dataStr = vInfo->fullName;
    ExprType etype = vInfo->type;
    eTypeMap[vIden] = etype;

    fIden->getToken().type = TokenType::Identifier;
    fIden->setName("caclulate");
    fIden->addChild(exprL), fIden->addChild(exprR);
    operCandy->getToken().type = TokenType::GetMem;
    operCandy->addChild(vIden), operCandy->addChild(fIden);
    return operCandy;
}

std::map<TokenType, std::string> operCandyStr;

void initOperCandyStr() {
    operCandyStr[TokenType::Add] = "@add";
    operCandyStr[TokenType::Sub] = "@sub";
    operCandyStr[TokenType::Mul] = "@mul";
    operCandyStr[TokenType::Div] = "@div";
    operCandyStr[TokenType::Mod] = "@mod";
    operCandyStr[TokenType::Shl] = "@shl";
    operCandyStr[TokenType::Shr] = "@shr";
    operCandyStr[TokenType::And] = "@and";
    operCandyStr[TokenType::Or] = "@or";
    operCandyStr[TokenType::Xor] = "@xor";
    operCandyStr[TokenType::Equ] = operCandyStr[TokenType::Neq] = "@compare";
    operCandyStr[TokenType::Gt] = operCandyStr[TokenType::Ge] = "@compare";
    operCandyStr[TokenType::Ls] = operCandyStr[TokenType::Le] = "@compare";
}

std::tuple<bool, ExprType> chkEType_Assign(OperatorNode *node) {
    ExpressionNode *exprL = node->getLeft(), *exprR = node->getRight();
    auto chkResR = chkEType(exprR);
    if (!std::get<0>(chkResR)) return std::make_tuple(false, ExprType());
    if (exprL->getType() != SyntaxNodeType::Operator && exprL->getToken().type == TokenType::MBrkL) {
        // the candy of this operator is "ArrayLikeObj.set(index, value)"
        OperatorNode *operL = (OperatorNode *)exprL;
        chkEType(operL->getLeft()), chkEType(operL->getRight());
        auto chkResLL = chkEType(operL->getLeft()), chkResLR = chkEType(operL->getRight());
        if (!std::get<0>(chkResLL) || !std::get<0>(chkResLR)) return std::make_tuple(false, ExprType());
        if (std::get<1>(chkResLL).dimc > 1) {
            printError(node->getToken().lineId, "The left operand of assign can not be a subarray");
            return eTypeMap[node] = ExprType(), std::make_tuple(false, ExprType());
        }
        // scan the function list and find set(...)
        ClassInfo *cls = std::get<1>(chkResLL).cls;
        if (isBaseCls(findCls("System.ArrayLike"), cls)) {
            const FunctionList &fList = cls->funcMap["set"];
            auto gsMap = makeSubstMap(cls->generCls, std::get<1>(chkResLL).generParams);
            for (auto func : fList) {
                auto chkRes = func->satisfy(gsMap, { std::get<1>(chkResLR), std::get<1>(chkResR) });
                if (!std::get<0>(chkRes)) continue;
                auto setMemOper = new OperatorNode();
                auto fCall = new IdentifierNode();
                fCall->getToken().type = TokenType::Identifier;
                fCall->getToken().dataStr = "set";
                fCall->addChild(operL->getRight());
                fCall->addChild(node->getRight());
                setMemOper->getToken().type = TokenType::GetMem;
                setMemOper->addChild(operL->getLeft());
                setMemOper->addChild(fCall);
                node->clearChildren();
                node->getParent()->replaceChild(node, setMemOper);
                delete node;
                return chkEType(setMemOper);
            }
        }
        printError(node->getToken().lineId, "The left operand of [] must be an array-like object");
    }
    auto chkResL = chkEType(exprL);
    if (!std::get<0>(chkResL)) return std::make_tuple(false, ExprType());
    auto etypeL = std::get<1>(chkResL), etypeR = std::get<1>(chkResR);
    if (etypeL.dimc != etypeR.dimc || isBaseCls(etypeL.cls, etypeR.cls) || etypeL.vtMdf == ValueTypeModifier::TrueValue) {
        // the error messages below are generated by Copilot :-)
        if (etypeL.dimc == 0 && isBaseCls(etypeL.cls, etypeR.cls)) {
            printError(node->getToken().lineId, "The left operand of assign can not be a subarray");
            return eTypeMap[node] = ExprType(), std::make_tuple(false, ExprType());
        }
        if (etypeL.dimc == 0 && etypeL.vtMdf == ValueTypeModifier::TrueValue) {
            printError(node->getToken().lineId, "The left operand of assign can not be a true value");
            return eTypeMap[node] = ExprType(), std::make_tuple(false, ExprType());
        }
        if (etypeL.dimc != etypeR.dimc) {
            printError(node->getToken().lineId, "The dimension of the left operand and the right operand must be the same");
            return eTypeMap[node] = ExprType(), std::make_tuple(false, ExprType());
        }
        printError(node->getToken().lineId, "The left operand and the right operand must be the same type");
        return eTypeMap[node] = ExprType(), std::make_tuple(false, ExprType());
    }
    return eTypeMap[node] = ExprType(voidCls), std::make_tuple(true, ExprType(voidCls));
}

FuncCallInfo tryMatchFunc(const FunctionList &fList, IdentifierNode *fNode, const GenerSubstMap &gsMap) {
    bool res = true;
    std::vector<ExprType> etypeList;
    for (size_t i = 0; i < fNode->getParamCount(); i++) {
        if (fNode->getParam(i) == nullptr) continue;
        auto chkRes = chkEType(fNode->getParam(i));
        res &= std::get<0>(chkRes), etypeList.push_back(std::get<1>(chkRes));
    }
    for (auto func : fList) {
        auto chkRes = func->satisfy(gsMap, etypeList);
        if (!std::get<0>(chkRes)) continue;
        return std::make_tuple(func, std::get<1>(chkRes), std::get<2>(chkRes));
    }
    return std::make_tuple(nullptr, ExprType(), GTableData());
}

std::tuple<bool, ExprType> chkEType_NewObj(OperatorNode *node) {
    bool res = true;
    if (node->getRight()->getType() == SyntaxNodeType::Identifier) {
        IdentifierNode *fNode = (IdentifierNode *)node->getRight();
        if (!fNode->isFuncCall()) {
            printError(node->getToken().lineId, "you must call a function to create a new object");
            return std::make_tuple(false, ExprType());
        }
        ExprType etype((IdentifierNode *)node->getRight());
        res = etype.setCls();
        if (!res) {
            printError(node->getToken().lineId, "invalid class name");
            return std::make_tuple(false, ExprType());
        }
        if (etype.cls->isGeneric) {
            printError(node->getToken().lineId, "you can not create a generic class");
            return std::make_tuple(false, ExprType());
        }
        if (etype.cls->generCls.size() != etype.generParams.size()) {
            printError(fNode->getToken().lineId, "Invalid generic type list");
            return std::make_tuple(false, ExprType());
        }
        auto gsMap = makeSubstMap(etype.cls->generCls, etype.generParams);
        auto chkRes = tryMatchFunc(etype.cls->funcMap["@constructer"], fNode, gsMap);
        if (std::get<0>(chkRes) == nullptr) return std::make_tuple(false, ExprType());
        funcCallMap[fNode] = chkRes;
        return std::make_tuple(true, std::get<1>(chkRes));
    } else if (node->getRight()->getType() == SyntaxNodeType::Operator
             && node->getRight()->getToken().type == TokenType::MBrkL) {
        uint32 dimc = 0;
        ExpressionNode *ctNode = node->getRight();
        while (ctNode->getToken().type == TokenType::MBrkL) {
            dimc++;
            OperatorNode *tmp = (OperatorNode *)ctNode;
            auto chkRes = chkEType(tmp->getRight());
            if (!std::get<0>(chkRes) || !isInteger(std::get<1>(chkRes)))
                printError(tmp->getToken().lineId, "The expression for size must be integer"),
                res = false;
            ctNode = tmp->getLeft();
        }
        if (ctNode->getType() != SyntaxNodeType::Identifier) {
            printError(ctNode->getToken().lineId, "invalid expression for type of array");
            return std::make_tuple(false, ExprType());
        } else {
            IdentifierNode *tNode = (IdentifierNode *)ctNode;
            ExprType etype(tNode);
            res &= etype.setCls();
            if (!res) printError(ctNode->getToken().lineId, "invalid class name");
            etype.dimc = dimc;
            return std::make_tuple(res, etype);
        }
    } else {
        printError(node->getToken().lineId, "invalid operand type");
        return std::make_tuple(false, ExprType());
    }
}

std::tuple<bool, ExprType> chkEType_GetMem(OperatorNode *node) {
    if (node == nullptr) return std::make_tuple(true, ExprType());
    
}

std::tuple<bool, ExprType> chkEType_Operator(OperatorNode *node) {
    if (node == nullptr) return std::make_tuple(true, ExprType());
    // the operator "=" must be handled specially, since that it can be used as a function call "mapLikeObj.set(index, value)" if the left expression is calling an element of an array-like object using "[]"
    if (node->getToken().type == TokenType::Assign) return chkEType_Assign(node);
    // the operator "$" must be handled specially, since that meaning of this operator relates to the operand of this opeator: 1. a new object with params 2. new array with size
    else if (node->getToken().type == TokenType::NewObj) return chkEType_NewObj(node);
    else if (node->getToken().type == TokenType::GetMem) return chkEType_GetMem(node);
    ExprType eType;
    bool res = true;
    auto chkResL = chkEType(node->getLeft()), chkResR = chkEType(node->getRight());
    if (!std::get<0>(chkResL) || !std::get<0>(chkResR)) return std::make_tuple(true, ExprType());
    auto operTk = node->getToken().type;
    auto etypeL = std::get<1>(chkResL), etypeR = std::get<1>(chkResR);
    // the priority is 1. operator candy 2. the basic type operator
    switch (operTk) {
        #pragma region basic operator for arithmetic
        case TokenType::Add: 
        case TokenType::Sub: 
        case TokenType::Mul:
        case TokenType::Div: {
            auto vInfo = findOperCandy(operCandyStr[operTk], etypeL, etypeR);
            // modify this expression into "operVar.calculate(left expression, right expression)"
            if (vInfo != nullptr) {
                auto operCandy = buildOperCandy(vInfo, node->getLeft(), node->getRight());
                node->getParent()->removeChild(node);
                node->clearChildren();
                delete node;
                return chkEType(operCandy);
            } else {
                eType = getExprType(etypeL, etypeR);
                eType.vtMdf = ValueTypeModifier::TrueValue;
                if (eType == ExprType()) res = false, printError(node->getToken().lineId, "invalid operand type");
            }
            break;
        }
        case TokenType::Shl:
        case TokenType::Shr:
        case TokenType::And:
        case TokenType::Or:
        case TokenType::Xor:
        case TokenType::Mod:
            auto vInfo = findOperCandy("@mod", etypeL, etypeR);
            // modify this expression into "operVar.calculate(left expression, right expression)"
            if (vInfo != nullptr) {
                auto operCandy = buildOperCandy(vInfo, node->getLeft(), node->getRight());
                node->getParent()->removeChild(node);
                node->clearChildren();
                return chkEType(operCandy);
            } else {
                if (!isInteger(etypeL) || !isInteger(etypeR)) 
                    res = false, printError(node->getToken().lineId, "invalid operand type");
                eType = getExprType(etypeL, etypeR);
                eType.vtMdf = ValueTypeModifier::TrueValue;
                if (eType == ExprType()) res = false, printError(node->getToken().lineId, "invalid operand type");
            }
            break;
        #pragma endregion
        #pragma region basic operator for compare
        case TokenType::Equ: 
        case TokenType::Neq: 
        case TokenType::Gt:
        case TokenType::Ge:
        case TokenType::Ls:
        case TokenType::Le: {
            auto vInfo = findOperCandy("@compare", etypeL, etypeR);
            // modify this expression into "operVar.calculate(left expression, right expression) == 0"
            if (vInfo != nullptr) {
                auto operCandy = buildOperCandy(vInfo, node->getLeft(), node->getRight()); 
                node->replaceChild(node->getLeft(), operCandy);
                auto zero = new ConstValueNode();
                zero->getToken().type = TokenType::ConstData;
                zero->getToken().data.uint64_v() = 0;
                node->replaceChild(node->getRight(), zero);
                return chkEType(node);
            } else {
                if (!isNumberEType(etypeL) || !isNumberEType(etypeR)) 
                    res = false, printError(node->getToken().lineId, "invalid operand type");
                eType = ExprType(int32Cls);
            }
            break;
        }
        #pragma endregion
        #pragma region basic operator for logic
        case TokenType::LogicAnd:
        case TokenType::LogicOr: {
            if (!isInteger(etypeL) || !isInteger(etypeR)) 
                res = false, printError(node->getToken().lineId, "invalid operand type");
            eType = ExprType(int32Cls);
            break;
        }
        case TokenType::LogicNot: {
            if (!isInteger(etypeR)) 
                res = false, printError(node->getToken().lineId, "invalid operand type");
            eType = ExprType(int32Cls);
            break;
        }
        #pragma endregion
        #pragma region basic operator for assign
        case TokenType::MBrkL: {
            if (etypeL.dimc > 0) {
                if (!isInteger(etypeR)) 
                    res = false, printError(node->getToken().lineId, "invalid operand type");
                eType = etypeL, etypeL.dimc--;
                eType.vtMdf = ValueTypeModifier::MemberRef;
            } else {
                // the candy of this operator is "mapLikeObj.get(index)"
                ClassInfo *cls = etypeL.cls;
                if (isBaseCls(findCls("System.MapLike"), cls)) {
                    const FunctionList &fList = cls->funcMap["get"];
                    auto gsMap = makeSubstMap(cls->generCls, etypeL.generParams);
                    for (auto func : fList) {
                        auto chkRes = func->satisfy(gsMap, { etypeR });
                        if (!std::get<0>(chkRes)) continue;
                        auto getMemOper = new OperatorNode();
                        auto fCall = new IdentifierNode();
                        fCall->getToken().type = TokenType::Identifier;
                        fCall->getToken().dataStr = "get";
                        fCall->addChild(node->getRight());
                        getMemOper->getToken().type = TokenType::GetMem;
                        getMemOper->addChild(node->getLeft());
                        getMemOper->addChild(fCall);
                        node->clearChildren();
                        node->getParent()->replaceChild(node, getMemOper);
                        delete node;
                        return chkEType(getMemOper);
                    }
                    res = false;
                } else res = false;
            }
            break;
        }
    }
    eTypeMap[node] = eType;
    return std::make_tuple(res, eType);
}

std::tuple<bool, ExprType> chkEType(ExpressionNode *node) {
    if (eTypeMap.find(node) != eTypeMap.end()) return std::make_tuple(true, eTypeMap[node]);
    ExprType eType;
    bool res = true;
    switch (node->getType()) {
        case SyntaxNodeType::Expression: {
            auto chkRes = chkEType(node->getContent());
            if (std::get<0>(chkRes)) eType = std::get<1>(chkRes);
            else res = false;
            break;
        }
        case SyntaxNodeType::Operator:
            return chkEType_Operator((OperatorNode *)node);
    }
    eTypeMap[node] = eType;
    return std::make_tuple(res, eType);
}