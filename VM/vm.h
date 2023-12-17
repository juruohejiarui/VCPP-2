#ifndef __VM_H__
#define __VM_H__

#include "tools.h"
#include "mmanage.h"
#include "rflsys.h"

enum DataTypeModifier {
    i8, u8, i16, u16, i32, u32, i64, u64, f32, f64, o, B, dtMdfUnknown,
};
enum ValueTypeModifier {
    MemberRef, VarRef, TrueVal, vlMdfUnknown,
};

typedef enum TCommand {
    none      , mov       , addmov    , submov    , mulmov    , divmov    , andmov    , ormov     , xormov    , shlmov    , 
    shrmov    , modmov    , add       , sub       , mul       , _div      , _and      , _or       , _xor      , shl       , 
    shr       , mod       , _not      , pinc      , sinc      , pdec      , sdec      , eq        , ne        , gt        , 
    ge        , ls        , le        , cvt       , pop       , push      , pvar      , pglo      , cpy       , setlocal  , 
    getarg    , _new      , arrnew    , gvl       , mem       , vmem      , arrmem    , call      , vcall     , jmp       , 
    jz        , jp        , setarg    , ret       , vret      , catostr   , strtoca   , getcls    , getmtds   , getflds   , 
    getctrs   , sys       ,
    tCmdUnknown,
};

typedef struct tmpRuntimeBlock {
    uint32 id;
    uint64 *relyBlkId, relyCount, strCount;
    char **relyList, **strList;
    uint8 *dataTemplate, *globalMemory;
    uint8 *vcode;
    uint64 *mainOffset;
    NamespaceTypeData *tdRoot;
} RuntimeBlock;

typedef struct tmpCallFrame { 
    uint64 offset;
    uint32 blkId;
    uint64 *var;
    uint64 cStack[16], *cStackTop;
    Object **oStack[16], **oStackTop;
} CallFrame;

int VM(const char *vobjPath);

#endif