#include "constexprcalc.h"

bool isInvalidOper(TokenType tk) {
    return tk == TokenType::Convert || tk == TokenType::TreatAs
            || tk == TokenType::NewObj
            || tk == TokenType::Inc || tk == TokenType::Dec
            || tk == TokenType::GetMem;
}

bool isConstExpr(ExpressionNode *expr) {
    switch (expr->getType()) {
        case SyntaxNodeType::ConstValue: return expr->getToken().type != TokenType::String;
        case SyntaxNodeType::Identifier: return false;
        case SyntaxNodeType::Operator: {
            bool res = true;
            OperatorNode *oper = (OperatorNode *)expr;
            if (isInvalidOper(oper->getToken().type)) return false;
            if (oper->getLeft() != nullptr) res &= isConstExpr(oper->getLeft());
            if (oper->getRight() != nullptr) res &= isConstExpr(oper->getRight());
            return res;
        }
    }
    return false;
}

DataTypeModifier getTgType(DataTypeModifier type1, DataTypeModifier type2) { return std::max(type1, type2); }
/// @brief calculate the const expr and delete the original expression tree
/// @param expr 
/// @return 
ConstValueNode *calcConstExpr(ExpressionNode *expr) {
    if (expr == nullptr) return nullptr;
    switch (expr->getType()) {
        case SyntaxNodeType::ConstValue: {
            ConstValueNode *cpy = new ConstValueNode(*(ConstValueNode *)expr);
            delete expr;
            return cpy;
        }
        case SyntaxNodeType::Operator: {
            OperatorNode *oper = (OperatorNode *)expr;
            ConstValueNode *l = calcConstExpr(oper->getLeft()), *r = calcConstExpr(oper->getRight()),
                *res = new ConstValueNode();
            UnionData ldt, rdt;
            if (l != nullptr) ldt = l->getToken().data, delete l;
            if (r != nullptr) rdt = r->getToken().data, delete r;
            auto convertToTg = [&ldt, &rdt]() {
                auto tgType = getTgType(ldt.type, rdt.type);
                ldt = ldt.convertTo(tgType), rdt = rdt.convertTo(tgType);
            };
            switch (oper->getToken().type) {
                case TokenType::Add:
                    convertToTg();
            }
        }
    }
    return nullptr;
}