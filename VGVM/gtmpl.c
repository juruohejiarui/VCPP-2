#include "gtmpl.h"

GVInstTmpl_Function(pvar, varId, _2) {
    CreateTemplInstList;
    if (stkTop + 1 <= stkRegNumber) 
        InstList_add(&temp, MOV_or_r(TM_qword, locVarDisp(varId), RBP, stkReg[stkTop + 1]), 1);
    else InstList_add(&temp, MOV_or_r(TM_qword, locVarDisp(varId), RBP, RCX), 1),
        InstList_add(&temp, MOV_r_or(TM_qword, RCX, clStkDisp(stkTop + 1), RBP), 0);
    InstList_addm(&temp, 3,
        MOV_or_r(TM_none, gtableDisp(gid1), RBP, RAX),
        CMP_i_r(TM_none, o, RAX),
        JNZ(0));
    int32 jzP1 = temp.size, jzP2;
    uint32 regId = stkTop + 1 <= stkRegNumber ? stkReg[stkTop + 1] : RCX;
    InstList_add(&temp, CMP_i_r(TM_qword, 0, regId), 0);
    InstList_add(&temp, JZ(0), 0);
    jzP2 = temp.size;
    InstList_addm(&temp, 2,
        INC_or(TM_qword, refCntOffset, regId),
        INC_or(TM_qword, rootRefCntOffset, regId));
    *(int32 *)(temp.data + jzP1 - 4) = (int32)(temp.size - jzP1);
    *(int32 *)(temp.data + jzP2 - 4) = (int32)(temp.size - jzP2);
    InstList_merge(getTgList(), &temp);
}

GVInstTmpl_Function(setvar, varId, _2) {
    int32 jzP1, jzP2;
    CreateTemplInstList;
    InstList_add(&temp, MOV_or_r(TM_none, gtableDisp(gid1), RBP, RAX), 1);
    InstList_add(&temp, CMP_i_r(TM_none, o, RAX), 0);
    InstList_add(&temp, JNZ(0), 0);
    jzP1 = temp.size;
    InstList_add(&temp, MOV_or_r(TM_qword, locVarDisp(varId), RBP, RCX), 0);
    InstList_add(&temp, CMP_i_r(TM_qword, 0, RCX), 0);  
    InstList_add(&temp, JZ(0), 0);
    jzP2 = temp.size;
    InstList_add(&temp, DEC_or(TM_qword, refCntOffset, RCX), 0);
    InstList_add(&temp, DEC_or(TM_qword, rootRefCntOffset, RCX), 0);
    *(int32 *)(temp.data + jzP1 - 4) = (int32)(temp.size - jzP1);
    *(int32 *)(temp.data + jzP2 - 4) = (int32)(temp.size - jzP2);

    int regId = (stkTop <= stkRegNumber ? stkReg[stkTop] : RCX);
    if (stkTop <= stkRegNumber)
        InstList_add(&temp, MOV_r_or(TM_qword, stkReg[stkTop], locVarDisp(varId), RBP), 0);
    else
        InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop), RBP, RCX), 0),
        InstList_add(&temp, MOV_r_or(TM_qword, RCX, locVarDisp(varId), RBP), 0);
    
    InstList_add(&temp, CMP_i_r(TM_none, o, RAX), 0);
    InstList_add(&temp, JNZ(0), 0);
    jzP1 = temp.size;
    InstList_add(&temp, CMP_i_r(TM_qword, 0, regId), 0);  
    InstList_add(&temp, JZ(0), 0);
    jzP2 = temp.size;
    InstList_add(&temp, INC_or(TM_qword, refCntOffset, regId), 0);
    InstList_add(&temp, INC_or(TM_qword, rootRefCntOffset, regId), 0);
    *(int32 *)(temp.data + jzP1 - 4) = (int32)(temp.size - jzP1);
    *(int32 *)(temp.data + jzP2 - 4) = (int32)(temp.size - jzP2);

    InstList_merge(getTgList(), &temp);
}

