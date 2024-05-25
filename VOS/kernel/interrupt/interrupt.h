#ifndef __INTERRUPT_INTERRUPT_H__
#define __INTERRUPT_INTERRUPT_H__

typedef struct {
	void (*enable)(u8 irqId);
	void (*disable)(u8 irqId);

	void (*install)(u8 irqId, void *arg);
	void (*uninstall)(u8 irqId);

	void (*ack)(u8 irqId);
} IntrController;

typedef u64 (*IntrHandler)(u64 arg, PtReg *regs);

#define IntrHandlerDeclare(name) u64 name(u64 num, PtReg *regs)

typedef struct {
	IntrController *controller;
	char *irqName;
	u64 param;
	
	IntrHandler handler;
} IntrDescriptor;

#define Intr_Num 24

IntrHandlerDeclare(Intr_noHandler);
IntrHandlerDeclare(Intr_keyboard);
IntrHandlerDeclare(Intr_timer);

int Intr_register(u64 irqId, void *arg, IntrHandler handler, u64 param, IntrController *controller, char *irqName);
void Intr_unregister(u64 irqId);

void Intr_init();

#endif