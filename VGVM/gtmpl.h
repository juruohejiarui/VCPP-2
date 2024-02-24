#pragma once

#include "vgvm.h"
#include "tmpl.h"

extern InstList *tgList;
extern RuntimeBlock *tgBlk;

#define GVInstTmpl_Function(vinstName, arg1Name, arg2Name) \
    void GVInstTmpl_##vinstName(uint8 gid1, uint8 gid2, uint32 stkTop, uint64 arg1Name, uint64 arg2Name)

GVInstTmpl_Function(pvar, varId, _2);
GVInstTmpl_Function(setvar, varId, _2);
GVInstTmpl_Function(pmem, offset, _2);
GVInstTmpl_Function(setmem, offset, _2);
GVInstTmpl_Function(parrmem, dimc, _2);
GVInstTmpl_Function(setarrmem, dimc, _2);
GVInstTmpl_Function(pop, _1, _2);
GVInstTmpl_Function(cpy, _1, _2);
GVInstTmpl_Function(cvt, _1, _2);