#ifndef __HARDWARE_H__
#define __HARDWARE_H__
#include "../hardware/cpu.h"
#include "../hardware/UEFI.h"
#include "../hardware/8259A.h"
#include "../hardware/APIC.h"
#include "../hardware/timer.h"
#include "../hardware/keyboard.h"
#include "../hardware/USB.h"

void HW_init();
void HW_initAdvance();

#endif