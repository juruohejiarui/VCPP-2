#ifndef __LIB_MEMOP_H__
#define __LIB_MEMOP_H__

#include "ds.h"

void *memset(void *addr, u8 dt, u64 size);
void *memcpy(void *src, void *dst, u64 size);
int memcmp(void *fir, void *sec, u64 size);

i64 strlen(u8 *str);


#endif