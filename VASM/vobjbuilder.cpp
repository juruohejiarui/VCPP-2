#include "vobjbuilder.h"

CommandInfo::CommandInfo(Command _command, uint32 lineId) {
    this->command = _command;
    this->lineId = lineId;
    vcode = 0;
}

#pragma region VASMPackage
VASMPackage::VASMPackage() {
    vcodeSize = mainAddr = globalMemory = 0;
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
    for (auto &pir : stringList) std::cout << pir << std::endl;  
    // std::cout << "expose labels: \n";
    // for (auto &pir : exposeMap) std::cout << pir.first << std::endl; 
    // std::cout << "rely paths: \n";
    // for (auto &pir : relyList) std::cout << pir << std::endl;   
    // std::cout << "extern labels: \n";
    // for (auto &pir : externMap) std::cout<< pir.first << " : " << pir.second << std::endl;  
    std::cout << "global memory : " << globalMemory << std::endl;
    for (auto &cInfo : commandList) {
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
    int pos = 0, rpos;
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
            // case PretreatCommand::EXPOSE:
            //     exposeMap[lst[2]] = 0; break;
            // case PretreatCommand::EXTERN:
            //     if (!externMap.count(lst[2])) 
            //         externMap.insert(std::make_pair(lst[2], externMap.size()));
            //     break;
            // case PretreatCommand::RELY:
            //     relyList.push_back(lst[2]);
            //     break;
            case PretreatCommand::GLOMEM:
                globalMemory += getUnionData(lst[2]).data.uint64_v;
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
        ValueTypeModifier vtMfr1 = ValueTypeModifier::unknown,  vtMfr2 = ValueTypeModifier::unknown;
        uint32 vcode = 0;
        switch (tcmd) {
            case TCommand::mov:
            case TCommand::addmov:
            case TCommand::submov:
            case TCommand::mulmov:
            case TCommand::divmov:
            case TCommand::andmov:
            case TCommand::ormov:
            case TCommand::xormov:
            case TCommand::shlmov:
            case TCommand::shrmov:
            case TCommand::modmov:
            case TCommand::add:
            case TCommand::sub:
            case TCommand::mul:
            case TCommand::div:
            case TCommand::_and:
            case TCommand::_or:
            case TCommand::_xor:
            case TCommand::eq:
            case TCommand::ne:
            case TCommand::gt:
            case TCommand::ge:
            case TCommand::ls:
            case TCommand::le:
                if (tcmd == TCommand::mov) {
                    printf("...");
                }
                dtMfr1 = getDataTypeModifier(cmdParts[0]);
                vtMfr1 = getValueTypeModifier(cmdParts[1]), vtMfr2 = getValueTypeModifier(cmdParts[2]);
                break;
            case TCommand::_not:
            case TCommand::pinc:
            case TCommand::pdec:
            case TCommand::sinc:
            case TCommand::sdec:
            case TCommand::pop:
            case TCommand::gvl:
            case TCommand::arrmem:
            case TCommand::mem:
            case TCommand::vmem:
                dtMfr1 = getDataTypeModifier(cmdParts[0]);
                vtMfr1 = getValueTypeModifier(cmdParts[1]);
                break;
            case TCommand::pvar:
            case TCommand::pglo:
            case TCommand::cpy:
                dtMfr1 = getDataTypeModifier(cmdParts[0]);
                break;
            case TCommand::cvt:
                dtMfr1 = getDataTypeModifier(cmdParts[0]), dtMfr2 = getDataTypeModifier(cmdParts[1]);
                vtMfr1 = getValueTypeModifier(cmdParts[2]);
                break;
            case TCommand::jz:
            case TCommand::jp:
                vtMfr1 = getValueTypeModifier(cmdParts[0]);
                break;
        }
        cInfo.vcode = (uint32)tcmd
                 | (((uint32)dtMfr1 & 15) << 16) | (((uint32)dtMfr2 & 15) << 20)
                 | (((uint32)vtMfr1 & 3) << 24) | (((uint32)vtMfr2 & 3) << 26);

        // get the arguments
        UnionData arg1;
        std::string argStr1;
        int argCnt = 0;
        
        switch (tcmd) {
            case TCommand::setlocal:
            case TCommand::getarg:
            case TCommand::setarg:
            case TCommand::arrnew:
            case TCommand::arrmem:
            case TCommand::pvar:
            case TCommand::push:
            case TCommand::mem:
            case TCommand::sys:
                if (lst.size() != 2) {
                    printError(lineId, "This argument need one data argument");
                    return false;
                }
                arg1 = getUnionData(lst[1]);
                if (!isInteger(arg1.type)) {
                    printError(lineId, "The argument of command " + tCommandString[(int)tcmd] + " must be integer.");
                    return false;
                }
                cInfo.argument.push_back(arg1);
                break;
            case TCommand::pglo:
            case TCommand::call:
            case TCommand::jz:
            case TCommand::jp:
            case TCommand::jmp:
            case TCommand::_new:
                if (lst.size() != 2) {
                    printError(lineId, "This argument need one string argument");
                    return false;
                }
                cInfo.argumentString = lst[1];
                break;
        }
        commandList.push_back(cInfo);
        vcodeSize += sizeof(uint32) + sizeof(uint64) * cInfo.argument.size() + sizeof(uint64) * (!cInfo.argumentString.empty());
    }
    return true;
}

