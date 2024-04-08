#ifndef __LIB_IO_H__
#define __LIB_IO_H__
#include "ds.h"
void IO_out8(u16 port, u8 data);
void IO_out32(u16 port, u32 data);
u8 IO_in8(u16 port);
u32 IO_in32(u16 port);
#endif