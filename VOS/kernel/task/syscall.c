#include "syscall.h"
#include "mgr.h"
#include "desc.h"
#include "../includes/interrupt.h"
#include "../includes/memory.h"
#include "../includes/log.h"

void Syscall_entry();
void Syscall_exit();

u64 Syscall_noSystemCall(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
    printk(WHITE, BLACK, "no such system call, arg1 = %#018lx, arg2 = %#018lx, arg3 = %#018lx, arg4 = %#018lx, arg5 = %#018lx\n",
        arg1, arg2, arg3, arg4, arg5);
    return 1919810;
}

u64 Syscall_enableIntr(u64 intrId, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
    printk(WHITE, BLACK, "enable interrupt: %ld\n", intrId);
    APIC_enableIntr(intrId);
    return 0;
}

u64 Syscall_abort(u64 intrId, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
    printk(WHITE, BLACK, "abort: %ld\n", intrId);
    while (1) ;
    return 0;
}

typedef u64 (*Syscall)(u64, u64, u64, u64, u64);
Syscall Syscall_list[Syscall_num] = { 
    [0] = Syscall_abort, 
    [1 ... Syscall_num - 1] = Syscall_noSystemCall };

u64 Syscall_handler(u64 index, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
    // switch stack and segment registers
    Init_TSS[0].rsp0 = Task_current->thread->rsp0;
    Gate_setTSS(
        Init_TSS[0].rsp0, Init_TSS[0].rsp1, Init_TSS[0].rsp2, Init_TSS[0].ist1, Init_TSS[0].ist2,
        Init_TSS[0].ist3, Init_TSS[0].ist4, Init_TSS[0].ist5, Init_TSS[0].ist6, Init_TSS[0].ist7);
    printk(WHITE, BLACK, "try to handle syscall : %ld\n", index);
    u64 res = (Syscall_list[index])(arg1, arg2, arg3, arg4, arg5);
    // switch to user level
    return res;
}

u64 Syscall_usrAPI(u64 index, u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
    // not necessary to switch the stack
    // directly use "syscall"
    u64 res;
    __asm__ volatile (
        "movq %1, %%rdi \n\t"
        "movq %2, %%rsi \n\t"
        "movq %3, %%rdx \n\t"
        "movq %4, %%rax \n\t"
        "movq %5, %%r8 \n\t"
        "movq %6, %%r9  \n\t"
        "syscall        \n\t"
        "movq %%rax, %0 \n\t"
         : "=m"(res) 
         : "m"(index), "m"(arg1), "m"(arg2), "m"(arg3), "m"(arg4), "m"(arg5)
         : "memory");
    return res;
}


void Task_switchToUsr(u64 (*entry)(), u64 arg) {
    PtReg regs;
    memset(&regs, 0, sizeof(PtReg));
    printk(RED, BLACK, "Task_switchToUsr: entry = %#018lx, arg = %#018lx\n", entry, arg);
    regs.rsp = Task_userStackEnd;
    regs.rdi = arg;
    regs.rcx = (u64)entry;
    regs.r11 = (1 << 9);
    regs.cs = Segment_userCode;
    regs.ds = Segment_userData;
    regs.es = Segment_userData;
    regs.ss = Segment_userData;
    __asm__ volatile (
        "subq %1, %%rsp     \n\t"
        "movq %%rsp, %0     \n\t"
        : "=m"(Task_current->thread->rsp0)
        : "a"(sizeof(PtReg))
        : "memory"
    );
    Task_current->thread->rsp = Task_current->thread->rsp3 = regs.rsp;
    memcpy(&regs, (void *)(Task_current->thread->rsp0), sizeof(PtReg));
    Init_TSS[0].rsp0 = Task_current->thread->rsp0;
    Gate_setTSS(
        Init_TSS[0].rsp0, Init_TSS[0].rsp1, Init_TSS[0].rsp2, Init_TSS[0].ist1, Init_TSS[0].ist2,
        Init_TSS[0].ist3, Init_TSS[0].ist4, Init_TSS[0].ist5, Init_TSS[0].ist6, Init_TSS[0].ist7);
    printk(RED, BLACK, "finish TSS\n");
    __asm__ volatile (
        "movq %0, %%rsp     \n\t"
        "jmp Syscall_exit     \n\t"
        :
        : "m"(Task_current->thread->rsp0)
        : "memory"
    );
}

u64 Task_initUsrLevel(u64 arg) {
    printk(WHITE, BLACK, "user level function, arg: %ld\n", arg);
    u64 res = Syscall_usrAPI((arg != 1), 0x14, 2, 3, 4, 5);
    printk(WHITE, BLACK, "syscall, res: %ld\n", res);
    int t = 10000000;
    while (1) 
        (--t == 0 ? printk(WHITE, BLACK, "%d ", Task_current->pid), t = 100000000 : 0);
    
    while (1) ;
}

void Init_syscall() {
    // set IA32_EFER.SCE
    IO_writeMSR(0xC0000080, IO_readMSR(0xC0000080) | 1);
    // set IA32_STAR
    IO_writeMSR(0xC0000081, ((u64)0x28 << 48) | ((u64)0x8 << 32));
    // set IA32_LSTAR
    IO_writeMSR(0xC0000082, (u64)Syscall_entry);
    // set IA32_FMASK
    IO_writeMSR(0xC0000084, 0x00);
    // set IA32_SFMASK
    IO_writeMSR(0xC0000085, 0x00);
}
