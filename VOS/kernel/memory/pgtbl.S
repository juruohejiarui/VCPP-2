#include "../includes/linkage.h"

ENTRY(getCR3)
    movq %cr3, %rax
    retq

ENTRY(setCR3)
    movq %rdi, %rax
    movq %rax, %cr3
    retq

ENTRY(flushTLB)
    movq %cr3, %rax
    movq %rax, %cr3
    retq