uint64 pmemGenericI8(Object *obj, uint64 offset) {
    obj->refCount--, obj->rootRefCount--;
    return (uint64)*(uint8 *)&obj->data[offset];
}
uint64 pmemGenericI16(Object *obj, uint64 offset) {
    obj->refCount--, obj->rootRefCount--;
    return (uint64)*(uint16 *)&obj->data[offset];
}
uint64 pmemGenericI32(Object *obj, uint64 offset) {
    obj->refCount--, obj->rootRefCount--;
    return (uint64)*(uint32 *)&obj->data[offset];
}
uint64 pmemGenericI64(Object *obj, uint64 offset) {
    obj->refCount--, obj->rootRefCount--;
    return (uint64)*(uint64 *)&obj->data[offset];
}
uint64 pmemGenericO(Object *obj, uint64 offset) {
    obj->refCount--, obj->rootRefCount--;
    Object *data = (Object *)*(uint64 *)&obj->data[offset];
    if (data != NULL) data->refCount++, data->rootRefCount++;
    return (uint64) data;
}
void *pmemGeneric[] = {
    pmemGenericI8, pmemGenericI8, pmemGenericI16, pmemGenericI16, pmemGenericI32, pmemGenericI32, pmemGenericI64, pmemGenericI64, 
    pmemGenericI32, pmemGenericI64, 
    pmemGenericO};

void setmemGenericI8(Object *obj, uint64 offset, uint64 data) {
    obj->refCount--, obj->rootRefCount--;
    *(uint8 *)&obj->data[offset] = *(uint8 *)&data;
}
void setmemGenericI16(Object *obj, uint64 offset, uint64 data) {
    obj->refCount--, obj->rootRefCount--;
    *(uint16 *)&obj->data[offset] = *(uint16 *)&data;
}
void setmemGenericI32(Object *obj, uint64 offset, uint64 data) {
    obj->refCount--, obj->rootRefCount--;
    *(uint32 *)&obj->data[offset] = *(uint32 *)&data;
}
void setmemGenericI64(Object *obj, uint64 offset, uint64 data) {
    obj->refCount--, obj->rootRefCount--;
    *(uint64 *)&obj->data[offset] = data;
}
void setmemGenericO(Object *obj, uint64 offset, Object * data) {
    if ((Object *)*(uint64 *)&obj->data[offset] != NULL) {
        Object *lstObj = (Object *)*(uint64 *)&obj->data[offset];
        lstObj->refCount--;
        if (!lstObj->refCount) refGC(obj);
    }
    obj->rootRefCount--, obj->refCount--;
    *(uint64 *)&obj->data[offset] = (uint64)data;
    obj->flag[offset / 8 / 64] |= (1ull << (offset / 8 % 64));
    if (data != NULL) data->rootRefCount--;
}
void *setmemGeneric[] = {
    setmemGenericI8, setmemGenericI8, setmemGenericI16, setmemGenericI16, setmemGenericI32, setmemGenericI32, setmemGenericI64, setmemGenericI64, 
    setmemGenericI32, setmemGenericI64, 
    setmemGenericO};

GVInstTmpl_Function(pmem, offset, _2) {
    CreateTemplInstList;
    prepareCallVMFunc(&temp, 1, stkTop - 1);
    if (stkTop <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop], RDI), stkTop == 1);
    else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop), RBP, RDI), stkTop == 1);
    InstList_add(&temp, XOR_r_r(TM_qword, RCX, RCX), 0);
    InstList_add(&temp, MOV_i_r(TM_qword, offset, RSI), 0);
    InstList_add(&temp, MOV_or_r(TM_none, gtableDisp(gid1), RBP, RCX), 0);
    InstList_add(&temp, MOV_i_r(TM_qword, (uint64)pmemGeneric, RAX), 0);
    InstList_add(&temp, MOV_rrs_r(TM_qword, RAX, RCX, 8, RAX), 0);
    InstList_add(&temp, CALL(RAX), 0);
    restoreFromCallVMFunc(&temp, 0, stkTop - 1);
    if (stkTop <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, RAX, stkReg[stkTop]), 0);
    else InstList_add(&temp, MOV_r_or(TM_qword, RAX, clStkDisp(stkTop), RBP), 0);
    InstList_merge(getTgList(), &temp);
}

GVInstTmpl_Function(setmem, offset, _2) {
    CreateTemplInstList;
    prepareCallVMFunc(&temp, 1, stkTop - 2);
    if (stkTop <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop], RDX), stkTop == 2);
    else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop), RBP, RDX), stkTop == 2);
    if (stkTop - 1 <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop - 1], RDI), 0);
    else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop - 1), RBP, RDI), 0);
    InstList_add(&temp, XOR_r_r(TM_qword, RCX, RCX), 0);
    InstList_add(&temp, MOV_or_r(TM_none, gtableDisp(gid1), RBP, RCX), 0);
    InstList_add(&temp, MOV_i_r(TM_qword, offset, RSI), 0);
    InstList_add(&temp, MOV_i_r(TM_qword, (uint64)setmemGeneric, RAX), 0);
    InstList_add(&temp, MOV_rrs_r(TM_qword, RAX, RCX, 8, RAX), 0);
    InstList_add(&temp, CALL(RAX), 0);
    restoreFromCallVMFunc(&temp, 0, stkTop - 2);
    InstList_merge(getTgList(), &temp);
}

