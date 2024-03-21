#ifndef __CPU_H__
#define __CPU_H__

#include "lib.h"

void getCPUID(u32 mop, u32 sop, u32 *a, u32 *b, u32 *c, u32 *d);
void initCPU();

#endif