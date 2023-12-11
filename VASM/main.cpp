#include "vobjbuilder.h"

using namespace std;

int main() {
    buildVObj(1, "./test.vasm", "./test.vtd", "./test.vdef", std::vector<std::string>(), "./test.vobj");
    return 0;
}