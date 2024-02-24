#include "syntaxnode.h"
#include "compiler.h"

using namespace std;

void printHelp() {
    printf(
        "-H --help       : print this help\n"
        "-R --rely       : add a rely library\n"
        "-S --source     : add a source file\n"
        "-t --type       : the type of the vobj file\n"
        "-o --output     : the path of the vobj file\n");
}

int main(int argc, char **argv) {
    if (argc == 1) {
        // addSource("./builtin/basic.vcpp");
        // compile(1, "basic.vobj");
        // addSource("./builtin/console.vcpp");
        // addRely("./basic.vobj");
        // compile(0, "console.vobj");
        addSource("./case6/algorithm.vcpp");
        addSource("./case6/array.vcpp");
        addSource("./case6/basic.vcpp");
        addSource("./case6/console.vcpp");
        addSource("./case6/math.vcpp");
        compile(0, "basic.vobj");
        return 0;
    }
    std::vector<std::string> argList(argc - 1);
    for (int i = 1; i < argc; i++) argList[i - 1] = std::string(argv[i]);
    std::string vobjPath;
    std::vector<std::string> relyList;
    std::vector<std::string> sourceList;
    uint8 vobjType;
    if (argList.size() == 1) {
        if (argList[0] == "-H" || argList[0] == "--help") printHelp();
        else { printError(0, "Invalid argument : " + argList[0]); return 1; }
        return 0;
    }
    for (int i = 0; i < argList.size(); i++) {
        if (argList[i] == "-R" || argList[i] == "--rely") relyList.push_back(argList[++i]);
        else if (argList[i] == "-S" || argList[i] == "--source") sourceList.push_back(argList[++i]);
        else if (argList[i] == "-t" || argList[i] == "--type") {
            std::string typeStr = argList[++i];
            if (typeStr == "exec") vobjType = 0;
            else if (typeStr == "lib") vobjType = 1;
            else { printError(0, "Invalid target type : " + typeStr); return 1; }
        }
        else if (argList[i] == "-o" || argList[i] == "--output") vobjPath = argList[++i];
        else { printError(0, "Invalid argument : " + argList[i]); return 1; }
    }
    if (vobjPath == "") { printError(0, "No vobj file path"); return 1; }
    for (auto &path : relyList) addRely(path);
    for (auto &path : sourceList) addSource(path);
    bool res = compile(vobjType, vobjPath);
    return (res == true);
}