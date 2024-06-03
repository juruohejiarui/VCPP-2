#include "timer.h"
#include "../../includes/log.h"
#include "../../includes/hardware.h"
#include "../../includes/memory.h"
#include "../../includes/task.h"

void Intr_SoftIrq_Timer_initIrq(TimerIrq *irq, u64 expireJiffies, void (*func)(void *data), void *data) {
	irq->data = data;
	irq->expireJiffies = HW_Timer_HPET_jiffies() + expireJiffies;
	irq->func = func;
	List_init(&irq->listEle);
}

void Intr_SoftIrq_Timer_addIrq(TimerIrq *irq) {
	RBTree_insert(&Task_current->softIrqTree, irq->expireJiffies, &irq->listEle);
}

void Intr_SoftIrq_Timer_updateState() {
	RBNode *minNode = RBTree_getMin(&Task_current->softIrqTree);
	if (minNode != NULL && minNode->val <= HW_Timer_HPET_jiffies()) Intr_SoftIrq_setState(Intr_SoftIrq_State_Timer);
}

void _doTimer(void *data) {
	// printk(BLACK, WHITE, "Timer (%ld)\t", HW_Timer_HPET_jiffies());
	RBNode *minNode = RBTree_getMin(&Task_current->softIrqTree);
	int prevFlag = (IO_getRflags() >> 9) & 1;
	while (minNode != NULL && minNode->val <= HW_Timer_HPET_jiffies()) {
		List *ele = minNode->head.next, *end = ele;
		for (; end->next != &minNode->head; end = end->next) ;
		RBTree_delNode(&Task_current->softIrqTree, minNode);
		minNode = RBTree_getMin(&Task_current->softIrqTree);
		for (; ; ele = ele->next) {
			TimerIrq *irq = container(ele, TimerIrq, listEle);
			irq->func(irq->data);
			if (ele == end) break;
		}
	}
}

void Intr_SoftIrq_Timer_init() {
	Intr_SoftIrq_register(0, _doTimer, NULL);
}