#pragma once

#include "../Tools/tools.h"
#include "../VASM/commandlist.h"

enum class TokenType {
    Identifier, ConstData, String,
    // Brackets
    SBrkL, SBrkR, MBrkL, MBrkR, LBrkL, LBrkR, GBrkL, GBrkR,
    // operators
    Comma, 
    Assign, AddAssign, SubAssign, MulAssign, DivAssign, ModAssign, AndAssign, OrAssign, XorAssign, ShlAssign, ShrAssign,
    Add, Sub, Mul, Div, Mod, And, Or, Xor, Shl, Shr, Not, LogicAnd, LogicOr, LogicNot,
    Equ, Neq, Gt, Ge, Ls, Le,
    Inc, Dec,
    GetMem, NewObj, Convert, TreatAs, GetChild, 

    TypeHint,
    // keywords
    If, Else, While, For, Switch, Case, Default, Continue, Break, Return, 
    VarDef, FuncDef, VarFuncDef, ClsDef, NspDef,
    Private, Protected, Public, Using, 
    
    // pretreat command
    Vasm, 
    ExprEnd,
    Unknown,
};
extern const std::string tokenTypeString[];
extern const size_t tokenTypeNumber, keywordTokenTypeNumber;

TokenType getTokenType(const std::string &str);
/// @brief Get the weight of operator
/// @param oper the token of operator
/// @return the weight of operator
uint32 getOperWeight(TokenType oper);

struct Token {
    TokenType type;
    UnionData data;
    std::string dataStr;
    
    uint32 lineId;

    Token();

    std::string toString() const ;
};

typedef std::vector<Token> TokenList;

/// @brief Generate a token list using the vcpp source, then return whether this token list is generated successfully
/// @param path the content of vcpp file
/// @param tkList the token list (as the result)
/// @return whether it is generated successfully
bool generateTokenList(const std::string &src, TokenList &tkList);

bool isBracketL(TokenType type);
bool isBracketR(TokenType type);
bool isOperator(TokenType type);
bool isAssignFamily(TokenType type);
bool isCmpOperator(TokenType type);
bool isMovOperator(TokenType type);
bool isVisibility(TokenType type);

IdenVisibility getVisibility(TokenType type);

TCommand getTCommand(TokenType tk);
