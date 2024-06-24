#include "spinlock.h"

void Task_SpinLock_init(SpinLock *lock) {
	lock->lock = 0;
}

void Task_SpinLock_lock(SpinLock *lock) {
	i64 a = 0, c = 1;
	__asm__ volatile (
		"pushq %%rax	\n\t"
		"pushq %%rcx	\n\t"
		"1:				\n\t"
		"movq %1, %%rcx	\n\t"
		"movq %2, %%rax	\n\t"
		"lock cmpxchg %%rcx, %0	\n\t"
		"jne 1b			\n\t"
		"popq %%rcx		\n\t"
		"popq %%rax		\n\t"
		: "=m"(lock->lock)
		: "r"(c), "r"(a)
		: "%rcx", "%rax"
	);
}

void Task_SpinLock_unlock(SpinLock *lock) {
	__asm__ volatile (
		"movq $0, %0	\n\t"
		: "+r"(lock->lock)
		:
		: "memory"
	);
}
