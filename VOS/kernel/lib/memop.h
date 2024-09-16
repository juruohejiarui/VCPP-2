#ifndef __LIB_MEMOP_H__
#define __LIB_MEMOP_H__

#include "ds.h"

#define NULL ((void *)0)

void *memset(void *addr, u8 dt, i64 size);
void *memcpy(void *src, void *dst, i64 size);
int memcmp(void *fir, void *sec, i64 size);

i64 strlen(u8 *str);

int strcmp(char *a, char *b);
int strncmp(char *a, char *b, i64 size);



#endif