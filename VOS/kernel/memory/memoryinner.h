#ifndef __MEMORYINNER_H__
#define __MEMORYINNER_H__

#include "../includes/memory.h"

/// @brief Get NUM consecutive pages
/// @param zoneSel the zone that the pages belong to
/// @param num the number of pages
/// @param flags the flags of the pages
Page *allocPages(int zoneSel, int num, u64 flags);
u64 initPage(Page *page, u64 flags);

#define pageOrder(pagePtr) ((u8 *)((u64)&pagePtr->attribute + sizeof(u64) - 1))
struct BuddyStruct {
    u64 *bitmap[BUDDY_MAX_ORDER + 1];
    Page *freePageList[BUDDY_MAX_ORDER + 1];
};

void Buddy_initMemory();
void Buddy_initStruct();
void Buddy_debug();

// alloc 2^K pages
Page *Buddy_alloc(u64 log2Size, u64 attribute);

// free pages
void Buddy_free(Page *pages);
#pragma endregion

#pragma region Slab System
typedef struct {
    List listEle;
    Page *page;
    u64 usingCnt, freeCnt;
    void *virtAddr;
    u64 colSize, colCnt;
    u64 *colMap;
} Slab;

typedef struct {
    u64 size, totUsing, totFree;
    Slab *cachePool;
    Slab *cacheDMAPool;
    void *( *constructer)(void *virtAddr, u64 arg);
    void *( *destructor)(void *virtAddr, u64 arg);
} SlabCache;
#pragma endregion

typedef struct {
    u64 pageTableSize;
    u64 *data;
} PageTable;

#define originalCR3 0x101000
// update the page table and flush the TLB (Translation Lookaside Buffer)
// #define flushTLB() \
// do { \
//     u64 tmpreg; \
//     __asm__ __volatile__( \
//         "movq %%cr3, %0\n\t" \
//         "movq %0, %%cr3\n\t" \
//         :"=r"(tmpreg) \
//         : \
//         : "memory"); \
// } while(0)

extern Page *PageTable_swpPage;
void PageTable_init();
// initialize the page table on the physics address PHY_ADDR and set its virtAddr to TARGET_VIRT_ADDR
void PageTable_initPageTable(u64 phyAddr, u64 *targetVirtAddr);

#endif