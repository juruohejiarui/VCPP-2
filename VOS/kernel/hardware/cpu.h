#ifndef __HARDWARE_CPU_H__
#define __HARDWARE_CPU_H__

#include "../includes/lib.h"

#define Hardware_CPUNumber 8

void HW_CPU_getID(u32 mop, u32 sop, u32 *eax, u32 *ebx, u32 *ecx, u32 *edx);

void HW_CPU_init();
#endif