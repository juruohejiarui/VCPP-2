#include "../includes/linkage.h"

ENTRY(getCR3)
    movq %cr3, %rax
    retq