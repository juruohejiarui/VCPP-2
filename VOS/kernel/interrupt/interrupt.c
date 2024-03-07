#include "../includes/interrupt.h"
#include "../includes/gate.h"
#include "../includes/lib.h"
#include "../includes/printk.h"

#define saveAll \
    "cld \n\t" \
    "pushq %rax \n\t" \
    "pushq %rax \n\t" \
    "movq %es, %rax \n\t" \
    "pushq %rax \n\t" \
    "movq %ds, %rax \n\t" \
    "pushq %rax \n\t" \
    "xorq %rax, %rax \n\t" \
    "pushq %rbp \n\t" \
    "pushq %rdi \n\t" \
    "pushq %rsi \n\t" \
    "pushq %rdx \n\t" \
    "pushq %rcx \n\t" \
    "pushq %rbx \n\t" \
    "pushq %r8 \n\t" \
    "pushq %r9 \n\t" \
    "pushq %r10 \n\t" \
    "pushq %r11 \n\t" \
    "pushq %r12 \n\t" \
    "pushq %r13 \n\t" \
    "pushq %r14 \n\t" \
    "pushq %r15 \n\t" \
    "movq $0x10, %rdx \n\t" \
    "movq %rdx, %ds \n\t" \
    "movq %rdx, %es \n\t"

#define IRQ_NAME2(nr) nr##Interrupt(void)
#define IRQ_NAME(nr) IRQ_NAME2(IRQ##nr)

#define buildIRQ(nr) \
void IRQ_NAME(nr); \
__asm__ ( \
    SYMBOL_NAME_STR(IRQ)#nr"Interrupt: \n\t " \
        "pushq $0x00 \n\t" \
        saveAll \
        "movq %rsp, %rdi \n\t" \
        "leaq ret_from_intr(%rip), %rax \n\t" \
        "pushq %rax \n\t" \
        "movq $"#nr", %rsi \n\t" \
        "jmp doIRQ \n\t" \
);

buildIRQ(0x20)
buildIRQ(0x21)
buildIRQ(0x22)
buildIRQ(0x23)
buildIRQ(0x24)
buildIRQ(0x25)
buildIRQ(0x26)
buildIRQ(0x27)
buildIRQ(0x28)
buildIRQ(0x29)
buildIRQ(0x2a)
buildIRQ(0x2b)
buildIRQ(0x2c)
buildIRQ(0x2d)
buildIRQ(0x2e)
buildIRQ(0x2f)
buildIRQ(0x30)
buildIRQ(0x31)
buildIRQ(0x32)
buildIRQ(0x33)
buildIRQ(0x34)
buildIRQ(0x35)
buildIRQ(0x36)
buildIRQ(0x37)

void (*interrupt[24])(void) = {
    IRQ0x20Interrupt, IRQ0x21Interrupt, IRQ0x22Interrupt, IRQ0x23Interrupt,
    IRQ0x24Interrupt, IRQ0x25Interrupt, IRQ0x26Interrupt, IRQ0x27Interrupt,
    IRQ0x28Interrupt, IRQ0x29Interrupt, IRQ0x2aInterrupt, IRQ0x2bInterrupt,
    IRQ0x2cInterrupt, IRQ0x2dInterrupt, IRQ0x2eInterrupt, IRQ0x2fInterrupt,
    IRQ0x30Interrupt, IRQ0x31Interrupt, IRQ0x32Interrupt, IRQ0x33Interrupt,
    IRQ0x34Interrupt, IRQ0x35Interrupt, IRQ0x36Interrupt, IRQ0x37Interrupt
};

void initInterrupt() {
    for (int i = 32; i < 56; i++) setIntrGate(i, 2, interrupt[i - 32]);
    
    printk(RED, BLACK, "8259A init\n");
    
    // init 8259A-master
    IO_out8(0x20, 0x11);
    IO_out8(0x21, 0x20);
    IO_out8(0x21, 0x04);
    IO_out8(0x21, 0x01);
    // init 8259A-slave
    IO_out8(0xa0, 0x11);
    IO_out8(0xa1, 0x28);
    IO_out8(0xa1, 0x02);
    IO_out8(0xa1, 0x01);
    // init 8259A-M/S OCW1
    IO_out8(0x21, 0xfd);
    IO_out8(0xa0, 0xff);

    sti();
}

void doIRQ(u64 regs, u64 nr) {
    u8 x;
    printk(RED, BLACK, "doIRQ: %#018lx\t", nr);
    x = IO_in8(0x60);
    printk(RED, BLACK, "key code: %#018lx\n", x);
    IO_out8(0x20, 0x20);
}