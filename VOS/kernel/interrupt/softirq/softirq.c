#include "../softirq.h"
#include "timer.h"
#include "../../includes/log.h"
#include "../../includes/task.h"

u64 Intr_SoftIrq_state = 0;
SoftIrq softIrqs[64] = {};

u64 Intr_SoftIrq_getState() { return Intr_SoftIrq_state; }
void Intr_SoftIrq_setState(u64 state) { Intr_SoftIrq_state |= state; }

void Intr_SoftIrq_init() {
	Intr_SoftIrq_state = 0;
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
	u64 state = Intr_SoftIrq_state;
	Intr_SoftIrq_state = 0;
	IO_sti();
	for (int i = 0; i < 64; i++)
		if ((state & (1 << i)) && softIrqs[i].handler != NULL) {
			if ((u64)softIrqs[i].handler > 0xffff800000000000 + 0x3000000 || (u64)softIrqs[i].handler < 0xffff800000000000)
				printk(WHITE, BLACK, "Invalid handler pointer %#018lx(%d)\n", softIrqs[i].handler, i);
			softIrqs[i].handler(softIrqs[i].data);
		}
	IO_cli();
}