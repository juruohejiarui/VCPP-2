#ifndef __SIMD_H__
#define __SIMD_H__

#include "lib.h"

typedef struct SIMD_XsaveArea {
	u8 legacyArea[512];
	union {
        struct {
            u64 xstate_bv;
            u64 xcomp_bv;
        };
        u8 header_area[64];
    };
    u8 extended_area[];
} SIMD_XsaveArea;

void SIMD_init();
void SIMD_enable();

u64 SIMD_XsaveAreaSize();

SIMD_XsaveArea *SIMD_allocXsaveArea(u64 kmallocArg, void (*destructor)(void *));

static __always_inline__ void SIMD_xsave(SIMD_XsaveArea *area) {
	__asm__ volatile ("xsave (%2)" :  : "a"(0x7), "d"(0), "c"(area) : "memory");
}

static __always_inline__ void SIMD_xrstor(SIMD_XsaveArea *area) {
	__asm__ volatile ("xrstor (%2)" : : "a"(0x7), "d"(0), "c"(area) : "memory");
}

#define SIMD_kernelAreaStart \
	do { \
		IO_cli(); \
		SIMD_clrTS(); \
	} while(0)

#define SIMD_kernelAreaEnd \
	do { \
		SIMD_setTS(); \
		IO_sti(); \
	} while (0)

#define SIMD_setTS() __asm__ volatile ( \
	"movq %%cr0, %%rax	\n\t" \
	"bts $3, %%rax		\n\t" \
	"movq %%rax, %%cr0	\n\t" \
	: \
	: \
	: "memory", "rax") \

#define SIMD_clrTS() __asm__ volatile ( \
	"movq %%cr0, %%rax	\n\t" \
	"btr $3, %%rax		\n\t" \
	"movq %%rax, %%cr0	\n\t" \
	: \
	: \
	: "memory", "rax") \

#define SIMD_getMXCSR() ({ \
	u32 val; \
	__asm__ volatile ( \
		"stmxcsr %0" : "=m"(val) : : "memory" \
	); \
	val; \
})

#define SIMD_setMXCSR(val) do { \
	u32 t = (val); \
	__asm__ volatile ( \
		"ldmxcsr %0" : : "m"(t) : "memory" \
	); \
} while(0) \

void SIMD_switchToCur();
#endif