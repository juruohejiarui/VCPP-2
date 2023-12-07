#include "vcodebuilder.h"

using namespace std;

int main() {
    getVCode(Command::B_mr_gvl);
    getVCode(Command::u32_r_mr_add);

    VASMPackage pkg;
    pkg.generate("./test.vasm");
    return 0;
}