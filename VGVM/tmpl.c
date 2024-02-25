#include "tmpl.h"
#include "gtmpl.h"
#include "vgvm.h"
#include "syscall.h"

InstList *tgList;
DArray *offList;
DArray *entryList;
// the entry of the jmp/jcc instruction
DArray *jmpList;
DArray *jmpDstOffset;
RuntimeBlock *tgBlk;

const uint32  
    refCntOffset        = memberOffset(Object, refCount), 
    rootRefCntOffset    = memberOffset(Object, rootRefCount), 
    objDataOffset       = memberOffset(Object, data),
    objGenOffset        = memberOffset(Object, genId),
    relyBlkOffset       = memberOffset(RuntimeBlock, relyBlk),
    relyListOffset      = memberOffset(RuntimeBlock, relyPath),
    entryListOffset     = memberOffset(RuntimeBlock, entryList),
    strListOffset       = memberOffset(RuntimeBlock, strList);

int curStkTop;

typedef enum RegisterState { RS_Unknown, RS_VarListAddr, RS_ParamListAddr } RegState;

InstList *getTgList() { return tgList; }

void setTargetBlock(RuntimeBlock *tgrBlk) {
    tgBlk = tgrBlk;
    tgBlk->instBlk = NULL;

    if (tgList != NULL) InstList_free(tgList);
    tgList = malloc(sizeof(InstList));
    InstList_init(tgList);
    if (offList != NULL) DArray_clear(offList);
    else offList = malloc(sizeof(DArray)), DArray_init(offList, 8);
    if (entryList != NULL) DArray_clear(entryList);
    else entryList = malloc(sizeof(DArray)), DArray_init(entryList, 8);
    if (jmpList != NULL) DArray_clear(jmpList);
    else jmpList = malloc(sizeof(DArray)), DArray_init(jmpList, 8);
    if (jmpDstOffset != NULL) DArray_clear(jmpDstOffset);
    else jmpDstOffset = malloc(sizeof(DArray)), DArray_init(jmpDstOffset, 8);
}

// actuallly the stkReg[0] will not be used and it is just a occupy symbol
const uint32 stkReg[] = { RCX, RDI, RSI, R8, R9, R10, R11, R12, R13, R14, R15 };
const uint32 argReg[] = {RDI, RSI, RDX, RCX, R8, R9};
const uint32 stkRegNumber = 10, stkSize = 16, gtableSize = 5;

// insert the assembly code for preparing for calling a function in VM
void prepareCallVMFunc(InstList *insl, int isEntry, int stkTop) {
    // save the registers in the stack
    int isFir = 1;
    for (int i = 1; i <= min(stkTop, stkRegNumber); i++, isFir = 0) InstList_add(insl, PUSH_r(stkReg[i]), isFir && isEntry);
}
void restoreFromCallVMFunc(InstList *insl, int isEntry, int stkTop) {
    // restore the registers from the stack
    int isFir = 1;
    for (int i = min(stkTop, stkRegNumber); i > 0; i--, isFir = 0) InstList_add(insl, POP_r(stkReg[i]), isFir && isEntry);
}

void setPause(InstList *temp) {
    InstList ls1 = genInstList_saveReg(0, 0), ls2 = genInstList_restoreReg(0, 0);
    InstList_merge(temp, &ls1);
    InstList_add(temp, MOV_i_r(TM_qword, (uint64)pauseVM, RAX), 0);
    InstList_add(temp, MOV_r_r(TM_qword, RBP, RDI), 0);
    InstList_add(temp, MOV_r_r(TM_qword, RSP, RSI), 0);
    InstList_add(temp, CALL(RAX), 0);
    InstList_merge(temp, &ls2);
}

#define VInstTmpl_Function(vinstName, arg1Name, arg2Name) \
    void VInstTmpl_##vinstName(TypeModifier dtMdf, int isSigned, uint32 stkTop, uint64 arg1Name, uint64 arg2Name)

#pragma region binary operator : + - * / % & | ^ ~ << >> and compare

