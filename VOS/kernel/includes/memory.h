#ifndef __MEMORY_H__
#define __MEMORY_H__
#include "lib.h"

#define PTRS_PER_PAGE   512
#define PAGE_OFFSET ((u64)0xffff800000000000)

#define PAGE_GDT_SHIFT 39
#define PAGE_1G_SHIFT 30
#define PAGE_2M_SHIFT 21
#define PAGE_4K_SHIFT 12

#define PAGE_2M_SIZE (1ul << PAGE_2M_SHIFT)
#define PAGE_4K_SIZE (1ul << PAGE_4K_SHIFT)

#define PAGE_2M_MASK (~(PAGE_2M_SIZE - 1))
#define PAGE_4K_MASK (~(PAGE_4K_SIZE - 1))

#define PAGE_2M_ALIGN(addr) (((u64)(addr) + PAGE_2M_SIZE - 1) & PAGE_2M_MASK)
#define PAGE_4K_ALIGN(addr) (((u64)(addr) + PAGE_4K_SIZE - 1) & PAGE_4K_MASK)

#define virtToPhy(addr) ((u64)(addr) - PAGE_OFFSET)
#define phyToVirt(addr) ((u64 *)((u64)addr + PAGE_OFFSET))

typedef struct tmpE820 {
    u64 address;
    u64 length;
    u32 type;
} __attribute__((packed)) E820;


typedef struct tmpPage Page;
typedef struct tmpZone Zone;

struct GlobalMemoryDescriptor {
    E820 e820[32];
    u64 e820Size;

    /// @brief the bitmap of 4K pages.
    /// (bitmap[i] >> j) & 1 == 1:  the (i * 64 + j)-th page is allocated; 
    ///                      == 0:  the (i * 64 + j)-th page is free                        
    u64 *bitmap;
    u64 bitmapLength;
    u64 bitmapSize;

    Page *pages;
    // the length of page array
    u64 pagesLength;
    // the memory occupy of page
    u64 pagesSize;

    Zone *zones;
    // the length of zone array
    u64 zonesLength;
    // the memory occupy of zone array
    u64 zonesSize;

    // the address of address of kernel
    u64 codeSt, codeEd, dataEd, brkEd;

    // the end address of the pages
    u64 endOfStruct;
};

#define ZONE_DMA        (1 << 0)
#define ZONE_NORMAL     (1 << 1)
#define ZONE_UNMAPED    (1 << 2)

// the flag for the page has been mapped
#define PAGE_PTable_Maped   (1 << 0)
// the flag for the page belongs to kernel initial program
#define PAGE_Kernel_Init    (1 << 1)
// the flag for the page belongs to kernel
#define PAGE_Kernel         (1 << 2)
// the flag for the page which is active
#define PAGE_Active         (1 << 3)
// the flag for the page which is shared by kernel to user
#define PAGE_Shared_K2U     (1 << 4)
// the flag for the page referred
#define PAGE_Referred       (1 << 5)

#define PAGE_GloAttribute   ((1 << 6) - 1)
// the flag for the head page in buddy system
#define PAGE_HeadPage       (1 << 6)

#define MAX_NR_ZONES    10

typedef struct { u64 pml4tData; } Pml4t;
typedef struct { u64 pdptData; } Pdpt;
typedef struct { u64 pdtData; } Pdt;
typedef struct { u64 ptData; } Pt;

struct tmpPage {
    List listEle;
    // the zone that this page belongs to
    Zone *blgZone;
    // the address of the page that managed by this struct
    u64 phyAddr;
    // the pos in buddy system.
    u64 posId;
    u64 attribute;
    u64 refCnt;
    u64 age;
};

struct tmpZone {
    Page *pages;
    u64 pagesLength;
    // the start physics address of this zone aligned to 4K
    u64 stAddr;
    // the end physics address of this zone aligned to 4K
    u64 edAddr;
    // the length if this zone (edAddr - edAddr)
    u64 length;
    // the attribute of this zone
    u64 attribute;

    struct GlobalMemoryDescriptor *blgGMD;

    u64 pageUsingCnt;
    u64 pageFreeCnt;

    u64 totPageLink;
};

extern struct GlobalMemoryDescriptor memManageStruct;

void initMemory();

/// @brief Get NUM consecutive pages
/// @param zoneSel the zone that the pages belong to
/// @param num the number of pages
/// @param flags the flags of the pages
Page *allocPages(int zoneSel, int num, u64 flags);

u64 *getCR3();
extern u64 *globalCR3;

#pragma region Buddy System
#define BUDDY_MAX_ORDER 11

#define pageOrder(pagePtr) ((u8 *)((u64)&pagePtr->attribute + sizeof(u64) - 1))
struct BuddyStruct {
    u64 *bitmap[BUDDY_MAX_ORDER + 1];
    Page *freePageList[BUDDY_MAX_ORDER + 1];
};

void Buddy_initMemory();
void Buddy_initStruct();
void Buddy_debug();

// alloc 2^K pages
Page *Buddy_alloc(u64 log2Size);

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
    void (*constructer)(void *virtAddr, u64 arg);
    void (*destructor)(void *virtAddr, u64 arg);
} SlabCache;
#pragma endregion
#endif