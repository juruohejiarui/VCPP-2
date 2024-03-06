#include "vobjbuilder.h"

CommandInfo::CommandInfo(Command _command, uint32 lineId) {
    this->command = _command;
    this->lineId = lineId;
    vcode = 0;
}

#pragma region VASMPackage
VASMPackage::VASMPackage() {
    vcodeSize = mainAddr = gloMem = 0;
}

bool VASMPackage::generate(const std::string &srcPath, bool ignoreHint) {
    bool succ = true;
    std::ifstream ifs(srcPath);
    if (!ifs.good()) {
        printMessage(("Unable to read file " + srcPath + "\n").c_str(), MessageType::Error);
        return false;
    }
    int lineId = 0;
    while (!ifs.eof()) {
        std::string line;
        std::getline(ifs, line);
        lineId++;
        succ &= generateLine(line, lineId, ignoreHint);
    }

    #ifndef NDEBUG
    std::cout << "labels: \n";
    for (auto &pir : labelOffset) std::cout << "Offset 0x" << std::setfill('0') << std::setbase(16) << std::setw(8) << pir.second << " : " << pir.first << std::endl;  
    std::cout << "hints: \n";
    for (auto &pir : hints) std::cout << "Offset 0x" << std::setfill('0') << std::setbase(16) << std::setw(8) << pir.first << " : " << pir.second << std::endl;  
    std::cout << "strings \n";
    for (auto &pir : strList) std::cout << pir << std::endl;  
    std::cout << "global memory : " << gloMem << std::endl;
    for (auto &cInfo : cmdList) {
        std::cout << std::setfill('0') << "0x" << std::setiosflags(std::ios::right) << std::setw(8) << cInfo.offset << " ";
        std::cout << std::setfill(' ') << std::setiosflags(std::ios::left) << std::setw(18) << commandString[(uint32)cInfo.command] << " ";
        std::cout << std::setw(7) << tCommandString[((uint32)cInfo.vcode) & ((1 << 16) - 1)] << ' ';
        for (auto &arg : cInfo.argument) std::cout << toString(arg) << " ";
        std::cout << cInfo.argumentString << std::endl;
    }
    #endif
    return succ;
}

