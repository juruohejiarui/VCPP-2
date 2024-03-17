#include "usrlvl.h"
#include "../includes/lib.h"
#include "../includes/printk.h"

void userLevelFunction() {
    printk(RED, BLACK, "user level function task is running\n");

    char str[] = "Hello world\n";
    u64 ret;
    __asm__ __volatile__ (
        "leaq sysexit_return_addr(%%rip), %%rdx \n\t"
        "movq %%rsp, %%rcx \n\t"
        "sysenter \n\t"
        "sysexit_return_addr: \n\t"
        : "=a"(ret)
        : "0"(1), "D"(str)
        : "memory"
    );
    while (1);
}
