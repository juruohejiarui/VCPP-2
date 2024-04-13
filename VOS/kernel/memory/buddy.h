#ifndef __MEMORY_BUDDY_H__
#define __MEMORY_BUDDY_H__
#include "desc.h"
void Buddy_init();
Page *Buddy_alloc(u64 log2Size, u64 attr);
void Buddy_free(Page *page);
#endif