#pragma endregion

#pragma region DataTypePackage
ClassTypeData::ClassTypeData() {
    dataTemplate = nullptr;
}
ClassTypeData::~ClassTypeData() {
    for (auto &pir : fields) if (pir.second != nullptr) delete pir.second;
    for (auto &pir : methods) if (pir.second != nullptr) delete pir.second;
    if (dataTemplate != nullptr) delete dataTemplate;
}

NamespaceTypeData::~NamespaceTypeData() {
    for (auto &pir : variables) if (pir.second != nullptr) delete pir.second;
    for (auto &pir : methods) if (pir.second != nullptr) delete pir.second;
    for (auto &pir : classes) if (pir.second != nullptr) delete pir.second;
}

DataTypePackage::DataTypePackage() { root = nullptr; }
DataTypePackage::~DataTypePackage()
{
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
            while (r + 1 < text.size() - 1 && (isLetter(text[r + 1]))) r++;
            token.type = TypeDataTokenType::Variable;
            token.dataString = text.substr(l + 2, r - l - 1);
        } else if (ch == 'F') {
            r += 2;
            while (r + 1 < text.size() - 1 && (isLetter(text[r + 1]) || text[r + 1] == '@' || text[r + 1] == '.')) r++;
            token.type = TypeDataTokenType::Function;
            token.dataString = text.substr(l + 2, r - l - 1);
        } else if (ch == 'C') {
            r += 2;
            while (r + 1 < text.size() - 1 && (isLetter(text[r + 1]))) r++;
            token.type = TypeDataTokenType::Class;
            token.dataString = text.substr(l + 2, r - l - 1);
        } else if (ch == 'N') {
            r += 2;
            while (r + 1 < text.size() - 1 && (isLetter(text[r + 1]))) r++;
            token.type = TypeDataTokenType::Namespace;
            token.dataString = text.substr(l + 2, r - l - 1);
        } else if (ch == 'v') {
            r += 2;
            while (r + 1 < text.size() - 1 && (isLetter(text[r + 1]))) r++;
            token.type = TypeDataTokenType::Visibility;
            token.data.data.uint32_v = (uint32)getIdentifierVisibility(text.substr(l + 2, r - l - 1));
        } else if (ch == ':') {
            r += 2;
            while (r + 1 < text.size() - 1 && (isLetter(text[r + 1]) || text[r + 1] == '.' || text[r + 1] == '[' || text[r + 1] == ']')) r++;
            token.type = TypeDataTokenType::TypeIdentifier;
            token.dataString = text.substr(l + 2, r - l - 1);
        } else if (ch == '{') {
            token.type = TypeDataTokenType::LBracketL;
            pos.push(tkList.size());
        } else if (ch == '}') {
            token.type = TypeDataTokenType::LBracketR;
            tkList[pos.top()].data.data.uint32_v = tkList.size();
            token.data.data.uint32_v = pos.top();
            pos.pop();
        }
        if (token.type != TypeDataTokenType::unknown) tkList.emplace_back(token);
    }
    return succ;
}
bool compileTypeDataFile(VariableTypeData *var, const std::vector<TypeDataToken> &tkList, uint32 l, uint32 &r) {
    var->visibility = (IdentifierVisibility)tkList[l + 1].data.data.uint32_v;
    var->offset = tkList[l + 2].data.data.uint64_v;
    var->type = tkList[l + 3].dataString;
    r = l + 3;
    return true;
}
bool compileTypeDataFile(MethodTypeData *mtd, const std::vector<TypeDataToken> &tkList, uint32 l, uint32 &r) {
    // get the visibility and result type
    mtd->visibility = (IdentifierVisibility)tkList[l + 1].data.data.uint32_v;
    mtd->resultType = tkList[l + 2].dataString, mtd->offset = 0;
    r = tkList[l + 3].data.data.uint32_v;
    // get the arguement list
    for (int i = l + 4; i < r; i++) mtd->argumentType.push_back(tkList[i].dataString);
    return true;
}
bool compileTypeDataFile(ClassTypeData *cls, const std::vector<TypeDataToken> &tkList, uint32 l, uint32 &r) {
    // get the size and visibility
    cls->visibility = (IdentifierVisibility)tkList[l + 1].data.data.uint32_v;
    cls->size = tkList[l + 2].data.data.uint64_v, cls->offset = 0;
    r = tkList[l + 3].data.data.uint32_v;
    uint32 fr = l + 4, to = fr;
    bool succ = true;
    while (to < r) {
        auto &tk = tkList[fr];
        switch (tk.type) {
            case TypeDataTokenType::Data: {
                cls->offsetMap.push_back(std::make_pair(tk.data.data.uint64_v, tkList[fr + 1].dataString));
                to = fr + 1;
                break;
            }
            case TypeDataTokenType::Function: {
                auto mtd = new MethodTypeData;
                mtd->name = tk.dataString;
                cls->methods.insert(std::make_pair(tk.dataString, mtd));
                succ &= compileTypeDataFile(mtd, tkList, fr, to);
                break;
            }
            case TypeDataTokenType::Variable: {
                auto var = new VariableTypeData;
                var->name = tk.dataString;
                cls->fields.insert(std::make_pair(tk.dataString, var));
                succ &= compileTypeDataFile(var, tkList, fr, to);
                break;
            }
        }
        fr = ++to;
    }
    return true;
}
bool compileTypeDataFile(NamespaceTypeData *nsp, const std::vector<TypeDataToken> &tkList, uint32 l, uint32 r) {
    uint32 fr = l + 1, to = l + 1;
    bool succ = true;
    while (to < r) {
        const auto &tk = tkList[fr];
        switch (tk.type) {
            case TypeDataTokenType::Class:
            {
                auto cls = new ClassTypeData;
                cls->name = tk.dataString;
                nsp->classes.insert(std::make_pair(tk.dataString, cls));
                succ &= compileTypeDataFile(cls, tkList, fr, to);
                break;
            }
            case TypeDataTokenType::Function:
            {
                auto mtd = new MethodTypeData;
                mtd->name = tk.dataString;
                nsp->methods.insert(std::make_pair(tk.dataString, mtd));
                succ &= compileTypeDataFile(mtd, tkList, fr, to);
                break;
            }
            case TypeDataTokenType::Variable:
            {
                auto var = new VariableTypeData;
                var->name = tk.dataString;
                nsp->variables.insert(std::make_pair(tk.dataString, var));
                succ &= compileTypeDataFile(var, tkList, fr, to);
                break;
            }
            case TypeDataTokenType::Namespace:
            {
                auto child = new NamespaceTypeData;
                child->name = tk.dataString;
                nsp->children.insert(std::make_pair(tk.dataString, child));
                to = tkList[fr + 1].data.data.uint32_v;
                succ &= compileTypeDataFile(child, tkList, fr + 1, tkList[fr + 1].data.data.uint32_v);
                break;
            }
        }
        fr = ++to;
    }
    return succ;
}
bool compileTypeDataFile(DataTypePackage *dtPkg, const std::vector<TypeDataToken> &tkList) {
    dtPkg->root = new NamespaceTypeData;
    return compileTypeDataFile(dtPkg->root, tkList, 0lu, tkList.size() - 1);
}

