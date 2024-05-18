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
        case SyntaxNodeType::Expression: return isConstExpr(expr->getContent());
    }
    return false;
}

DataTypeModifier getTgType(DataTypeModifier type1, DataTypeModifier type2) { return std::max(type1, type2); }

#define calcBinary(oper) \
    switch (ldt.type) { \
        case DataTypeModifier::c: ldt.int8_v() = ldt.int8_v() oper rdt.int8_v(); break; \
        case DataTypeModifier::b: ldt.uint8_v() = ldt.uint8_v() oper rdt.uint8_v(); break; \
        case DataTypeModifier::i16: ldt.int16_v() = ldt.int16_v() oper rdt.int16_v(); break; \
        case DataTypeModifier::u16: ldt.uint16_v() = ldt.uint16_v() oper rdt.uint16_v(); break; \
        case DataTypeModifier::i32: ldt.int32_v() = ldt.int32_v() oper rdt.int32_v(); break; \
        case DataTypeModifier::u32: ldt.uint32_v() = ldt.uint32_v() oper rdt.uint32_v(); break; \
        case DataTypeModifier::i64: ldt.int64_v() = ldt.int64_v() oper rdt.int64_v(); break; \
        case DataTypeModifier::u64: ldt.uint64_v() = ldt.uint64_v() oper rdt.uint64_v(); break; \
        case DataTypeModifier::f32: ldt.float32_v() = ldt.float32_v() oper rdt.float32_v(); break; \
        case DataTypeModifier::f64: ldt.float64_v() = ldt.float64_v() oper rdt.float64_v(); break; \
    } \

#define calcBinaryInt(oper) \
    switch (ldt.type) { \
        case DataTypeModifier::c: ldt.int8_v() = ldt.int8_v() oper rdt.int8_v(); break; \
        case DataTypeModifier::b: ldt.uint8_v() = ldt.uint8_v() oper rdt.uint8_v(); break; \
        case DataTypeModifier::i16: ldt.int16_v() = ldt.int16_v() oper rdt.int16_v(); break; \
        case DataTypeModifier::u16: ldt.uint16_v() = ldt.uint16_v() oper rdt.uint16_v(); break; \
        case DataTypeModifier::i32: ldt.int32_v() = ldt.int32_v() oper rdt.int32_v(); break; \
        case DataTypeModifier::u32: ldt.uint32_v() = ldt.uint32_v() oper rdt.uint32_v(); break; \
        case DataTypeModifier::i64: ldt.int64_v() = ldt.int64_v() oper rdt.int64_v(); break; \
        case DataTypeModifier::u64: ldt.uint64_v() = ldt.uint64_v() oper rdt.uint64_v(); break; \
    } \

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
                case TokenType::Add:    convertToTg(); calcBinary(+); break;
                case TokenType::Sub:    convertToTg(); calcBinary(-); break;
                case TokenType::Mul:    convertToTg(); calcBinary(*); break;
                case TokenType::Div:    convertToTg(); calcBinary(/); break;
                case TokenType::Mod:    convertToTg(); calcBinaryInt(%); break;
                case TokenType::And:    convertToTg(); calcBinaryInt(&); break;
                case TokenType::Or:     convertToTg(); calcBinaryInt(|); break;
                case TokenType::Xor:    convertToTg(); calcBinaryInt(^); break;
                case TokenType::Shl:    convertToTg(); calcBinaryInt(<<); break;
                case TokenType::Shr:    convertToTg(); calcBinaryInt(>>); break;
                case TokenType::Not: {
                    switch (ldt.type) {
                        case DataTypeModifier::c:
                        case DataTypeModifier::b:
                            ldt.uint8_v() = ~ldt.uint8_v(); break;
                        case DataTypeModifier::i16:
                        case DataTypeModifier::u16:
                            ldt.uint16_v() = ~ldt.uint16_v(); break;
                        case DataTypeModifier::i32:
                        case DataTypeModifier::u32:
                            ldt.uint32_v() = ~ldt.uint32_v(); break;
                        case DataTypeModifier::i64:
                        case DataTypeModifier::u64:
                            ldt.uint64_v() = ~ldt.uint64_v(); break;
                    }
                }
            }
            res->getToken().data = ldt;
            res->getToken().type = TokenType::ConstData;
            return res;
        }
        case SyntaxNodeType::Expression: {
            ConstValueNode *cpy = calcConstExpr(expr->getContent());
            delete expr;
            return cpy;
        }
    }
    return nullptr;
}