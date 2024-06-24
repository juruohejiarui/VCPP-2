#ifndef __TASK_SPINLOCK_H__
#define __TASK_SPINLOCK_H__

#include "../includes/lib.h"
typedef struct {
	volatile i64 lock;
} SpinLock;

void Task_SpinLock_init(SpinLock *lock);
void Task_SpinLock_lock(SpinLock *lock);
void Task_SpinLock_unlock(SpinLock *lock);
#endif