#include "syntaxnode.h"
#include "idensys.h"

using namespace std;

int main() {
    std::ifstream ifs("basic.vcpp", std::ios::in);
    TokenList tkList;
    std::string str = "";
    while (!ifs.eof()) {
        std::string line;
        std::getline(ifs, line);
        str.append(line + '\n');
    }
    bool succ = generateTokenList(str, tkList);
    // for (size_t i = 0; i < tkList.size(); i++) {
        // printf("%9d ", i);
        // std::cout << tkList[i].toString() << std::endl;
    // }
    if (!succ) {
        printError(0, "Unsucc...");
        return 0;
    }
    RootNode *node = buildRootNode(SyntaxNodeType::SourceRoot, tkList);
    RootList roots;
    roots.push_back(node);
    freopen("test.out", "w", stdout);
    if (node != nullptr) succ &= buildIdenSystem(roots);
    debugPrintNspStruct(rootNsp);
    return 0;
}