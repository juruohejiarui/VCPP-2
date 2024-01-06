#include "geninner.h"

std::map<ExpressionNode *, ExprType> eTypeMap;
std::map<IdentifierNode *, FuncCallInfo> funcCallMap;
std::map<IdentifierNode *, VarCallInfo> varCallMap;

const ExprType &getEType(ExpressionNode *node) { return eTypeMap[node]; }
const FuncCallInfo &getFuncCallInfo(IdentifierNode *node) { return funcCallMap[node]; }
const VarCallInfo &getVarCallInfo(IdentifierNode *node) { return varCallMap[node]; }

std::tuple<bool, ExprType> chkEType(ExpressionNode *node);

std::tuple<bool, ExprType> chkEType_Operator(OperatorNode *node) {
    ExprType eType;
    bool res = true;
    eTypeMap[node] = eType;
    return std::make_tuple(res, eType);
}

std::tuple<bool, ExprType> chkEType(ExpressionNode *node) {
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