#define binaryOperator(opName, list, stkTop) \
    do { \
        if (stkTop <= stkRegNumber) \
            InstList_add(&list, opName##_r_r(dtMdf, stkReg[stkTop], stkReg[(stkTop) - 1]), 1); \
        else if (stkTop == stkRegNumber + 1) \
            InstList_add(&list, opName##_or_r(dtMdf, clStkDisp(stkTop), RBP, stkReg[stkRegNumber]), 1); \
        else { \
            InstList_add(&list, MOV_or_r(dtMdf, clStkDisp(stkTop), RBP, RCX), 1); \
            InstList_add(&list, opName##_r_or(dtMdf, RCX, clStkDisp(stkTop - 1), RBP), 0); \
        } \
    } while(0)

#define floatBinaryOperator(opName, list, stkTop) \
    do { \
        if (stkTop <= stkRegNumber) \
            InstList_add(&list, MOV_r_x(dtMdf, stkReg[stkTop], XMM0), 1), \
            InstList_add(&list, MOV_r_x(dtMdf, stkReg[stkTop - 1], XMM1), 0), \
            InstList_add(&list, opName##_x_x(dtMdf, XMM0, XMM1), 0), \
            InstList_add(&list, MOV_x_r(dtMdf, XMM1, stkReg[stkTop - 1]), 0); \
        else if (stkTop == stkRegNumber + 1) \
            InstList_add(&list, MOV_r_x(dtMdf, stkReg[stkTop - 1], XMM0), 1), \
            InstList_add(&list, opName##_or_x(dtMdf, clStkDisp(stkTop), RBP, XMM0), 0), \
            InstList_add(&list, MOV_x_r(dtMdf, XMM0, stkReg[stkTop - 1]), 0); \
        else \
            InstList_add(&list, MOV_or_x(dtMdf, clStkDisp(stkTop - 1), RBP, XMM0), 1), \
            InstList_add(&list, opName##_or_x(dtMdf, clStkDisp(stkTop), RBP, XMM0), 0), \
            InstList_add(&list, MOV_x_or(dtMdf, XMM0, clStkDisp(stkTop - 1), RBP), 0); \
    } while(0)

VInstTmpl_Function(add, _1, _2) {
    CreateTemplInstList;
    if (isInterger(dtMdf)) binaryOperator(ADD, temp, stkTop);
    else if (isFloat(dtMdf)) floatBinaryOperator(ADD, temp, stkTop);
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(sub, _1, _2) {
    CreateTemplInstList;
    if (isInterger(dtMdf)) binaryOperator(SUB, temp, stkTop);
    else if (isFloat(dtMdf)) floatBinaryOperator(SUB, temp, stkTop);
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(mul, _1, _2) {
    CreateTemplInstList;
    if (isInterger(dtMdf)) {
        if (stkTop - 1 <= stkRegNumber)
            InstList_add(&temp, MOV_r_r(dtMdf, stkReg[stkTop - 1], RAX), 1);
        else InstList_add(&temp, MOV_or_r(dtMdf, clStkDisp(stkTop - 1), RBP, RAX), 1);

        if (stkTop <= stkRegNumber)
            InstList_add(&temp, (isSigned ? IMUL_r : MUL_r)(dtMdf, stkReg[stkTop]), 0);
        else InstList_add(&temp, (isSigned ? IMUL_or : MUL_or)(dtMdf, clStkDisp(stkTop), RBP), 0);
        
        if (stkTop - 1 <= stkRegNumber)
            InstList_add(&temp, MOV_r_r(dtMdf, RAX, stkReg[stkTop - 1]), 0);
        else InstList_add(&temp, MOV_r_or(dtMdf, RAX, clStkDisp(stkTop - 1), RBP), 0);
    } else if (isFloat(dtMdf)) floatBinaryOperator(MUL, temp, stkTop);
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(_div, _1, _2) {
    CreateTemplInstList;
    if (isInterger(dtMdf)) {
        InstList_add(&temp, XOR_r_r(TM_qword, RDX, RDX), 0);
        if (stkTop - 1 <= stkRegNumber)
            InstList_add(&temp, MOV_r_r(dtMdf, stkReg[stkTop - 1], RAX), 0);
        else InstList_add(&temp, MOV_or_r(dtMdf, clStkDisp(stkTop - 1), RBP, RAX), 0);

        if (stkTop <= stkRegNumber)
            InstList_add(&temp, (isSigned ? IDIV_r : DIV_r)(dtMdf, stkReg[stkTop]), 0);
        else InstList_add(&temp, (isSigned ? IDIV_or : DIV_or)(dtMdf, clStkDisp(stkTop), RBP), 0);
        
        if (stkTop - 1 <= stkRegNumber)
            InstList_add(&temp, MOV_r_r(dtMdf, RAX, stkReg[stkTop - 1]), 0);
        else InstList_add(&temp, MOV_r_or(dtMdf, RAX, clStkDisp(stkTop - 1), RBP), 0);
    } else if (isFloat(dtMdf)) floatBinaryOperator(DIV, temp, stkTop);
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(mod, _1, _2) {
    CreateTemplInstList;
    InstList_add(&temp, XOR_r_r(TM_qword, RDX, RDX), 1);
    if (stkTop - 1 <= stkRegNumber)
        InstList_add(&temp, MOV_r_r(dtMdf, stkReg[stkTop - 1], RAX), 0);
    else InstList_add(&temp, MOV_or_r(dtMdf, clStkDisp(stkTop - 1), RBP, RAX), 0);
    if (dtMdf == TM_none) InstList_add(&temp, XOR_AH_AH(), 0);
    if (stkTop <= stkRegNumber)
        InstList_add(&temp, 
            (isSigned ? IDIV_r : DIV_r)(dtMdf, stkReg[stkTop]),
            0);
    else 
        InstList_add(&temp,
            (isSigned ? IDIV_or : DIV_or)(dtMdf, clStkDisp(stkTop), RBP),
            0);
    if (dtMdf == TM_none) InstList_add(&temp, MOV_AH_DL(), 0);
    if (stkTop - 1 <= stkRegNumber)
        InstList_add(&temp, MOV_r_r(dtMdf, RDX, stkReg[stkTop - 1]), 0);
    else InstList_add(&temp, MOV_r_or(dtMdf, RDX, clStkDisp(stkTop - 1), RBP), 0);
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(_and, _1, _2) {
    CreateTemplInstList;
    if (isInterger(dtMdf)) binaryOperator(AND, temp, stkTop);
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(_or, _1, _2) {
    CreateTemplInstList;
    if (isInterger(dtMdf)) binaryOperator(OR, temp, stkTop);
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(_xor, _1, _2) {
    CreateTemplInstList;
    if (isInterger(dtMdf)) binaryOperator(XOR, temp, stkTop);
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(_not, _1, _2) {
    CreateTemplInstList;
    if (isInterger(dtMdf)) {
        if (stkTop <= stkRegNumber) InstList_add(&temp, NOT_r(dtMdf, stkReg[stkTop]), 1);
        else InstList_add(&temp, NOT_or(dtMdf, clStkDisp(stkTop), RBP), 1);
    }
    InstList_merge(tgList, &temp);
}

#define Shift(InstSigned, InstUnsigned, dtMdf, stkTop) \
    do { \
        CreateTemplInstList; \
        if (stkTop <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop], CL), 1); \
        else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop), RBP, CL), 1); \
        if (stkTop - 1 <= stkRegNumber) \
            InstList_add(&temp, (isSigned ? InstSigned##_CL_r : InstUnsigned##_CL_r)(dtMdf, stkReg[stkTop - 1]), 0); \
        else InstList_add(&temp, (isSigned ? InstSigned##_CL_or : InstUnsigned##_CL_or)(dtMdf, clStkDisp(stkTop - 1), RBP), 0); \
        InstList_merge(tgList, &temp); \
    } while(0) 

VInstTmpl_Function(shl, _1, _2) {
    Shift(SAL, SHL, dtMdf, stkTop);
}

VInstTmpl_Function(shr, _1, _2) {
    Shift(SAR, SHR, dtMdf, stkTop);
}

#define cmpInteger(dtMdf, stkTop) \
    do { \
        if ((stkTop) <= stkRegNumber) \
            InstList_add(&temp, CMP_r_r(dtMdf, stkReg[stkTop], stkReg[(stkTop) - 1]), 1); \
        else if ((stkTop) == stkRegNumber + 1) \
            InstList_add(&temp, CMP_or_r(dtMdf, clStkDisp(stkTop), RBP, stkReg[(stkTop) - 1]), 1); \
        else { \
            InstList_add(&temp, MOV_or_r(dtMdf, clStkDisp((stkTop) - 1), RBP, RAX), 1); \
            InstList_add(&temp, CMP_r_or(dtMdf, clStkDisp(stkTop), RBP, RAX), 0); \
        } \
    } while(0)
#define cmpFloat(dtMdf, stkTop) \
    do { \
        if ((stkTop) <= stkRegNumber) \
            InstList_add(&temp, MOV_r_x(dtMdf, stkReg[stkTop - 1], XMM0), 1), \
            InstList_add(&temp, MOV_r_x(dtMdf, stkReg[stkTop], XMM1), 0), \
            InstList_add(&temp, UCOMI_x_x(dtMdf, XMM1, XMM0), 0); \
        else if ((stkTop) == stkRegNumber + 1) \
            InstList_add(&temp, MOV_r_x(dtMdf, stkReg[stkTop - 1], XMM0), 1), \
            InstList_add(&temp, UCOMI_or_x(dtMdf, clStkDisp(stkTop), RBP, XMM0), 0); \
        else InstList_add(&temp, MOV_or_x(dtMdf, clStkDisp(stkTop - 1), RBP, XMM0), 1), \
            InstList_add(&temp, UCOMI_or_x(dtMdf, clStkDisp(stkTop), RBP, XMM0), 0); \
    } while(0)

#define setFlag0(list, stkPos) \
    do { \
        if ((stkPos) <= stkRegNumber) InstList_add(&list, XOR_r_r(TM_qword, stkReg[stkPos], stkReg[stkPos]), 0); \
        else { \
            InstList_add(&list, XOR_r_r(TM_qword, RAX, RAX), 0); \
            InstList_add(&list, MOV_r_or(TM_qword, RAX, clStkDisp(stkTop), RBP), 0); \
        } \
    } while(0)
#define setFlag1(list, stkPos) \
    do { \
        if ((stkPos) <= stkRegNumber) InstList_add(&list, MOV_i_r(TM_qword, 1, stkReg[stkPos]), 0); \
        else { \
            InstList_add(&list, MOV_i_r(TM_qword, 1, RAX), 0); \
            InstList_add(&list, MOV_r_or(TM_qword, RAX, clStkDisp(stkTop), RBP), 0); \
        } \
    } while(0)

#define setCmpResult(JCCNameSigned, JCCNameUnsigned, isSigned, list, stkPos) \
    do { \
        uint32 jccP, setFlagP, endP; \
        InstList_add(&list, (isSigned ? JCCNameSigned : JCCNameUnsigned)(0), 0); \
        jccP = list.size; \
        setFlag0(list, (stkPos)); \
        InstList_add(&list, JMP_i(0), 0); \
        setFlagP = list.size; \
        *(uint32 *)&list.data[jccP - sizeof(uint32)] = setFlagP - jccP; \
        setFlag1(list, (stkPos)); \
        endP = list.size; \
        *(uint32 *)&list.data[setFlagP - sizeof(uint32)] = endP - setFlagP; \
    } while(0) 

#define VInstTmpl_CmpFunction(vinstName, JCCNameSigned, JCCNameUnsigned) \
VInstTmpl_Function(vinstName, _1, _2) { \
    InstList temp; \
    InstList_init(&temp); \
    if (isInterger(dtMdf)) cmpInteger(dtMdf, stkTop); \
    else if (isFloat(dtMdf)) cmpFloat(dtMdf, stkTop); \
    setCmpResult(JCCNameSigned, JCCNameUnsigned, isSigned, temp, stkTop - 1); \
    InstList_merge(tgList, &temp); \
}

VInstTmpl_CmpFunction(eq, JZ, JZ)
VInstTmpl_CmpFunction(ne, JNZ, JNZ)
VInstTmpl_CmpFunction(ls, JL, JC)
VInstTmpl_CmpFunction(le, JLE, JBE)
VInstTmpl_CmpFunction(gt, JG, JNBE)
VInstTmpl_CmpFunction(ge, JGE, JNC)

#pragma endregion

#pragma region basic variable operation
VInstTmpl_Function(pvar, varId, _2) {
    CreateTemplInstList;
    
    if (dtMdf == TM_float32) dtMdf = TM_dword;
    else if (dtMdf == TM_float64) dtMdf = TM_qword;

    if (stkTop + 1 <= stkRegNumber)
        // 这样可以省去清空计算栈的步骤
        InstList_add(&temp, MOV_or_r(TM_qword, locVarDisp(varId), RBP, stkReg[stkTop + 1]), 1);
    else {
        InstList_add(&temp, MOV_or_r(TM_qword, locVarDisp(varId), RBP, RCX), 1);
        InstList_add(&temp, MOV_r_or(TM_qword, RCX, clStkDisp(stkTop + 1), RBP), 0);
    }
    if (dtMdf == TM_object) {
        uint32 regId = stkTop + 1 <= stkRegNumber ? stkReg[stkTop + 1] : RCX;
        // if stkTop + 1 > stkRegNumber, the value of RCX is already the address of the object
        InstList_add(&temp, CMP_i_r(TM_qword, 0, regId), 0);
        InstList_add(&temp, JZ(0), 0);
        uint32 jzP = temp.size, endP;
        InstList_add(&temp, INC_or(TM_qword, refCntOffset, regId), 0);
        InstList_add(&temp, INC_or(TM_qword, rootRefCntOffset, regId), 0);  
        endP = temp.size;
        *(uint32 *)&temp.data[jzP - 4] = endP - jzP;  
    }
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(setvar, varId, _2) {
    CreateTemplInstList;
    SBool setEntry = false;
    if (dtMdf == TM_object) {
        InstList_add(&temp, MOV_or_r(TM_qword, locVarDisp(varId), RBP, RCX), 1);
        InstList_add(&temp, XOR_r_r(TM_qword, RDX, RDX), 0);
        InstList_add(&temp, CMP_r_r(TM_qword, RDX, RCX), 0);
        InstList_add(&temp, JZ(0), 0);
        uint32 jzP = temp.size, jzP1, end;
        InstList_add(&temp, DEC_or(TM_qword, refCntOffset, RCX), 0);
        InstList_add(&temp, DEC_or(TM_qword, rootRefCntOffset, RCX), 0);
        InstList_add(&temp, CMP_r_or(TM_qword, RDX, refCntOffset, RCX), 0);
        InstList_add(&temp, JNZ(0), 0);
        jzP1 = temp.size;
        prepareCallVMFunc(&temp, 0, stkTop);
        InstList_add(&temp, MOV_i_r(TM_qword, (uint64)refGC, RAX), 0);
        InstList_add(&temp, MOV_r_r(TM_qword, RCX, RDI), 0);
        InstList_add(&temp, CALL(RAX), 0);
        restoreFromCallVMFunc(&temp, 0, stkTop);
        *(uint32 *)(temp.data + jzP - 4) = temp.size - jzP;
        *(uint32 *)(temp.data + jzP1 - 4) = temp.size - jzP1;
        setEntry = true;
    }

    if (dtMdf == TM_float32) dtMdf = TM_dword;
    else if (dtMdf == TM_float64) dtMdf = TM_qword;

    TypeModifier tmp = (dtMdf == TM_object ? TM_qword : dtMdf);
    if (stkTop <= stkRegNumber)
        InstList_add(&temp, MOV_r_or(tmp, stkReg[stkTop], locVarDisp(varId), RBP), !setEntry);
    else {
        InstList_add(&temp, MOV_or_r(tmp, clStkDisp(stkTop), RBP, RCX), !setEntry);
        InstList_add(&temp, MOV_r_or(tmp, RCX, locVarDisp(varId), RBP), 0);
    }
    InstList_merge(tgList, &temp);
}

// call function "getGloAddr" and store the address in RAX
#define Tmpl_GetGloAddr(list, stkTop, varOffset) \
    do { \
        prepareCallVMFunc(&list, 1, stkTop); \
        InstList_add(&list, MOV_or_r(TM_qword, blgBlkDisp, RBP, RDI), (stkTop == 0)); \
        InstList_add(&list, MOV_i_r(TM_qword, varOffset, RSI), 0); \
        InstList_add(&list, MOV_i_r(TM_qword, (uint64)getGloAddr, RAX), 0); \
        InstList_add(&list, CALL(RAX), 0); \
        restoreFromCallVMFunc(&list, 0, stkTop); \
    } while(0) \

VInstTmpl_Function(pglo, varOffset, _2) {
    CreateTemplInstList;
    Tmpl_GetGloAddr(temp, stkTop, varOffset);
    if (dtMdf == TM_float32) dtMdf = TM_dword;
    else if (dtMdf == TM_float64) dtMdf = TM_qword;

    TypeModifier tmp = (dtMdf == TM_object ? TM_qword : dtMdf);
    if (stkTop + 1 <= stkRegNumber) {
        if (!is64Bit(dtMdf)) InstList_add(&temp, XOR_r_r(TM_qword, stkReg[stkTop + 1], stkReg[stkTop + 1]), 0);
        InstList_add(&temp, MOV_mr_r(tmp, RAX, stkReg[stkTop + 1]), 0);
    } else {
        if (!is64Bit(dtMdf)) InstList_add(&temp, XOR_r_r(TM_qword, RCX, RCX), 0);
        InstList_add(&temp, MOV_mr_r(tmp, RAX, RCX), 0);
        InstList_add(&temp, MOV_r_or(tmp, RCX, clStkDisp(stkTop + 1), RBP), 0);
    } 
    if (dtMdf == TM_object) {
        uint32 regId = stkTop + 1 <= stkRegNumber ? stkReg[stkTop + 1] : RCX;
        InstList_add(&temp, CMP_i_r(TM_qword, 0, regId), 0);
        InstList_add(&temp, JZ(0), 0);
        uint32 jzP = temp.size;
        InstList_add(&temp, INC_or(TM_qword, refCntOffset, regId), 0);
        InstList_add(&temp, INC_or(TM_qword, rootRefCntOffset, regId), 0);
        *(int32 *)(temp.data + jzP - 4) = (int32)(temp.size - jzP);
    }
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(setglo, varOffset, _2) {
    CreateTemplInstList;
    Tmpl_GetGloAddr(temp, stkTop, varOffset);
    if (dtMdf == TM_object) {
        InstList_add(&temp, MOV_mr_r(TM_qword, RAX, RCX), 0);
        InstList_add(&temp, CMP_i_r(TM_qword, 0, RCX), 0);
        InstList_add(&temp, JZ(0), 0);
        int32 jzP = temp.size;
        InstList_add(&temp, DEC_or(TM_qword, refCntOffset, RCX), 0);
        InstList_add(&temp, DEC_or(TM_qword, rootRefCntOffset, RCX), 0);
        *(int32 *)(temp.data + jzP - 4) = (int32)(temp.size - jzP);
    }
    if (dtMdf == TM_float32) dtMdf = TM_dword;
    else if (dtMdf == TM_float64) dtMdf = TM_qword;
    TypeModifier tmp = (dtMdf == TM_object ? TM_qword : dtMdf);
    if (stkTop <= stkRegNumber) InstList_add(&temp, MOV_r_mr(tmp, stkReg[stkTop], RAX), 0);
    else InstList_add(&temp, MOV_or_r(tmp, clStkDisp(stkTop), RBP, RCX), 0),
        InstList_add(&temp, MOV_r_mr(tmp, RCX, RAX), 0);
    InstList_merge(tgList, &temp);
}

// RCX -> object, RDX -> the address of the member
#define Tmpl_getMemAddr(stkTop, offset) \
    do { \
        if ((stkTop) <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[(stkTop)], RCX), 1); \
        else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop), RBP, RCX), 1); \
        /* obj.refCount--, obj.rootRefCount--; */ \
        InstList_add(&temp, DEC_or(TM_qword, refCntOffset, RCX), 0); \
        InstList_add(&temp, DEC_or(TM_qword, rootRefCntOffset, RCX), 0); \
        InstList_add(&temp, MOV_or_r(TM_qword, objDataOffset, RCX, RDX), 0); \
        InstList_add(&temp, ADD_i_r(TM_qword, offset, RDX), 0); \
    } while(0) \

VInstTmpl_Function(pmem, offset, _2) {
    CreateTemplInstList;
    Tmpl_getMemAddr(stkTop, offset);

    if (dtMdf == TM_float32) dtMdf = TM_dword;
    else if (dtMdf == TM_float64) dtMdf = TM_qword;

    TypeModifier tmp = (dtMdf == TM_object ? TM_qword : dtMdf);
    if (stkTop <= stkRegNumber) {
        if (!is64Bit(dtMdf)) InstList_add(&temp, XOR_r_r(TM_qword, stkReg[stkTop], stkReg[stkTop]), 0);
        InstList_add(&temp, MOV_mr_r(tmp, RDX, stkReg[stkTop]), 0);
    } else {
        if (!is64Bit(dtMdf)) InstList_add(&temp, XOR_r_r(TM_qword, RAX, RAX), 0);
        InstList_add(&temp, MOV_mr_r(tmp, RDX, RAX), 0);
        InstList_add(&temp, MOV_r_or(tmp, RAX, clStkDisp(stkTop), RBP), 0);
    }
    if (dtMdf == TM_object) {
        uint32 regId = stkTop <= stkRegNumber ? stkReg[stkTop] : RAX;
        InstList_add(&temp, CMP_i_r(TM_qword, 0, regId), 0);
        InstList_add(&temp, JZ(0), 0);
        int32 jzP = temp.size;
        InstList_add(&temp, INC_or(TM_qword, refCntOffset, regId), 0);
        InstList_add(&temp, INC_or(TM_qword, rootRefCntOffset, regId), 0);
        *(int32 *)(temp.data + jzP - 4) = ((int32)temp.size - jzP);
    }
    InstList_merge(tgList, &temp);
}
VInstTmpl_Function(setmem, offset, _2) {
    CreateTemplInstList;
    Tmpl_getMemAddr(stkTop - 1, offset);
    if (dtMdf == TM_object) {
        InstList_add(&temp, PUSH_r(RCX), 0);
        InstList_add(&temp, PUSH_r(RDX), 0);
        prepareCallVMFunc(&temp, 0, stkTop);
        InstList_add(&temp, MOV_r_r(TM_qword, RCX, RDI), 0);
        InstList_add(&temp, MOV_mr_r(TM_qword, RDX, RSI), 0);
        InstList_add(&temp, MOV_i_r(TM_qword, (uint64)disconnectMember, RAX), 0);
        InstList_add(&temp, CALL(RAX), 0);
        restoreFromCallVMFunc(&temp, 0, stkTop);
        InstList_add(&temp, POP_r(RDX), 0);
        InstList_add(&temp, POP_r(RCX), 0);
    }

    if (dtMdf == TM_float32) dtMdf = TM_dword;
    else if (dtMdf == TM_float64) dtMdf = TM_qword;

    TypeModifier tmp = dtMdf == TM_object ? TM_qword : dtMdf;
    if (stkTop <= stkRegNumber) 
        InstList_add(&temp, MOV_r_mr(tmp, stkReg[stkTop], RDX), 0);
    else {
        InstList_add(&temp, MOV_or_r(tmp, clStkDisp(stkTop), RBP, RAX), 0);
        InstList_add(&temp, MOV_r_mr(tmp, RAX, RDX), 0);
    }
    if (dtMdf == TM_object) {
        if (stkTop <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop], RAX), 0);
        prepareCallVMFunc(&temp, 0, stkTop - 2);
        InstList_add(&temp, MOV_r_r(TM_qword, RCX, RDI), 0);
        InstList_add(&temp, MOV_mr_r(TM_qword, RDX, RSI), 0);
        InstList_add(&temp, MOV_i_r(TM_qword, offset, RDX), 0);
        InstList_add(&temp, MOV_i_r(TM_qword, (uint64)connectMember, RAX), 0);
        InstList_add(&temp, CALL(RAX), 0);
        restoreFromCallVMFunc(&temp, 0, stkTop - 2);
    }
    InstList_merge(tgList, &temp);
}
VInstTmpl_Function(parrmem, dimc, _2) {
    CreateTemplInstList;
    // obj(RCX) = (Object *)stkReg[stkTop - dimc]
    if (stkTop - dimc <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop - dimc], RCX), 1);
    else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop - dimc), RBP, RCX), 1);
    // obj.refCount--, obj.rootRefCount--;
    InstList_add(&temp, DEC_or(TM_qword, refCntOffset, RCX), 0);
    InstList_add(&temp, DEC_or(TM_qword, rootRefCntOffset, RCX), 0);
    // obj->data(RCX), index(RDX)
    InstList_add(&temp, MOV_or_r(TM_qword, objDataOffset, RCX, RCX), 0);
    InstList_add(&temp, MOV_i_r(TM_qword, dimc * 8, RBX), 0);
    for (int i = 0; i < dimc; i++) {
        InstList_add(&temp, MOV_or_r(TM_qword, i * 8, RCX, RAX), 0);
        if (stkTop - dimc + i + 1 <= stkRegNumber) InstList_add(&temp, MUL_r(TM_qword, stkReg[stkTop - dimc + i + 1]), 0);
        else InstList_add(&temp, MUL_or(TM_qword, clStkDisp(stkTop - dimc + i + 1), RBP), 0);
        InstList_add(&temp, ADD_r_r(TM_qword, RAX, RBX), 0);
    }
    InstList_add(&temp, ADD_r_r(TM_qword, RBX, RCX), 0);

    if (dtMdf == TM_float32) dtMdf = TM_dword;
    else if (dtMdf == TM_float64) dtMdf = TM_qword;

    TypeModifier tmp = (dtMdf == TM_object ? TM_qword : dtMdf);
    if (stkTop - dimc <= stkRegNumber) {
        if (!is64Bit(dtMdf)) InstList_add(&temp, XOR_r_r(TM_qword, stkReg[stkTop - dimc], stkReg[stkTop - dimc]), 0);
        InstList_add(&temp, MOV_mr_r(tmp, RCX, stkReg[stkTop - dimc]), 0);
    } else {
        if (!is64Bit(dtMdf)) InstList_add(&temp, XOR_r_r(TM_qword, RAX, RAX), 0);
        InstList_add(&temp, MOV_mr_r(tmp, RCX, RAX), 0);
        InstList_add(&temp, MOV_r_or(tmp, RAX, clStkDisp(stkTop - dimc), RBP), 0);
    }

    if (dtMdf == TM_object) {
        uint32 regId = stkTop - dimc <= stkRegNumber ? stkReg[stkTop - dimc] : RAX;
        InstList_add(&temp, CMP_i_r(TM_qword, 0, regId), 0);
        InstList_add(&temp, JZ(0), 0);
        uint32 jzP = temp.size;
        InstList_add(&temp, INC_or(TM_qword, refCntOffset, regId), 0);
        InstList_add(&temp, INC_or(TM_qword, rootRefCntOffset, regId), 0);
        *(int32 *)(temp.data + jzP - 4) = temp.size - jzP;
    }
    InstList_merge(tgList, &temp);
}   
VInstTmpl_Function(setarrmem, dimc, _2) {
    /*
    Object *obj = stk[stkTop - dimc - 1];
    obj->refCount--, obj->rootRefCount--;
    index = 0;
    for (int i = 0; i < dimc; i++) index += *(uint64 *)(obj->data + (i * 8)) * stk[stkTop - dimc + i];
    *(dataType)(obj->data + dimc * 8 + index) = stk[stkTop];
    */
    CreateTemplInstList;
    // obj(RCX) = (Object *)stkReg[stkTop - dimc - 1]
    if (stkTop - dimc - 1 <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop - dimc - 1], RCX), 1);
    else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop - dimc - 1), RBP, RCX), 1);
    // obj.refCount--, obj.rootRefCount--;
    InstList_add(&temp, DEC_or(TM_qword, refCntOffset, RCX), 0);
    InstList_add(&temp, DEC_or(TM_qword, rootRefCntOffset, RCX), 0);
    if (dtMdf == TM_object) InstList_add(&temp, PUSH_r(RCX), 0);
    // obj->data((RDX))
    InstList_add(&temp, MOV_or_r(TM_qword, objDataOffset, RCX, RCX), 0);
    InstList_add(&temp, MOV_i_r(TM_qword, dimc * 8, RBX), 0);
    for (int i = 0; i < dimc; i++) {
        InstList_add(&temp, MOV_or_r(TM_qword, i * 8, RCX, RAX), 0);
        if (stkTop - dimc + i <= stkRegNumber) InstList_add(&temp, MUL_r(TM_qword, stkReg[stkTop - dimc + i]), 0);
        else InstList_add(&temp, MUL_or(TM_qword, clStkDisp(stkTop - dimc + i), RBP), 0);
        InstList_add(&temp, ADD_r_r(TM_qword, RAX, RBX), 0);
    }
    InstList_add(&temp, ADD_r_r(TM_qword, RBX, RCX), 0);

    if (dtMdf == TM_object) {
        InstList_addm(&temp, 3, POP_r(RDX), PUSH_r(RDX), PUSH_r(RCX));
        prepareCallVMFunc(&temp, 0, stkTop);
        InstList_addm(&temp, 4,
            MOV_r_r(TM_qword, RDX, RDI),
            MOV_mr_r(TM_qword, RCX, RSI),
            MOV_i_r(TM_qword, (uint64)disconnectMember, RAX), 
            CALL(RAX));
        restoreFromCallVMFunc(&temp, 0, stkTop);
        InstList_add(&temp, POP_r(RCX), 0);
    }

    if (dtMdf == TM_float32) dtMdf = TM_dword;
    else if (dtMdf == TM_float64) dtMdf = TM_qword;

    if ((isInterger(dtMdf)) || dtMdf == TM_object) {
        TypeModifier tmp = dtMdf == TM_object ? TM_qword : dtMdf;
        if (stkTop <= stkRegNumber) 
            InstList_add(&temp, MOV_r_mr(tmp, stkReg[stkTop], RCX), 0);
        else {
            InstList_add(&temp, MOV_or_r(tmp, clStkDisp(stkTop), RBP, RAX), 0);
            InstList_add(&temp, MOV_r_mr(tmp, RAX, RCX), 0);
        }
    }
    
    if (dtMdf == TM_object) {
        if (stkTop <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop], RAX), 0);
        InstList_add(&temp, POP_r(RDX), 0);
        prepareCallVMFunc(&temp, 0, stkTop - dimc - 2);
        InstList_add(&temp, MOV_r_r(TM_qword, RDX, RDI), 0);
        InstList_add(&temp, MOV_r_r(TM_qword, RAX, RSI), 0);
        InstList_add(&temp, MOV_r_r(TM_qword, RCX, RDX), 0);
        InstList_add(&temp, SUB_or_r(TM_qword, objDataOffset, RDI, RDX), 0);
        InstList_add(&temp, MOV_i_r(TM_qword, (uint64)connectMember, RAX), 0);
        InstList_add(&temp, CALL(RAX), 0);
        restoreFromCallVMFunc(&temp, 0, stkTop - dimc - 2);
    }
    InstList_merge(tgList, &temp);
}