bool VASMPackage::generateLine(const std::string &line, int lineId, bool ignoreHints) {
    size_t pos = 0, rpos;
    // skip the separate character in the front of the line
    while (pos < line.size() && isSeparator(line[pos])) pos++;
    // it is an empty line
    if (pos == line.size()) return true;
    // this line is a pretreat command
    std::vector<std::string> lst;
    for (rpos = pos; pos < line.size(); pos = ++rpos) {
        if (isSeparator(line[pos])) continue;
        std::string newPart;
        if (line[pos] == '\"') {
            newPart = getString(line, pos, rpos);
            if (rpos == line.size()) {
                printError(lineId, "Invalid string");
                return false;
            }
        } else if (isNumber(line[pos])) {
            while (rpos + 1 < line.size() && (isLetter(line[rpos + 1]) || isNumber(line[rpos + 1]) || line[rpos + 1] == '.')) rpos++;
            newPart = line.substr(pos, rpos - pos + 1);
        } else if (isLetter(line[pos]) || line[pos] == '.' || line[pos] == '@') {
            while (rpos + 1 < line.size() && (isNumber(line[rpos + 1]) || isLetter(line[rpos + 1]) || line[rpos + 1] == '.' || line[rpos + 1] == '@'))
                rpos++;
            newPart = line.substr(pos, rpos - pos + 1);
        } else if (line[pos] == '#') {
            newPart = '#';
        }
        lst.push_back(newPart);
    }

    // this line is pretreat line
    if (lst[0] == "#") {
        PretreatCommand cmd = getPretreatCommand(lst[1]);
        if (cmd == PretreatCommand::unknown) {
            printError(lineId, "Invalid pretreat command " + lst[1]);
            return false;
        }
        switch (cmd) {
            case PretreatCommand::STRING:
                strList.push_back(lst[2]);
                break;
            case PretreatCommand::GLOMEM:
                gloMem += getUnionData(lst[2]).data.uint64_v;
                break;
            case PretreatCommand::HINT:
                hints.push_back(std::make_pair(vcodeSize, lst[2]));
                break;
            case PretreatCommand::LABEL:
                if (labelOffset.count(lst[2])) {
                    printError(lineId, "Multiple definition of label " + lst[2]);
                    return false;
                }
                labelOffset[lst[2]] = vcodeSize;
                // this is a label for function entry
                if (lst[2][0] != '@') funcLabelInfo.insert(std::make_pair(lst[2], std::make_pair(funcLabelInfo.size(), vcodeSize)));
                break;
        }
    } else {
        std::vector<std::string> cmdParts;
        stringSplit(lst[0], '_', cmdParts);
        TCommand tcmd = getTCommand(cmdParts.back());
        Command cmd = getCommand(lst[0]);
        if (tcmd == TCommand::unknown) {
            printError(lineId, "Invalid command name " + cmdParts.back());
            return false;
        } else if (cmd == Command::unknown) {
            printError(lineId, "Invalid command name " + lst[0]);
            return false;
        }
        auto cInfo = CommandInfo(cmd, lineId);
        cInfo.offset = vcodeSize;
        // caluclate the modifiers and vcode
        DataTypeModifier dtMfr1 = DataTypeModifier::unknown,    dtMfr2 = DataTypeModifier::unknown;
        ValueTypeModifier vtMfr1 = ValueTypeModifier::Unknown,  vtMfr2 = ValueTypeModifier::Unknown;
        uint32 vcode = 0;
        switch (tcmd) {
            case TCommand::push:
            case TCommand::pop:
            case TCommand::cpy:
            case TCommand::add:
            case TCommand::sub:
            case TCommand::mul:
            case TCommand::div:
            case TCommand::mod:
            case TCommand::_and:
            case TCommand::_or:
            case TCommand::_xor:
            case TCommand::_not:
            case TCommand::shl:
            case TCommand::shr:
            case TCommand::eq:
            case TCommand::ne:
            case TCommand::ls:
            case TCommand::gt:
            case TCommand::le:
            case TCommand::ge:
                if (cmdParts.size() != 2) {
                    printError(lineId, "This command need one modifier");
                    return false;
                }
                dtMfr1 = getDataTypeModifier(cmdParts[0]);
                break;
            case TCommand::cvt:
                if (cmdParts.size() != 3) {
                    printError(lineId, "This command need two modifiers");
                    return false;
                }
                dtMfr1 = getDataTypeModifier(cmdParts[0]);
                dtMfr2 = getDataTypeModifier(cmdParts[1]);
                break;
            default: {
                if (cmdParts.back().size() < 3) break;
                std::string suf = cmdParts.back().substr(cmdParts.back().size() - 3);
                if (suf == "mem" || suf == "var" || suf == "glo") {
                    if (cmdParts.size() != 2) {
                        printError(lineId, "This command need one modifier");
                        return false;
                    }
                    dtMfr1 = getDataTypeModifier(cmdParts[0]);
                } else if (cmdParts.size() != 1) {
                    printError(lineId, "This command need no modifier");
                    return false;
                }
                break;
            }
        }
        cInfo.vcode = (uint32)tcmd
                 | (((uint32)dtMfr1 & 15) << 16) | (((uint32)dtMfr2 & 15) << 20)
                 | (((uint32)vtMfr1 & 3) << 24) | (((uint32)vtMfr2 & 3) << 26);

        // get the arguments
        UnionData arg1, arg2;
        std::string argStr1;
        int argCnt = 0;
        
        switch (tcmd) {
            case TCommand::jz:
            case TCommand::jp:
            case TCommand::jmp:
            case TCommand::call:
            case TCommand::plabel:
            case TCommand::newobj:
                if (lst.size() != 2) {
                    printError(lineId, "This command need one string argument");
                    return false;
                }
                cInfo.argumentString = lst[1];
                break;
            case TCommand::push:
            case TCommand::setflag:
            case TCommand::setarg:
            case TCommand::getarg:
            case TCommand::setlocal:
            case TCommand::pstr:
            case TCommand::newarr:
            case TCommand::setgtbl:
            case TCommand::getgtbl:
            case TCommand::getgtblsz:
            case TCommand::sys:
                if (lst.size() != 2) {
                    printError(lineId, "This command need one argument");
                    return false;
                }
                cInfo.argument.push_back(getUnionData(lst[1]));
                break;
            case TCommand::setclgtbl:
                arg1 = getUnionData(lst[1]);
                arg2 = getUnionData(lst[2]);
                if (!isInteger(arg1.type) || !isInteger(arg2.type)) {
                    printError(lineId, "The command of command " + tCommandString[(int)tcmd] + " must be integer.");
                    return false;
                }
                cInfo.argument.push_back(arg1);
                cInfo.argument.push_back(arg2);
                break;
            default: {
                if (cmdParts.back().size() < 3) break;
                std::string suf = cmdParts.back().substr(cmdParts.back().size() - 3);
                if (suf == "glo") {
                    if (lst.size() != 2) {
                        printError(lineId, "This command need one string arguments");
                        return false;
                    }
                    cInfo.argumentString = lst[1];
                } else if (suf == "var" || suf == "mem") {
                    if (lst.size() != 2) {
                        printError(lineId, "This command need one aruguemnt");
                        return false;
                    }
                    cInfo.argument.push_back(getUnionData(lst[1]));
                    break;
                }
                break;
            }
        }
        cmdList.push_back(cInfo);
        vcodeSize += sizeof(uint32) + sizeof(uint64) * cInfo.argument.size() + sizeof(uint64) * (!cInfo.argumentString.empty());
    }
    return true;
}

#pragma endregion

#pragma region DataTypePackage
ClassTypeData::ClassTypeData() { }
ClassTypeData::~ClassTypeData() {
    for (auto var : varList) if (var != nullptr) delete var;
    for (auto func : funcList) if (func != nullptr) delete func;
}

NamespaceTypeData::~NamespaceTypeData() {
    for (auto var : varList) if (var != nullptr) delete var;
    for (auto func : funcList) if (func != nullptr) delete func;
    for (auto cls : clsList) if (cls != nullptr) delete cls;
    for (auto nsp : nspList) if (nsp != nullptr) delete nsp;
}

