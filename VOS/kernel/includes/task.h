#ifndef __TASK_H__
#define __TASK_H__

#include "memory.h"
#include "gate.h"

struct tmpTaskStruct {
    PageTable *pgd;
    u64 rsp;
    u64 rip;
    
};
#endif