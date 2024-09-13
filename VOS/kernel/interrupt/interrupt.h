#ifndef __INTERRUPT_INTERRUPT_H__
#define __INTERRUPT_INTERRUPT_H__

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

#define irqName2(preffix, num) preffix##num##Interrupt(void)
#define irqName(preffix, num) irqName2(preffix, num)

#define Intr_buildIrq(preffix, num, dispatcher)   \
__noinline__ void irqName(preffix, num);      \
 __asm__ ( \
    ".section .text     \n\t" \
    ".global "SYMBOL_NAME_STR(preffix)#num"Interrupt    \n\t" \
    SYMBOL_NAME_STR(preffix)#num"Interrupt: \n\t" \
    "cli       			\n\t" \
    "pushq $0   		\n\t" \
    saveAll \
    "movq %rsp, %rdi        \n\t" \
    "movq $"#num", %rsi     \n\t" \
    "leaq Intr_retFromIntr(%rip), %rax 	\n\t" \
	"pushq %rax			\n\t" \
    "jmp "#dispatcher"   \n\t" \
); \

typedef struct {
	void (*enable)(u8 irqId);
	void (*disable)(u8 irqId);

	void (*install)(u8 irqId, void *arg);
	void (*uninstall)(u8 irqId);

	void (*ack)(u8 irqId);
} IntrController;

typedef u64 (*IntrHandler)(u64 arg, PtReg *regs);

#define IntrHandlerDeclare(name) u64 name(u64 arg, PtReg *regs)

typedef struct {
	IntrController *controller;
	char *irqName;
	u64 param;
	IntrHandler handler;
} IntrDescriptor;

#define Intr_Num 24

// the default handler for each interrupt
IntrHandlerDeclare(Intr_noHandler);

/// @brief Register and enable the IRQ_ID-th interrupt request.
/// @param irqId the index of interrupt
/// @param arg the argument for installer
/// @param handler the handler of this interrupt
/// @param param the parameter for handler
/// @param controller the controller of this interrupt
/// @param irqName the identifier of this interrupt
/// @return if it is successful. 0: successful other: fail
int Intr_register(u64 irqId, void *arg, IntrHandler handler, u64 param, IntrController *controller, char *irqName);
/// @brief unregister and disable the IRQ_ID-th interrput request.
/// @param irqId 
void Intr_unregister(u64 irqId);

void Intr_init();
#endif