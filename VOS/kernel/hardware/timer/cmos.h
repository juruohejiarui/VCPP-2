#ifndef __HARDWARE_TIMER_CMOS_H__
#define __HARDWARE_TIMER_CMOS_H__

#include "../../includes/lib.h"

typedef struct {
	u8 second;
	u8 minute;
	u8 hour;
	u8 day;
	u8 month;
	u16 year;
} CMOSDateTime;

void HW_Timer_CMOS_init();
void HW_Timer_CMOS_getDateTime(CMOSDateTime *dateTime);

#endif