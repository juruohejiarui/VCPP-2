#ifndef __MEMORY_H__
#define __MEMORY_H__
#include "lib.h"

#define PTRS_PER_PAGE   512
#define PAGE_OFFSET ((u64)0xffff800000000000)

#define PAGE_GDT_SHIFT 39

typedef struct __tmpE820 {
    u64 addr;
    u64 len;
    u32 type;    
} __attribute__((packed)) E820;
typedef struct __tmpGlobalMemoryDescriptor {
    E820 e820[32];
    u64 e820Len;
} GlobalMemoryDescriptor;

extern GlobalMemoryDescriptor memManageStruct;

void initMemory();
#endif