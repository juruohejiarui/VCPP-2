#ifndef __MEMORY_PGTABLE_H__
#define __MEMORY_PGTABLE_H__

#include "../includes/lib.h"

#define Init_virtAddrStart  (0xffff800000000000ul)

#define PageTable_MapSize_4K (1 << 0)
#define PageTable_MapSize_2M (1 << 1)
#define PageTable_MapSize_1G (1 << 2)

u64 getCR3();
void setCR3(u64 cr3);
void flushTLB();

typedef struct { u64 entry[512]; } PageTable;
void PageTable_init();
// allocate a page table and return the physical address of the page table

// build the corresponding page table of V_ADDR and
// if P_ADDR != 0: map it to P_ADDR 
// if P_ADDR == 0: remains the entry of PLD no presents.
void PageTable_map(u64 vAddr, u64 pAddr);

void PageTable_unmap(u64 vAddr);

u64 PageTable_fork();

#endif