DataTypePackage::DataTypePackage() { root = nullptr; }
DataTypePackage::~DataTypePackage() {
    if (root != nullptr) delete root;
}

enum class TypeDataTokenType {
    Function, Variable, Class, Namespace, TypeIdentifier, Data, Visibility, LBracketL, LBracketR, unknown,
};

struct TypeDataToken {
    TypeDataTokenType type;
    UnionData data;
    std::string dataString;
};

bool getTypeDataTokenList(std::vector<TypeDataToken> &tkList, const std::string &text) {
    bool succ = true;
    std::stack<int> pos;
    for (uint32 l = 0, r = 0; l < text.size(); l = ++r) {
        char ch = text[l];
        TypeDataToken token;
        token.type = TypeDataTokenType::unknown;
        token.data.data.uint64_v = 0;
        token.dataString = "";
        if (ch == '0') {
            while (r < text.size() - 1 && (text[r + 1] == 'x' || isNumber(text[r + 1]) || ('a' <= text[r + 1] && text[r + 1] <= 'f') || ('A' <= text[r + 1] && text[r + 1] <= 'F'))) r++;
            token.type = TypeDataTokenType::Data;
            token.dataString = text.substr(l, r - l + 1);
            token.data = getUnionData(token.dataString);
            token.data.type = DataTypeModifier::u64;
        } else if (ch == 'V') {
            r += 2;
            while (r + 1 < text.size() - 1 && (isNumber(text[r + 1]) || isLetter(text[r + 1]) || text[r + 1] == '@' || text[r + 1] == '.')) r++;
            token.type = TypeDataTokenType::Variable;
            token.dataString = text.substr(l + 2, r - l - 1);
        } else if (ch == 'F') {
            r += 2;
            while (r + 1 < text.size() - 1 && (isNumber(text[r + 1]) || isLetter(text[r + 1]) || text[r + 1] == '@' || text[r + 1] == '.')) r++;
            token.type = TypeDataTokenType::Function;
            token.dataString = text.substr(l + 2, r - l - 1);
        } else if (ch == 'C') {
            r += 2;
            while (r + 1 < text.size() - 1 && (isNumber(text[r + 1]) || isLetter(text[r + 1]) || text[r + 1] == '@' || text[r + 1] == '.')) r++;
            token.type = TypeDataTokenType::Class;
            token.dataString = text.substr(l + 2, r - l - 1);
        } else if (ch == 'N') {
            r += 2;
            while (r + 1 < text.size() - 1 && (isLetter(text[r + 1]) || isNumber(text[r + 1]) || text[r + 1] == '.')) r++;
            token.type = TypeDataTokenType::Namespace;
            token.dataString = text.substr(l + 2, r - l - 1);
        } else if (ch == 'v') {
            r += 2;
            while (r + 1 < text.size() - 1 && (isLetter(text[r + 1]))) r++;
            token.type = TypeDataTokenType::Visibility;
            token.data.uint8_v() = (uint8)getIdenVisibility(text.substr(l + 2, r - l - 1));
        } else if (ch == ':') {
            r += 2;
            while (r + 1 < text.size() - 1 && (isNumber(text[r + 1]) || isLetter(text[r + 1]) || text[r + 1] == '@' || text[r + 1] == '.' || text[r + 1] == '[' || text[r + 1] == ']')) r++;
            token.type = TypeDataTokenType::TypeIdentifier;
            token.dataString = text.substr(l + 2, r - l - 1);
        } else if (ch == '{') {
            token.type = TypeDataTokenType::LBracketL;
            pos.push(tkList.size());
        } else if (ch == '}') {
            token.type = TypeDataTokenType::LBracketR;
            tkList[pos.top()].data.uint64_v() = tkList.size();
            token.data.uint64_v() = pos.top();
            pos.pop();
        }
        if (token.type != TypeDataTokenType::unknown) tkList.emplace_back(token);
    }
    return succ;
}
bool compileTypeDataFile(VariableTypeData *var, const std::vector<TypeDataToken> &tkList, uint32 l, uint32 &r) {
    var->name = tkList[l].dataString;
    var->visibility = (IdenVisibility)tkList[l + 1].data.uint8_v();
    var->type = tkList[l + 2].dataString;
    var->offset = tkList[l + 3].data.uint64_v();
    r = l + 3;
    return true;
}
bool compileTypeDataFile(FunctionTypeData *mtd, const std::vector<TypeDataToken> &tkList, uint32 l, uint32 &r) {
    // follow the order in vobjbuilder.h
    mtd->name = tkList[l].dataString;
    mtd->labelName = tkList[l + 1].dataString;
    mtd->visibility = (IdenVisibility)tkList[l + 2].data.uint8_v();
    mtd->gtableSize = tkList[l + 3].data.uint8_v();
    mtd->resType = tkList[l + 4].dataString;
    r = tkList[l + 5].data.uint64_v();
    if (!r) return false;
    for (uint32 i = l + 6; i < r; i++) 
        mtd->argTypes.push_back(tkList[i].dataString);
    return true;
}
bool compileTypeDataFile(ClassTypeData *cls, const std::vector<TypeDataToken> &tkList, uint32 l, uint32 &r) {
    cls->name = tkList[l].dataString;
    cls->baseClsName = tkList[l + 1].dataString;
    cls->visibility = (IdenVisibility)tkList[l + 2].data.uint8_v();
    cls->size = tkList[l + 3].data.uint64_v();
    cls->gtableSize = tkList[l + 4].data.uint8_v();
    cls->gtableOffset = tkList[l + 5].data.uint64_v();
    r = tkList[l + 6].data.uint64_v();
    if (!r) return false;
    for (uint32 i = l + 7; i < r; i++) {
        uint32 j = i;
        bool res = true;
        switch (tkList[i].type) {
            case TypeDataTokenType::Variable: {
                VariableTypeData *vInfo = new VariableTypeData();
                cls->varList.push_back(vInfo);
                res = compileTypeDataFile(vInfo, tkList, i, j);
                if (!res) return false;
                break;
            }
            case TypeDataTokenType::Function: {
                FunctionTypeData *fInfo = new FunctionTypeData();
                cls->funcList.push_back(fInfo);
                res = compileTypeDataFile(fInfo, tkList, i, j);
                if (!res) return false;
                break;
            }
            default: {
                printError(0, "Invalid content in vtd file...");
                return false;
            }
        }
        i = j;
    }
    return true;
}
bool compileTypeDataFile(NamespaceTypeData *nsp, const std::vector<TypeDataToken> &tkList, uint32 l, uint32 r) {
    uint32 st;
    // the root namespace has no "name"
    if (tkList[l].type == TypeDataTokenType::Namespace)
        st = l + 2,
        nsp->name = tkList[l].dataString;
    else st = l + 1;
    for (uint32 to = st; to < r; st = ++to) {
        bool res = true;
        switch (tkList[st].type) {
            case TypeDataTokenType::Variable: {
                VariableTypeData *vInfo = new VariableTypeData();
                nsp->varList.push_back(vInfo);
                res = compileTypeDataFile(vInfo, tkList, st, to);
                if (!res) return false;
                break;
            }
            case TypeDataTokenType::Function: {
                FunctionTypeData *fInfo = new FunctionTypeData();
                nsp->funcList.push_back(fInfo);
                res = compileTypeDataFile(fInfo, tkList, st, to);
                if (!res) return false;
                break;
            }
            case TypeDataTokenType::Class: {
                ClassTypeData *cInfo = new ClassTypeData();
                nsp->clsList.push_back(cInfo);
                res = compileTypeDataFile(cInfo, tkList, st, to);
                if (!res) return false;
                break;
            }
            case TypeDataTokenType::Namespace: {
                NamespaceTypeData *nInfo = new NamespaceTypeData();
                nsp->nspList.push_back(nInfo);
                to = tkList[st + 1].data.uint64_v();
                res = compileTypeDataFile(nInfo, tkList, st, to);
                if (!res) return false;
                break;
            }
        }
    }
    return true;
}
bool compileTypeDataFile(DataTypePackage *dtPkg, const std::vector<TypeDataToken> &tkList) {
    dtPkg->root = new NamespaceTypeData;
    return compileTypeDataFile(dtPkg->root, tkList, 0lu, tkList.size() - 1);
}

