#include "timer.h"
#include "../../includes/log.h"
#include "../../includes/hardware.h"

void _doTimer(void *data) {
	printk(BLACK, WHITE, "Timer (%ld)\n", HW_Timer_HPET_jiffies());
}

void Intr_SoftIrq_Timer_init() {
	Intr_SoftIrq_register(0, _doTimer, NULL);
}