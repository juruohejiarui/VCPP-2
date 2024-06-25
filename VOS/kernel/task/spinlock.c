#include "spinlock.h"

void Task_SpinLock_init(SpinLock *lock) {
	lock->lock = 0;
}
