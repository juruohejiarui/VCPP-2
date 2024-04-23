#ifndef __HARDWARE_APIC_H__
#define __HARDWARE_APIC_H__
#include "../includes/lib.h"

u64 APIC_getReg_IA32_APIC_BASE();
void APIC_setReg_IA32_APIC_BASE(u64 value);

void Init_APIC();
#endif