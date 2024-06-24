#include "spinlock.h"

void Task_SpinLock_init(SpinLock *lock) {
	lock->lock = 1;
}

void Task_SpinLock_lock(SpinLock *lock) {
	__asm__ volatile (
		"1:				\n\t"
		"lock decq %0	\n\t"
		"jns 3f			\n\t"
		"2:				\n\t"
		"pause			\n\t"
		"cmpq $0, %0	\n\t"
		"jle 2b			\n\t"
		"jmp 1b			\n\t"
		"3:				\n\t"
		: "=m"(lock->lock)
		:
		: "memory"
	);
}

void Task_SpinLock_unlock(SpinLock *lock) {
	__asm__ volatile (
		"movq $1, %0	\n\t"
		: "=m"(lock->lock)
		:
		: "memory"
	);
}
