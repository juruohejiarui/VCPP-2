#pragma once
#include "tools.h"

typedef struct tmpInstructionList {
    uint64 size, cap, entryCount;
    uint64 *entry;
    uint8 *data;
} InstList;
typedef struct tmpInstruction {
    uint8 *data;
    uint64 size;
} Inst;

void Inst_init(Inst *inst, uint64 size);

void InstList_init(InstList *list);
void InstList_free(InstList *list);
void InstList_add(InstList *list, Inst ins, int isEntry);
void InstList_addm(InstList *list, uint32 cnt, ...);
void InstList_merge(InstList *dst, InstList *src);

void setJCCDelta(InstList *list, uint64 jmpEntry, uint64 dstEntry);
void setJMPDelta(InstList *list, uint64 jmpEntry, uint64 dstEntry);

typedef enum tmpRegister {
    // x64 registers
    RAX = 0, RCX, RDX, RBX, RSP, RBP, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15,
    // x86 32bit registers
    EAX = 0, ECX, EDX, EBX, ESP, EBP, ESI, EDI, R8d, R9d, R10d, R11d, R12d, R13d, R14d, R15d,
    // x64 16bit registers
    AX = 0, CX, DX, BX, SP, BP, SI, DI, R8w, R9w, R10w, R11w, R12w, R13w, R14w, R15w,
    // x64 8bit registers
    AL = 0, CL, DL, BL, SPL, BPL, SIL, DIL, R8b, R9b, R10b, R11b, R12b, R13b, R14b, R15b,
    XMM0 = 0, XMM1, XMM2, XMM3, XMM4, XMM5, XMM6, XMM7, XMM8, XMM9, XMM10, XMM11, XMM12, XMM13, XMM14, XMM15,
} Register;

typedef enum tmpTypeModifier {
    TM_none, TM_word, TM_dword, TM_qword, TM_byte, TM_sbyte, TM_sword, TM_sdword, TM_sqword, TM_float32, TM_float64, TM_float128,
    TM_object, TM_generic0, TM_generic1, TM_generic2, TM_generic3, TM_generic4, TM_unknown
} TypeModifier;

extern uint32 dataSize[];

SBool isInterger(TypeModifier dtmdf);
SBool isFloat(TypeModifier dtmdf);
SBool is64Bit(TypeModifier dtmdf);
SBool isGeneric(TypeModifier dtmdf);

TypeModifier getTypeModfier(enum DataTypeModifier dtMdf);

// for instruction that looks like INST a, b
// E.g MOV, ADD, SUB, AND, XOR, OR
#define FuncTable_BinaryOperatorInstruction(instName) \
    /* ##instName## %src, %dst */ \
    Inst instName##_r_r(TypeModifier dtmdf, uint32 src, uint32 dst); \
    /* ##instName## $imm, %dst */ \
    Inst instName##_i_r(TypeModifier dtmdf, uint64 imm, uint32 dst); \
    /* ##instName## (%src), %dst */ \
    Inst instName##_mr_r(TypeModifier dtmdf, uint32 src, uint32 dst); \
    Inst instName##_r_mr(TypeModifier dtmdf, uint32 src, uint32 dst); \
    Inst instName##_or_r(TypeModifier dtMdf, int32 offset, uint32 reg, uint32 dst); \
    Inst instName##_rrs_r(TypeModifier dtMdf, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst); \
    Inst instName##_orrs_r(TypeModifier dtMdf, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst); \
    Inst instName##_r_or(TypeModifier dtMdf, uint32 src, int32 offset, uint32 reg); \
    Inst instName##_r_rrs(TypeModifier dtMdf, uint32 src, uint32 reg1, uint32 reg2, uint8 scale); \
    Inst instName##_r_orrs(TypeModifier dtMdf, uint32 src, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale);
#define FuncTable_FloatBinaryOperatorInstruction(instName) \
    Inst instName##_x_x(TypeModifier dtmdf, uint32 src, uint32 dst); \
    Inst instName##_mr_x(TypeModifier dtmdf, uint32 reg, uint32 dst); \
    Inst instName##_or_x(TypeModifier dtmdf, int32 offset, uint32 reg1, uint32 dst); \
    Inst instName##_rrs_x(TypeModifier dtmdf, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst); \
    Inst instName##_orrs_x(TypeModifier dtmdf, uint32 offset, uint32 reg2, uint32 reg3, uint8 scale, uint32 dst);

#define FuncTable_SingleOperatorInstruction(instName) \
    Inst instName##_r(TypeModifier dtmdf, uint32 reg); \
    Inst instName##_mr(TypeModifier dtmdf, uint32 reg); \
    Inst instName##_or(TypeModifier dtmdf, int32 offset, uint32 reg); \

