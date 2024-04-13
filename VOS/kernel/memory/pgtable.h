#ifndef __MEMORY_PGTABLE_H__
#define __MEMORY_PGTABLE_H__

#include "../includes/lib.h"

#define Init_virtAddrStart  (0xffff800000000000ul)

u64 getCR3();
void setCR3(u64 cr3);
void flushTLB();

typedef struct { u64 entry[512]; } PageTable;
PageTable *PageTable_alloc();
void PageTable_free(PageTable *pageTable);

#endif