#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include "../interrupt/gate.h"
#include "../interrupt/trap.h"

extern void (*intrList[24])(void);

void Init_interrupt();

void Intr_disableIntr();
void Intr_enableIntr();
#endif