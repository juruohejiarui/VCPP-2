#include "tokenlist.h"

const std::string tokenTypeString[] = {
    "Identifier", "ConstData", "String",
    // Brackets
    "SBrkL", "SBrkR", "MBrkL", "MBrkR", "LBrkL", "LBrkR", "GBrkL", "GBrkR",
    // operators
    "Comma", 
    "Assign", "AddAssign", "SubAssign", "MulAssign", "DivAssign", "ModAssign", "AndAssign", "OrAssign", "XorAssign", "ShlAssign", "ShrAssign",
    "Add", "Sub", "Mul", "Div", "Mod", "And", "Or", "Xor", "Shl", "Shr", "Not", "LogicAnd", "LogicOr", "LogicNot",
    "Equ", "Neq", "Gt", "Ge", "Ls", "Le",
    "Inc", "Dec",
    "GetMem", "NewObj", "Convert", "TreatAs", "GetChild",

    "TypeHint",
    // keywords
    "If", "Else", "While", "For", "Continue", "Break", "Return", 
    "VarDef", "FuncDef", "VarFuncDef", "ClsDef", "NspDef",
    "Private", "Protected", "Public", "Using", 

    // pretreat command
    "Vasm", 
    "ExprEnd",
    "Unknown"
};

const std::string keywordString[] = {
    "if", "else", "while", "for", "continue", "break", "return", "var", "func", "varfunc", "class", "namespace", "private", "protected", "public", "using",
};
const size_t tokenTypeNumber = 68, keywordTokenTypeNumber = 16;


TokenType getKeyworkType(const std::string str) {
    for (int i = 0; i < keywordTokenTypeNumber; i++) if (str == keywordString[i]) return TokenType(i + (int)TokenType::If);
    return TokenType::Unknown;
}

TokenType getTokenType(const std::string& str) {
    for (int i = 0; i < tokenTypeNumber; i++) if (tokenTypeString[i] == str) return (TokenType)i;
    return TokenType::Unknown;
}

const uint32 operWeight[] = {
    1, /*, */ 
    2/* = */, 2/*+=*/, 2/*-=*/, 2/**=*/, 2/*/=*/, 2/*%=*/, 2/*&=*/, 2/*|=*/, 2/*^=*/, 2/*<<=*/, 2/*>>=*/,
    11/*+*/, 11/*-*/, 12/***/, 12/*/*/, 12/*%*/, 7/*&*/, 5/*|*/, 6/*^*/, 10/*<<*/, 10/*>>*/, 13/*~*/, 4/*&&*/, 3/*||*/, 13/*!*/,
    8/*==*/, 8/*!=*/, 9/*>*/, 9/*>=*/, 9/*<*/, 9/*<=*/,
    13/*++*/, 13/*--*/,
    14/*.*/, 14/*$*/, 14/*=>*/, 14/* as */, 14/*::*/,
};

uint32 getOperWeight(TokenType oper) {
    // the weight of []
    if (oper == TokenType::MBrkL || oper == TokenType::MBrkR) return 15;
    if (oper < TokenType::Comma || oper > TokenType::TreatAs) return -1;
    return operWeight[(int32)oper - (int32)TokenType::Comma];
}

