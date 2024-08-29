#ifndef __INTERRUPT_GATE_H__
#define __INTERRUPT_GATE_H__

#include "../includes/lib.h"

typedef struct { u8 dt[8]; } GDTItem;
extern GDTItem gdtTable[];
typedef struct { u8 dt[16]; } IDTItem;
extern IDTItem idtTable[];
// the TSS table for BSP
extern u32 tss64Table[26];

void Intr_Gate_loadTR(u16 n);

void Intr_Gate_setIntr(u64 idtIndex, u8 istIndex, void *codeAddr);
void Intr_Gate_setSMPIntr(int cpuId, u64 idtIndex, u8 istIndex, void *codeAddr);
void Intr_Gate_setTrap(u64 idtIndex, u8 istIndex, void *codeAddr);
void Intr_Gate_setSystem(u64 idtIndex, u8 istIndex, void *codeAddr);
void Intr_Gate_setSysIntr(u64 idtIndex, u8 istIndex, void *codeAddr);

void Intr_Gate_setTSS(
        u32 *tss64Table,
        u64 rsp0, u64 rsp1, u64 rsp2, u64 ist1, u64 ist2,
        u64 ist3, u64 ist4, u64 ist5, u64 ist6, u64 ist7);

void Intr_Gate_setTSSstruct(u32 *tss64Table, TSS *tssStruct);

void Intr_Gate_setTSSDesc(u64 idx, u32 *tssAddr);
#endif