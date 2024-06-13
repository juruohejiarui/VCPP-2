#ifndef __MEMORY_BUDDY_H__
#define __MEMORY_BUDDY_H__
#include "desc.h"

int MM_Buddy_getOrder(Page *page);
void MM_Buddy_setOrder(Page *page, int order);

void MM_Buddy_init();
Page *MM_Buddy_alloc(u64 log2Size, u64 attr);
// allocate a page frame below 4G
Page *MM_Buddy_alloc4G(u64 log2Size, u64 attr);
void MM_Buddy_free(Page *page);
void MM_Buddy_debugLog(int range);

// divide a page frame which is allocated and returns the head page of the right buddy.
Page *MM_Buddy_divPageFrame(Page *headPage);


#endif