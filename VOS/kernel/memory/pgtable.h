#ifndef __MEMORY_PGTABLE_H__
#define __MEMORY_PGTABLE_H__

#include "../includes/lib.h"

#define Init_virtAddrStart  (0xffff800000000000ul)

#define PageTable_MapSize_4K (1 << 0)
#define PageTable_MapSize_2M (1 << 1)
#define PageTable_MapSize_1G (1 << 2)

#define getCR3() \
	({ \
		u64 cr3; \
		__asm__ volatile ( \
			"movq %%cr3, %0" \
			: "=r"(cr3) \
			: \
			: "memory" \
		); \
		cr3; \
	})
#define setCR3(cr3) \
	do { \
		__asm__ volatile ( \
			"movq %0, %%cr3" \
			: \
			: "r"(cr3) \
			: "memory" \
		); \
	} while (0)
#define flushTLB() \
	do { \
		__asm__ volatile ( \
			"movq %%cr3, %%rax	\n\t" \
			"movq %%rax, %%cr3	\n\t" \
			: \
			: \
			: "rax", "memory" \
		); \
	} while (0)

typedef struct { u64 entry[512]; } PageTable;
void PageTable_init();
// allocate a page table and return the physical address of the page table
u64 PageTable_alloc();

// build the corresponding page table of V_ADDR and
// if P_ADDR != 0: map it to P_ADDR 
// if P_ADDR == 0: remains the entry of PLD no presents.
void PageTable_map(u64 cr3, u64 vAddr, u64 pAddr);

void PageTable_unmap(u64 cr3, u64 vAddr);

u64 PageTable_getPldEntry(u64 cr3, u64 vAddr);

u64 PageTable_fork();

#endif