#ifndef NDEBUG
void debugPrintTypeData(VariableTypeData *var, int dep) {
    std::cout << getIndent(dep) << "variable : " << var->name << std::endl;
    std::cout << getIndent(dep + 1) << "visibility : " << idenVisibilityStr[(uint32)var->visibility] << std::endl;
    std::cout << getIndent(dep + 1) << "type       : " << var->type << std::endl;
    std::cout << getIndent(dep + 1) << "offset     : 0x" << var->offset << std::endl;  
}
void debugPrintTypeData(FunctionTypeData *mtd, int dep) {
    std::cout << getIndent(dep) << "function : " << mtd->name << std::endl;
    std::cout << getIndent(dep + 1) << "visibility  : " << idenVisibilityStr[(uint32)mtd->visibility] << std::endl;
    std::cout << getIndent(dep + 1) << "result type : " << mtd->resType << std::endl;
    std::cout << getIndent(dep + 1) << "offset      : 0x" << std::setbase(16) << mtd->index << std::endl;
    for (auto &arg : mtd->argTypes) std::cout << getIndent(dep + 1) << arg << std::endl;
}
void debugPrintTypeData(ClassTypeData *cls, int dep) {
    std::cout << getIndent(dep) << "class : " << cls->name << std::endl;
    std::cout << getIndent(dep + 1) << "visibility : " << idenVisibilityStr[(uint32)cls->visibility] << std::endl;
    std::cout << getIndent(dep + 1) << "size       : 0x" << std::setbase(16) << cls->size << std::endl;
    for (auto &func : cls->funcList) debugPrintTypeData(func, dep + 1);
    for (auto &var : cls->varList) debugPrintTypeData(var, dep + 1);
}
void debugPrintTypeData(NamespaceTypeData *nsp, int dep = 0) {
    std::cout << getIndent(dep) << "namespace : " << nsp->name << std::endl;
    for (auto &nsp : nsp->nspList) debugPrintTypeData(nsp, dep + 1);
    for (auto &cls : nsp->clsList) debugPrintTypeData(cls, dep + 1);
    for (auto &func : nsp->funcList) debugPrintTypeData(func, dep + 1);
    for (auto &var : nsp->varList) debugPrintTypeData(var, dep + 1);
}
#endif
bool DataTypePackage::generate(const std::string &filePath) {
    std::vector<TypeDataToken> tkList;
    std::string text;
    std::ifstream ifs(filePath);
    if (!ifs.good()) {
        printError(0, "Cannot read from file : " + filePath);
        return false;
    }
    bool succ = true;
    while (!ifs.eof()) {
        std::string line;
        std::getline(ifs, line), text += line + '\n';
    }
    // get the token list
    if (!getTypeDataTokenList(tkList, text)) return false;
    // compile this file
    int tdtSize = 0;
    if (!compileTypeDataFile(this, tkList)) return false;

    #ifndef NDEBUG
    debugPrintTypeData(this->root);
    #endif
    return succ;
}

