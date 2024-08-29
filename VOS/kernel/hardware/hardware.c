#include "../includes/hardware.h"
#include "PCIe.h"
#include "UEFI.h"
#include "APIC.h"
#include "timer.h"
#include "cpu.h"
#include "keyboard.h"
#include "USB.h"

void HW_init() {
    HW_UEFI_init();
    HW_CPU_init();
    HW_PCIe_init();
    HW_APIC_init();
    HW_Timer_init();
    HW_Keyboard_init();
}

void HW_initAdvance() {
	HW_USB_init();
}