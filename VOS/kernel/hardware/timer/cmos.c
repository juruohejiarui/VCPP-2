#include "cmos.h"

void HW_Timer_CMOS_init() {
	// enable periodic interrupt
	
	// nothing to do
}

#define CMOS_read(addr) ({ \
	IO_out8(0x70, 0x80 | addr); \
	IO_in8(0x71); \
})

void HW_Timer_CMOS_getDateTime(CMOSDateTime *dateTime) {
	do {
		dateTime->year = CMOS_read(0x09) + CMOS_read(0x32) * 0x100;
		dateTime->month = CMOS_read(0x08);
		dateTime->day = CMOS_read(0x07);
		dateTime->hour = CMOS_read(0x04);
		dateTime->minute = CMOS_read(0x02);
		dateTime->second = CMOS_read(0x00);
	} while (dateTime->second != CMOS_read(0x00));
	IO_out8(0x70, 0x00);
}