#pragma endregion

#pragma region const operator
VInstTmpl_Function(push, data, _2) {
    CreateTemplInstList;
    if (data == 0) {
        if (stkTop + 1 <= stkRegNumber) InstList_add(&temp, XOR_r_r(TM_qword, stkReg[stkTop + 1], stkReg[stkTop + 1]), 1);
        else InstList_add(&temp, MOV_r_r(TM_qword, RCX, RCX), 1),
            InstList_add(&temp, MOV_r_or(TM_qword, RCX, clStkDisp(stkTop + 1), RBP), 0);
    } else {
        if (stkTop + 1 <= stkRegNumber) InstList_add(&temp, MOV_i_r(TM_qword, data, stkReg[stkTop + 1]), 1);
        else {
            InstList_add(&temp, MOV_i_r(TM_qword, data, RCX), 1);
            InstList_add(&temp, MOV_r_or(TM_qword, RCX, clStkDisp(stkTop + 1), RBP), 0);
        }
    }
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(pop, _1, _2) {
    CreateTemplInstList;
    if (stkTop <= stkRegNumber) InstList_add(&temp, XOR_r_r(TM_qword, stkReg[stkTop], stkReg[stkTop]), 1);
    else {
        InstList_add(&temp, MOV_i_r(TM_qword, 0, RCX), 1);
        InstList_add(&temp, MOV_r_or(TM_qword, RCX, clStkDisp(stkTop), RBP), 0);
    }
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(plabel, data, _2) {
    CreateTemplInstList;
    prepareCallVMFunc(&temp, 1, stkTop);
    InstList_add(&temp, MOV_or_r(TM_qword, blgBlkDisp, RBP, RDI), stkTop == 0);
    InstList_add(&temp, MOV_i_r(TM_qword, (data >> 48), RSI), 0);
    InstList_add(&temp, MOV_i_r(TM_qword, (uint64)getRelyId, RAX), 0);
    InstList_add(&temp, CALL(RAX), 0);
    restoreFromCallVMFunc(&temp, 0, stkTop);
    InstList_add(&temp, SHL_i_r(TM_qword, 48, RAX), 0);
    InstList_add(&temp, MOV_i_r(TM_qword, data & ((1ull << 48) - 1), RCX), 0);
    InstList_add(&temp, OR_r_r(TM_qword, RCX, RAX), 0);
    if (stkTop + 1 <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, RAX, stkReg[stkTop + 1]), 0);
    else InstList_add(&temp, MOV_r_or(TM_qword, RAX, clStkDisp(stkTop + 1), RBP), 0);
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(setflag, data, _2) {
    CreateTemplInstList;
    InstList_add(&temp, MOV_i_r(TM_qword, data, RAX), 1);
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(pushflag, _1, _2) {
    CreateTemplInstList;
    if (stkTop + 1 <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, RAX, stkReg[stkTop + 1]), 1);
    else InstList_add(&temp, MOV_r_or(TM_qword, RAX, clStkDisp(stkTop + 1), RBP), 1);
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(cpy, _1, _2) {
    CreateTemplInstList;
    if (stkTop + 1 <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop], stkReg[stkTop + 1]), 1);
    else if (stkTop == stkRegNumber) InstList_add(&temp, MOV_r_or(TM_qword, stkReg[stkTop], clStkDisp(stkTop + 1), RBP), 1);
    else {
        InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop), RBP, RAX), 1);
        InstList_add(&temp, MOV_r_or(TM_qword, RAX, clStkDisp(stkTop + 1), RBP), 0);
    }
    if (dtMdf == TM_object) {
        uint32 regId = stkTop <= stkRegNumber ? stkReg[stkTop] : RAX;
        InstList_add(&temp, CMP_i_r(TM_qword, 0, regId), 0);
        InstList_add(&temp, JZ(0), 0);
        int32 jzP = temp.size;
        InstList_add(&temp, INC_or(TM_qword, refCntOffset, regId), 0);
        InstList_add(&temp, INC_or(TM_qword, rootRefCntOffset, regId), 0);
        *(int32 *)(temp.data + jzP - 4) = (int32)temp.size - jzP;
    }
    InstList_merge(tgList, &temp);
}
VInstTmpl_Function(pstr, id, _2) {
    CreateTemplInstList;
    InstList_add(&temp, MOV_or_r(TM_qword, blgBlkDisp, RBP, RAX), 1);
    InstList_add(&temp, MOV_or_r(TM_qword, strListOffset, RAX, RAX), 0);
    InstList_add(&temp, MOV_or_r(TM_qword, id * sizeof(uint64), RAX, RAX), 0);
    if (stkTop + 1 <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, RAX, stkReg[stkTop + 1]), 0);
    else InstList_add(&temp, MOV_r_or(TM_qword, RAX, clStkDisp(stkTop + 1), RBP), 0);
    InstList_add(&temp, INC_or(TM_qword, refCntOffset, RAX), 0);
    InstList_add(&temp, INC_or(TM_qword, rootRefCntOffset, RAX), 0);
    InstList_merge(tgList, &temp);
}
#pragma endregion

