#include "tokenlist.h"

const std::string tokenTypeString[] = {
    "Identifier", "ConstData", "String",
    // Brackets
    "SBrkL", "SBrkR", "MBrkL", "MBrkR", "LBrkL", "LBrkR", "GBrkL", "GBrkR",
    // operators
    "Assign", "AddAssign", "SubAssign", "MulAssign", "DivAssign", "ModAssign", "AndAssign", "OrAssign", "XorAssign", "ShlAssign", "ShrAssign",
    "Add", "Sub", "Mul", "Div", "Mod", "And", "Or", "Xor", "Shl", "Shr", "Not", "LogicAnd", "LogicOr", "LogicNot",
    "Equ", "Neq", "Gt", "Ge", "Ls", "Le",
    "Inc", "Dec",
    "GetMem", "NewObj", "Convert",

    "TypeHint",
    // keywords
    "If", "Else", "While", "For", "Continue", "Break", "Return", 
    "VarDef", "FuncDef", "VarFuncDef", "ClsDef", "NspDef",
    "Private", "Public", "Protected",

    // pretreat command
    "Vasm", 
    "ExprEnd",
    "Unknown"
};

const std::string keywordString[] = {
    "if", "else", "while", "for", "continue", "break", "return", "var", "func", "varfunc", "class", "namespace", "private", "public", "protected",
};
const size_t tokenTypeNumber = 65, keywordTokenTypeNumber = 15;


TokenType getKeyworkType(const std::string str) {
    for (int i = 0; i < keywordTokenTypeNumber; i++) if (str == keywordString[i]) return TokenType(i + (int)TokenType::If);
    return TokenType::Unknown;
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
            if (isLetter(line[l])) {
                while (r + 1 < line.size() && (isLetter(line[r + 1]) || isNumber(line[r + 1])))
                    r++;
                tk.dataStr = line.substr(l, r - l + 1);
                tk.type = getKeyworkType(tk.dataStr);
                if (tk.dataStr == "as") tk.type = TokenType::Convert;
                else if (tk.type == TokenType::Unknown) tk.type = TokenType::Identifier;
            } else if (isNumber(line[l])) {
                tk.type = TokenType::ConstData;
                while (r + 1 < line.size() && (isLetter(line[r + 1]) || isNumber(line[r + 1]))) r++;
                tk.dataStr = line.substr(l, r - l + 1);
                tk.data = getUnionData(tk.dataStr);
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
            } else if (line[l] == '-') { // - -= --
                if (l + 1 < line.size() && line[l + 1] == '-') r++, tk.type = TokenType::Dec;
                else if (l + 1 < line.size() && line[l + 1] == '=') r++, tk.type = TokenType::SubAssign;
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
            } else if (line[l] == '{') {
                tk.type = TokenType::LBrkL;
                brkStk.push(tkList.size());
            } else if (line[l] == '}') {
                tk.type = TokenType::LBrkR;
                if (brkStk.empty() || tkList[brkStk.top()].type != TokenType::LBrkL) {
                    printError(lId, "can not match bracket of \"<$ $>\"");
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
                    printError(lId, "can not match bracket of \"<$ $>\"");
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
                    printError(lId, "can not match bracket of \"<$ $>\"");
                    return false;
                }
                tkList[brkStk.top()].data.uint64_v() = tkList.size();
                tk.data.uint64_v() = brkStk.top();
                brkStk.pop();
            }
            if (tk.type != TokenType::Unknown) tkList.emplace_back(tk);
        }
    }
    return false;
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
