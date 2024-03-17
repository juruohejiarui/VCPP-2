#include "geninner.h"

std::map<ExpressionNode *, ExprType> eTypeMap;
std::map<IdentifierNode *, FuncCallInfo> funcCallMap;
std::map<IdentifierNode *, VarCallInfo> varCallMap;
std::map<ConstValueNode *, size_t> strCallMap;
std::vector<std::string> strList;

inline bool isInteger(const ExprType &eType) { return eType.dimc == 0 && isIntegerCls(eType.cls); }
inline bool isFloat(const ExprType &eType) { return eType.dimc == 0 && isFloatCls(eType.cls); }
inline bool isNumberEType(const ExprType &eType) { return isInteger(eType) || isFloat(eType); }

const ExprType &getEType(ExpressionNode *node) { return eTypeMap[node]; }
const FuncCallInfo &getFuncCallInfo(IdentifierNode *node) { return funcCallMap[node]; }
const VarCallInfo &getVarCallInfo(IdentifierNode *node) { return varCallMap[node]; }
const size_t getStrId(ConstValueNode *node) { return strCallMap[node]; }
const std::string getString(size_t id) { return strList[id]; }
const std::vector<std::string> &getStrList() { return strList; }

ETChkRes chkEType(ExpressionNode *node);

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
    vIden->setName(vInfo->fullName);
    ExprType etype = vInfo->type;
    eTypeMap[vIden] = etype;
    varCallMap[vIden] = std::make_tuple(vInfo, etype);

    fIden->getToken().type = TokenType::Identifier;
    fIden->setName("calculate");
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

