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

/// @brief get the enablers state of soft interrupt
/// @return the state
u64 Intr_SoftIrq_getState();
/// @brief enable some states of enablers
/// @param state 
void Intr_SoftIrq_setState(u64 state);

void Intr_SoftIrq_init();

/// @brief register the IRQ-th soft interrupt
/// @param irq the index of soft interrupt
/// @param handler the handler function pointer
/// @param data the parameter for the handler
void Intr_SoftIrq_register(u8 irq, SoftIrqHandler handler, void *data);
/// @brief unregister the IRQ-th soft interrupt
/// @param irq the index of soft interrupt
void Intr_SoftIrq_unregister(u8 irq);

#endif