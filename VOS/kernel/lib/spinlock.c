#include "spinlock.h"

void SpinLock_init(SpinLock *lock) {
	lock->lock = 0;
}