#pragma region advanced variable operation
#define Tmpl_IntBinarySetvar(instName) \
    do { \
        if (stkTop <= stkRegNumber) InstList_add(&temp, instName##_r_or(dtMdf, stkReg[stkTop], locVarDisp(varId), RBP), 1); \
        else InstList_add(&temp, MOV_or_r(dtMdf, clStkDisp(stkTop), RBP, RAX), 1), \
            InstList_add(&temp, instName##_r_or(dtMdf, RAX, locVarDisp(varId), RBP), 0); \
    } while(0)

#define Tmpl_FloatBinarySetvar(instName) \
    do { \
        InstList_add(&temp, MOV_or_x(dtMdf, locVarDisp(varId), RBP, XMM0), 1); \
        if (stkTop <= stkRegNumber) \
            InstList_add(&temp, MOV_r_x(dtMdf, stkReg[stkTop], XMM1), 0), \
            InstList_add(&temp, instName##_x_x(dtMdf, XMM1, XMM0), 0); \
        else InstList_add(&temp, instName##_or_x(dtMdf, clStkDisp(stkTop), RBP, XMM0), 0); \
        InstList_add(&temp, MOV_x_or(dtMdf, XMM0, locVarDisp(varId), RBP), 0); \
    } while(0)

#pragma region local variable
VInstTmpl_Function(addvar, varId, _2) {
    CreateTemplInstList;
    if (isInterger(dtMdf)) Tmpl_IntBinarySetvar(ADD);
    else if (isFloat(dtMdf)) Tmpl_FloatBinarySetvar(ADD);
    InstList_merge(tgList, &temp);
}
VInstTmpl_Function(subvar, varId, _2) {
    CreateTemplInstList;
    if (isInterger(dtMdf)) Tmpl_IntBinarySetvar(SUB);
    else if (isFloat(dtMdf)) Tmpl_FloatBinarySetvar(SUB);
    InstList_merge(tgList, &temp);
}
VInstTmpl_Function(andvar, varId, _2) {
    CreateTemplInstList;
    if (isInterger(dtMdf)) Tmpl_IntBinarySetvar(AND);
    InstList_merge(tgList, &temp);
}
VInstTmpl_Function(orvar, varId, _2) {
    CreateTemplInstList;
    if (isInterger(dtMdf)) Tmpl_IntBinarySetvar(OR);
    InstList_merge(tgList, &temp);
}
VInstTmpl_Function(xorvar, varId, _2) {
    CreateTemplInstList;
    if (isInterger(dtMdf)) Tmpl_IntBinarySetvar(XOR);
    InstList_merge(tgList, &temp);
}
VInstTmpl_Function(mulvar, varId, _2) {
    CreateTemplInstList;
    if (isInterger(dtMdf)) {
        InstList_add(&temp, XOR_r_r(TM_qword, RAX, RAX), 1);
        if (stkTop <= stkRegNumber) InstList_add(&temp, MOV_r_r(dtMdf, stkReg[stkTop], RAX), 0);
        else InstList_add(&temp, MOV_or_r(dtMdf, clStkDisp(stkTop), varId, RAX), 0);
        InstList_add(&temp, (isSigned ? IMUL_or : MUL_or)(dtMdf, locVarDisp(varId), RBP), 0);
        InstList_add(&temp, MOV_r_or(dtMdf, RAX, locVarDisp(varId), RBP), 0);
    } else if (isFloat(dtMdf)) Tmpl_FloatBinarySetvar(MUL);
    InstList_merge(tgList, &temp);
}
VInstTmpl_Function(divvar, varId, _2) {
    CreateTemplInstList;
    if (isInterger(dtMdf)) {
        InstList_add(&temp, XOR_r_r(TM_qword, RAX, RAX), 1);
        InstList_add(&temp, XOR_r_r(TM_qword, RDX, RDX), 0);
        if (stkTop <= stkRegNumber) InstList_add(&temp, MOV_r_r(dtMdf, stkReg[stkTop], RAX), 0);
        else InstList_add(&temp, MOV_or_r(dtMdf, clStkDisp(stkTop), varId, RAX), 0);
        InstList_add(&temp, (isSigned ? IDIV_or : DIV_or)(dtMdf, locVarDisp(varId), RBP), 0);
        InstList_add(&temp, MOV_r_or(dtMdf, RAX, locVarDisp(varId), RBP), 0);
    } else if (isFloat(dtMdf)) Tmpl_FloatBinarySetvar(DIV);
    InstList_merge(tgList, &temp);
}
VInstTmpl_Function(modvar, varId, _2) {
    CreateTemplInstList;
    if (isInterger(dtMdf)) {
        InstList_add(&temp, XOR_r_r(TM_qword, RAX, RAX), 1);
        InstList_add(&temp, XOR_r_r(TM_qword, RDX, RDX), 0);
        if (dtMdf == TM_none) InstList_add(&temp, XOR_AH_AH(), 0);
        if (stkTop <= stkRegNumber) InstList_add(&temp, MOV_r_r(dtMdf, stkReg[stkTop], RAX), 0);
        else InstList_add(&temp, MOV_or_r(dtMdf, clStkDisp(stkTop), varId, RAX), 0);
        InstList_add(&temp, (isSigned ? IDIV_or : DIV_or)(dtMdf, locVarDisp(varId), RBP), 0);
        if (dtMdf == TM_none) InstList_add(&temp, MOV_AH_DL(), 0);
        InstList_add(&temp, MOV_r_or(dtMdf, RDX, locVarDisp(varId), RBP), 0);
    }
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(shlvar, varId, _2) {
    CreateTemplInstList;
    SBool isFir = true;
    if (stkTop <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop], RCX), isFir);
    else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop), varId, RCX), isFir);
    InstList_add(&temp, (isSigned ? SAL_CL_or : SHL_CL_or)(dtMdf, locVarDisp(varId), RBP), 0);
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(shrvar, varId, _2) {
    CreateTemplInstList;
    SBool isFir = true;
    if (stkTop <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop], RCX), isFir);
    else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop), varId, RCX), isFir);
    InstList_add(&temp, (isSigned ? SAR_CL_or : SHR_CL_or)(dtMdf,locVarDisp(varId), RBP), 0);
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(sincvar, varId, _2) {
    CreateTemplInstList;
    if (isInterger(dtMdf)) {
        if (stkTop + 1 <= stkRegNumber) InstList_add(&temp, MOV_or_r(dtMdf, locVarDisp(varId), RBP, stkReg[stkTop + 1]), 1);
        else {
            InstList_add(&temp, MOV_or_r(dtMdf, locVarDisp(varId), RBP, RCX), 1);
            InstList_add(&temp, MOV_r_or(dtMdf, RCX, clStkDisp(stkTop + 1), RBP), 0);
        }
        InstList_add(&temp, INC_or(dtMdf, locVarDisp(varId), RBP), 0);
    }
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(sdecvar, varId, _2) {
    CreateTemplInstList;
    if (isInterger(dtMdf)) {
        if (stkTop + 1 <= stkRegNumber) InstList_add(&temp, MOV_or_r(dtMdf, locVarDisp(varId), RBP, stkReg[stkTop + 1]), 1);
        else {
            InstList_add(&temp, MOV_or_r(dtMdf, locVarDisp(varId), RBP, RCX), 1);
            InstList_add(&temp, MOV_r_or(dtMdf, RCX, clStkDisp(stkTop + 1), RBP), 0);
        }
        InstList_add(&temp, DEC_or(dtMdf, locVarDisp(varId), RBP), 0);
    }
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(pincvar, varId, _2) {
   CreateTemplInstList;
    if (isInterger(dtMdf)) {
        InstList_add(&temp, INC_or(dtMdf, locVarDisp(varId), RBP), 1);
        if (stkTop + 1 <= stkRegNumber) InstList_add(&temp, MOV_or_r(dtMdf, locVarDisp(varId), RBP, stkReg[stkTop + 1]), 0);
        else {
            InstList_add(&temp, MOV_or_r(dtMdf, locVarDisp(varId), RBP, RCX), 0);
            InstList_add(&temp, MOV_r_or(dtMdf, RCX, clStkDisp(stkTop + 1), RBP), 0);
        }
    }
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(pdecvar, varId, _2) {
    CreateTemplInstList;
    InstList_add(&temp, MOV_or_r(dtMdf, locVarDisp(varId), RBP, RCX), 0);
    if (isInterger(dtMdf)) {
        InstList_add(&temp, DEC_or(dtMdf, locVarDisp(varId), RBP), 1);
        if (stkTop + 1 <= stkRegNumber) InstList_add(&temp, MOV_or_r(dtMdf, locVarDisp(varId), RBP, stkReg[stkTop + 1]), 0);
        else {
            InstList_add(&temp, MOV_or_r(dtMdf, locVarDisp(varId), RBP, RCX), 0);
            InstList_add(&temp, MOV_r_or(dtMdf, RCX, clStkDisp(stkTop + 1), RBP), 0);
        }
    }
    InstList_merge(tgList, &temp);
}
#pragma endregion

#pragma region global variable
#define Tmpl_IntBinaryAddr(instName) \
    do { \
        if (stkTop <= stkRegNumber) InstList_add(&temp, instName##_r_mr(dtMdf, stkReg[stkTop], RAX), 0); \
        else InstList_add(&temp, MOV_or_r(dtMdf, clStkDisp(stkTop), RBP, RCX), 0), \
            InstList_add(&temp, instName##_r_mr(dtMdf, RCX, RAX), 0); \
    } while (0) 

#define Tmpl_FloatBinaryAddr(instName) \
    do { \
        InstList_add(&temp, MOV_mr_x(dtMdf, RAX, XMM0), 0); \
        if (stkTop <= stkRegNumber) \
            InstList_add(&temp, MOV_r_x(dtMdf, stkReg[stkTop], XMM1), 0), \
            InstList_add(&temp, instName##_x_x(dtMdf, XMM1, XMM0), 0); \
        else InstList_add(&temp, instName##_or_x(dtMdf, clStkDisp(stkTop), RBP, XMM0), 0); \
        InstList_add(&temp, MOV_x_mr(dtMdf, XMM0, RAX), 0); \
    } while(0) \
// stkBtm_SelfChange represents the Position of the basic information for getting the address
#define VInstTmpl_AdvanceSetAddr(field, argName, stkBtm_SelfChange, Tmpl_GetAddr, Tmpl_GetAddr_SelfChange) \
VInstTmpl_Function(add##field, argName, _2) { \
    CreateTemplInstList; \
    Tmpl_GetAddr; \
    if (isInterger(dtMdf)) Tmpl_IntBinaryAddr(ADD); \
    else Tmpl_FloatBinaryAddr(ADD); \
    InstList_merge(tgList, &temp); \
} \
VInstTmpl_Function(sub##field, argName, _2) { \
    CreateTemplInstList; \
    Tmpl_GetAddr; \
    if (isInterger(dtMdf)) Tmpl_IntBinaryAddr(SUB); \
    else Tmpl_FloatBinaryAddr(SUB); \
    InstList_merge(tgList, &temp); \
} \
VInstTmpl_Function(and##field, argName, _2) { \
    CreateTemplInstList; \
    Tmpl_GetAddr; \
    Tmpl_IntBinaryAddr(AND); \
    InstList_merge(tgList, &temp); \
} \
VInstTmpl_Function(or##field, argName, _2) { \
    CreateTemplInstList; \
    Tmpl_GetAddr; \
    Tmpl_IntBinaryAddr(OR); \
    InstList_merge(tgList, &temp); \
} \
VInstTmpl_Function(xor##field, argName, _2) { \
    CreateTemplInstList; \
    Tmpl_GetAddr; \
    Tmpl_IntBinaryAddr(XOR); \
    InstList_merge(tgList, &temp); \
} \
VInstTmpl_Function(mul##field, argName, _2) { \
    CreateTemplInstList; \
    Tmpl_GetAddr; \
    if (isInterger(dtMdf)) { \
        InstList_add(&temp, MOV_r_r(TM_qword, RAX, RCX), 0); \
        InstList_add(&temp, MOV_mr_r(dtMdf, RCX, RAX), 0); \
        if (stkTop <= stkRegNumber) InstList_add(&temp, (isSigned ? IMUL_r : MUL_r)(dtMdf, stkReg[stkTop]), 0); \
        else InstList_add(&temp, (isSigned ? IMUL_or : MUL_or)(dtMdf, clStkDisp(stkTop), RBP), 0); \
        InstList_add(&temp, MOV_r_mr(dtMdf, RAX, RCX), 0); \
    } else Tmpl_FloatBinaryAddr(MUL); \
    InstList_merge(tgList, &temp); \
} \
VInstTmpl_Function(div##field, argName, _2) { \
    CreateTemplInstList; \
    Tmpl_GetAddr; \
    if (isInterger(dtMdf)) { \
        InstList_add(&temp, MOV_r_r(TM_qword, RAX, RCX), 0); \
        InstList_add(&temp, MOV_mr_r(dtMdf, RCX, RAX), 0); \
        InstList_add(&temp, XOR_r_r(TM_qword, RDX, RDX), 0); \
        if (stkTop <= stkRegNumber) InstList_add(&temp, (isSigned ? IDIV_r : DIV_r)(dtMdf, stkReg[stkTop]), 0); \
        else InstList_add(&temp, (isSigned ? IDIV_or : DIV_or)(dtMdf, clStkDisp(stkTop), RBP), 0); \
        InstList_add(&temp, MOV_r_mr(dtMdf, RAX, RCX), 0); \
    } else Tmpl_FloatBinaryAddr(DIV); \
    InstList_merge(tgList, &temp); \
} \
VInstTmpl_Function(mod##field, argName, _2) { \
    CreateTemplInstList; \
    Tmpl_GetAddr; \
    InstList_add(&temp, MOV_r_r(TM_qword, RAX, RCX), 0); \
    InstList_add(&temp, MOV_mr_r(dtMdf, RCX, RAX), 0); \
    InstList_add(&temp, XOR_r_r(TM_qword, RDX, RDX), 0); \
    if (dtMdf == TM_none) InstList_add(&temp, XOR_AH_AH(), 0); \
    if (stkTop <= stkRegNumber) InstList_add(&temp, (isSigned ? IDIV_r : DIV_r)(dtMdf, stkReg[stkTop]), 0); \
    else InstList_add(&temp, (isSigned ? IDIV_or : DIV_or)(dtMdf, clStkDisp(stkTop), RBP), 0); \
    if (dtMdf == TM_none) InstList_add(&temp, MOV_AH_DL(), 0); \
    InstList_add(&temp, MOV_r_mr(dtMdf, RDX, RCX), 0); \
    InstList_merge(tgList, &temp); \
} \
VInstTmpl_Function(shl##field, argName, _2) { \
    CreateTemplInstList; \
    Tmpl_GetAddr; \
    if (stkTop <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop], RCX), 0); \
    else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop), RBP, RCX), 0); \
    InstList_add(&temp, (isSigned ? SAL_CL_mr : SHL_CL_mr)(dtMdf, RAX), 0); \
    InstList_merge(tgList, &temp); \
} \
VInstTmpl_Function(shr##field, argName, _2) { \
    CreateTemplInstList; \
    Tmpl_GetAddr; \
    if (stkTop <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop], RCX), 0); \
    else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop), RBP, RCX), 0); \
    InstList_merge(tgList, &temp); \
} \
VInstTmpl_Function(sinc##field, argName, _2) { \
    CreateTemplInstList; \
    Tmpl_GetAddr_SelfChange; \
    if ((stkBtm_SelfChange) <= stkRegNumber) { \
        if (!is64Bit(dtMdf)) InstList_add(&temp, XOR_r_r(TM_qword, stkReg[(stkBtm_SelfChange)], stkReg[(stkBtm_SelfChange)]), 0); \
        InstList_add(&temp, MOV_mr_r(dtMdf, RAX, stkReg[(stkBtm_SelfChange)]), 0); \
    } else { \
        if (!is64Bit(dtMdf)) InstList_add(&temp, XOR_r_r(TM_qword, RCX, RCX), 0); \
        InstList_add(&temp, MOV_mr_r(dtMdf, RAX, RCX), 0), \
        InstList_add(&temp, MOV_r_or(dtMdf, RCX, clStkDisp((stkBtm_SelfChange)), RBP), 0); \
    } \
    InstList_add(&temp, INC_mr(dtMdf, RAX), 0); \
    InstList_merge(tgList, &temp); \
} \
VInstTmpl_Function(sdec##field, argName, _2) { \
    CreateTemplInstList; \
    Tmpl_GetAddr_SelfChange; \
    if ((stkBtm_SelfChange) <= stkRegNumber) { \
        if (!is64Bit(dtMdf)) InstList_add(&temp, XOR_r_r(TM_qword, stkReg[(stkBtm_SelfChange)], stkReg[(stkBtm_SelfChange)]), 0); \
        InstList_add(&temp, MOV_mr_r(dtMdf, RAX, stkReg[(stkBtm_SelfChange)]), 0); \
    } else { \
        if (!is64Bit(dtMdf)) InstList_add(&temp, XOR_r_r(TM_qword, RCX, RCX), 0); \
        InstList_add(&temp, MOV_mr_r(dtMdf, RAX, RCX), 0); \
        InstList_add(&temp, MOV_r_or(dtMdf, RCX, clStkDisp((stkBtm_SelfChange)), RBP), 0); \
    } \
    InstList_add(&temp, DEC_mr(dtMdf, RAX), 0); \
    InstList_merge(tgList, &temp); \
} \
VInstTmpl_Function(pinc##field, argName, _2) { \
    CreateTemplInstList; \
    Tmpl_GetAddr_SelfChange; \
    InstList_add(&temp, INC_mr(dtMdf, RAX), 0); \
    if ((stkBtm_SelfChange) <= stkRegNumber) { \
        if (!is64Bit(dtMdf)) InstList_add(&temp, XOR_r_r(TM_qword, stkReg[(stkBtm_SelfChange)], stkReg[(stkBtm_SelfChange)]), 0); \
        InstList_add(&temp, MOV_mr_r(dtMdf, RAX, stkReg[(stkBtm_SelfChange)]), 0); \
    } else { \
        if (!is64Bit(dtMdf)) InstList_add(&temp, XOR_r_r(TM_qword, RCX, RCX), 0); \
        InstList_add(&temp, MOV_mr_r(dtMdf, RAX, RCX), 0), \
        InstList_add(&temp, MOV_r_or(dtMdf, RCX, clStkDisp((stkBtm_SelfChange)), RBP), 0); \
    } \
    InstList_merge(tgList, &temp); \
} \
VInstTmpl_Function(pdec##field, argName, _2) { \
    CreateTemplInstList; \
    Tmpl_GetAddr_SelfChange; \
    InstList_add(&temp, DEC_mr(dtMdf, RAX), 0); \
    if ((stkBtm_SelfChange) <= stkRegNumber) { \
        if (!is64Bit(dtMdf)) InstList_add(&temp, XOR_r_r(TM_qword, stkReg[(stkBtm_SelfChange)], stkReg[(stkBtm_SelfChange)]), 0); \
        InstList_add(&temp, MOV_mr_r(dtMdf, RAX, stkReg[(stkBtm_SelfChange)]), 0); \
    } else { \
        if (!is64Bit(dtMdf)) InstList_add(&temp, XOR_r_r(TM_qword, RCX, RCX), 0); \
        InstList_add(&temp, MOV_mr_r(dtMdf, RAX, RCX), 0), \
        InstList_add(&temp, MOV_r_or(dtMdf, RCX, clStkDisp((stkBtm_SelfChange)), RBP), 0); \
    } \
    InstList_merge(tgList, &temp); \
}