bool VOBJPackage::read(const std::string &path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.good()) {
        printError(0, "Can read from file " + path);
        return false;
    }
    UnionData dt(DataTypeModifier::b);
    readData(ifs, dt), type = dt.uint8_v();

    // ignore the relied libraries
    dt.type = DataTypeModifier::u64;
    readData(ifs, dt);
    std::string tmp;
    for (uint64 i = 0; i < dt.uint64_v(); i++) readString(ifs, tmp);
    readString(ifs, this->definition);
    // read type data
    auto readTD = [&]() {
        auto readVar = [&]() -> VariableTypeData * {
            VariableTypeData *var = new VariableTypeData;
            // name
            readString(ifs, var->name);
            // visibility
            dt.type = DataTypeModifier::b, readData(ifs, dt), var->visibility = (IdenVisibility)dt.uint8_v();
            // type
            readString(ifs, var->type);
            // offset
            dt.type = DataTypeModifier::u64, readData(ifs, dt), var->offset = dt.uint64_v();

            return var;
        };
        auto readMtd = [&]() -> FunctionTypeData * {
            FunctionTypeData *mtd = new FunctionTypeData;
            // name
            readString(ifs, mtd->name);
            // label name
            readString(ifs, mtd->labelName);
            // visibility
            dt.type = DataTypeModifier::b, readData(ifs, dt), mtd->visibility = (IdenVisibility)dt.uint8_v();
            // resType
            readString(ifs, mtd->resType);
            // gtable size
            dt.type = DataTypeModifier::b, readData(ifs, dt),
            mtd->gtableSize = dt.uint8_v();
            // offset
            dt.type = DataTypeModifier::u64, readData(ifs, dt), mtd->index = dt.uint64_v();
            // arg count
            readData(ifs, dt), mtd->argTypes.resize(dt.uint64_v(), "");
            // arg types
            for (auto &arg : mtd->argTypes) readString(ifs, arg);
            return mtd;
        };
        auto readCls = [&]() -> ClassTypeData * {
            ClassTypeData *cls = new ClassTypeData;
            // name
            readString(ifs, cls->name);
            // base class name
            readString(ifs, cls->baseClsName);
            // visibility
            dt.type = DataTypeModifier::b, readData(ifs, dt), cls->visibility = (IdenVisibility)dt.uint32_v();
            // size
            dt.type = DataTypeModifier::u64;
            readData(ifs, dt), cls->size = dt.uint64_v();
            // gtableSize
            dt.type = DataTypeModifier::b;
            readData(ifs, dt), cls->gtableSize = dt.uint64_v();
            // gtableOffset
            dt.type = DataTypeModifier::u64;
            readData(ifs, dt), cls->gtableOffset = dt.uint64_v();
            // members
            uint64 varCount, funcCount;
            readData(ifs, dt), varCount = dt.uint64_v();
            readData(ifs, dt), funcCount = dt.uint64_v();
            while (varCount--) cls->varList.push_back(readVar());
            while (funcCount--) cls->funcList.push_back(readMtd());
            return cls;
        };
        auto readNsp = [&](auto &&self) -> NamespaceTypeData * {
            auto *nsp = new NamespaceTypeData;
            // name
            readString(ifs, nsp->name);
            // members
            dt.type = DataTypeModifier::u64;
            uint64 varCnt = 0, funcCnt = 0, clsCnt = 0, nspCnt = 0;
            readData(ifs, dt), varCnt = dt.uint64_v();
            readData(ifs, dt), funcCnt = dt.uint64_v();
            readData(ifs, dt), clsCnt = dt.uint64_v();
            readData(ifs, dt), nspCnt = dt.uint64_v();
            while (varCnt--) nsp->varList.push_back(readVar());
            while (funcCnt--) nsp->funcList.push_back(readMtd());
            while (clsCnt--) nsp->clsList.push_back(readCls());
            while (nspCnt--) nsp->nspList.push_back(self(self));
            return nsp;
        };
        this->dataTypePkg.root = readNsp(readNsp);
    };
    readTD();
    // read vcode

    return true;
}

