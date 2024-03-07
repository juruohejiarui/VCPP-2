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

struct GlobalMemoryDescriptor {
    E820 e820[32];
    u64 e820Size;
};

typedef struct { u64 pml4tData; } Pml4t;
typedef struct { u64 pdptData; } Pdpt;
typedef struct { u64 pdtData; } Pdt;
typedef struct { u64 ptData; } Pt;

extern struct GlobalMemoryDescriptor memManageStruct;

void initMemory();
#endif