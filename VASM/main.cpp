#include "vcodebuilder.h"

using namespace std;

int main() {
    getVCode(Command::B_mr_gvl);
    getVCode(Command::u32_r_mr_add);
    return 0;
}