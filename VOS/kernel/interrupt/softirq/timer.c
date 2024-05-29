#include "timer.h"
#include "../../includes/log.h"
#include "../../includes/hardware.h"
#include "../../includes/memory.h"

static RBTree _rqTree;

void Intr_SoftIrq_Timer_initIrq(TimerIrq *irq, u64 expireJiffies, void (*func)(void *data), void *data) {
	irq->data = data;
	irq->expireJiffies = HW_Timer_HPET_jiffies() + expireJiffies;
	irq->func = func;
	List_init(&irq->listEle);
}

void _testTimer(void *data) {
	printk(BLUE, WHITE, "test_timer");
}

void Intr_SoftIrq_Timer_addIrq(TimerIrq *irq) {
	RBTree_insert(&_rqTree, irq->expireJiffies, &irq->listEle);
}

void Intr_SoftIrq_Timer_updateState() {
	RBNode *minNode = RBTree_getMin(&_rqTree);
	if (minNode != NULL && minNode->val <= HW_Timer_HPET_jiffies()) Intr_SoftIrq_setState(Intr_SoftIrq_State_Timer);
}

void _doTimer(void *data) {
	printk(BLACK, WHITE, "Timer (%ld)\n", HW_Timer_HPET_jiffies());
	RBNode *minNode = RBTree_getMin(&_rqTree);
	while (minNode != NULL && minNode->val <= HW_Timer_HPET_jiffies()) {
		for (List *ele = minNode->head.next; ele != &minNode->head; ele = ele->next) {
			TimerIrq *irq = container(ele, TimerIrq, listEle);
			irq->func(irq->data);
		}
		RBTree_delNode(&_rqTree, minNode);
		minNode = RBTree_getMax(&_rqTree);
	}
}

void Intr_SoftIrq_Timer_init() {
	RBTree_init(&_rqTree);
	TimerIrq *testIrq = (TimerIrq *)kmalloc(sizeof(TimerIrq), 0);
	Intr_SoftIrq_Timer_initIrq(testIrq, 5, _testTimer, NULL);
	Intr_SoftIrq_Timer_addIrq(testIrq);
	RBTree_debug(&_rqTree);
	Intr_SoftIrq_register(0, _doTimer, NULL);
}