bool generateTokenList(const std::string &src, TokenList &tkList) {
    std::vector<std::string> lines;
    stringSplit(src, '\n', lines);
    int lId = 0;

    static std::stack<size_t> brkStk;
    while (!brkStk.empty()) brkStk.pop();

    for (const auto &line : lines) {
        size_t pos = 0;
        lId++;
        while (pos < line.size() && isSeparator(line[pos])) pos++;
        for (size_t l = pos, r = pos; l < line.size(); l = ++r) {
            Token tk;
            tk.lineId = lId;
            if (line[l] == '\"') {
                tk.type = TokenType::String;
                tk.dataStr = getString(line, l, r);
            } else if (line[l] == '\'') {
                tk.type = TokenType::ConstData;
                tk.data.type = DataTypeModifier::c;
                if (line[l + 1] == '\'') tk.data.int8_v() = '\0';
                else if (line[l + 1] == '\\') {
                    if (line[l + 2] == 'n') tk.data.int8_v() = '\n';
                    else if (line[l + 2] == 't') tk.data.int8_v() = '\t';
                    else if (line[l + 2] == 'r') tk.data.int8_v() = '\r';
                    else if (line[l + 2] == '\\') tk.data.int8_v() = '\\';
                    else if (line[l + 2] == '\'') tk.data.int8_v() = '\'';
                    else if (line[l + 2] == '\"') tk.data.int8_v() = '\"';
                    else if (line[l + 2] == '0') tk.data.int8_v() = '\0';
                    else {
                        printError(lId, "unknown escape character");
                        return false;
                    }
                    r += 2;
                } else tk.data.int8_v() = line[l + 1], r++;
                if (line[r + 1] != '\'') {
                    printError(lId, "can not match \'");
                    return false;
                }
                r++;
            } else if (isLetter(line[l]) || line[l] == '@') {
                while (r + 1 < line.size() && (isLetter(line[r + 1]) || isNumber(line[r + 1]) || line[r + 1] == '@'))
                    r++;
                tk.dataStr = line.substr(l, r - l + 1);
                tk.type = getKeyworkType(tk.dataStr);
                if (tk.dataStr == "as") tk.type = TokenType::TreatAs;
                else if (tk.type == TokenType::Unknown) tk.type = TokenType::Identifier;
            } else if (isNumber(line[l])) {
                tk.type = TokenType::ConstData;
                while (r + 1 < line.size() && (isLetter(line[r + 1]) || isNumber(line[r + 1]) || line[r + 1] == '.')) r++;
                tk.dataStr = line.substr(l, r - l + 1);
                tk.data = getUnionData(tk.dataStr);
            } else if (line[l] == '=') {
                if (l + 1 < line.size() && line[l + 1] == '=') r++, tk.type = TokenType::Equ;
                else tk.type = TokenType::Assign;  
            } else if (line[l] == '/') { // hint / /=
                if (l + 1 < line.size()) {
                    if (line[l + 1] == '/') r = line.size() - 1; // the rest of this line is hint
                    else if (line[l + 1] == '=') r++, tk.type = TokenType::DivAssign;
                    else tk.type = TokenType::Div;
                } else tk.type = TokenType::Div;
            } else if (line[l] == '%') {
                if (l + 1 < line.size() && line[l + 1] == '=') r++, tk.type = TokenType::ModAssign;
                else tk.type = TokenType::Mod;
            } else if (line[l] == '*') { // * *=
                if (l + 1 < line.size() && line[l + 1] == '=') r++, tk.type = TokenType::MulAssign;
                else tk.type = TokenType::Mul;
            } else if (line[l] == '+') { // + ++ +=
                if (l + 1 < line.size() && line[l + 1] == '+') r++, tk.type = TokenType::Inc;
                else if (l + 1 < line.size() && line[l + 1] == '=') r++, tk.type = TokenType::AddAssign;
                else tk.type = TokenType::Add;
            } else if (line[l] == '-') { // - -= -- ->
                if (l + 1 < line.size() && line[l + 1] == '-') r++, tk.type = TokenType::Dec;
                else if (l + 1 < line.size() && line[l + 1] == '=') r++, tk.type = TokenType::SubAssign;
                else if (l + 1 < line.size() && line[l + 1] == '>') r++, tk.type = TokenType::Convert;
                else tk.type = TokenType::Sub;
            } else if (line[l] == '<') { // < <= << <<= <$
                if (l + 1 < line.size()) {
                    // the left bracket of generic symbol
                    if (line[l + 1] == '$') {
                        r++, tk.type = TokenType::GBrkL;
                        brkStk.push(tkList.size());
                    }
                    else if (line[l + 1] == '=') r++, tk.type = TokenType::Le;
                    else if (line[l + 1] == '<') {
                        if (l + 2 < line.size() && line[l + 2] == '=') r += 2, tk.type = TokenType::ShlAssign;
                        else r++, tk.type = TokenType::Shl;
                    } else tk.type = TokenType::Ls;
                } else tk.type = TokenType::Ls;
            } else if (line[l] == '>') { // > >= >> >>= 
                if (l + 1 < line.size()) {
                    if (line[l + 1] == '=') r++, tk.type = TokenType::Ge;
                    else if (line[l + 1] == '>') {
                        if (l + 2 < line.size() && line[l + 2] == '=') r += 2, tk.type = TokenType::ShrAssign;
                        else r++, tk.type = TokenType::Shr;
                    } else tk.type = TokenType::Gt;
                } else tk.type = TokenType::Gt;
            } else if (line[l] == '!') { // ! !=
                if (l + 1 < line.size() && line[l + 1] == '=') r++, tk.type = TokenType::Neq;
                else tk.type = TokenType::LogicNot;
            } else if (line[l] == '&') { // & && &=
                if (l + 1 < line.size() && line[l + 1] == '&') r++, tk.type = TokenType::LogicAnd;
                else if (l + 1 < line.size() && line[l + 1] == '=') r++, tk.type = TokenType::AndAssign;
                else tk.type = TokenType::And;
            } else if (line[l] == '|') { // | || |=
                if (l + 1 < line.size() && line[l + 1] == '|') r++, tk.type = TokenType::LogicOr;
                else if (l + 1 < line.size() && line[l + 1] == '=') r++, tk.type = TokenType::OrAssign;
                else tk.type = TokenType::Or;
            } else if (line[l] == '^') { // ^ ^=
                if (l + 1 < line.size() && line[l + 1] == '=') r++, tk.type = TokenType::XorAssign;
                else tk.type = TokenType::Xor;
            } else if (line[l] == '$') {
                if (l + 1 < line.size() && line[l + 1] == '>') {
                    r++, tk.type = TokenType::GBrkR;
                    if (brkStk.empty() || tkList[brkStk.top()].type != TokenType::GBrkL) {
                        printError(lId, "can not match bracket of \"<$ $>\"");
                        return false;
                    }
                    tkList[brkStk.top()].data.uint64_v() = tkList.size();
                    tk.data.uint64_v() = brkStk.top();
                    brkStk.pop();
                } else tk.type = TokenType::NewObj;
            } else if (line[l] == ':') {
                if (l + 1 < line.size() && line[l + 1] == ':') {
                    r++, tk.type = TokenType::GetChild;
                } else tk.type = TokenType::TypeHint;
            } else if (line[l] == ',') {
                tk.type = TokenType::Comma;
            } else if (line[l] == '.') {
                tk.type = TokenType::GetMem;
            } else if (line[l] == '{') {
                tk.type = TokenType::LBrkL;
                brkStk.push(tkList.size());
            } else if (line[l] == '}') {
                tk.type = TokenType::LBrkR;
                if (brkStk.empty() || tkList[brkStk.top()].type != TokenType::LBrkL) {
                    printError(lId, "can not match bracket of \"{ }\"");
                    return false;
                }
                tkList[brkStk.top()].data.uint64_v() = tkList.size();
                tk.data.uint64_v() = brkStk.top();
                brkStk.pop();
            } else if (line[l] == '[') {
                tk.type = TokenType::MBrkL;
                brkStk.push(tkList.size());
            } else if (line[l] == ']') {
                tk.type = TokenType::MBrkR;
                if (brkStk.empty() || tkList[brkStk.top()].type != TokenType::MBrkL) {
                    printError(lId, "can not match bracket of \"[ ]\"");
                    return false;
                }
                tkList[brkStk.top()].data.uint64_v() = tkList.size();
                tk.data.uint64_v() = brkStk.top();
                brkStk.pop();
            } else if (line[l] == '(') {
                tk.type = TokenType::SBrkL;
                brkStk.push(tkList.size());
            } else if (line[l] == ')') {
                tk.type = TokenType::SBrkR;
                if (brkStk.empty() || tkList[brkStk.top()].type != TokenType::SBrkL) {
                    printError(lId, "can not match bracket of \"( )\"");
                    return false;
                }
                tkList[brkStk.top()].data.uint64_v() = tkList.size();
                tk.data.uint64_v() = brkStk.top();
                brkStk.pop();
            } else if (line[l] == ';') tk.type = TokenType::ExprEnd;
            if (tk.type != TokenType::Unknown) tkList.emplace_back(tk);
        }
    }
    return true;
}