ETChkRes chkEType_Assign(OperatorNode *node) {
    ExpressionNode *exprL = node->getLeft(), *exprR = node->getRight();
    ETChkRes chkResR = chkEType(exprR);
    if (!std::get<0>(chkResR)) return std::make_tuple(false, ExprType());
    if (exprL->getType() == SyntaxNodeType::Operator && exprL->getToken().type == TokenType::MBrkL) {
        // the candy of this operator is "ArrayLikeObj.set(index, value)"
        OperatorNode *operL = (OperatorNode *)exprL;
        ETChkRes chkResLL = chkEType(operL->getLeft()), chkResLR = chkEType(operL->getRight());
        if (!std::get<0>(chkResLL) || !std::get<0>(chkResLR)) return std::make_tuple(false, ExprType());
        if (std::get<1>(chkResLL).dimc > 1) {
            printError(node->getToken().lineId, "The left operand of assign can not be a subarray");
            return eTypeMap[node] = ExprType(), std::make_tuple(false, ExprType());
        }
        // just a array
        if (std::get<1>(chkResLL).dimc == 0) {
            // scan the function list and find set(...)
            ClassInfo *cls = std::get<1>(chkResLL).cls;
            if (isBaseCls(findCls("System.Container.MapLike"), cls)) {
                const FunctionList &fList = cls->funcMap["set"];
                auto gsMap = makeSubstMap(cls->generCls, std::get<1>(chkResLL).generParams);
                for (auto func : fList) {
                    auto chkRes = func->satisfy(gsMap, { std::get<1>(chkResLR), std::get<1>(chkResR) });
                    if (!std::get<0>(chkRes)) continue;
                    auto setMemOper = new OperatorNode();
                    auto fCall = new IdentifierNode();
                    ExpressionNode *eLL = operL->getLeft(), *eLR = operL->getRight(), *eR = node->getRight();
                    SyntaxNode *par = node->getParent();
                    operL->clearChildren();
                    node->clearChildren();
                    delete operL;
                    fCall->getToken().type = TokenType::Identifier;
                    fCall->setName("set");
                    fCall->addChild(eLR);
                    fCall->addChild(eR);
                    setMemOper->getToken().type = TokenType::GetMem;
                    setMemOper->addChild(eLL);
                    setMemOper->addChild(fCall);
                    par->replaceChild(node, setMemOper);
                    delete node;
                    return chkEType(setMemOper);
                }
                printError(node->getToken().lineId, "No member function \"set\" satisfies this operand requirement.");
            }
        }
    }
    auto chkResL = chkEType(exprL);
    if (!std::get<0>(chkResL)) return std::make_tuple(false, ExprType());
    auto etypeL = std::get<1>(chkResL), etypeR = std::get<1>(chkResR);
    if (etypeL.dimc != etypeR.dimc || !isBaseCls(etypeL.cls, etypeR.cls) || etypeL.vtMdf == ValueTypeModifier::TrueValue) {
        // the error messages below are generated by Copilot :-)
        if (etypeL.dimc == 0 && !isBaseCls(etypeL.cls, etypeR.cls)) {
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

ETChkRes chkEType_NewObj(OperatorNode *node) {
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
        GenerSubstMap gsMap = makeSubstMap(etype.cls->generCls, etype.generParams);
        FuncCallInfo chkRes = tryMatchFunc(etype.cls->funcMap["@constructer"], fNode, gsMap);
        if (std::get<0>(chkRes) == nullptr) return std::make_tuple(false, ExprType());
        funcCallMap[fNode] = chkRes;
        eTypeMap[node] = etype;
        return std::make_tuple(true, etype);
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
            eTypeMap[node] = etype;
            return std::make_tuple(res, etype);
        }
    } else {
        printError(node->getToken().lineId, "invalid operand type");
        return std::make_tuple(false, ExprType());
    }
}

ETChkRes chkEType_GetMem(OperatorNode *node) {
    if (node == nullptr) return std::make_tuple(true, ExprType());
    auto chkResL = chkEType(node->getLeft());
    if (!std::get<0>(chkResL) || std::get<1>(chkResL).dimc > 0) {
        printError(node->getLeft()->getToken().lineId, "invalid left operand of \".\"");
        return std::tuple(false, ExprType());
    }
    if (node->getRight()->getType() != SyntaxNodeType::Identifier) {
        printError(node->getRight()->getToken().lineId, "the right operand of \".\" must be an identifier");
        return std::tuple(false, ExprType());
    }
    const ExprType &etypeL = std::get<1>(chkResL);
    ExprType etype;
    IdentifierNode *mem = (IdentifierNode *)node->getRight();
    // if the left operand is "@this" , it is valid to call the protected or private members
    bool res = true, 
        isThis = (node->getLeft()->getType() == SyntaxNodeType::Identifier
                 && ((IdentifierNode *)node->getLeft())->getName() == "@this");
    // a variable member
    if (!mem->isFuncCall()) {
        auto vIter = etypeL.cls->fieldMap.find(mem->getName());
        // if a valid member is found
        if (vIter != etypeL.cls->fieldMap.end()
         && (vIter->second->visibility == IdenVisibility::Public || isThis)) {
            etype = vIter->second->type;
            // substitute
            etype = subst(etype, makeSubstMap(etypeL.cls->generCls, etypeL.generParams));
            etype.vtMdf = ValueTypeModifier::MemberRef;
            varCallMap[mem] = std::make_tuple(vIter->second, etype);
        }
        else res = false;
    } else {
        auto fIter = etypeL.cls->funcMap.find(mem->getName());
        GenerSubstMap gsMap = makeSubstMap(etypeL.cls->generCls, etypeL.generParams);
        // search for the function list
        if (fIter == etypeL.cls->funcMap.end()) res = false;
        else {
            std::vector<ExprType> paramList;
            for (size_t i = 0; i < mem->getParamCount(); i++)
                if (mem->getParam(i) != nullptr) {
                    auto chkResP = chkEType(mem->getParam(i));
                    if (!std::get<0>(chkResP)) res = false;
                    paramList.push_back(std::get<1>(chkResP));
                }
            FunctionInfo *tgFunc = nullptr;
            FuncCallInfo clInfo;
            for (FunctionInfo *func : fIter->second) {
                auto chkResF = func->satisfy(gsMap, paramList);
                if (!std::get<0>(chkResF)) continue;
                tgFunc = func;
                funcCallMap[mem] = clInfo = std::make_tuple(func, std::get<1>(chkResF), std::get<2>(chkResF));
                break;
            }
            if (tgFunc == nullptr && !(tgFunc->visibility == IdenVisibility::Public || isThis)) {
                funcCallMap[mem] = std::make_tuple(nullptr, ExprType(), GTableData());
                printError(mem->getToken().lineId, "can not find function that satisfies the param list");
            }
            etype = std::get<1>(clInfo);
            // if this function is a common func, then change this expression into func(obj, ...)
            if (tgFunc->getDefNode()->getType() == SyntaxNodeType::FuncDef) {
                SyntaxNode *par = node->getParent();
                ExpressionNode *eL = node->getLeft(), *eR = node->getRight();
                node->clearChildren();
                par->replaceChild(node, eR);
                eR->insertChild(2, eL);
                eTypeMap[eR] = etype;
                delete node;
                node = nullptr;
            }
        }
    }
    if (node != nullptr) eTypeMap[node] = etype;
    return std::make_tuple(res, etype);
}

ETChkRes chkEType_Convert(OperatorNode *node) {
    auto chkResL = chkEType(node->getLeft());
    if (!std::get<0>(chkResL)) return std::make_tuple(false, ExprType());
    ExprType etypeR((IdentifierNode *)node->getRight());
    if (!etypeR.setCls()) {
        printError(node->getRight()->getToken().lineId, "Undefined class : " + etypeR.clsName);
        return std::make_tuple(false, ExprType());
    }
    if (std::get<1>(chkResL).isObject() && etypeR == ExprType(uint64Cls)) {
        return std::make_tuple(true, ExprType(etypeR));
    }
    if (isNumberEType(std::get<1>(chkResL)) && isNumberEType(etypeR))
        return std::tuple(true, etypeR);
    if (!isBaseCls(etypeR.cls, std::get<1>(chkResL).cls)) {
        printError(node->getToken().lineId, "You can only convert the value of derived class into base class");
        return std::make_tuple(false, ExprType());
    }
    ExprType etypeT = std::get<1>(chkResL);
    while (etypeT.cls != etypeR.cls) etypeT = etypeT.convertToBase();
    if (etypeT != etypeR) {
        printError(node->getToken().lineId, "Unable to convert " + std::get<1>(chkResL).toDebugString() + " into " + etypeR.toDebugString());
        return std::make_tuple(false, ExprType());
    }
    return std::make_tuple(true, etypeR);
}

ETChkRes chkEType_TreatAs(OperatorNode *node) {
    auto chkResL = chkEType(node->getLeft());
    if (!std::get<0>(chkResL)) return std::make_tuple(false, ExprType());
    ExprType etypeR((IdentifierNode *)node->getRight());
    if (!etypeR.setCls()) {
        printError(node->getRight()->getToken().lineId, "Undefined class : " + etypeR.clsName);
        return std::make_tuple(false, ExprType());
    }
    return std::make_tuple(true, etypeR);
}

ETChkRes chkEType_Operator(OperatorNode *node) {
    if (node == nullptr) return std::make_tuple(true, ExprType());
    switch (node->getToken().type) {
        // the operator "=" must be handled specially, since that it can be used as a function call "mapLikeObj.set(index, value)" if the left expression is calling an element of an array-like object using "[]"
        case TokenType::Assign: return chkEType_Assign(node);
        // the operator "$" must be handled specially, since that meaning of this operator relates to the operand of this opeator: 1. a new object with params 2. new array with size
        case TokenType::NewObj: return chkEType_NewObj(node);
        case TokenType::GetMem: return chkEType_GetMem(node);
        case TokenType::Convert: return chkEType_Convert(node);
        case TokenType::TreatAs: return chkEType_TreatAs(node);
    }    
    ExprType etype;
    bool res = true;
    ETChkRes chkResL = chkEType(node->getLeft()), chkResR = chkEType(node->getRight());
    if (!std::get<0>(chkResL) || !std::get<0>(chkResR)) return std::make_tuple(false, ExprType());
    TokenType operTk = node->getToken().type;
    const ExprType &etypeL = std::get<1>(chkResL), etypeR = std::get<1>(chkResR);
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
                SyntaxNode *par = node->getParent();
                ExpressionNode *eL = node->getLeft(), *eR = node->getRight();
                node->clearChildren();
                auto operCandy = buildOperCandy(vInfo, eL, eR);
                par->replaceChild(node, operCandy);
                delete node;
                return chkEType(operCandy);
            } else {
                etype = getExprType(etypeL, etypeR);
                etype.vtMdf = ValueTypeModifier::TrueValue;
                if (etype == ExprType()) res = false, printError(node->getToken().lineId, "invalid operand type");
            }
            break;
        }
        case TokenType::Shl:
        case TokenType::Shr:
        case TokenType::And:
        case TokenType::Or:
        case TokenType::Xor:
        case TokenType::Mod: {
            auto vInfo = findOperCandy(operCandyStr[operTk], etypeL, etypeR);
            // modify this expression into "operVar.calculate(left expression, right expression)"
            if (vInfo != nullptr) {
                SyntaxNode *par = node->getParent();
                ExpressionNode *eL = node->getLeft(), *eR = node->getRight();
                node->clearChildren();
                auto operCandy = buildOperCandy(vInfo, eL, eR);
                par->replaceChild(node, operCandy);
                delete node;
                return chkEType(operCandy);
            } else {
                if (!isInteger(etypeL) || !isInteger(etypeR)) 
                    res = false, printError(node->getToken().lineId, "invalid operand type");
                etype = getExprType(etypeL, etypeR);
                etype.vtMdf = ValueTypeModifier::TrueValue;
                if (etype == ExprType()) res = false, printError(node->getToken().lineId, "invalid operand type");
            }
            break;
        }
        #pragma endregion
        case TokenType::Dec:
        case TokenType::Inc: {
            if ((node->getLeft() == nullptr) == (node->getRight() == nullptr))
                res = false, printError(node->getToken().lineId, "This operator has only one operand");
            else {
                etype = (node->getLeft() == nullptr ? etypeR : etypeL);
                if (etype.vtMdf == ValueTypeModifier::TrueValue)
                    res = false, printError(node->getToken().lineId, "This operator needs reference value");
                else if (!isInteger(etype))
                    res = false, printError(node->getToken().lineId, "This operator needs integer value");
                else etype.vtMdf = ValueTypeModifier::TrueValue;
            }
            break;
        }
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
                auto eL = node->getLeft(), eR = node->getRight();
                node->clearChildren();
                auto operCandy = buildOperCandy(vInfo, eL, eR); 
                node->addChild(operCandy);
                auto zero = new ConstValueNode();
                zero->getToken().type = TokenType::ConstData;
                zero->getToken().data.uint64_v() = 0;
                zero->getToken().data.type = DataTypeModifier::i32;
                node->addChild(zero);
                return chkEType(node);
            } else {
                if (!isNumberEType(etypeL) || !isNumberEType(etypeR) || etypeL != etypeR) 
                    res = false, printError(node->getToken().lineId, "invalid operand type");
                etype = ExprType(int32Cls);
            }
            break;
        }
        #pragma endregion
        #pragma region basic operator for logic
        case TokenType::LogicAnd:
        case TokenType::LogicOr: {
            if (!isInteger(etypeL) || !isInteger(etypeR)) 
                res = false, printError(node->getToken().lineId, "invalid operand type");
            etype = ExprType(int32Cls);
            break;
        }
        case TokenType::LogicNot: {
            if (!isInteger(etypeR)) 
                res = false, printError(node->getToken().lineId, "invalid operand type");
            etype = ExprType(int32Cls);
            break;
        }
        #pragma endregion
        #pragma region basic operator for assign
        case TokenType::AddAssign:
        case TokenType::SubAssign:
        case TokenType::MulAssign:
        case TokenType::DivAssign: {
            if (etypeL.vtMdf == ValueTypeModifier::TrueValue) {
                res = false;
                printError(node->getToken().lineId, "Can not assign to const value");
            }
            if (etypeL != etypeR) {
                res = false;
                printError(node->getToken().lineId, "invalid operand");
            }
            etype = voidCls;
            break;
        }
        case TokenType::ModAssign:
        case TokenType::ShlAssign:
        case TokenType::ShrAssign:
        case TokenType::AndAssign:
        case TokenType::OrAssign:
        case TokenType::XorAssign: {
             if (etypeL.vtMdf == ValueTypeModifier::TrueValue) {
                res = false;
                printError(node->getToken().lineId, "Can not assign to const value");
            }
            if (etypeL != etypeR || !isInteger(etypeL) || !isInteger(etypeR)) {
                res = false;
                printError(node->getToken().lineId, "invalid operand");
            }
            etype = voidCls;
            break;
        }
        #pragma endregion
        case TokenType::Comma:
            etype = etypeR;
            break;
        case TokenType::MBrkL: {
            if (etypeL.dimc > 0) {
                if (!isInteger(etypeR)) 
                    res = false, printError(node->getToken().lineId, "invalid operand type");
                etype = etypeL, etype.dimc--;
                etype.vtMdf = ValueTypeModifier::MemberRef;
            } else {
                // the candy of this operator is "mapLikeObj.get(index)"
                ClassInfo *cls = etypeL.cls;
                if (isBaseCls(findCls("System.Container.MapLike"), cls)) {
                    const FunctionList &fList = cls->funcMap["get"];
                    auto gsMap = makeSubstMap(cls->generCls, etypeL.generParams);
                    for (auto func : fList) {
                        auto chkRes = func->satisfy(gsMap, { etypeR });
                        if (!std::get<0>(chkRes)) continue;
                        auto getMemOper = new OperatorNode();
                        auto fCall = new IdentifierNode();
                        ExpressionNode *eL = node->getLeft(), *eR = node->getRight();
                        SyntaxNode *par = node->getParent();
                        node->clearChildren();
                        fCall->getToken().type = TokenType::Identifier;
                        fCall->setName("get");
                        fCall->addChild(eR);
                        getMemOper->getToken().type = TokenType::GetMem;
                        getMemOper->addChild(eL);
                        getMemOper->addChild(fCall);
                        par->replaceChild(node, getMemOper);
                        delete node;
                        return chkEType(getMemOper);
                    }
                    res = false;
                } else res = false;
            }
            break;
        }
    }
    eTypeMap[node] = etype;
    return std::make_tuple(res, etype);
}

