#include "timer.h"
#include "../../includes/log.h"
#include "../../includes/hardware.h"
#include "../../includes/memory.h"
#include "../../includes/task.h"

static u64 _timerIdCnt;

void Intr_SoftIrq_Timer_initIrq(TimerIrq *irq, u64 expireJiffies, void (*func)(TimerIrq *irq, void *data), void *data) {
	irq->data = data;
	irq->expireJiffies = HW_Timer_HPET_jiffies() + expireJiffies;
	irq->func = func;
	irq->id = _timerIdCnt++;
}

void Intr_SoftIrq_Timer_addIrq(TimerIrq *irq) {
	IO_maskIntrPreffix
	RBTree_insNode(&Task_current->timerTree, &irq->rbNode);
	IO_maskIntrSuffix
}

void Intr_SoftIrq_Timer_updateState() {
	RBNode *minNode = RBTree_getMin(&Task_current->timerTree);
	if (minNode != NULL && container(minNode, TimerIrq, rbNode)->expireJiffies <= HW_Timer_HPET_jiffies())
		Intr_SoftIrq_setState(Intr_SoftIrq_State_Timer);
}

// the most simple one
void Intr_SoftIrq_Timer_mdelay(u64 msec) {
	u64 stJiffies = HW_Timer_HPET_jiffies();
	while (HW_Timer_HPET_jiffies() - stJiffies < msec) IO_hlt();
}

int Intr_SoftIrq_Timer_comparator(RBNode *a, RBNode *b) {
	TimerIrq	*irq1 = container(a, TimerIrq, rbNode),
				*irq2 = container(b, TimerIrq, rbNode);
	return (irq1->expireJiffies != irq2->expireJiffies ? (irq1->expireJiffies < irq2->expireJiffies) : (irq1->id < irq2->id));
}

void _doTimer(void *data) {
	// printk(BLACK, WHITE, "Timer (%ld)\t", HW_Timer_HPET_jiffies());
	IO_maskIntrPreffix
	RBNode *minNode = RBTree_getMin(&Task_current->timerTree);
	TimerIrq *irq;
	u64 jiffies = HW_Timer_HPET_jiffies();
	while (minNode != NULL && (irq = container(minNode, TimerIrq, rbNode))->expireJiffies <= jiffies) {
		// execute the function
		irq->func(irq, irq->data);
		RBNode *nxt = RBTree_getNext(&Task_current->timerTree, minNode);
		RBTree_delNode(&Task_current->timerTree, minNode);
		minNode = nxt;
	}
	IO_maskIntrSuffix
}

void Intr_SoftIrq_Timer_init() {
	_timerIdCnt = 0;
	Intr_SoftIrq_register(0, _doTimer, NULL);
}