GVInstTmpl_Function(parrmem, dimc, _2) {
    CreateTemplInstList;
    prepareCallVMFunc(&temp, 1, stkTop - dimc - 1);
    if (stkTop - dimc <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop - dimc], RAX), stkTop - dimc - 1 == 0);
    else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop - dimc), RBP, RAX), stkTop - dimc - 1 == 0);
    InstList_add(&temp, PUSH_r(RAX), 0);
    InstList_add(&temp, MOV_or_r(TM_qword, objDataOffset, RAX, RCX), 0);
    InstList_add(&temp, MOV_i_r(TM_qword, dimc * sizeof(uint64), RBX), 0);
    for (int i = 0; i < dimc; i++) {
        int regId = stkTop - dimc + i + 1;
        InstList_add(&temp, MOV_or_r(TM_qword, i * sizeof(uint64), RCX, RAX), 0);
        if (regId <= stkRegNumber) InstList_add(&temp, MUL_r(TM_qword, stkReg[regId]), 0);
        else InstList_add(&temp, MUL_or(TM_qword, clStkDisp(regId), RBP), 0);
        InstList_add(&temp, ADD_r_r(TM_qword, RAX, RBX), 0);
    }
    InstList_add(&temp, POP_r(RDI), 0);
    InstList_add(&temp, MOV_r_r(TM_qword, RBX, RSI), 0);
    InstList_add(&temp, XOR_r_r(TM_qword, RCX, RCX), 0);
    InstList_add(&temp, MOV_or_r(TM_none, gtableDisp(gid1), RBP, RCX), 0);
    InstList_add(&temp, MOV_i_r(TM_qword, (uint64)pmemGeneric, RAX), 0);
    InstList_add(&temp, MOV_rrs_r(TM_qword, RAX, RCX, 8, RAX), 0);
    InstList_add(&temp, CALL(RAX), 0);
    restoreFromCallVMFunc(&temp, 0, stkTop - dimc - 1);
    if (stkTop - dimc <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, RAX, stkReg[stkTop - dimc]), 0);
    else InstList_add(&temp, MOV_r_or(TM_qword, RAX, clStkDisp(stkTop - dimc), RBP), 0);
    InstList_merge(getTgList(), &temp);
}

GVInstTmpl_Function(setarrmem, dimc, _2) {
    CreateTemplInstList;
    prepareCallVMFunc(&temp, 1, stkTop - dimc - 2);
    if (stkTop - dimc - 1 <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop - dimc - 1], RAX), stkTop - dimc - 2 == 0);
    else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop - dimc), RBP, RAX), stkTop - dimc - 2 == 0);
    InstList_add(&temp, PUSH_r(RAX), 0);
    InstList_add(&temp, MOV_or_r(TM_qword, objDataOffset, RAX, RCX), 0);
    InstList_add(&temp, MOV_i_r(TM_qword, dimc * sizeof(uint64), RBX), 0);
    for (int i = 0; i < dimc; i++) {
        int regId = stkTop - dimc + i;
        InstList_add(&temp, MOV_or_r(TM_qword, i * sizeof(uint64), RCX, RAX), 0);
        if (regId <= stkRegNumber) InstList_add(&temp, MUL_r(TM_qword, stkReg[regId]), 0);
        else InstList_add(&temp, MUL_or(TM_qword, clStkDisp(regId), RBP), 0);
        InstList_add(&temp, ADD_r_r(TM_qword, RAX, RBX), 0);
    }
    if (stkTop <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop], RDX), 0);
    else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop), RBP, RDX), 0);
    InstList_add(&temp, MOV_r_r(TM_qword, RBX, RSI), 0);
    InstList_add(&temp, POP_r(RDI), 0);
    InstList_add(&temp, XOR_r_r(TM_qword, RCX, RCX), 0);
    InstList_add(&temp, MOV_or_r(TM_none, gtableDisp(gid1), RBP, RCX), 0);
    InstList_add(&temp, MOV_i_r(TM_qword, (uint64)setmemGeneric, RAX), 0);
    InstList_add(&temp, MOV_rrs_r(TM_qword, RAX, RCX, 8, RAX), 0);
    InstList_add(&temp, CALL(RAX), 0);
    restoreFromCallVMFunc(&temp, 0, stkTop - dimc - 2);
    InstList_merge(getTgList(), &temp);
}

GVInstTmpl_Function(cpy, _1, _2) {
    InstList temp;

}
