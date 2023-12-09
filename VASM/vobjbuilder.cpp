#include "vobjbuilder.h"

CommandInfo::CommandInfo(Command _command) {
    this->command = _command;
    vcode = 0;
}

#pragma region VASMPackage
VASMPackage::VASMPackage() {
    vcodeSize = mainAddr = globalMemory = 0;
    relyList.push_back("");
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
    std::cout << "expose labels: \n";
    for (auto &pir : exposeMap) std::cout << pir.first << std::endl; 
    std::cout << "rely paths: \n";
    for (auto &pir : relyList) std::cout << pir << std::endl;   
    std::cout << "extern labels: \n";
    for (auto &pir : externMap) std::cout<< pir.first << " : " << pir.second << std::endl;  
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
        } else if (isLetter(line[pos]) || line[pos] == '.' || line[rpos + 1] == '@') {
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
            case PretreatCommand::EXPOSE:
                exposeMap[lst[2]] = 0; break;
            case PretreatCommand::EXTERN:
                if (!externMap.count(lst[2])) 
                    externMap.insert(std::make_pair(lst[2], externMap.size()));
                break;
            case PretreatCommand::RELY:
                relyList.push_back(lst[2]);
                break;
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
        auto cInfo = CommandInfo(cmd);
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
                 | (((uint32)dtMfr1) << 16) | (((uint32)dtMfr2) << 20)
                 | (((uint32)vtMfr1) << 24) | (((uint32)vtMfr2) << 26);

        // get the arguments
        UnionData arg1;
        std::string argStr1;
        int argCnt = 0;
        
        switch (tcmd) {
            case TCommand::setlocal:
            case TCommand::getarg:
            case TCommand::arrnew:
            case TCommand::arrmem:
            case TCommand::pvar:
            case TCommand::pglo:
            case TCommand::push:
                arg1 = getUnionData(lst[1]);
                if (!isInteger(arg1.type)) {
                    printError(lineId, "The argument of command " + tCommandString[(int)tcmd] + " must be integer.\n");
                    return false;
                }
                cInfo.argument.push_back(arg1);
                break;
            case TCommand::mem:
            case TCommand::call:
            case TCommand::jz:
            case TCommand::jp:
            case TCommand::jmp:
            case TCommand::_new:
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
ClassTypeData::~ClassTypeData() {
    for (auto &pir : fields) if (pir.second != nullptr) delete pir.second;
    for (auto &pir : methods) if (pir.second != nullptr) delete pir.second;
}

NamespaceTypeData::~NamespaceTypeData() {
    for (auto &pir : variables) if (pir.second != nullptr) delete pir.second;
    for (auto &pir : methods) if (pir.second != nullptr) delete pir.second;
    for (auto &pir : classes) if (pir.second != nullptr) delete pir.second;
}

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
        if (ch == '0') {
            while (r < text.size() - 1 && (text[r + 1] == 'x' || isNumber(text[r + 1]))) r++;
            token.type = TypeDataTokenType::Data;
            token.dataString = text.substr(l, r - l + 1);
            token.data = getUnionData(token.dataString);
            token.data.type = DataTypeModifier::u64;
        } else if (ch == 'V') {
            r += 2;
            while (r + 1 < text.size() - 1 && (isLetter(text[r + 1]))) r++;
            token.type = TypeDataTokenType::Variable;
            token.dataString = text.substr(l + 2, r - l + 1);
        } else if (ch == 'F') {
            r += 2;
            while (r + 1 < text.size() - 1 && (isLetter(text[r + 1]) || text[r + 1] == '@' || text[r + 1] == '.')) r++;
            token.type = TypeDataTokenType::Function;
            token.dataString = text.substr(l + 2, r - l + 1);
        } else if (ch == 'C') {
            r += 2;
            while (r + 1 < text.size() - 1 && (isLetter(text[r + 1]))) r++;
            token.type = TypeDataTokenType::Class;
            token.dataString = text.substr(l + 2, r - l + 1);
        } else if (ch == 'N') {
            r += 2;
            while (r + 1 < text.size() - 1 && (isLetter(text[r + 1]))) r++;
            token.type = TypeDataTokenType::Namespace;
            token.dataString = text.substr(l + 2, r - l + 1);
        } else if (ch == 'v') {
            r += 2;
            while (r + 1 < text.size() - 1 && (isLetter(text[r + 1]))) r++;
            token.type = TypeDataTokenType::Visibility;
            token.data.data.uint32_v = (uint32)getIdentifierVisibility(text.substr(l + 2, r - l + 1));
        } else if (ch == ':') {
            r += 2;
            while (r + 1 < text.size() - 1 && (isLetter(text[r + 1]) || text[r + 1])) r++;
            token.type = TypeDataTokenType::TypeIdentifier;
            token.dataString = text.substr(l + 2, r - l + 1);
        } else if (ch == '{') {
            token.type = TypeDataTokenType::LBracketL;
            pos.push(tkList.size());
        } else if (ch == '}') {
            token.type = TypeDataTokenType::LBracketR;
            tkList[pos.top()].data.data.uint32_v = tkList.size();
            token.data.data.uint32_v = pos.top();
            pos.pop();
        }
        tkList.emplace_back(token);
    }
    return succ;
}
bool compileTypeDataFile(VariableTypeData *var, const std::vector<TypeDataToken> &tkList, uint32 l, uint32 &r) {
    var->visibility = (IdentifierVisibility)tkList[l + 1].data.data.uint32_v;
    var->offset = tkList[l + 2].data.data.uint64_v;
    r = l + 2;
    return true;
}
bool compileTypeDataFile(MethodTypeData *mtd, const std::vector<TypeDataToken> &tkList, uint32 l, uint32 &r) {
    // get the visibility and result type
    mtd->visibility = (IdentifierVisibility)tkList[l + 1].data.data.uint32_v;
    mtd->resultType = tkList[l + 2].dataString;
    r = tkList[l + 3].data.data.uint32_v;
    // get the arguement list
    for (int i = l + 4; i < r; i++) mtd->argumentType.push_back(tkList[i].dataString);
    return true;
}
bool compileTypeDataFile(ClassTypeData *cls, const std::vector<TypeDataToken> &tkList, uint32 l, uint32 &r) {
    // get the size and visibility
    cls->size = tkList[l + 1].data.data.uint64_v, cls->offset = 0;
    cls->visibility = (IdentifierVisibility)tkList[l + 2].data.data.uint32_v;
    r = tkList[l + 3].data.data.uint32_v;
    uint32 fr = l + 4, to = fr;
    bool succ = true;
    while (to < r) {
        const auto &tk = tkList[fr];
        switch (tk.type) {
            case TypeDataTokenType::Function:
            {
                auto mtd = new MethodTypeData;
                mtd->name = tk.dataString;
                cls->methods.insert(std::make_pair(tk.dataString, mtd));
                succ &= compileTypeDataFile(mtd, tkList, fr, to);
                break;
            }
            case TypeDataTokenType::Variable:
            {
                auto var = new VariableTypeData;
                var->name = tk.dataString;
                cls->fields.insert(std::make_pair(tk.dataString, var));
                succ &= compileTypeDataFile(var, tkList, fr + 1, to);
                break;
            }
        }
        fr = ++to;
    }
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
                succ &= compileTypeDataFile(var, tkList, fr + 1, to);
                break;
            }
            case TypeDataTokenType::Namespace:
            {
                auto nsp = new NamespaceTypeData;
                nsp->name = tk.dataString;
                nsp->children.insert(std::make_pair(tk.dataString, nsp));
                to = tkList[l + 1].data.data.uint32_v;
                succ &= compileTypeDataFile(nsp, tkList, l + 1, tkList[l + 1].data.data.uint32_v);
                break;
            }
        }
        fr = ++to;
    }
    return succ;
}
bool compileTypeDataFile(DataTypePackage *dtPkg, const std::vector<TypeDataToken> &tkList) {
    dtPkg->root = new NamespaceTypeData;
    return compileTypeDataFile(dtPkg->root, tkList, 1lu, tkList.size() - 1);
}
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
    return succ;
}

bool VOBJPackage::read(const std::string &path) {
    return false;
}

#pragma endregion

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
    VOBJPackage vobjPkg;
    bool succ = vobjPkg.vasmPackage.generate(vasmPath) && vobjPkg.dataTypePackage.generate(typeDataPath);
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
    return succ;
}
