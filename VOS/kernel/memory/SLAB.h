#ifndef __MEMORY_SLAB_H__
#define __MEMORY_SLAB_H__

#include "../includes/lib.h"
#include "desc.h"

typedef struct {
    List listEle;
    Page *page;

    u64 usingCnt;
    u64 freeCnt;

    void *virtAddr;

    u64 colLen;
    u64 colCnt;
    u64 *colMap;
} Slab;

typedef struct {
    u64 size;
    u64 usingCnt, freeCnt;
    Slab *slabs;
    void *(*constructer)(void *virtAddr, u64 arg);
    void *(*destructor)(void *virtAddr, u64 arg);
} SlabCache;

void Slab_init();
void Slab_debugLog();

/// @brief allocate a memory block for kernel process from the slab system
/// @param size the size of the memory block
/// @param arg the argument for the constructer
/// @return the pointer to the memory block
void *kmalloc(u64 size, u64 arg);
/// @brief free a memory block for kernel process from the slab system
/// @param addr 
void kfree(void *addr);
#endif