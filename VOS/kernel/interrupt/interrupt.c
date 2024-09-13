#include "gate.h"
#include "../includes/hardware.h"
#include "../includes/linkage.h"
#include "../includes/log.h"
#include "../includes/task.h"
#include "../includes/smp.h"
#include "interrupt.h"
#include "softirq.h"

extern void restoreAll();

#define buildIrq(x) Intr_buildIrq(irq, x, Intr_irqdispatch)

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

buildIrq(0xc8)
buildIrq(0xc9)
buildIrq(0xca)
buildIrq(0xcb)
buildIrq(0xcc)
buildIrq(0xcd)
buildIrq(0xce)
buildIrq(0xcf)
buildIrq(0xd0)
buildIrq(0xd1)

void (*smpIntrList[10])(void) = {
    irq0xc8Interrupt, irq0xc9Interrupt,
    irq0xcaInterrupt, irq0xcbInterrupt,
    irq0xccInterrupt, irq0xcdInterrupt,
    irq0xceInterrupt, irq0xcfInterrupt,
    irq0xd0Interrupt, irq0xd1Interrupt
};

IntrHandlerDeclare(Intr_noHandler) {
	printk(RED, BLACK, "No handler for interrupt %d processor:%d\n", arg, SMP_getCurCPUIndex());
	return 0;
}

IntrDescriptor Intr_descriptor[Intr_Num];

IntrDescriptor Intr_smpDescriptor[10];


int Intr_register(u64 irqId, void *arg, IntrHandler handler, u64 param, IntrController *controller, char *irqName) {
	IntrDescriptor *desc = (irqId & 0x80 ? &Intr_smpDescriptor[irqId - 0xc8] : &Intr_descriptor[irqId - 0x20]);
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
	IntrDescriptor *desc = (irqId & 0x80 ? &Intr_smpDescriptor[irqId - 0xc8] : &Intr_descriptor[irqId - 0x20]);
	desc->controller->disable(irqId);
	desc->controller->uninstall(irqId);

	desc->controller = NULL;
	desc->irqName = NULL;
	desc->param = 0;
	desc->handler = NULL;
}

__noinline__ u64 Intr_irqdispatch(u64 rsp, u64 irqId) {
    u64 res = 0;
    switch (irqId & 0x80) {
        case 0x00 : {
            IntrDescriptor *desc = &Intr_descriptor[irqId - 0x20];
            if (desc->handler != NULL)
                res = desc->handler(desc->param, (PtReg *)rsp);
            else res = Intr_noHandler(irqId, (PtReg *)rsp);
        	if (desc->controller != NULL && desc->controller->ack != NULL) desc->controller->ack(irqId);
            break;
        }
        case 0x80 : {
            IntrDescriptor *desc = &Intr_smpDescriptor[irqId - 0xc8];
            if (desc->handler != NULL) res = desc->handler(desc->param, (PtReg *)rsp);
            else res = Intr_noHandler(irqId, (PtReg *)rsp);
            HW_APIC_edgeAck(irqId);
            break;
        }
    }
	
	return res;
}

void Intr_init() {
	for (int i = 32; i < 56; i++) Intr_Gate_setIntr(i, 0, intrList[i - 32]);
    for (int i = 200; i < 210; i++) Intr_Gate_setIntr(i, 0, smpIntrList[i - 200]);
	memset(Intr_descriptor, 0, sizeof(Intr_descriptor));
    memset(Intr_smpDescriptor, 0, sizeof(Intr_smpDescriptor));
	Intr_SoftIrq_init();
}