#pragma endregion

bool getOffset(VOBJPackage &vobjPkg, std::map<std::string, uint64> &mIndex, std::map<std::string, uint64> &cSize, std::map<std::string, uint64> &vOffset, uint32 bid) {
    uint32 curOffset = 0;
    // get the offset of classes
    auto getClassSize = [&]() -> bool {
        auto scanNsp = [&](auto &&self, NamespaceTypeData *nsp, std::string pfx) -> bool {
            bool succ = true;
            for (auto &child : nsp->nspList) succ &= self(self, child, pfx + child->name + ".");
            for (auto &cls : nsp->clsList) {
                std::string fullName = pfx + cls->name;
                if (cSize.count(fullName)) {
                    printError(0, "Multiple definition of class : " + fullName);
                    return false;
                }
                cSize.insert(std::make_pair(fullName, cls->size));
            }
            return succ;
        };
        return scanNsp(scanNsp, vobjPkg.dataTypePkg.root, std::string(""));
    };
    // get the offset of methods from vcode
    auto getMethodOffset = [&]() -> bool {
        auto scanMtd = [&](FunctionTypeData *mtd, std::string pfx) -> bool {
            const std::string &lbl = mtd->labelName;
            auto iter = vobjPkg.vasmPkg.funcLabelInfo.find(lbl);
            if (iter == vobjPkg.vasmPkg.funcLabelInfo.end()) {
                printError(0, "Can not find the function label : " + lbl);
                return false;
            }
            mtd->index = (((uint64)bid) << 48) | iter->second.first;
            mIndex.insert(std::make_pair(lbl, iter->second.first));
            return true;
        };
        auto scanCls = [&](ClassTypeData *cls, std::string pfx) -> bool {
            pfx += cls->name + ".";
            bool succ = true;
            for (FunctionTypeData *func : cls->funcList)
                succ &= scanMtd(func, pfx);
            return succ;
        };
        auto scanNsp = [&](auto &&self, NamespaceTypeData *nsp, std::string pfx) -> bool {
            bool succ = true;
            for (ClassTypeData *cls : nsp->clsList)
                succ &= scanCls(cls, pfx);
            for (FunctionTypeData *func : nsp->funcList)
                succ &= scanMtd(func, pfx);
            for (NamespaceTypeData *child : nsp->nspList)
                succ &= self(self, child, pfx + child->name + ".");
            return succ;
        };
        return scanNsp(scanNsp, vobjPkg.dataTypePkg.root, std::string(""));
    };
    // modify the offset of variables using bid
    auto getVariableOffset = [&]() -> bool {
        auto scanNsp = [&](auto &&self, NamespaceTypeData *nsp, std::string pfx) -> bool {
            bool succ = true;
            for (auto *nsp : nsp->nspList)
                succ &= self(self, nsp, pfx + nsp->name + ".");
            for (auto *var : nsp->varList) {
                auto fullName = pfx + var->name;
                if (vOffset.count(fullName)) {
                    printError(0, "Multiple definition of " + fullName);
                    return false;
                }
                var->offset |= ((uint64)bid) << 48;
                vOffset.insert(std::make_pair(fullName, var->offset));
            }
            return succ;
        };
        return scanNsp(scanNsp, vobjPkg.dataTypePkg.root, std::string(""));
    };
    return getClassSize() && getMethodOffset() && getVariableOffset();
}

