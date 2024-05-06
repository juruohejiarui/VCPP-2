#include "syscall.h"
#include "desc.h"
#include "../includes/memory.h"
#include "../includes/log.h"

void Syscall_entry();
void Syscall_exit();

u64 Syscall_handler(u64 index, ...) {
    printk(WHITE, BLACK, "try to handle syscall : %ld\n", index);
    return 1919810;
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


void Task_switchToUsr(u64 (*entry)(), u64 rspUser, u64 arg) {
    PtReg regs;
    memset(&regs, 0, sizeof(PtReg));
    printk(RED, BLACK, "Task_switchToUsr: entry = %#018lx, arg = %#018lx\n", entry, arg);
    regs.rcx = (u64)entry;
    regs.rdi = arg;
    regs.r11 = (1 << 9); // enable interrupt
    u64 rspKernel = 0, res = 0;
    __asm__ __volatile__ ( 
        "movq %%rsp, %0     \n\t"
        "movq %%rsp, %1     \n\t"
        "subq %%rax, %%rsp  \n\t"
        : "=m"(rspKernel), "=m"(regs.r12)
        : "a"(sizeof(PtReg))
        : "memory");
    memcpy(&regs, (void *)(rspKernel - sizeof(PtReg)), sizeof(PtReg));
    printk(WHITE, BLACK, "try to switch to user level\n");
    __asm__ __volatile__ (
        "jmp Syscall_exit   \n\t"
        "movq %%rax, %0     \n\t"
        : "=m"(res)
        :
        : "memory");
}

u64 Task_initUsrLevel(u64 arg) {
    printk(WHITE, BLACK, "user level function, arg: %ld\n", arg);
    u64 res;
    __asm__ __volatile__ (
        "movq $10, %%rdi    \n\t"
        "syscall           \n\t"
        "movq %%rax, %0     \n\t"
        : "=m"(res)
        :
        : "memory"
    );
    printk(WHITE, BLACK, "syscall, res: %ld\n", res);
    while (1);
}

void Init_syscall() {
    // set IA32_EFER.SCE
    IO_writeMSR(0xC0000080, IO_readMSR(0xC0000080) | 1);
    // set IA32_STAR
    IO_writeMSR(0xC0000081, ((u64)0x20 << 48) | ((u64)0x0 << 32));
    // set IA32_LSTAR
    IO_writeMSR(0xC0000082, (u64)Syscall_entry);
    // set IA32_FMASK
    IO_writeMSR(0xC0000084, 0x00);
    // set IA32_SFMASK
    IO_writeMSR(0xC0000085, 0x00);
}
