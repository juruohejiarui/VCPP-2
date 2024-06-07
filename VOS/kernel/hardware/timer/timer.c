#include "cmos.h"
#include "hpet.h"
#include "../timer.h"

void HW_Timer_init() {
	HW_Timer_CMOS_init();
	HW_Timer_HPET_init();
}