bool getRelyOffset(VOBJPackage &vobjPkg, std::map<std::string, uint64> &mIndex, std::map<std::string, uint64> &cSize, std::map<std::string, uint64> &vOffset, uint32 bid) {
    uint32 curOffset = 0;
    // get the offset of classes
    auto getClassOffset = [&]() -> bool {
        auto scanNsp = [&](auto &&self, NamespaceTypeData *nsp, std::string pfx) -> bool {
            bool succ = true;
            for (auto &child : nsp->nspList) succ &= self(self, child, pfx + child->name + ".");
            for (auto &cls : nsp->clsList)
                cSize.insert(std::make_pair(pfx + cls->name, cls->size));
            return succ;
        };
        return scanNsp(scanNsp, vobjPkg.dataTypePkg.root, std::string(""));
    };
    // load the information from the structure
    auto getMethodOffset = [&]() -> bool {
        auto scanMtd = [&](FunctionTypeData *mtd, std::string pfx) -> bool {
            auto fullName = mtd->labelName;
            mIndex.insert(std::make_pair(fullName, (((uint64)bid) << 48) | mtd->index));
            return true;
        };
        auto scanCls = [&](ClassTypeData *cls, std::string pfx) -> bool {
            pfx += cls->name + ".";
            bool succ = true;
            for (FunctionTypeData *func : cls->funcList)
                succ &= scanMtd(func, pfx);
            return succ;
        };
        auto scanNsp = [&](auto &&self, NamespaceTypeData *nsp, std::string pfx) -> bool {
            bool succ = true;
            for (FunctionTypeData *func : nsp->funcList)
                succ &= scanMtd(func, pfx);
            for (ClassTypeData *cls : nsp->clsList)
                succ &= scanCls(cls, pfx);
            for (NamespaceTypeData *child : nsp->nspList)
                succ &= self(self, child, pfx + child->name + ".");
            return succ;
        };
        return scanNsp(scanNsp, vobjPkg.dataTypePkg.root, std::string(""));
    };
    // modify the offset of variables using bid
    auto getVariableOffset = [&]() -> bool {
        auto scanNsp = [&](auto &&self, NamespaceTypeData *nsp, std::string pfx) -> bool {
            bool succ = true;
            for (NamespaceTypeData *child : nsp->nspList)
                succ &= self(self, child, pfx + child->name + ".");
            for (VariableTypeData *var : nsp->varList) {
                auto fullName = pfx + var->name;
                vOffset.insert(std::make_pair(fullName, (((uint64)bid) << 48) | var->offset));
            }
            return succ;
        };
        return scanNsp(scanNsp, vobjPkg.dataTypePkg.root, std::string(""));
    };
    return getClassOffset() && getMethodOffset() && getVariableOffset();
}
bool applyOffset(VOBJPackage &vobjPkg,std::map<std::string, uint64> &mIndex, std::map<std::string, uint64> &cSize, std::map<std::string, uint64> &vOffset) {
    auto &cmdls = vobjPkg.vasmPkg.cmdList;
    bool succ = false;
    UnionData data(DataTypeModifier::u64);
    for (uint32 i = 0; i < cmdls.size(); i++) {
        auto &cmd = cmdls[i];
        auto tcmd = (TCommand)(cmd.vcode & ((1 << 16) - 1));
        switch(tcmd) {
            case TCommand::jz:
            case TCommand::jp:
            case TCommand::jmp:
                cmd.argument.push_back(UnionData(vobjPkg.vasmPkg.labelOffset[cmd.argumentString]));
                break;
            case TCommand::call: 
            case TCommand::plabel: {
                const auto &mtdName = cmd.argumentString;
                if (mIndex.count(mtdName)) data.data.uint64_v = mIndex[mtdName], cmd.argument.push_back(data);
                else {
                    printError(cmd.lineId, "Can not find method : " + mtdName);
                    succ = false;
                }
                break;
            }
            case TCommand::newobj: {
                const auto &clsName = cmd.argumentString;
                if (cSize.count(clsName)) data.data.uint64_v = cSize[clsName], cmd.argument.push_back(data);
                else {
                    printError(cmd.lineId, "Can not find class : " + clsName);
                    succ = false;
                }
                break;
            }
            default: {
                if (commandString[(int)cmd.command].size() < 3) break;
                std::string suf = commandString[(int)cmd.command].substr(commandString[(int)cmd.command].size() - 3);
                if (suf == "glo") {
                    const auto &varName = cmd.argumentString;
                    if (vOffset.count(varName)) data.data.uint64_v = vOffset[varName], cmd.argument.push_back(data);
                    else {
                        printError(cmd.lineId, "Can not find global variable : " + varName);
                        succ = false;
                    }
                }
                break;
            }
        }
    }
    return succ;
}