// there is no basic information in the stack for getting the address
VInstTmpl_AdvanceSetAddr(glo, varOffset, stkTop + 1, Tmpl_GetGloAddr(temp, stkTop, varOffset), Tmpl_GetGloAddr(temp, stkTop, varOffset))

#pragma endregion

#pragma region member variable
#define Tmpl_GetMemAddr_Advance(stkTop, offset) \
    do { \
        if ((stkTop) <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop], RAX), 1); \
        else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop), RBP, RAX), 1); \
        InstList_add(&temp, DEC_or(TM_qword, refCntOffset, RAX), 0); \
        InstList_add(&temp, DEC_or(TM_qword, rootRefCntOffset, RAX), 0); \
        InstList_add(&temp, MOV_or_r(TM_qword, objDataOffset, RAX, RAX), 0); \
        InstList_add(&temp, ADD_i_r(TM_qword, offset, RAX), 0); \
    } while(0) \

// there is an address of the object as the stack for getting the address of the member
VInstTmpl_AdvanceSetAddr(mem, varOffset, stkTop, Tmpl_GetMemAddr_Advance(stkTop - 1, varOffset), Tmpl_GetMemAddr_Advance(stkTop, varOffset))
#pragma endregion

#pragma region array member variable
#define Tmpl_GetArrmemAddr_Advance(stkTop, dimc) \
    do { \
        if ((stkTop) - (dimc) <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[(stkTop) - (dimc)], RAX), 1); \
        else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp((stkTop) - (dimc)), RBP, RAX), 1); \
        InstList_add(&temp, DEC_or(TM_qword, refCntOffset, RAX), 0); \
        InstList_add(&temp, DEC_or(TM_qword, rootRefCntOffset, RAX), 0); \
        InstList_add(&temp, MOV_or_r(TM_qword, objDataOffset, RAX, RCX), 0); \
        InstList_add(&temp, XOR_r_r(TM_qword, RBX, RBX), 0); \
        for (int i = 0; i < dimc; i++) { \
            int regId = (stkTop) - (dimc) + i + 1; \
            if (regId <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[regId], RAX), 0); \
            else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(regId), RBP, RAX), 0); \
            InstList_add(&temp, MUL_or(TM_qword, i * sizeof(uint64), RCX), 0); \
            InstList_add(&temp, ADD_r_r(TM_qword, RAX, RBX), 0); \
        } \
        InstList_add(&temp, MOV_r_r(TM_qword, RCX, RAX), 0); \
        InstList_add(&temp, ADD_r_r(TM_qword, RBX, RAX), 0); \
        InstList_add(&temp, ADD_i_r(TM_qword, dimc * sizeof(uint64), RAX), 0); \
    } while(0)

