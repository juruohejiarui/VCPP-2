#include "timer.h"
#include "../../includes/log.h"
#include "../../includes/hardware.h"
#include "../../includes/memory.h"
#include "../../includes/task.h"

static u64 _timerIdCnt;

static RBTree _timerTree;
static SpinLock _lock;

void Intr_SoftIrq_Timer_initIrq(TimerIrq *irq, u64 expireJiffies, void (*func)(TimerIrq *irq, void *data), void *data) {
	irq->data = data;
	irq->expireJiffies = HW_Timer_HPET_jiffies() + expireJiffies;
	irq->func = func;
	irq->id = _timerIdCnt++;
}

void Intr_SoftIrq_Timer_addIrq(TimerIrq *irq) {
	IO_maskIntrPreffix
	SpinLock_lock(&_lock);
	// printk(BLACK, WHITE, "I");
	RBTree_insNode(&_timerTree, &irq->rbNode);
	SpinLock_unlock(&_lock);
	IO_maskIntrSuffix
}

void Intr_SoftIrq_Timer_updateState() {
	SpinLock_lock(&_lock);
	RBNode *minNode = RBTree_getMin(&_timerTree);
	if (minNode && container(minNode, TimerIrq, rbNode)->expireJiffies <= HW_Timer_HPET_jiffies())
		Intr_SoftIrq_setState(0, Intr_SoftIrq_State_Timer);
	SpinLock_unlock(&_lock);
}

// the most simple one
void Intr_SoftIrq_Timer_mdelay(u64 msec) {
	if (!(IO_getRflags() & (1 << 9))) {
		printk(WHITE, BLACK, "HPET: interrupt is masked, can not execute mdelay()\n.");
		return ;
	}
	u64 stJiffies = HW_Timer_HPET_jiffies();
	while (HW_Timer_HPET_jiffies() - stJiffies < msec) IO_hlt();
}

int Intr_SoftIrq_Timer_comparator(RBNode *a, RBNode *b) {
	TimerIrq	*irq1 = container(a, TimerIrq, rbNode),
				*irq2 = container(b, TimerIrq, rbNode);
	return (irq1->expireJiffies != irq2->expireJiffies ? (irq1->expireJiffies < irq2->expireJiffies) : (irq1->id < irq2->id));
}

void _doTimer(void *data) {
	IO_cli();
	SpinLock_lock(&_lock);
	RBNode *minNode = RBTree_getMin(&_timerTree);
	SpinLock_unlock(&_lock);
	TimerIrq *irq;
	u64 jiffies = HW_Timer_HPET_jiffies();
	while (minNode != NULL && (irq = container(minNode, TimerIrq, rbNode))->expireJiffies <= jiffies) {
		// execute the function
		SpinLock_lock(&_lock);
		RBTree_delNode(&_timerTree, minNode);
		minNode = RBTree_getMin(&_timerTree);
		SpinLock_unlock(&_lock);
		IO_sti();
		irq->func(irq, irq->data);
		IO_cli();
	}
	SpinLock_unlock(&_lock);
	IO_sti();
}

void Intr_SoftIrq_Timer_init() {
	_timerIdCnt = 0;
	RBTree_init(&_timerTree, Intr_SoftIrq_Timer_comparator);
	SpinLock_init(&_lock);
	Intr_SoftIrq_register(0, _doTimer, NULL);
}