#include "ds.h"

typedef struct {
	volatile i64 value;
} Atomic;

static __always_inline__ void Atomic_add(Atomic *atomic, i64 vl) {
	__asm__ volatile ( " lock addq %1, %0	\n\t"
		: "=m"(atomic->value) : "r"(vl) : "memory");
}

static __always_inline__ void Atomic_sub(Atomic *atomic, i64 vl) {
	__asm__ volatile ( " lock subq %1, %0	\n\t"
		: "=m"(atomic->value) : "r"(vl) : "memory");
}

static __always_inline__ void Atomic_inc(Atomic *atomic) {
	__asm__ volatile ( " lock incq %0	\n\t"
		: "=m"(atomic->value) : "m"(atomic->value) : "memory");
}

static __always_inline__ void Atomic_dec(Atomic *atomic) {
	__asm__ volatile ( " lock decq %0	\n\t"
		: "=m"(atomic->value) : "m"(atomic->value) : "memory");
}