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

void Intr_setIstIndex(int ist);

#endif