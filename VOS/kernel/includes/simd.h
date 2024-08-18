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

u64 SIMD_XsaveAreaSize();

SIMD_XsaveArea *SIMD_allocXsaveArea(u64 kmallocArg, void (*destructor)(void *));

static __always_inline__ void SIMD_xsave(SIMD_XsaveArea *area) {
	__asm__ volatile ("xsave %0" : "=m"(area) : : "memory");
}

static __always_inline__ void SIMD_xrstor(SIMD_XsaveArea *area) {
	__asm__ volatile ("xrstor %0" : "=m"(area) : : "memory");
}

void SIMD_switchToCur();
#endif