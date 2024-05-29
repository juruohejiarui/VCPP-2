#ifndef __INTERRUPT_SOFTIRQ_TIMER_H__
#define __INTERRUPT_SOFTIRQ_TIMER_H__

#include "../softirq.h"

typedef struct {
	List listEle;
	u64 expireJiffies;
	void (*func)(void *data);
	void *data;
} TimerIrq;

void Intr_SoftIrq_Timer_init();

void Intr_SoftIrq_Timer_updateState();

#endif