VInstTmpl_AdvanceSetAddr(arrmem, dimc, stkTop - dimc, Tmpl_GetArrmemAddr_Advance(stkTop - 1, dimc), Tmpl_GetArrmemAddr_Advance(stkTop, dimc))
#pragma endregion

#pragma endregion

#pragma region jump operator
int32 tempData;
VInstTmpl_Function(jmp, off, _2) {
    DArray_push(jmpList, &tgList->size);
    DArray_push(jmpDstOffset, &off);
    InstList_add(tgList, JMP_i(0), 1);
}

VInstTmpl_Function(jz, off, _2) {
    if (stkTop <= stkRegNumber) InstList_add(tgList, CMP_i_r(TM_qword, 0, stkReg[stkTop]), 1);
    else {
        InstList_add(tgList, MOV_or_r(TM_qword, clStkDisp(stkTop), RBP, RCX), 1);
        InstList_add(tgList, CMP_i_r(TM_qword, 0, RCX), 0);
    }
    DArray_push(jmpList, &tgList->size);
    DArray_push(jmpDstOffset, &off);
    InstList_add(tgList, JZ(0), 0);
}
VInstTmpl_Function(jp, off, _2)  {
    if (stkTop <= stkRegNumber) InstList_add(tgList, CMP_i_r(TM_qword, 0, stkReg[stkTop]), 1);
    else {
        InstList_add(tgList, MOV_or_r(TM_qword, clStkDisp(stkTop), RBP, RCX), 1);
        InstList_add(tgList, CMP_i_r(TM_qword, 0, RCX), 0);
    }
    DArray_push(jmpList, &tgList->size);
    DArray_push(jmpDstOffset, &off);
    InstList_add(tgList, JNZ(0), 0);
}
#pragma endregion

