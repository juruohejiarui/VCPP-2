#ifndef __MEMORY_BUDDY_H__
#define __MEMORY_BUDDY_H__
#include "desc.h"

int Page_getOrder(Page *page);
void Page_setOrder(Page *page, int order);

void Buddy_init();
Page *Buddy_alloc(u64 log2Size, u64 attr);
void Buddy_free(Page *page);
void Buddy_debugLog(int range);

// divide a page frame which is allocated and returns the head page of the right buddy.
Page *Buddy_dividePageFrame(Page *headPage);


#endif