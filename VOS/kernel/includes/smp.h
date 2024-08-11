#ifndef __SMP_H__
#define __SMP_H__

#include "lib.h"
#include "hardware.h"

typedef struct SMP_CPUInfoPkg {
	struct TaskStruct *simdRegDomain;
	u32 *tssTable;
	u32 trIdx;
} SMP_CPUInfoPkg;

extern u8 SMP_APUBootStart[];
extern u8 SMP_APUBootEnd[];

void SMP_init();

u32 SMP_getCurCPUIndex();

SMP_CPUInfoPkg *SMP_getCPUInfoPkg(u32 idx);
#endif