#pragma region function operator

VInstTmpl_Function(vcall, _1, _2) {
    CreateTemplInstList;
    prepareCallVMFunc(&temp, 1, stkTop - 1);
    if (stkTop <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop], RDI), stkTop == 1);
    else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop), RBP, RDI), stkTop == 1);
    InstList_addm(&temp, 2,
        MOV_i_r(TM_qword, (uint64)callFunc, RAX),
        CALL(RAX));
    restoreFromCallVMFunc(&temp, 0, stkTop - 1);
    if (stkTop <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, RAX, stkReg[stkTop]), 0);
    else InstList_add(&temp, MOV_or_r(TM_qword, RAX, clStkDisp(stkTop), RBP), 0);
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(call, data, _2) {
    CreateTemplInstList;
    uint64 blkId = (data >> 48), offset = (data & ((1ull << 48) - 1));
    /*
    if (curFrm->blgBlk->relyBlk[blkId] == NULL)
        curFrm->blgBlk->relyBlk[blkId] = loadRuntimeBlock(curFrm->blgBlk->relyList[blkId]);
    RuntimeBlock *rblk = curFrm->blgBlk->relyBlk[blkId];
    (uint64 (*)())(rblk->entryList[offset])();
    */
    InstList_add(&temp, MOV_or_r(TM_qword, blgBlkDisp, RBP, RCX), 1);
    InstList_add(&temp, MOV_or_r(TM_qword, relyBlkOffset, RCX, RDX), 0);
    InstList_add(&temp, MOV_or_r(TM_qword, blkId * 8, RDX, RAX), 0);
    if (blkId > 0) {
        InstList_add(&temp, CMP_i_r(TM_qword, 0, RAX), 0);
        InstList_add(&temp, JNZ(0), 0);
        uint32 jnzP = temp.size;
        prepareCallVMFunc(&temp, false, stkTop);
        InstList_addm(&temp, 6,
            PUSH_r(RDX),
            MOV_or_r(TM_qword, relyListOffset, RCX, RDI),
            MOV_or_r(TM_qword, blkId * 8, RDI, RDI),
            MOV_i_r(TM_qword, (uint64)loadRuntimeBlock, RAX),
            CALL(RAX),
            POP_r(RDX));
        restoreFromCallVMFunc(&temp, false, stkTop);
        uint32 end = temp.size;
        *(int32 *)(temp.data + jnzP - 4) = end - jnzP;
        InstList_add(&temp, MOV_r_or(TM_qword, RAX, blkId * 8, RDX), 0);
    }
    prepareCallVMFunc(&temp, 0, stkTop);
    InstList_addm(&temp, 5,
        MOV_r_r(TM_qword, RAX, RDI),
        MOV_or_r(TM_qword, entryListOffset, RAX, RAX),
        MOV_i_r(TM_qword, offset, RCX),
        MOV_rrs_r(TM_qword, RAX, RCX, 8, RAX),
        CALL(RAX));
    restoreFromCallVMFunc(&temp, 0, stkTop);
    // get the return value from the function and then push it to the calculate stack:
    if (stkTop + 1 <= stkRegNumber) 
        InstList_add(&temp, MOV_r_r(TM_qword, RAX, stkReg[stkTop + 1]), 0);
    else InstList_add(&temp, MOV_r_or(TM_qword, RAX, clStkDisp(stkTop + 1), RBP), 0);
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(ret, _1, _2) {
    CreateTemplInstList;
    InstList_add(&temp, XOR_r_r(TM_qword, RAX, RAX), 1);
    InstList_addm(&temp, 2, LEAVE(), RET());
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(vret, _1, _2) {
    CreateTemplInstList;
    if (stkTop <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop], RAX), 1);
    else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop), RBP, RAX), 1);
    InstList_addm(&temp, 2, LEAVE(), RET());
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(getarg, argCnt, _2) {
    CreateTemplInstList;
    InstList_add(&temp, MOV_i_r(TM_qword, (uint64)&paramData, RAX), 1);
    for (int i = 0; i < argCnt; i++)
        InstList_add(&temp, MOV_or_r(TM_qword, i * 8, RAX, RCX), 0),
        InstList_add(&temp, MOV_r_or(TM_qword, RCX, locVarDisp(i), RBP), 0);
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(setarg, argCnt, _2) {
    CreateTemplInstList;
    InstList_add(&temp, MOV_i_r(TM_qword, (uint64)&paramData, RAX), 1);
    for (int i = 0; i < argCnt; i++)
        if (stkTop - argCnt + i + 1 <= stkRegNumber) 
            InstList_add(&temp, MOV_r_or(TM_qword, stkReg[stkTop - argCnt + i + 1], i * 8, RAX), 0);
        else {
            InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop - argCnt + i + 1), RBP, RCX), 0);
            InstList_add(&temp, MOV_r_or(TM_qword, RCX, i * 8, RAX), 0);
        }
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(setlocal, varCnt, _2) {
    CreateTemplInstList;
    // InstList_add(&temp, ENDBR64(), 1);
    InstList_add(&temp, PUSH_r(RBP), 1);
    InstList_addm(&temp, 7,
        MOV_r_r(TM_qword, RSP, RBP),
        SUB_i_r(TM_qword, (varCnt + stkSize - stkRegNumber) * 8 + 16, RSP),
        MOV_r_or(TM_qword, RDI, blgBlkDisp, RBP),
        MOV_i_r(TM_qword, (uint64)gtable, RAX),
        MOV_mr_r(TM_qword, RAX, RAX),
        MOV_r_or(TM_qword, RAX, gtableDisp(0), RBP),
        XOR_r_r(TM_qword, RAX, RAX));
    for (int i = 0; i < varCnt; i++) InstList_add(&temp, MOV_r_or(TM_qword, RAX, locVarDisp(i), RBP), 0); 
    InstList_merge(tgList, &temp);
}

#pragma endregion

#pragma region object allocation
VInstTmpl_Function(newobj, size, _2) {
    CreateTemplInstList;
    prepareCallVMFunc(&temp, 1, stkTop);
    InstList_add(&temp, MOV_i_r(TM_qword, size, RDI), stkTop == 0);
    InstList_addm(&temp, 2, MOV_i_r(TM_qword, (uint64)newObject, RAX), CALL(RAX));
    restoreFromCallVMFunc(&temp, 0, stkTop);
    if (stkTop + 1 <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, RAX, stkReg[stkTop + 1]), 0);
    else InstList_add(&temp, MOV_r_or(TM_qword, RAX, clStkDisp(stkTop + 1), RBP), 0);
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(newarr, dimc, _2) {
    /*
    for (int i = 0; i <= dimc; i++) tempData[i] = stk[stkTop - dimc + i];
    for (int i = 1; i <= dimc; i++) tempData[i] *= tempData[i - 1];
    Object *obj = newObject(tempData[i] + dimc * sizeof(uint64));
    obj->refCount++, obj->rootRefCount++;
    for (int i = 0; i < dimc; i++) *(uint64 *)(obj->data + i * sizeof(uint64)) = tempData[i];
    stk[stkTop - dimc + i] = obj;
    */
    CreateTemplInstList;
    if (stkTop - dimc <= stkRegNumber)
        InstList_add(&temp, MOV_r_r(TM_qword, stkReg[stkTop - dimc], RAX), 1);
    else 
        InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(stkTop - dimc), RBP, RAX), 1);
    for (int i = 0; i < dimc; i++) {
        uint32 regId = stkTop - dimc + i + 1;
        if (regId <= stkRegNumber) 
            InstList_add(&temp, MUL_r(TM_qword, stkReg[regId]), 0),
            InstList_add(&temp, MOV_r_r(TM_qword, RAX, stkReg[regId]), 0);
        else 
            InstList_add(&temp, MUL_or(TM_qword, clStkDisp(regId), RBP), 0),
            InstList_add(&temp, MOV_r_or(TM_qword, RAX, clStkDisp(regId), RBP), 0);
    }
    prepareCallVMFunc(&temp, 0, stkTop - 1);
    InstList_addm(&temp, 4,
        ADD_i_r(TM_qword, sizeof(uint64) * dimc, RAX),
        MOV_r_r(TM_qword, RAX, RDI),
        MOV_i_r(TM_qword, (uint64)newObject, RAX),
        CALL(RAX));
    restoreFromCallVMFunc(&temp, 0, stkTop - 1);
    InstList_add(&temp, MOV_or_r(TM_qword, objDataOffset, RAX, RCX), 0);
    for (int i = 0; i < dimc; i++) {
        uint32 regId = stkTop - dimc + i;
        if (regId <= stkRegNumber)
            InstList_add(&temp, MOV_r_or(TM_qword, stkReg[regId], i * 8, RCX), 0);
        else 
            InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(regId), RBP, RDX), 0),
            InstList_add(&temp, MOV_r_or(TM_qword, RDX, i * 8, RCX), 0);
    }
    if (stkTop - dimc <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, RAX, stkReg[stkTop - dimc]), 0);
    else InstList_add(&temp, MOV_r_or(TM_qword, RAX, clStkDisp(stkTop - dimc), RBP), 0);
    InstList_merge(tgList, &temp);
}

#pragma endregion

