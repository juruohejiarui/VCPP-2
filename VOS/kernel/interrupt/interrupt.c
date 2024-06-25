#include "gate.h"
#include "../includes/hardware.h"
#include "../includes/linkage.h"
#include "../includes/log.h"
#include "../includes/task.h"
#include "interrupt.h"
#include "softirq.h"

extern void restoreAll();

#define saveAll \
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
    "movq %rdx, %ss \n\t" \
    "movq %rdx, %ds \n\t" \
    "movq %rdx, %es \n\t" \

#define irqName2(num) num##Interrupt(void)
#define irqName(num) irqName2(irq##num)

#define buildIrq(num)   \
void irqName(num);      \
__asm__ ( \
    SYMBOL_NAME_STR(irq)#num"Interrupt: \n\t" \
    "cli       			\n\t" \
    "pushq $0   		\n\t" \
    saveAll \
    "movq %rsp, %rdi \n\t" \
    "leaq "SYMBOL_NAME_STR(irq)#num"Interrupt_end(%rip), %rax	\n\t" \
    "pushq %rax 		\n\t" \
    "movq $"#num", %rsi \n\t" \
    "leaq Intr_irqdispatch(%rip), %rax 	\n\t" \
	"jmp *%rax			\n\t" \
	SYMBOL_NAME_STR(irq)#num"Interrupt_end: \n\t" \
	"jmp Intr_retFromIntr \n\t" \
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

IntrHandlerDeclare(Intr_noHandler) {
	printk(RED, BLACK, "No handler for interrupt %d\n", arg);
	return 0;
}

IntrDescriptor Intr_descriptor[Intr_Num];

int Intr_register(u64 irqId, void *arg, IntrHandler handler, u64 param, IntrController *controller, char *irqName) {
	IntrDescriptor *desc = &Intr_descriptor[irqId - 0x20];
	desc->controller = controller;
	desc->irqName = irqName;
	desc->param = param;
	desc->handler = handler;
	if (desc->controller != NULL) {
		desc->controller->install(irqId, arg);
		desc->controller->enable(irqId);
	}
	return 0;
}

void Intr_unregister(u64 irqId) {
	IntrDescriptor *desc = &Intr_descriptor[irqId - 0x20];
	desc->controller->disable(irqId);
	desc->controller->uninstall(irqId);

	desc->controller = NULL;
	desc->irqName = NULL;
	desc->param = 0;
	desc->handler = NULL;
}

u64 Intr_irqdispatch(u64 rsp, u64 irqId) {
	IntrDescriptor *desc = &Intr_descriptor[irqId - 0x20];
	u64 res = 0;
	if (desc->handler != NULL)
		res = desc->handler(desc->param, (PtReg *)rsp);
	else res = Intr_noHandler(irqId, (PtReg *)rsp);
	if (desc->controller != NULL && desc->controller->ack != NULL) desc->controller->ack(irqId);
	return res;
}

void Intr_init() {
	for (int i = 32; i < 56; i++) Intr_Gate_setIntr(i, 0, intrList[i - 32]);
	memset(Intr_descriptor, 0, sizeof(Intr_descriptor));

	Intr_SoftIrq_init();
}

void Intr_setIstIndex(int ist) {
	IO_Func_maskIntrPreffix
	for (int i = 32; i < 56; i++) Intr_Gate_setIntr(i, ist, intrList[i - 32]);
	IO_Func_maskIntrSuffix
}