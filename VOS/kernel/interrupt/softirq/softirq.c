#include "../softirq.h"
#include "timer.h"
#include "../../includes/log.h"
#include "../../includes/task.h"
#include "../../includes/smp.h"

u64 Intr_SoftIrq_state[Hardware_CPUNumber];
SoftIrq softIrqs[64] = {};

u64 Intr_SoftIrq_getState() { return Intr_SoftIrq_state[SMP_getCurCPUIndex()]; }
void Intr_SoftIrq_setState(int cpuId, u64 state) { Intr_SoftIrq_state[cpuId] |= state; }

void Intr_SoftIrq_init() {
	memset(Intr_SoftIrq_state, 0, sizeof(Intr_SoftIrq_state));
	memset(softIrqs, 0, sizeof(softIrqs));

	Intr_SoftIrq_Timer_init();
}

void Intr_SoftIrq_register(u8 irq, SoftIrqHandler handler, void *data) {
	softIrqs[irq].handler = handler;
	softIrqs[irq].data = data;
}

void Intr_SoftIrq_unregister(u8 irq) {
	softIrqs[irq].handler = NULL;
	softIrqs[irq].data = NULL;
}

void Intr_SoftIrq_dispatch() {
	IO_sti();
	u64 *state = &Intr_SoftIrq_state[Task_current->cpuId];
	for (int i = 0; i < 64; i++)
		if ((*state & (1 << i)) && softIrqs[i].handler != NULL)
			softIrqs[i].handler(softIrqs[i].data),
			*state &= ~(1ul << i);
	IO_cli();
}