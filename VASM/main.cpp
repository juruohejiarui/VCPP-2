#include "vobjbuilder.h"

using namespace std;

int main() {
    buildVObj(1, "./test.vasm", "./test.tdt", "./test.vdef", std::vector<std::string>(), "./test.vobj");
    return 0;
}