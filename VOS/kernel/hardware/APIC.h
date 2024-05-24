#ifndef __HARDWARE_APIC_H__
#define __HARDWARE_APIC_H__
#include "../includes/lib.h"

u64 Hardware_APIC_getReg_IA32_APIC_BASE();
void HW_APIC_setReg_IA32_APIC_BASE(u64 value);

void HW_APIC_writeRTE(u8 index, u64 val);

void HW_APIC_disableAll();
void HW_APIC_enableAll();
void HW_APIC_suspend();
void HW_APIC_resume();

void HW_APIC_disableIntr(u8 intrId);
void HW_APIC_enableIntr(u8 intrId);

void HW_APIC_init();

int HW_APIC_finishedInit();
#endif