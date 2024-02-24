#include "compiler.h"

std::set<std::string> relySet;
RootList rootList;

bool buildRoot(SyntaxNodeType rootType, const std::string &str) {
    TokenList symTkList;
    bool res = generateTokenList(str, symTkList);
    if (!res) return false;
    RootNode *rootNode = buildRootNode(rootType, symTkList);
    if (rootNode == nullptr) return false;
    rootList.push_back(rootNode);
    return true;
}

bool addRely(const std::string &path) {
    if (relySet.count(path)) return true;
    relySet.insert(path);
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.good()) {
        printError(0, "Fail to read from file " + path);
        return false;
    }
    // read : vobj type, this data should be ignored
    UnionData dt; dt.type = DataTypeModifier::b;
    readData(ifs, dt);
    // read rely list
    dt.type = DataTypeModifier::u64;
    readData(ifs, dt);
    bool res = true;
    for (size_t i = 0; i < dt.uint64_v(); i++) {
        std::string str;
        readString(ifs, str);
        res &= addRely(str);
    }
    if (!res) {
        printError(0, "fail to load the rely library of " + path);
        return false;
    }
    std::string def;
    readString(ifs, def);
    res &= buildRoot(SyntaxNodeType::SymbolRoot, def);
    if (!res) {
        printError(0, "fail to load the symbol of " + path);
        return false;
    } 
    ifs.close();
    return true;
}

bool addSource(const std::string &path) {
    TokenList symTkList;
    std::ifstream ifs(path, std::ios::in);
    if (!ifs.good()) {
        printError(0, "Fail to read from file " + path);
        return false;
    }
    std::stringstream buf;
    buf << ifs.rdbuf();
    std::string str = buf.str();
    ifs.close();
    return buildRoot(SyntaxNodeType::SourceRoot, str);
}

bool compile(uint8 type, const std::string &vobjPath) {
    auto chkRes = buildIdenSystem(rootList);
    if (!std::get<0>(chkRes)) return false;
    std::string preName = vobjPath.substr(0, vobjPath.size() - 5);
    std::string vasmPath = preName + ".vasm", vtdPath = preName + ".vtd", vdefPath = preName + ".vdef";
    std::string vobjType = (type ? "-t exec" : "-t lib");
    std::string rely = "";
    for (std::string str : relySet) rely.append(" -rely " + str);
    bool res = generateVCode(vasmPath);
    res &= generateSymbol(vtdPath, vdefPath);
    if (!res) return false;
    // write the pretreat command @GLOMEM
    std::ofstream ifs(vasmPath, std::ios::app);
    ifs << "\n#GLOMEM " << toString(std::get<1>(chkRes), 16) << "\n";
    ifs.close();
    int finRes = system(("../build/VASM/VASM -vasm " + vasmPath + " -vtd " + vtdPath + " -vdef " + vdefPath + " " + rely + " " + vobjType + " -o " + vobjPath).c_str());
    return finRes == 0;
}
