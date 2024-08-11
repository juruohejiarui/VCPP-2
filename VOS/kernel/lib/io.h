// this file is used to define some functions to operate the io port and registers
#ifndef __LIB_IO_H__
#define __LIB_IO_H__
#include "ds.h"
void IO_out8(u16 port, u8 data);
void IO_out16(u16 port, u16 data);
void IO_out32(u16 port, u32 data);
u8 IO_in8(u16 port);
u16 IO_in16(u16 port);
u32 IO_in32(u16 port);

void IO_writeMSR(u64 msrAddr, u64 data);
u64 IO_readMSR(u64 msrAddr);

#define IO_mfence() __asm__ volatile ("mfence \n\t" : : : "memory")

#define IO_getRflags() \
	({ \
		u64 rflags; \
		__asm__ volatile ( \
			"pushfq     \n\t" \
			"popq %0    \n\t" \
			: "=m"(rflags) \
			: \
			: "memory"); \
		rflags; \
	})

#define IO_sti() __asm__ volatile ("sti \n\t" : : : "memory")
#define IO_cli() __asm__ volatile ("cli \n\t" : : : "memory")

#define IO_hlt() __asm__ volatile ("hlt \n\t" : : : "memory")
#define IO_nop() __asm__ volatile ("nop \n\t" : : : "memory")

#define IO_clts() __asm__ volatile ("clts \n\t" : : : "memory")

#define IO_setCR(id, vl) do { \
	u64 vr = (vl); \
	__asm__ volatile ( \
		"movq %0, %%rax	\n\t"\
		"movq %%rax, %%cr"#id"\n\t" \
		: "=m"(vr) \
		: \
		: "rax", "memory"); \
} while (0)

#define IO_getCR(id)  ({ \
	u64 vl; \
	__asm__ volatile ( \
		"movq %%cr"#id", %%rax	\n\t"\
		"movq %%rax, %0" \
		: "=m"(vl) \
		: \
		: "rax", "memory"); \
	vl; \
})

#define IO_getXCR(id) ({ \
	u64 res = 0; \
	__asm__ volatile ( \
		"movq $"#id", %%rcx	\n\t" \
		"xorq %%rax, %%rax	\n\t" \
		"xgetbv				\n\t" \
		"shlq $32, %%rdx	\n\t" \
		"orq %%rdx, %%rax	\n\t" \
		"movq %%rax, %0		\n\t" \
		: "=m"(res) \
		: \
		: "rax", "rcx", "rdx", "memory" \
	); \
	res; \
})

#define IO_setXCR(id, vl) do { \
	u64 vr = (vl); \
	__asm__ volatile ( \
		"movq $"#id", %%rcx	\n\t" \
		"movq %0, %%rax		\n\t" \
		"movq %%rax, %%rdx	\n\t" \
		"shrq $32, %%rdx	\n\t" \
		"xsetbv				\n\t" \
		: "=m"(vr) \
		: \
		: "rax", "rcx", "rdx", "memory" \
	); \
} while (0) \

#define IO_maskIntrPreffix \
	int __prevFlag = (IO_getRflags() >> 9) & 1; \
	if (__prevFlag) IO_cli();

#define IO_maskIntrSuffix \
	if (__prevFlag) IO_sti();

u64 IO_getRIP();
#endif