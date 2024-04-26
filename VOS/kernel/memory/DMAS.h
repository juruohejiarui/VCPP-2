#ifndef __MEMORY_DMAS_H__
#define __MEMORY_DMAS_H__

#include "desc.h"

#define DMAS_virtAddrStart  ((void *)0xffff880000000000ul)
#define DMAS_virtAddrEnd    ((void *)0xffffC80000000000ul)

#define DMAS_physAddrStart  0x0000000000000000ul
#define DMAS_physAddrEnd    0x0000400000000000ul

#define DMAS_size           0x0000400000000000ul

#define DMAS_virt2Phys(virtAddr) ((u64)(virtAddr) - (u64)DMAS_virtAddrStart + DMAS_physAddrStart)
#define DMAS_phys2Virt(physAddr) ((void *)((u64)(physAddr) - DMAS_physAddrStart + (u64)DMAS_virtAddrStart))

void DMAS_init();

#endif