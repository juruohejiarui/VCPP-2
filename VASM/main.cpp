#include "vobjbuilder.h"

using namespace std;

int main() {
    std::cout << getCwd() << std::endl;

    buildVObj(0, "./builtin/console.vasm", "./builtin/console.vtd", "./builtin/console.vdef", std::vector<std::string>(), "./console.vobj");
    buildVObj(0, "./builtin/math.vasm", "./builtin/math.vtd", "./builtin/math.vdef", std::vector<std::string>(), "./math.vobj");
    buildVObj(1, "./test.vasm", "./test.vtd", "./test.vdef", std::vector<std::string>({ "./console.vobj", "./math.vobj" }), "./test.vobj");
    return 0;
}