#ifndef NDEBUG
void debugPrintTypeData(VariableTypeData *var, int dep) {
    std::cout << getIndent(dep) << "variable : " << var->name << std::endl;
    std::cout << getIndent(dep + 1) << "visibility : " << identifierVisibilityString[(uint32)var->visibility] << std::endl;
    std::cout << getIndent(dep + 1) << "type       : " << var->type << std::endl;
    std::cout << getIndent(dep + 1) << "offset     : 0x" << var->offset << std::endl;  
}
void debugPrintTypeData(MethodTypeData *mtd, int dep) {
    std::cout << getIndent(dep) << "function : " << mtd->name << std::endl;
    std::cout << getIndent(dep + 1) << "visibility  : " << identifierVisibilityString[(uint32)mtd->visibility] << std::endl;
    std::cout << getIndent(dep + 1) << "result type : " << mtd->resultType << std::endl;
    std::cout << getIndent(dep + 1) << "offset      : 0x" << std::setbase(16) << mtd->offset << std::endl;
    for (auto &arg : mtd->argumentType) std::cout << getIndent(dep + 1) << arg << std::endl;
}
void debugPrintTypeData(ClassTypeData *cls, int dep) {
    std::cout << getIndent(dep) << "class : " << cls->name << std::endl;
    std::cout << getIndent(dep + 1) << "visibility : " << identifierVisibilityString[(uint32)cls->visibility] << std::endl;
    std::cout << getIndent(dep + 1) << "size       : 0x" << std::setbase(16) << cls->size << std::endl;
    std::cout << getIndent(dep + 1) << "offset     : 0x" << std::setbase(16) << cls->offset << std::endl;
    for (auto &pir : cls->methods) debugPrintTypeData(pir.second, dep + 1);
    for (auto &pir : cls->fields) debugPrintTypeData(pir.second, dep + 1);
}
void debugPrintTypeData(NamespaceTypeData *nsp, int dep = 0) {
    std::cout << getIndent(dep) << "namespace : " << nsp->name << std::endl;
    for (auto &pir : nsp->children) debugPrintTypeData(pir.second, dep + 1);
    for (auto &pir : nsp->methods) debugPrintTypeData(pir.second, dep + 1);
    for (auto &pir : nsp->variables) debugPrintTypeData(pir.second, dep + 1);
    for (auto &pir : nsp->classes) debugPrintTypeData(pir.second, dep + 1);
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

    // read type data
    
    auto readTD = [&]() {
        auto readVar = [&]() -> VariableTypeData * {
            VariableTypeData *var = new VariableTypeData;
            readString(ifs, var->name);
            dt.type = DataTypeModifier::u32, readData(ifs, dt), var->visibility = (IdentifierVisibility)dt.uint32_v();
            dt.type = DataTypeModifier::u64, readData(ifs, dt), var->offset = dt.uint64_v();
            readString(ifs, var->type);
            return var;
        };
        auto readMtd = [&]() -> MethodTypeData * {
            MethodTypeData *mtd = new MethodTypeData;
            readString(ifs, mtd->name);
            dt.type = DataTypeModifier::u32, readData(ifs, dt), mtd->visibility = (IdentifierVisibility)dt.uint32_v();
            dt.type = DataTypeModifier::u64, readData(ifs, dt), mtd->offset = dt.uint64_v();
            readString(ifs, mtd->resultType);
            readData(ifs, dt), mtd->argumentType.resize(dt.uint64_v(), "");
            for (auto &arg : mtd->argumentType) readString(ifs, arg);
            return mtd;
        };
        auto readCls = [&]() -> ClassTypeData * {
            ClassTypeData *cls = new ClassTypeData;
            readString(ifs, cls->name);
            dt.type = DataTypeModifier::u32, readData(ifs, dt), cls->visibility = (IdentifierVisibility)dt.uint32_v();
            dt.type = DataTypeModifier::u64;
            readData(ifs, dt), cls->size = dt.uint64_v();
            readData(ifs, dt), cls->offset = dt.uint64_v();
            cls->dataTemplate = new uint8[cls->size];
            ifs.read((char *)cls->dataTemplate, sizeof(uint8) * cls->size);
            uint64 mtdCnt = 0, fldCnt = 0;
            readData(ifs, dt), mtdCnt = dt.uint64_v();
            readData(ifs, dt), fldCnt = dt.uint64_v(); 
            while (mtdCnt--) {
                auto mtd = readMtd();
                cls->methods.insert(std::make_pair(mtd->name, mtd));
            }
            while (fldCnt--) {
                auto var = readVar();
                cls->fields.insert(std::make_pair(var->name, var));
            }
            return cls;
        };
        auto readNsp = [&](auto &&self) -> NamespaceTypeData * {
            auto *nsp = new NamespaceTypeData;
            readString(ifs, nsp->name);
            dt.type = DataTypeModifier::u64;
            uint64 nCnt = 0, cCnt = 0, mCnt = 0, vCnt = 0;
            readData(ifs, dt), nsp->dataTemplateSize = dt.uint64_v();
            readData(ifs, dt), nCnt = dt.uint64_v();
            readData(ifs, dt), cCnt = dt.uint64_v();
            readData(ifs, dt), mCnt = dt.uint64_v();
            readData(ifs, dt), vCnt = dt.uint64_v();
            while (nCnt--) {
                NamespaceTypeData *child = self(self);
                nsp->children.insert(std::make_pair(child->name, child));
            }
            while (cCnt--) {
                auto *cls = readCls();
                nsp->classes.insert(std::make_pair(cls->name, cls));
            }
            while (mCnt--) {
                auto *mtd = readMtd();
                nsp->methods.insert(std::make_pair(mtd->name, mtd));
            }
            while (vCnt--) {
                auto *var = readVar();
                nsp->variables.insert(std::make_pair(var->name, var));
            }
            return nsp;
        };
        this->dataTypePackage.root = readNsp(readNsp);
    };
    readTD();
    // read vcode

    return true;
}

