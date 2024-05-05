#include "syscall.h"
#include "desc.h"
#include "../includes/memory.h"
#include "../includes/log.h"

void Syscall_entry();
void Syscall_exit();

u64 Syscall_handler(u64 index, ...) {

}

u64 Syscall_usrAPI(u64 index, ...) {
    // not necessary to switch the stack
    // directly use "syscall"
    u64 res;
    __asm__ __volatile__ (
        "syscall        \n\t"
        "movq %%rax, %0 \n\t"
         : "=m"(res) 
         :
         : "memory");
    return res;
}


void Task_switchToUsr(u64 (*entry)(), u64 arg) {
    PtReg regs;
    memset(&regs, 0, sizeof(PtReg));
    regs.rcx = (u64)entry;
    regs.r11 = 0;
    u64 rspKernel = 0;
    __asm__ __volatile__ ( "movq %%rsp, %0" : "=m"(rspKernel) : : "memory");
    printk(RED, BLACK, "current rsp = %#018lx\n", rspKernel);
    memcpy(&regs, (void *)(rspKernel - sizeof(PtReg)), sizeof(PtReg));
    printk(RED, BLACK, "try to switch to User level rcx = %#018lx\n", regs.rcx);
    __asm__ __volatile__ (
        "subq %0, %%rsp     \n\t"
        "jmp Syscall_exit   \n\t"
         :
         : "a"(sizeof(PtReg)), "D"(arg)
         : "memory");
}

u64 Task_initUsrLevel(u64 arg) {
    printk(WHITE, BLACK, "user level function, arg: %ld\n", arg);
    while (1);
}

void Init_syscall() {
    // set IA32_EFER.SCE
    IO_writeMSR(0xc0000080, IO_readMSR(0xc0000080) | 1);
    // write STAR and LSTAR
    IO_writeMSR(0xc0000081, 0x20ul << 48);
    IO_writeMSR(0xc0000082, (u64)Syscall_entry);
}
