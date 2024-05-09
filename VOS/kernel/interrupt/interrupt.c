#include "gate.h"
#include "../includes/hardware.h"
#include "../includes/linkage.h"
#include "../includes/log.h"
#include "../includes/task.h"
#include "../includes/interrupt.h"

#define saveAll \
    "cld            \n\t" \
    "pushq %rax     \n\t" \
    "pushq %rax     \n\t" \
    "movq %es, %rax \n\t" \
    "pushq %rax     \n\t" \
    "movq %ds, %rax \n\t" \
    "pushq %rax     \n\t" \
    "xorq %rax, %rax\n\t" \
    "pushq %rbp     \n\t" \
    "pushq %rdi     \n\t" \
    "pushq %rsi     \n\t" \
    "pushq %rdx     \n\t" \
    "pushq %rcx     \n\t" \
    "pushq %rbx     \n\t" \
    "pushq %r8      \n\t" \
    "pushq %r9      \n\t" \
    "pushq %r10     \n\t" \
    "pushq %r11     \n\t" \
    "pushq %r12     \n\t" \
    "pushq %r13     \n\t" \
    "pushq %r14     \n\t" \
    "pushq %r15     \n\t" \
    "movq $0x10, %rdx\n\t" \
    "movq %rdx, %ds \n\t" \
    "movq %rdx, %es \n\t" \

#define irqName2(num) num##Interrupt(void)
#define irqName(num) irqName2(irq##num)

#define buildIrq(num)   \
void irqName(num);      \
__asm__ ( \
    SYMBOL_NAME_STR(irq)#num"Interrupt: \n\t" \
    "pushq $0 \n\t" \
    saveAll \
    "movq %rsp, %rdi \n\t" \
    "leaq Intr_retFromIntr(%rip), %rax \n\t" \
    "pushq %rax \n\t" \
    "movq $"#num", %rsi \n\t" \
    "jmp irqHandler \n\t" \
); \

buildIrq(0x20)
buildIrq(0x21)
buildIrq(0x22)
buildIrq(0x23)
buildIrq(0x24)
buildIrq(0x25)
buildIrq(0x26)
buildIrq(0x27)
buildIrq(0x28)
buildIrq(0x29)
buildIrq(0x2a)
buildIrq(0x2b)
buildIrq(0x2c)
buildIrq(0x2d)
buildIrq(0x2e)
buildIrq(0x2f)
buildIrq(0x30)
buildIrq(0x31)
buildIrq(0x32)
buildIrq(0x33)
buildIrq(0x34)
buildIrq(0x35)
buildIrq(0x36)
buildIrq(0x37)

void (*intrList[24])(void) = {
    irq0x20Interrupt, irq0x21Interrupt, irq0x22Interrupt, irq0x23Interrupt,
    irq0x24Interrupt, irq0x25Interrupt, irq0x26Interrupt, irq0x27Interrupt,
    irq0x28Interrupt, irq0x29Interrupt, irq0x2aInterrupt, irq0x2bInterrupt,
    irq0x2cInterrupt, irq0x2dInterrupt, irq0x2eInterrupt, irq0x2fInterrupt,
    irq0x30Interrupt, irq0x31Interrupt, irq0x32Interrupt, irq0x33Interrupt,
    irq0x34Interrupt, irq0x35Interrupt, irq0x36Interrupt, irq0x37Interrupt
};

void Intr_timer(u64 rsp) {
    if (Task_countDown()) {
        for (int i = 0; i < 30; i++) printk(BLACK, WHITE, "%#04lx(%%rsp) = %#018lx\t", i * 8, ((u64 *)rsp)[i]);
        putchar(BLACK, WHITE, '\n');
        u64 *rip = (u64 *)(rsp + 0x98);
        Task_current->thread->rip = *rip;
        *rip = (u64)Task_switchTask;
    }
}

void Intr_keyboard() {
    u8 x = IO_in8(0x60);
    printk(RED, BLACK, "\tkey: %#08x", x);
}

void irqHandler(u64 regs, u64 num) {
    // printk(RED, BLACK, "irqHandler: %d", num);
    switch (num) {
        case 0x24:
            Intr_timer(regs);
            break;
        case 0x22:
            Intr_keyboard();
            break;
    }
    __asm__ __volatile__ (
        "movq $0x00, %%rdx  \n\t"
        "movq $0x00, %%rax  \n\t"
        "movq $0x80b, %%rcx \n\t"
        "wrmsr              \n\t"
        :
        : 
        : "memory"
    );
}

void Init_interrupt() {
    for (int i = 32; i < 56; i++) Gate_setIntr(i, 2, intrList[i - 32]);
#ifdef APIC
    Init_APIC();
    // enable keyboard interrupt
    APIC_enableIntr(0x12);
#else
    Init_8259A();
#endif
}
void Intr_disableIntr() {
    __asm__ __volatile__ (
        "cli \n\t"
    );
}

void Intr_enableIntr() {
    __asm__ __volatile__ (
        "sti \n\t"
    );
}