#include "vm.h"
#include <time.h>

int main(void) {
    freopen("test.in", "r", stdin);
    VM("./test.vobj");
    return 0;
}