#pragma endregion

bool getOffset(VOBJPackage &vobjPkg, std::map<std::string, uint64> &mOffset, std::map<std::string, uint64> &cOffset, std::map<std::string, uint64> &vOffset, uint32 bid) {
    uint32 curOffset = 0;
    // get the offset of classes
    auto getClassOffset = [&]() -> bool {
        auto scanNsp = [&](auto &&self, NamespaceTypeData *nsp, std::string pfx) -> bool {
            bool succ = true;
            nsp->dataTemplateSize = 0;
            for (auto &pir : nsp->children) {
                succ &= self(self, pir.second, pfx + pir.first + ".");
                if (succ) nsp->dataTemplateSize += pir.second->dataTemplateSize;
            }
            for (auto &pir : nsp->classes) {
                auto cls = pir.second;
                cls->offset = curOffset, curOffset += cls->size;
                std::string fullName = pfx + pir.first;
                if (cOffset.count(fullName)) {
                    printError(0, "Multiple definition of class : " + fullName);
                    return false;
                }
                cls->offset |= ((uint64)bid) << 48;
                cOffset.insert(std::make_pair(pfx + pir.first, cls->offset));
                nsp->dataTemplateSize += cls->size;
            }
            return succ;
        };
        return scanNsp(scanNsp, vobjPkg.dataTypePackage.root, std::string(""));
    };
    // get the offset of methods from vcode
    auto getMethodOffset = [&]() -> bool {
        auto scanMtd = [&](MethodTypeData *mtd, std::string pfx) -> bool {
            auto fullName = pfx + mtd->name;
            if (!vobjPkg.vasmPackage.labelOffset.count(fullName)) {
                printError(0, "Can not find the label : " + fullName);
                return false;
            }
            mtd->offset = (((uint64)bid) << 48) | vobjPkg.vasmPackage.labelOffset[fullName];
            mOffset.insert(std::make_pair(fullName, mtd->offset));
            return true;
        };
        auto scanCls = [&](ClassTypeData *cls, std::string pfx) -> bool {
            pfx += cls->name + ".";
            bool succ = true;
            for (auto &pir : cls->methods) succ &= scanMtd(pir.second, pfx);
            return succ;
        };
        auto scanNsp = [&](auto &&self, NamespaceTypeData *nsp, std::string pfx) -> bool {
            bool succ = true;
            for (auto &pir : nsp->children) succ &= self(self, pir.second, pfx + pir.first + ".");
            for (auto &pir : nsp->methods) succ &= scanMtd(pir.second, pfx);
            for (auto &pir : nsp->classes) {
                succ &= scanCls(pir.second, pfx);
                // get the date template
                auto cls = pir.second;
                cls->dataTemplate = new uint8[cls->size];
                std::sort(cls->offsetMap.begin(), cls->offsetMap.end(), [](std::pair<uint64, std::string> &a, std::pair<uint64, std::string> &b) { return a.first < b.first; });
                for (auto &pir : cls->offsetMap)
                    *((uint64*)&cls->dataTemplate[pir.first]) = cls->methods[pir.second]->offset;
            }
            return succ;
        };
        return scanNsp(scanNsp, vobjPkg.dataTypePackage.root, std::string(""));
    };
    // modify the offset of variables using bid
    auto getVariableOffset = [&]() -> bool {
        auto scanNsp = [&](auto &&self, NamespaceTypeData *nsp, std::string pfx) -> bool {
            bool succ = true;
            for (auto &pir : nsp->children) succ &= self(self, pir.second, pfx + pir.first + ".");
            for (auto &pir : nsp->variables) {
                auto fullName = pfx + pir.first;
                if (vOffset.count(fullName)) {
                    printError(0, "Multiple definition of " + fullName);
                    return false;
                }
                pir.second->offset |= ((uint64)bid) << 48;
                vOffset.insert(std::make_pair(fullName, pir.second->offset));
            }
            return succ;
        };
        return scanNsp(scanNsp, vobjPkg.dataTypePackage.root, std::string(""));
    };
    return getClassOffset() && getMethodOffset() && getVariableOffset();
}