ETChkRes chkEType_Const(ConstValueNode *node) {
    ExprType etype;
    if (node->getToken().type == TokenType::String) {
        strList.push_back(node->getToken().dataStr);
        strCallMap[node] = strList.size() - 1;
        etype = ExprType(int8Cls), etype.dimc = 1;
    } else {
        switch (node->getToken().data.type) {
            case DataTypeModifier::b: etype.cls = uint8Cls; break;
            case DataTypeModifier::c: etype.cls = int8Cls; break;
            case DataTypeModifier::i16: etype.cls = int16Cls; break;
            case DataTypeModifier::u16: etype.cls = uint16Cls; break;
            case DataTypeModifier::i32: etype.cls = int32Cls; break;
            case DataTypeModifier::u32: etype.cls = uint32Cls; break;
            case DataTypeModifier::i64: etype.cls = int64Cls; break;
            case DataTypeModifier::u64: etype.cls = uint64Cls; break;
            case DataTypeModifier::f32: etype.cls = float32Cls; break;
            case DataTypeModifier::f64: etype.cls = float32Cls; break;
            default: return std::make_tuple(false, etype);
        }
    }
    eTypeMap[node] = etype;
    return std::make_tuple(true, etype);
}

ETChkRes chkEType_Iden(IdentifierNode *node) {
    auto genThis = []() -> IdentifierNode * {
        IdentifierNode *idenThis = new IdentifierNode();
        idenThis->setName("@this");
        varCallMap[idenThis] = findVar("@this");
        eTypeMap[idenThis] = std::get<1>(varCallMap[idenThis]);
        return idenThis;
    };
    if (node->getName() == "@base") {
        if (!node->isFuncCall()) {
            printError(node->getToken().lineId, "\"@base\" can only be used as a function");
            return std::make_tuple(false, ExprType());
        }
        if (getCurFunc() == nullptr || getCurFunc()->blgCls == nullptr) {
            printError(node->getToken().lineId, "\"@base\" can only be used in the constructer of classes.");
            return std::make_tuple(false, ExprType());
        }
        // no need to use constructer for base class if its base class is "object"
        if (getCurCls()->baseCls == objectCls) {
            if (node->getParamCount() > 1 || node->getParam(0) != nullptr) {
                printError(node->getToken().lineId, "there is no such constructer for class \"object\".");
                return std::make_tuple(false, ExprType());
            }
            eTypeMap[node] = ExprType(basicCls);
            return std::make_tuple(true, ExprType(basicCls));
        }
        GenerSubstMap gsMap = makeSubstMap(getCurCls()->baseCls->generCls, getCurCls()->generParams);
        GTableData gtbl;
        gtbl.insert(getCurCls()->baseCls->generCls, gsMap);
        std::vector<ExprType> pList;
        bool res = true;
        for (size_t i = 0; i < node->getParamCount(); i++) 
            if (node->getParam(i) != nullptr) {
                auto chkRes = chkEType(node->getParam(i));
                if (!std::get<0>(chkRes)) res = false;
                pList.push_back(std::get<1>(chkRes));
            }
        if (!res) return std::make_tuple(false, ExprType());
        ClassInfo *bsCls = getCurCls()->baseCls;
        auto fLPir = bsCls->funcMap.find("@constructer");
        if (fLPir == bsCls->funcMap.end()) {
            printError(node->getToken().lineId, "class \"" + bsCls->fullName + "\" has no constructer.");
            return std::make_tuple(false, ExprType());
        }
        for (auto func : fLPir->second) {
            auto chkRes = func->satisfy(gsMap, pList);
            if (!std::get<0>(chkRes)) continue;
            // add "@this" into the param list
            auto idenThis = genThis();
            node->insertChild(2, idenThis);
            funcCallMap[node] = std::make_tuple(func, voidCls, gtbl);
            eTypeMap[node] = voidCls;
            return std::make_tuple(true, voidCls);
        }
        return std::make_tuple(false, ExprType());
    } else if (node->getName() == "@vasm") {
        if (!node->isFuncCall()) {
            printError(node->getToken().lineId, "\"@vasm\" can only be used as a function");
            return std::make_tuple(false, ExprType());
        }
        bool res = true;
        std::vector<ExprType> paramList;
        if (node->getParamCount() != 1) {
            printError(node->getToken().lineId, "@vasm() needs one char[] param");
            return std::make_tuple(false, ExprType());
        }
        ExprType tmpChArr = ExprType(int8Cls); tmpChArr.dimc = 1;
        auto chkResP = chkEType(node->getParam(0));
        if (!std::get<0>(chkResP) || std::get<1>(chkResP) != tmpChArr) return std::make_tuple(false, ExprType());
        eTypeMap[node] = ExprType(voidCls);
        return std::make_tuple(true, ExprType(voidCls));
    } else if (node->getName() == "@setPause") {
        if (!node->isFuncCall()) {
            printError(node->getToken().lineId, "\"@setPause\" can only be used as a function");
            return std::make_tuple(false, ExprType());
        }
        if (node->getParamCount() != 1 && node->getParam(0) != nullptr) {
            printError(node->getToken().lineId, "@setPause have no param");
            return std::make_tuple(false, ExprType());
        }
        eTypeMap[node] = ExprType(voidCls);
        return std::make_tuple(true, ExprType(voidCls));
    }
    ExprType etype;
    bool res = true;
    if (node->isFuncCall()) {
        std::vector<ExprType> paramList;
        for (size_t i = 0; i < node->getParamCount(); i++)
            if (node->getParam(i) != nullptr) {
                auto chkResP = chkEType(node->getParam(i));
                if (!std::get<0>(chkResP)) res = false;
                paramList.push_back(std::get<1>(chkResP));
            }
        FuncCallInfo clInfo = findFunc(node->getName(), paramList);
        if (std::get<0>(clInfo) == nullptr) {
            printError(node->getToken().lineId, "can not find function that satisfies the param list");
            funcCallMap[node] = std::make_tuple(nullptr, ExprType(), GTableData());
            return std::make_tuple(false, ExprType());
        }
        funcCallMap[node] = clInfo;
        etype = std::get<1>(clInfo);
        const FunctionInfo *func = std::get<0>(clInfo);
        // a member function
        if (func->blgCls != nullptr) {
            // generate an identifier "@this"
            IdentifierNode *idenThis = genThis();
            // a function of itself
            if (func->blgCls == getCurCls()) {
                // if this is a varfunc, then add a preffix : "@this." and check again
                if (func->getDefNode()->getType() == SyntaxNodeType::VarFuncDef) {
                    OperatorNode *getMem = new OperatorNode();
                    SyntaxNode *par = node->getParent();
                    par->replaceChild(node, getMem);
                    getMem->addChild(idenThis), getMem->addChild(node);
                    getMem->getToken().type = TokenType::GetMem;
                    eTypeMap[getMem] = std::get<1>(clInfo);
                } else { // if this is a common function, then add a param "@this"
                    node->insertChild(2, idenThis);
                    eTypeMap[node] = std::get<1>(clInfo);
                }
            } else { // a function of base class
                node->insertChild(2, idenThis);
                eTypeMap[node] = std::get<1>(clInfo);
            }
        // if it is a global function, then the expression not need to be changed.
        } else eTypeMap[node] = etype;
    } else { // this identifier is a variable
        VarCallInfo clInfo = findVar(node->getName());
        if (std::get<0>(clInfo) == nullptr) {
            printError(node->getToken().lineId, "can not find variable \"" + node->getName() + "\"");
            varCallMap[node] = std::make_tuple(nullptr, ExprType());
            return std::make_tuple(false, ExprType());
        }
        VariableInfo *vInfo = std::get<0>(clInfo);
        etype = std::get<1>(clInfo);
        varCallMap[node] = clInfo;
        // member variable
        if (vInfo->blgFunc == nullptr && vInfo->blgCls != nullptr) {
            IdentifierNode *idenThis = genThis();
            OperatorNode *getMem = new OperatorNode();
            getMem->getToken().type = TokenType::GetMem;
            SyntaxNode *par = node->getParent();
            par->replaceChild(node, getMem);
            getMem->addChild(idenThis), getMem->addChild(node);
            eTypeMap[getMem] = std::get<1>(clInfo);
        } else eTypeMap[node] = etype;
    }
    return std::make_tuple(res, etype);
}

ETChkRes chkEType(ExpressionNode *node) {
    if (eTypeMap.find(node) != eTypeMap.end()) return std::make_tuple(true, eTypeMap[node]);
    if (node == nullptr) return std::make_tuple(true, ExprType());
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
        case SyntaxNodeType::ConstValue:
            return chkEType_Const((ConstValueNode *)node);
        case SyntaxNodeType::Identifier:
            return chkEType_Iden((IdentifierNode *)node);
    }
    eTypeMap[node] = eType;
    return std::make_tuple(res, eType);
}