bool writeVObj(uint8 type, VOBJPackage &vobjPkg, const std::vector<std::string> &relyList, const std::string &target,
    const std::map<std::string, uint64> &mOffset, const std::map<std::string, uint64> &cSize, std::map<std::string, uint64> &vOffset) {
    std::ofstream ofs(target, std::ios::binary);
    UnionData data;
    data.type = DataTypeModifier::b, data.data.uint8_v = type, writeData(ofs, data);
    auto writeTypeData = [&]() {
        auto writeVar = [&](VariableTypeData *var) -> void {
            writeString(ofs, var->name);
            writeData(ofs, UnionData((uint8)var->visibility));
            writeString(ofs, var->type);
            writeData(ofs, UnionData(var->offset));
        };
        auto writeMtd = [&](FunctionTypeData *mtd) {
            writeString(ofs, mtd->name);
            writeString(ofs, mtd->labelName);
            writeData(ofs, UnionData((uint8)mtd->visibility));
            writeString(ofs, mtd->resType);
            writeData(ofs, UnionData(mtd->gtableSize));
            writeData(ofs, UnionData(mtd->index));
            writeData(ofs, UnionData((uint64)mtd->argTypes.size()));
            for (auto &arg : mtd->argTypes) writeString(ofs, arg);
        };
        auto writeCls = [&](ClassTypeData *cls) -> void {
            writeString(ofs, cls->name);
            writeString(ofs, cls->baseClsName);
            writeData(ofs, UnionData((uint8)cls->visibility));
            writeData(ofs, UnionData(cls->size));
            writeData(ofs, UnionData((uint8)cls->gtableSize));
            writeData(ofs, UnionData((uint64)cls->gtableOffset));
            writeData(ofs, UnionData((uint64)cls->varList.size()));
            writeData(ofs, UnionData((uint64)cls->funcList.size()));
            for (auto &var : cls->varList) writeVar(var);
            for (auto &func : cls->funcList) writeMtd(func);
        };
        auto writeNsp = [&](auto &&self, NamespaceTypeData *nsp) -> void {
            writeString(ofs, nsp->name);
            writeData(ofs, UnionData((uint64)nsp->varList.size()));
            writeData(ofs, UnionData((uint64)nsp->funcList.size()));
            writeData(ofs, UnionData((uint64)nsp->clsList.size()));
            writeData(ofs, UnionData((uint64)nsp->nspList.size()));
            for (auto &var : nsp->varList) writeVar(var);
            for (auto &func : nsp->funcList) writeMtd(func);
            for (auto &cls : nsp->clsList) writeCls(cls);
            for (auto &child : nsp->nspList) self(self, child);
        };
        writeNsp(writeNsp, vobjPkg.dataTypePkg.root);
    };
    writeData(ofs, UnionData((uint64)relyList.size()));
    for (auto &rely : relyList) writeString(ofs, rely);
    writeString(ofs, vobjPkg.definition);
    writeTypeData();

    // write the function entry list
    writeData(ofs, UnionData((uint64)vobjPkg.vasmPkg.funcLabelInfo.size()));
    std::vector<uint64> tmpFuncOffset(vobjPkg.vasmPkg.funcLabelInfo.size());
    for (auto &pir : vobjPkg.vasmPkg.funcLabelInfo)
        tmpFuncOffset[pir.second.first] = pir.second.second;
    for (size_t i = 0; i < tmpFuncOffset.size(); i++) writeData(ofs, UnionData(tmpFuncOffset[i]));

    // main entry offset
    if (vobjPkg.vasmPkg.funcLabelInfo.count("main"))
        writeData(ofs, UnionData(vobjPkg.vasmPkg.funcLabelInfo["main"].first));
    else writeData(ofs, UnionData((uint64)0));

    writeData(ofs, UnionData((uint64)vobjPkg.vasmPkg.strList.size()));
    for (auto &str : vobjPkg.vasmPkg.strList) writeString(ofs, str);
    writeData(ofs, UnionData(vobjPkg.vasmPkg.gloMem));
    writeData(ofs, UnionData(vobjPkg.vasmPkg.vcodeSize));
    for (auto &cmd : vobjPkg.vasmPkg.cmdList) {
        writeData(ofs, UnionData(cmd.vcode));
        for (auto &arg : cmd.argument) arg.type = DataTypeModifier::u64, writeData(ofs, arg);
        #ifndef NDEBUG
        printf("%#010x ", cmd.vcode);
        for (auto &arg : cmd.argument) printf("%#018llx ", arg.uint64_v());
        putchar('\n');
        #endif
    }
    ofs.close();
    return true;
}

/// @brief build a vobj file using the vasm file, the typedata file, the definition file, and the relied dynamic libraries
/// @param type the type of this vobj file
/// @param vasmPath the path of vasm file (.vasm)
/// @param typeDataPath the path of type data file(.tdt)
/// @param defPath the path of definition file (.vdef)
/// @param relyList the paths of relied dynamic libraries (.vobj)
/// @param target the path of the target file (.vobj)
/// @return if it is successful
bool buildVObj( uint8 type,
                const std::string &vasmPath, 
                const std::string &typeDataPath,
                const std::string &defPath, 
                const std::vector<std::string> &relyList,
                const std::string &target) {
    // load the files
    VOBJPackage vobjPkg;
    vobjPkg.type = type;
    std::vector<VOBJPackage> relyPkg(relyList.size());
    bool succ = vobjPkg.vasmPkg.generate(vasmPath) && vobjPkg.dataTypePkg.generate(typeDataPath);
    if (!succ) {
        printError(0, "fail to compile vtd file");
        return false;
    }
    std::ifstream ifs(defPath, std::ios::in);
    if (!ifs.good()) {
        printError(0, "Cannot read from file : " + defPath);
        return false;
    } else {
        vobjPkg.definition = "";
        std::string line = "";
        while (!ifs.eof())
            std::getline(ifs, line), line.append("\n"), vobjPkg.definition.append(line);
        ifs.close();
    }
    for (int i = 0; i < relyList.size(); i++) relyPkg[i].read(relyList[i]);

    // get the offset of functions shown in tdt file
    std::map<std::string, uint64> mIndex, cSize, vOffset;
    succ &= getOffset(vobjPkg, mIndex, cSize, vOffset, 0);
    uint32 bid = 0;
    for (auto &rely : relyPkg) succ &= getRelyOffset(rely, mIndex, cSize, vOffset, ++bid);
    if (!succ) return false;

    #ifndef NDEBUG
    std::cout << "global variables : " << std::endl;
    for (auto &pir : vOffset) std::cout << pir.first << " 0x" << std::setbase(16) << pir.second << std::endl;
    std::cout << "methods : " << std::endl;
    for (auto &pir : mIndex) std::cout << pir.first << " 0x" << std::setbase(16) << pir.second << std::endl;
    std::cout << "classes : " << std::endl;
    for (auto &pir : cSize) std::cout << pir.first << " 0x" << std::setbase(16) << pir.second << std::endl;
    #endif
    // change the strings in vcode into offset
    succ = applyOffset(vobjPkg, mIndex, cSize, vOffset);

    writeVObj(type, vobjPkg, relyList, target, mIndex, cSize, vOffset);
    return succ;
}
