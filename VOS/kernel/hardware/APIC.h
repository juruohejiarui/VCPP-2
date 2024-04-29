#ifndef __HARDWARE_APIC_H__
#define __HARDWARE_APIC_H__
#include "../includes/lib.h"

u64 APIC_getReg_IA32_APIC_BASE();
void APIC_setReg_IA32_APIC_BASE(u64 value);

void APIC_writeRTE(u8 index, u64 val);

void APIC_disableAll();
void APIC_enableAll();

void APIC_disableIntr(u8 intrId);
void APIC_enableIntr(u8 intrId);

void Init_APIC();
#endif