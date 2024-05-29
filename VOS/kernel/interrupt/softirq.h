#ifndef __INTERRUPT_SOFTIRQ_H__
#define __INTERRUPT_SOFTIRQ_H__
#include "../includes/lib.h"

#include "./softirq/timer.h"

#define Intr_SoftIrq_State_Timer (1 << 0)

typedef void (*SoftIrqHandler)(void *data);

typedef struct {
	SoftIrqHandler handler;
	void *data;
} SoftIrq;

u64 Intr_SoftIrq_getState();
void Intr_SoftIrq_setState(u64 state);

void Intr_SoftIrq_init();

void Intr_SoftIrq_register(u8 irq, SoftIrqHandler handler, void *data);
void Intr_SoftIrq_unregister(u8 irq);

#endif