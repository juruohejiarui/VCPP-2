// this file is used to define some functions to operate the io port and registers
#ifndef __LIB_IO_H__
#define __LIB_IO_H__
#include "ds.h"
void IO_out8(u16 port, u8 data);
void IO_out32(u16 port, u32 data);
u8 IO_in8(u16 port);
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

#define IO_Func_maskIntrPreffix \
	int __prevFlag = (IO_getRflags() >> 9) & 1; \
	if (__prevFlag) IO_cli();

#define IO_Func_maskIntrSuffix \
	if (__prevFlag) IO_sti();

u64 IO_getRIP();
#endif