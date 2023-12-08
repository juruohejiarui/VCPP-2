#include "vobjbuilder.h"

using namespace std;

int main() {

    VASMPackage pkg;
    pkg.generate("./test.vasm");
    return 0;
}