#ifndef __INTERRUPT_GATE_H__
#define __INTERRUPT_GATE_H__

#include "../includes/lib.h"

typedef struct { u8 dt[8]; } GDTItem;
extern GDTItem gdtTable[];
typedef struct { u8 dt[16]; } IDTItem;
extern IDTItem idtTable[];
extern u32 tss64Table[26];

void loadTR(u16 n);

void setIntrGate(u64 idtIndex, u8 istIndex, void *codeAddr);
void setTrapGate(u64 idtIndex, u8 istIndex, void *codeAddr);
void setSystemGate(u64 idtIndex, u8 istIndex, void *codeAddr);
void setSystemIntrGate(u64 idtIndex, u8 istIndex, void *codeAddr);

void setTSS64(
        u64 rsp0, u64 rsp1, u64 rsp2, u64 ist1, u64 ist2, 
        u64 ist3, u64 ist4, u64 ist5, u64 ist6, u64 ist7);
#endif