bool getRelyOffset(VOBJPackage &vobjPkg, std::map<std::string, uint64> &mOffset, std::map<std::string, uint64> &cOffset, std::map<std::string, uint64> &vOffset, uint32 bid) {
    uint32 curOffset = 0;
    // get the offset of classes
    auto getClassOffset = [&]() -> bool {
        auto scanNsp = [&](auto &&self, NamespaceTypeData *nsp, std::string pfx) -> bool {
            bool succ = true;
            for (auto &pir : nsp->children) succ &= self(self, pir.second, pfx + pir.first + ".");
            for (auto &pir : nsp->classes) {
                std::string fullName = pfx + pir.first;
                cOffset.insert(std::make_pair(pfx + pir.first, (((uint64)bid) << 48) | pir.second->offset));
            }
            return succ;
        };
        return scanNsp(scanNsp, vobjPkg.dataTypePackage.root, std::string(""));
    };
    // get the offset of methods from vcode
    auto getMethodOffset = [&]() -> bool {
        auto scanMtd = [&](MethodTypeData *mtd, std::string pfx) -> bool {
            auto fullName = pfx + mtd->name;
            mOffset.insert(std::make_pair(fullName, (((uint64)bid) << 48) | mtd->offset));
            return true;
        };
        auto scanCls = [&](ClassTypeData *cls, std::string pfx) -> bool {
            pfx += cls->name + ".";
            bool succ = true;
            for (auto &pir : cls->methods) succ &= scanMtd(pir.second, pfx);
            return succ;
        };
        auto scanNsp = [&](auto &&self, NamespaceTypeData *nsp, std::string pfx) -> bool {
            bool succ = true;
            for (auto &pir : nsp->children) succ &= self(self, pir.second, pfx + pir.first + ".");
            for (auto &pir : nsp->methods) succ &= scanMtd(pir.second, pfx);
            for (auto &pir : nsp->classes) succ &= scanCls(pir.second, pfx);
            return succ;
        };
        return scanNsp(scanNsp, vobjPkg.dataTypePackage.root, std::string(""));
    };
    // modify the offset of variables using bid
    auto getVariableOffset = [&]() -> bool {
        auto scanNsp = [&](auto &&self, NamespaceTypeData *nsp, std::string pfx) -> bool {
            bool succ = true;
            for (auto &pir : nsp->children) succ &= self(self, pir.second, pfx + pir.first + ".");
            for (auto &pir : nsp->variables) {
                auto fullName = pfx + pir.first;
                vOffset.insert(std::make_pair(fullName, (((uint64)bid) << 48) | pir.second->offset));
            }
            return succ;
        };
        return scanNsp(scanNsp, vobjPkg.dataTypePackage.root, std::string(""));
    };
    return getClassOffset() && getMethodOffset() && getVariableOffset();
}
bool applyOffset(VOBJPackage &vobjPkg,std::map<std::string, uint64> &mOffset, std::map<std::string, uint64> &cOffset, std::map<std::string, uint64> &vOffset) {
    auto &cmdls = vobjPkg.vasmPackage.commandList;
    bool succ = false;
    UnionData data(DataTypeModifier::u64);
    for (uint32 i = 0; i < cmdls.size(); i++) {
        auto &cmd = cmdls[i];
        auto tcmd = (TCommand)(cmd.vcode & ((1 << 16) - 1));
        switch(tcmd) {
            case TCommand::jz:
            case TCommand::jp:
            case TCommand::jmp:
                cmd.argument.push_back(UnionData(vobjPkg.vasmPackage.labelOffset[cmd.argumentString]));
                break;
            case TCommand::call: {
                const auto &mtdName = cmd.argumentString;
                if (mOffset.count(mtdName)) data.data.uint64_v = mOffset[mtdName], cmd.argument.push_back(data);
                else {
                    printError(cmd.lineId, "Can not find method : " + mtdName);
                    succ = false;
                }
                break;
            }
            case TCommand::_new: {
                const auto &clsName = cmd.argumentString;
                if (cOffset.count(clsName)) data.data.uint64_v = cOffset[clsName], cmd.argument.push_back(data);
                else {
                    printError(cmd.lineId, "Can not find class : " + clsName);
                    succ = false;
                }
                break;
            }
            case TCommand::pglo: {
                const auto &varName = cmd.argumentString;
                if (vOffset.count(varName)) data.data.uint64_v = vOffset[varName], cmd.argument.push_back(data);
                else {
                    printError(cmd.lineId, "Can not find global variable : " + varName);
                    succ = false;
                }
                break;
            }
        }
    }
    return succ;
}

