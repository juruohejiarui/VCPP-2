#ifndef __INTERRUPT_TRAP_H__
#define __INTERRUPT_TRAP_H__

void Intr_Trap_setSysVec();

void Intr_Trap_printRegs(u64 rsp);
#endif