Token::Token() {
    this->type = TokenType::Unknown;
    this->data = UnionData(0ull);
    this->lineId = 0;
}

std::string Token::toString() const {
    std::string res = tokenTypeString[(int)this->type];
    if (type == TokenType::ConstData || (type >= TokenType::SBrkL && type <= TokenType::GBrkR))
        res.append(" " + std::to_string(data.data.uint64_v));
    else if (type == TokenType::Identifier) res.append(" " + dataStr);
    return res;
}

bool isBracketL(TokenType type) { return type == TokenType::LBrkL || type == TokenType::MBrkL || type == TokenType::SBrkL || type == TokenType::GBrkL; }
bool isBracketR(TokenType type) { return type == TokenType::LBrkR || type == TokenType::MBrkR || type == TokenType::SBrkR || type == TokenType::GBrkR; }
bool isOperator(TokenType type) { return type >= TokenType::Comma && type <= TokenType::TreatAs; }
bool isCmpOperator(TokenType type) { return type >= TokenType::Equ && type <= TokenType::Le; }
bool isMovOperator(TokenType type) { return type >= TokenType::Assign && type <= TokenType::ShrAssign; }
bool isVisibility(TokenType type) { return type == TokenType::Private || type == TokenType::Public || type == TokenType::Protected; }
bool isAssignFamily(TokenType type) { return type >= TokenType::Assign && type <= TokenType::ShrAssign; }

IdenVisibility getVisibility(TokenType type) {
    if (type == TokenType::Private) return IdenVisibility::Private;
    if (type == TokenType::Public) return IdenVisibility::Public;
    if (type == TokenType::Protected) return IdenVisibility::Protected;
    return IdenVisibility::Unknown;
}



TCommand getTCommand(TokenType tk) {
    switch (tk) {
        case TokenType::Add: return TCommand::add;
        case TokenType::Sub: return TCommand::sub;
        case TokenType::Mul: return TCommand::mul;
        case TokenType::Div: return TCommand::div;
        case TokenType::Mod: return TCommand::mod;
        case TokenType::Shl: return TCommand::shl;
        case TokenType::Shr: return TCommand::shr;
        case TokenType::And: return TCommand::_and;
        case TokenType::Or: return TCommand::_or;
        case TokenType::Xor: return TCommand::_xor;
        case TokenType::Equ: return TCommand::eq;
        case TokenType::Neq: return TCommand::ne;
        case TokenType::Gt: return TCommand::gt;
        case TokenType::Ge: return TCommand::ge;
        case TokenType::Ls: return TCommand::ls;
        case TokenType::Le: return TCommand::le;
    }
    return TCommand::unknown;
}