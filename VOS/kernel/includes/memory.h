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


extern struct tmpPage;
typedef struct tmpPage Page;
extern struct tmpZone;
typedef struct tmpZone Zone;

struct GlobalMemoryDescriptor {
    E820 e820[32];
    u64 e820Size;

    /// @brief the bitmap of 4K pages.
    /// (bitmap[i] >> j) & 1 == 0:  the (i * 64 + j)-th page is allocated; 
    ///                      == 1:  the (i * 64 + j)-th page is free                        
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

typedef struct { u64 pml4tData; } Pml4t;
typedef struct { u64 pdptData; } Pdpt;
typedef struct { u64 pdtData; } Pdt;
typedef struct { u64 ptData; } Pt;

struct tmpPage {
    // the zone that this page belongs to
    Zone *blgZone;
    // the address of the page that managed by this struct
    u64 phyAddr;
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
#endif