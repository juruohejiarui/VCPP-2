#pragma once

#include "../Tools/tools.h"

enum class TokenType {
    Identifier, ConstData,

};
extern std::string TokenTypeString[];
TokenType getTokenType(const std::string &str);

struct Token {
    TokenType type;
    UnionData data;
    std::string dataStr;
    
    uint32 lineId;

    std::string toString() const ;
};

typedef std::vector<Token> TokenList;

/// @brief Generate a token list using the vcpp file whose path is PATH, then return whether this token list is generated successfully
/// @param path the path of vcpp file
/// @param tkList the token list (as the result)
/// @return whether it is generated successfully
bool generateTokenList(const std::string &path, TokenList &tkList);
