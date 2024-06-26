#ifndef __LIB_SPINLOCK_H__
#define __LIB_SPINLOCK_H__

#include "ds.h"
typedef struct {
	volatile i64 lock;
} SpinLock;

void SpinLock_init(SpinLock *lock);

#define SpinLock_lock(locker) do { \
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
} while (0)

#define SpinLock_unlock(locker) do { \
	__asm__ volatile ( \
		"movq $0, %0	\n\t" \
		: "+r"((locker)->lock) \
		: \
		: "memory" \
	); \
} while (0)
#endif