#include "vobjbuilder.h"

using namespace std;

void printHelp() {
    printf( "-H --help          : print help message. \n"
            "-vasm <path>       : set vasm path\n"
            "-vtd <path>        : set vtd file path\n"
            "-vdef <path>       : set vdef file path\n"
            "-rely <path>       : add a rely vobj file\n"
            "-t exec/lib        : set the type of the target\n"
            "-o <path>          : set the target path\n");
}

int main(int argc, char **argv) {
    if (argc == 1) {
        buildVObj(0, "./basic.vasm", "./basic.vtd", "./basic.vdef", {}, "./basic.vobj");
        // buildVObj(0, "./builtin/console.vasm", "./builtin/console.vtd", "./builtin/console.vdef", { "./basic.vobj" }, "./console.vobj");
        // buildVObj(1, "./test.vasm", "./test.vtd", "./test.vdef", { "./basic.vobj", "./console.vobj" }, "./test.vobj");
        return 0;
    }
    std::vector<std::string> argList(argc - 1);
    for (int i = 1; i < argc; i++) argList[i - 1] = std::string(argv[i]);
    std::string vasmPath, vtdPath, vdefPath, vobjPath;
    std::vector<std::string> relyList;
    uint8 vobjType;
    if (argList.size() == 1) {
        if (argList[0] == "-H" || argList[0] == "--help") printHelp();
        else printError(0, "Invalid argument : " + argList[0]);
        return 0;
    }
    for (int i = 0; i < argList.size(); i++) {
        if (argList[i] == "-vasm") vasmPath = argList[++i];
        else if (argList[i] == "-vtd") vtdPath = argList[++i];
        else if (argList[i] == "-vdef") vdefPath = argList[++i];
        else if (argList[i] == "-rely") relyList.push_back(argList[++i]);
        else if (argList[i] == "-t") {
            std::string typeStr = argList[++i];
            if (typeStr == "exec") vobjType = 0;
            else if (typeStr == "lib") vobjType = 1;
            else printError(0, "Invalid target type : " + typeStr);
        }
        else if (argList[i] == "-o") vobjPath = argList[++i];
        else printError(0, "Invalid argument : " + argList[i]);
    }
    if (vasmPath == "") printError(0, "No vasm file path");
    if (vtdPath == "") printError(0, "No vtd file path");
    if (vdefPath == "") printError(0, "No vdef file path");
    if (vobjPath == "") printError(0, "No vobj file path");
    bool res = buildVObj(vobjType, vasmPath, vtdPath, vdefPath, relyList, vobjPath);

    // buildVObj(0, "./builtin/basic.vasm", "./builtin/basic.vtd", "./builtin/basic.vdef", std::vector<std::string>(), "./basic.vobj");
    // buildVObj(0, "./builtin/math.vasm", "./builtin/math.vtd", "./builtin/math.vdef", std::vector<std::string>(), "./math.vobj");
    // buildVObj(1, "./test.vasm", "./test.vtd", "./test.vdef", std::vector<std::string>({ "./console.vobj", "./math.vobj" }), "./test.vobj");
    return (res == true);
}