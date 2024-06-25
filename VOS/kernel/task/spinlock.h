#ifndef __TASK_SPINLOCK_H__
#define __TASK_SPINLOCK_H__

#include "../includes/lib.h"
typedef struct {
	volatile i64 lock;
} SpinLock;

void Task_SpinLock_init(SpinLock *lock);

#define Task_SpinLock_lock(locker) { \
	i64 a = 0, c = 1; \
	__asm__ volatile ( \
		"pushq %%rax	\n\t" \
		"pushq %%rcx	\n\t" \
		"1:				\n\t" \
		"movq %1, %%rcx	\n\t" \
		"movq %2, %%rax	\n\t" \
		"lock cmpxchg %%rcx, %0	\n\t" \
		"jne 1b			\n\t" \
		"popq %%rcx		\n\t" \
		"popq %%rax		\n\t" \
		: "=m"((locker)->lock) \
		: "r"(c), "r"(a) \
		: "%rcx", "%rax" \
	); \
}

#define Task_SpinLock_unlock(locker) { \
	__asm__ volatile ( \
		"movq $0, %0	\n\t" \
		: "+r"((locker)->lock) \
		: \
		: "memory" \
	); \
}
#endif