#define FuncTable_ShiftOperatroInstruction(instName) \
    Inst instName##_i_r(TypeModifier dtmdf, uint8 imm, uint32 reg); \
    Inst instName##_i_mr(TypeModifier dtmdf, uint8 imm, uint32 reg); \
    Inst instName##_i_or(TypeModifier dtmdf, uint8 imm, uint32 offset, uint32 reg); \
    Inst instName##_CL_r(TypeModifier dtmdf, uint32 reg); \
    Inst instName##_CL_mr(TypeModifier dtmdf, uint32 reg); \
    Inst instName##_CL_or(TypeModifier dtmdf, uint32 offset, uint32 reg);

FuncTable_BinaryOperatorInstruction(MOV)
FuncTable_BinaryOperatorInstruction(ADD)
FuncTable_BinaryOperatorInstruction(SUB)

FuncTable_BinaryOperatorInstruction(AND)
FuncTable_BinaryOperatorInstruction(OR)
FuncTable_BinaryOperatorInstruction(XOR)
FuncTable_BinaryOperatorInstruction(CMP)

FuncTable_FloatBinaryOperatorInstruction(ADD)
FuncTable_FloatBinaryOperatorInstruction(SUB)
FuncTable_FloatBinaryOperatorInstruction(MUL)
FuncTable_FloatBinaryOperatorInstruction(DIV)
FuncTable_FloatBinaryOperatorInstruction(UCOMI)

/* movb %ah, %dl */
Inst XOR_AH_AH();
Inst MOV_AH_DL();
Inst MOV_r_x(TypeModifier dtmdf, uint32 reg, uint32 xmm);
Inst MOV_mr_x(TypeModifier dtmdf, uint32 reg, uint32 xmm);
Inst MOV_or_x(TypeModifier dtmdf, uint32 offset, uint32 reg, uint32 xmm);
Inst MOV_rrs_x(TypeModifier dtmdf, uint32 reg1, uint32 reg2, uint8 scale, uint32 xmm);
Inst MOV_orrs_x(TypeModifier dtmdf, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale, uint32 xmm);
Inst MOV_x_r(TypeModifier dtmdf, uint32 xmm, uint32 reg);
Inst MOV_x_mr(TypeModifier dtmdf, uint32 xmm, uint32 reg);
Inst MOV_x_or(TypeModifier dtmdf, uint32 xmm, uint32 offset, uint32 reg);
Inst MOV_x_rrs(TypeModifier dtmdf, uint32 xmm, uint32 reg1, uint32 reg2, uint8 scale);
Inst MOV_x_orrs(TypeModifier dtmdf, uint32 xmm, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale);

// leaq (%src), %dst
Inst LEA_mr_r(uint32 src, uint32 dst);
// leaq offset(%reg), %dst
Inst LEA_or_r(uint32 offset, uint32 reg, uint32 dst);
// leaq (%reg, %reg, scale), %dst
Inst LEA_rrs_r(uint32 reg1, uint32 reg2, uint8 scale, uint32 dst);
// leaq offset(%reg, %reg, scale), %dst
Inst LEA_orrs_r(uint32 offset, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst);

FuncTable_SingleOperatorInstruction(IMUL)
FuncTable_SingleOperatorInstruction(MUL)
FuncTable_SingleOperatorInstruction(IDIV)
FuncTable_SingleOperatorInstruction(DIV)
FuncTable_SingleOperatorInstruction(INC)
FuncTable_SingleOperatorInstruction(DEC)
FuncTable_SingleOperatorInstruction(NOT)
 
FuncTable_ShiftOperatroInstruction(SHL)
FuncTable_ShiftOperatroInstruction(SHR)
FuncTable_ShiftOperatroInstruction(SAL)
FuncTable_ShiftOperatroInstruction(SAR)

Inst PUSH_r(uint32 reg);
Inst PUSH_mr(uint32 reg);
Inst PUSH_or(uint32 offset, uint32 reg);
Inst POP_r(uint32 reg);
Inst POP_or(uint32 offset, uint32 reg);

// jmp *%reg
Inst JMP_r(uint32 reg);
// jmp $delta
Inst JMP_i(uint32 delta);

// jz offset
Inst JZ(uint32 delta);

// jnz offset
Inst JNZ(uint32 delta);

Inst JC(uint32 delta);
Inst JNC(uint32 delta);
Inst JBE(uint32 delta);
Inst JNBE(uint32 delta);
Inst JL(uint32 delta);
Inst JLE(uint32 delta);
Inst JG(uint32 delta);
Inst JGE(uint32 delta);

// call *%reg
Inst CALL(uint32 reg);
Inst LEAVE();
// ret
Inst RET();

Inst ENDBR64();

InstList genInstList_saveReg(int isEntry, int ignoreRAX);
InstList genInstList_restoreReg(int isEntry, int ignoreRAX);