bool writeVObj(uint8 type, VOBJPackage &vobjPkg, const std::vector<std::string> &relyList, const std::string &target,
    const std::map<std::string, uint64> &mOffset, const std::map<std::string, uint64> &cOffset, std::map<std::string, uint64> &vOffset) {
    std::ofstream ofs(target, std::ios::binary);
    UnionData data;
    data.type = DataTypeModifier::b, data.data.uint8_v = type, writeData(ofs, data);
    auto writeTypeData = [&]() {
        auto writeVar = [&](VariableTypeData *var) -> void {
            writeString(ofs, var->name);
            writeData(ofs, UnionData((uint32)var->visibility));
            writeData(ofs, UnionData(var->offset));
            writeString(ofs, var->type);
        };
        auto writeMtd = [&](MethodTypeData *mtd) {
            writeString(ofs, mtd->name);
            writeData(ofs, UnionData((uint32)mtd->visibility));
            writeData(ofs, UnionData(mtd->offset));
            writeString(ofs, mtd->resultType);
            writeData(ofs, UnionData((uint64)mtd->argumentType.size()));
            for (auto &arg : mtd->argumentType) writeString(ofs, arg);
        };
        auto writeCls = [&](ClassTypeData *cls) -> void {
            writeString(ofs, cls->name);
            writeData(ofs, UnionData((uint32)cls->visibility));
            writeData(ofs, UnionData(cls->size));
            writeData(ofs, UnionData(cls->offset));
            ofs.write((char *)cls->dataTemplate, sizeof(uint8) * cls->size);
            writeData(ofs, UnionData((uint64)cls->methods.size()));
            writeData(ofs, UnionData((uint64)cls->fields.size()));
            for (auto &pir : cls->methods) writeMtd(pir.second);
            for (auto &pir : cls->fields) writeVar(pir.second);
        };
        auto writeNsp = [&](auto &&self, NamespaceTypeData *nsp) -> void {
            writeString(ofs, nsp->name);
            writeData(ofs, UnionData(nsp->dataTemplateSize));
            writeData(ofs, UnionData((uint64)nsp->children.size()));
            writeData(ofs, UnionData((uint64)nsp->classes.size()));
            writeData(ofs, UnionData((uint64)nsp->methods.size()));
            writeData(ofs, UnionData((uint64)nsp->variables.size()));
            for (auto &pir : nsp->children)     self(self, pir.second);
            for (auto &pir : nsp->classes)      writeCls(pir.second);
            for (auto &pir : nsp->methods)      writeMtd(pir.second);
            for (auto &pir : nsp->variables)    writeVar(pir.second);
        };
        writeNsp(writeNsp, vobjPkg.dataTypePackage.root);
    };

    writeTypeData();
    writeData(ofs, UnionData((uint64)relyList.size()));
    for (auto &rely : relyList) writeString(ofs, rely);
    writeString(ofs, vobjPkg.definition);
    if (vobjPkg.vasmPackage.labelOffset.count("main"))
        writeData(ofs, UnionData(vobjPkg.vasmPackage.labelOffset["main"]));
    else writeData(ofs, UnionData((uint64)0));
    writeData(ofs, UnionData((uint64)vobjPkg.vasmPackage.stringList.size()));
    for (auto &str : vobjPkg.vasmPackage.stringList) writeString(ofs, str);
    writeData(ofs, UnionData(vobjPkg.vasmPackage.globalMemory));
    writeData(ofs, UnionData(vobjPkg.vasmPackage.vcodeSize));
    for (auto &cmd : vobjPkg.vasmPackage.commandList) {
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
    bool succ = vobjPkg.vasmPackage.generate(vasmPath) && vobjPkg.dataTypePackage.generate(typeDataPath);
    if (!succ) return false;
    std::ifstream ifs(defPath);
    if (!ifs.good()) {
        printError(0, "Cannot read from file : " + defPath);
        return false;
    } else {
        vobjPkg.definition = "";
        std::string line = "";
        while (!ifs.eof())
            std::getline(ifs, line), line.append("\n"), vobjPkg.definition.append(line + '\n');
    }
    for (int i = 0; i < relyList.size(); i++) relyPkg[i].read(relyList[i]);

    // get the offset of functions shown in tdt file
    std::map<std::string, uint64> mOffset, cOffset, vOffset;
    succ &= getOffset(vobjPkg, mOffset, cOffset, vOffset, 0);
    uint32 bid = 0;
    for (auto &rely : relyPkg) succ &= getRelyOffset(rely, mOffset, cOffset, vOffset, ++bid);
    if (!succ) return false;

    #ifndef NDEBUG
    std::cout << "global variables : " << std::endl;
    for (auto &pir : vOffset) std::cout << pir.first << " 0x" << std::setbase(16) << pir.second << std::endl;
    std::cout << "methods : " << std::endl;
    for (auto &pir : mOffset) std::cout << pir.first << " 0x" << std::setbase(16) << pir.second << std::endl;
    std::cout << "classes : " << std::endl;
    for (auto &pir : cOffset) std::cout << pir.first << " 0x" << std::setbase(16) << pir.second << std::endl;
    #endif
    // change the strings in vcode into offset
    succ = applyOffset(vobjPkg, mOffset, cOffset, vOffset);

    writeVObj(type, vobjPkg, relyList, target, mOffset, cOffset, vOffset);
    return succ;
}