#pragma region gtable support
VInstTmpl_Function(setgtbl, cnt, _2) {
    CreateTemplInstList;
    InstList_add(&temp, MOV_i_r(TM_qword, (uint64)&gtable, RAX), 0);
    for (int i = 0; i < cnt; i++) {
        int regId = stkTop - cnt + i + 1;
        if (regId <= stkRegNumber) InstList_add(&temp, MOV_r_or(TM_none, stkReg[regId], sizeof(uint8) * i, RAX), 0);
        else
            InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(regId), RBP, RCX), 0),
            InstList_add(&temp, MOV_r_or(TM_none, RCX, sizeof(uint8) * i, RAX), 0);
    }
    InstList_merge(tgList, &temp);
}
VInstTmpl_Function(setclgtbl, offset, cnt) {
    CreateTemplInstList;
    InstList_add(&temp, MOV_or_r(TM_qword, locVarDisp(0), RBP, RAX), 1);
    InstList_addm(&temp, 3, 
        MOV_or_r(TM_qword, objDataOffset, RAX, RAX),
        MOV_or_r(TM_qword, offset, RAX, RCX),
        MOV_r_or(TM_qword, RCX, gtableDisp(0), RBP));
    InstList_merge(tgList, &temp);
}
VInstTmpl_Function(getgtbl, id, _2) {
    CreateTemplInstList;
    if (stkTop + 1 <= stkRegNumber) {
        InstList_add(&temp, XOR_r_r(TM_qword, stkReg[stkTop + 1], stkReg[stkTop + 1]), 1);
        InstList_add(&temp, MOV_or_r(TM_none, gtableDisp(id), RBP, stkReg[stkTop + 1]), 0);
    } else {
        InstList_add(&temp, XOR_r_r(TM_qword, RAX, RAX), 1);
        InstList_add(&temp, MOV_or_r(TM_none, gtableDisp(id), RBP, RAX), 0);
        InstList_add(&temp, MOV_r_or(TM_qword, RAX, clStkDisp(stkTop + 1), RBP), 0);
    }
    InstList_merge(tgList, &temp);
}
VInstTmpl_Function(getgtblsz, id, _2) {
    CreateTemplInstList;
    InstList_add(&temp, XOR_r_r(TM_qword, RCX, RCX), 1);
    InstList_add(&temp, MOV_or_r(TM_none, gtableDisp(id), RBP, RCX), 0);
    InstList_add(&temp, MOV_i_r(TM_qword, (uint64)gtablesz, RAX), 0);
    if (stkTop + 1 <= stkRegNumber)
        InstList_add(&temp, MOV_rrs_r(TM_qword, RAX, RCX, 8, stkReg[stkTop + 1]), 0);
    else InstList_add(&temp, MOV_rrs_r(TM_qword, RAX, RCX, 8, RDX), 0),
        InstList_add(&temp, MOV_r_or(TM_qword, RDX, clStkDisp(stkTop + 1), RBP), 0);
    InstList_merge(tgList, &temp);
}
#pragma endregion

VInstTmpl_Function(sys, id, _2) {
    CreateTemplInstList;
    prepareCallVMFunc(&temp, 1, stkTop - syscallArgCnt[id]);
    for (int i = 0; i < syscallArgCnt[id]; i++) {
        int regId = stkTop - syscallArgCnt[id] + i + 1;
        if (regId <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, stkReg[regId], argReg[i]), i == 0 && stkTop - syscallArgCnt[id] == 0);
        else InstList_add(&temp, MOV_or_r(TM_qword, clStkDisp(regId), RBP, argReg[i]), i == 0 && stkTop - syscallArgCnt[id] == 0);
    }
    InstList_add(&temp, MOV_i_r(TM_qword, (uint64)syscall[id], RAX), syscallArgCnt[id] == 0 && stkTop == 0);
    InstList_add(&temp, CALL(RAX), 0);
    restoreFromCallVMFunc(&temp, 0, stkTop - syscallArgCnt[id]);
    {
        int regId = stkTop - syscallArgCnt[id] + 1;
        if (regId <= stkRegNumber) InstList_add(&temp, MOV_r_r(TM_qword, RAX, stkReg[regId]), 0);
        else InstList_add(&temp, MOV_r_or(TM_qword, RAX, clStkDisp(regId), RBP), 0);
    }
    InstList_merge(tgList, &temp);
}

VInstTmpl_Function(setpause, _1, _2) {
    setPause(tgList);
}
SBool isSigned(uint32 dtMdf) { return (dtMdf >= i8 && dtMdf <= u64) ? !(dtMdf & 1) : false; }

#define defCase(vinstName, stkTopChg) \
    case vinstName: \
        VInstTmpl_##vinstName(tMdf1, isSigned(dtMdf1), curStkTop, arg1, arg2), curStkTop += (stkTopChg); \
        break;
#define defGCase(vinstName, stkTopChg) \
    case vinstName: \
        GVInstTmpl_##vinstName(dtMdf1 - gv0, dtMdf2 - gv0, curStkTop, arg1, arg2), curStkTop += (stkTopChg); \
        break;

void addVInst(uint64 offset, uint32 vinst, uint32 dtMdf1, uint32 dtMdf2, uint64 arg1, uint64 arg2) {
    DArray_push(offList, (uint8 *)&offset);
    DArray_push(entryList, (uint8 *)&tgList->entryCount);
    TypeModifier tMdf1 = getTypeModfier(dtMdf1), tMdf2 = getTypeModfier(dtMdf2);
    if (isGeneric(tMdf1) || isGeneric(tMdf2)) {
        switch (vinst) {
            defGCase(pvar,      +1);
            defGCase(setvar,    -1);
            defGCase(pmem,      +0);
            defGCase(setmem,    -2);
            defGCase(parrmem,   -arg1);
            defGCase(setarrmem, -(arg1 + 2));
            default: assert(1 == 0); break;
        }
        return ;
    }
    switch (vinst) {
        defCase(setlocal,   0);
        defCase(add,        -1)
        defCase(sub,        -1);
        defCase(mul,        -1);
        defCase(_div,       -1);
        defCase(mod,        -1);
        defCase(_and,       -1);
        defCase(_or,        -1);
        defCase(_xor,       -1);
        defCase(_not,       0);
        defCase(shl,        -1);
        defCase(shr,        -1);
        defCase(setvar,     -1);
        defCase(pvar,       +1);
        defCase(setglo,     -1);
        defCase(pglo,       +1);
        defCase(setmem,     -2);
        defCase(pmem,       0);
        defCase(parrmem,    -arg1);
        defCase(setarrmem,  -(arg1 + 2));
        defCase(push,       +1);
        defCase(plabel,     +1);
        defCase(pstr,       +1);
        defCase(cpy,        +1);
        defCase(pop,        -1);
        defCase(addvar,     -1);
        defCase(subvar,     -1);
        defCase(andvar,     -1);
        defCase(orvar,      -1);
        defCase(xorvar,     -1);
        defCase(mulvar,     -1);
        defCase(divvar,     -1);
        defCase(modvar,     -1);
        defCase(shlvar,     -1);
        defCase(shrvar,     -1);
        defCase(sincvar,    +1);
        defCase(sdecvar,    +1);
        defCase(pincvar,    +1);
        defCase(pdecvar,    +1);
        defCase(addglo,     -1);
        defCase(subglo,     -1);
        defCase(andglo,     -1);
        defCase(orglo,      -1);
        defCase(xorglo,     -1);
        defCase(mulglo,     -1);
        defCase(divglo,     -1);
        defCase(modglo,     -1);
        defCase(shlglo,     -1);
        defCase(shrglo,     -1);
        defCase(sincglo,    +1);
        defCase(sdecglo,    +1);
        defCase(pincglo,    +1);
        defCase(pdecglo,    +1);
        defCase(addmem,     -2);
        defCase(submem,     -2);
        defCase(andmem,     -2);
        defCase(ormem,      -2);
        defCase(xormem,     -2);
        defCase(mulmem,     -2);
        defCase(divmem,     -2);
        defCase(modmem,     -2);
        defCase(shlmem,     -2);
        defCase(shrmem,     -2);
        defCase(sincmem,    0);
        defCase(sdecmem,    0);
        defCase(pincmem,    0);
        defCase(pdecmem,    0);
        defCase(addarrmem,  -(arg1 + 2))
        defCase(subarrmem,  -(arg1 + 2))
        defCase(mularrmem,  -(arg1 + 2))
        defCase(divarrmem,  -(arg1 + 2))
        defCase(modarrmem,  -(arg1 + 2))
        defCase(andarrmem,  -(arg1 + 2))
        defCase(orarrmem,  -(arg1 + 2))
        defCase(xorarrmem,  -(arg1 + 2))
        defCase(sincarrmem,  -arg1)
        defCase(sdecarrmem,  -arg1)
        defCase(pincarrmem,  -arg1)
        defCase(pdecarrmem,  -arg1)
        defCase(eq,         -1);
        defCase(ne,         -1);
        defCase(ls,         -1);
        defCase(le,         -1);
        defCase(gt,         -1);
        defCase(ge,         -1);
        defCase(jz,         -1);
        defCase(jp,         -1);
        defCase(jmp,        0);
        defCase(vcall,      0);
        defCase(call,       +1)
        defCase(getarg,     0);
        defCase(setarg,     -arg1)
        defCase(vret,       -1)
        defCase(ret,        -1)
        defCase(newobj,     +1)
        defCase(newarr,     -arg1)
        defCase(setgtbl,    -arg1)
        defCase(setclgtbl,  0)
        defCase(getgtbl,    +1)
        defCase(getgtblsz,  +1)
        defCase(sys,        -syscallArgCnt[arg1] + 1);
        defCase(setflag,    0)
        defCase(pushflag,   +1)
        defCase(setpause,   0)
        default:
            assert(1 == 0);
            break;
    }
}

void setInstBlk() {
    tgBlk->instBlk = mmap(NULL, max(1024, tgList->size), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memcpy(tgBlk->instBlk, tgList->data, sizeof(uint8) * tgList->size);

    tgBlk->entryListSize = *(uint64 *)DArray_get(offList, offList->len - 1);
    tgBlk->entryList = mallocArray(void *, tgBlk->entryListSize);
    for (uint64 i = 0; i < offList->len; i++)
        tgBlk->entryList[*(uint64 *)DArray_get(offList, i)] = tgBlk->instBlk + (uint64)tgList->entry[*(uint64 *)DArray_get(entryList, i)];
    // set the offset of jmp and jcc
    for (uint64 i = 0; i < jmpList->len; i++) {
        // the address(offset) of the jcc/jmp instruction
        uint64 jmpAddr = *(uint64 *)DArray_get(jmpList, i), dstOffset = *(uint64 *)DArray_get(jmpDstOffset, i);
        uint8 *pfx = ((uint8 *)tgBlk->instBlk + jmpAddr);
        // it is jcc
        if (*pfx == 0x0F)
            *(int32 *)(pfx + 2) = (int32)((uint64)tgBlk->entryList[dstOffset] - (uint64)tgBlk->instBlk) - ((int32)jmpAddr + 6);
        else
            *(int32 *)(pfx + 1) = (int32)((uint64)tgBlk->entryList[dstOffset] - (uint64)tgBlk->instBlk) - ((int32)jmpAddr + 5);
    }

    Debug_saveJITCode(tgBlk->instBlk, tgList->size, "test.log");
}