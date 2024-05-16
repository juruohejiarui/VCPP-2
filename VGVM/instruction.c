#include "instruction.h"
#include "insttmpl.h"

uint32 dataSize[] = { 1, 2, 4, 8, TM_byte, TM_sbyte, TM_sword, TM_sdword, TM_sqword, 4, 8, TM_float128,
    8, TM_generic0, TM_generic1, TM_generic2, TM_generic3, TM_generic4, TM_unknown };
SBool isInterger(TypeModifier dtmdf) { return dtmdf >= TM_none && dtmdf <= TM_qword; }
SBool isFloat(TypeModifier dtmdf) { return dtmdf >= TM_float32 && dtmdf <= TM_float64; }
SBool is64Bit(TypeModifier dtmdf) {
    return dtmdf == TM_qword || dtmdf == TM_float64 || dtmdf == TM_object;
}

SBool isGeneric(TypeModifier dtmdf) {
    return dtmdf >= TM_generic0 && dtmdf <= TM_generic4;
}

TypeModifier getTypeModfier(enum DataTypeModifier dtMdf)
{
    if (dtMdf >= gv0 && dtMdf <= gv4) return dtMdf - gv0 + TM_generic0;
    else if (dtMdf == o) return TM_object;
    else if (dtMdf == f32) return TM_float32;
    else if (dtMdf == f64) return TM_float64;
    else if (dtMdf == i64 || dtMdf == u64) return TM_qword;
    else if (dtMdf == i32 || dtMdf == u32) return TM_dword;
    else if (dtMdf == i16 || dtMdf == u16) return TM_word;
    else if (dtMdf == i8 || dtMdf == u8) return TM_none;

    return TM_unknown;
}

void Inst_init(Inst *inst, uint64 size) { inst->data = malloc(size), inst->size = size; }

void InstList_init(InstList *list)
{
    list->size = list->cap = 0, list->entryCount = 0;
    list->entry = NULL, list->data = NULL;
}

void InstList_free(InstList *list) {
    if (list->entry != NULL) free(list->entry);
    if (list->data != NULL) free(list->data);
    free(list);
}

void InstList_resize(InstList *list) {
    if (list->size <= list->cap) return ;
    if (!list->cap) list->cap = 1;
    while (list->cap < list->size) list->cap <<= 1;
    list->data = realloc(list->data, list->cap);
}

void InstList_add(InstList *list, Inst ins, int isEntry) {
    list->size += ins.size;
    InstList_resize(list);
    memcpy(list->data + list->size - ins.size, ins.data, ins.size);
    free(ins.data);
    if (isEntry) {
        list->entryCount++;
        list->entry = realloc(list->entry, list->entryCount * sizeof(uint64));
        list->entry[list->entryCount - 1] = list->size - ins.size;
    }
}

void InstList_addm(InstList *list, uint32 cnt, ...) {
    va_list argp;
    va_start(argp, cnt);
    for (uint32 i = 0; i < cnt; i++) InstList_add(list, va_arg(argp, Inst), 0);
}

void InstList_merge(InstList *dst, InstList *src) {
    dst->size += src->size;
    InstList_resize(dst);
    memcpy(dst->data + dst->size - src->size, src->data, src->size);
    // merge entry
    dst->entryCount += src->entryCount;
    dst->entry = realloc(dst->entry, dst->entryCount * sizeof(uint8 *));
     
    for (int i = 0; i < src->entryCount; i++)
        dst->entry[dst->entryCount - src->entryCount + i] = dst->size - src->size + src->entry[i];
    if (src->data != NULL) free(src->data), src->data = NULL;
    if (src->entry != NULL) free(src->entry), src->entry = NULL;
}

void setJCCDelta(InstList *list, uint64 jmpEntry, uint64 dstEntry) {
    int32 delta = (int32)((uint64)list->entry[dstEntry] & ((1ull << 32) - 1)) - (int32)((uint64)list->entry[jmpEntry] & ((1ull << 32) - 1)) - 6;
    uint8 *deltaAddr = (uint8 *)(list->entry[jmpEntry] + 2);
    *(int32 *)deltaAddr = delta;
}

void setJMPDelta(InstList *list, uint64 jmpEntry, uint64 dstEntry) {
    int32 delta = (int32)((uint64)list->entry[dstEntry] & ((1ull << 32) - 1)) - (int32)((uint64)list->entry[jmpEntry] & ((1ull << 32) - 1)) - 5;
    uint8 *deltaAddr = (uint8 *)(list->entry[jmpEntry] + 1);
    *(int32 *)deltaAddr = delta;
}

uint8 fakeLog2(uint8 x) {
    uint8 ret = 0;
    while (x > 1) x >>= 1, ret++;
    return ret;
}

#pragma region mov
Inst MOV_r_r(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x89, 0x88, 0xC0, dtmdf, src, dst)
}

Inst MOV_i_r(TypeModifier dtmdf, uint64 imm, uint32 dst) {
    Inst ins;
    switch (dtmdf) {
        // 8bit: mov reg, imm8
        case TM_none:
            ins.size = 2, ins.data = malloc(2);
            ins.data[0] = 0xB0 + (uint8)dst;
            ins.data[1] = (uint8)imm;
            break;
        // 16bit: mov reg, imm16
        case TM_word:
            ins.size = 4, ins.data = malloc(3);
            ins.data[0] = 0x66;
            ins.data[1] = 0xB8 + (uint8)dst;
            *(uint16 *)(ins.data + 2) = (uint16)imm;
            break;
        // 32bit: movl reg, imm32
        case TM_dword:
            ins.size = 5, ins.data = malloc(5);
            ins.data[0] = 0xB8 + (uint8)dst;
            *(uint32 *)(ins.data + 1) = (uint32)imm;
            break;
        // 64bit: movq reg, imm64
        case TM_qword:
            ins.size = 10, ins.data = malloc(10);
            // 0100 + REX.W=1 + 00 + REX.B
            ins.data[0] = 0x48 + (uint8)((dst >> 3) & 1);
            ins.data[1] = 0xB8 + (uint8)(dst & 0x7);
            *(uint64 *)(ins.data + 2) = (uint64)imm;
            break;
        
    }
    return ins;
}

Inst MOV_mr_r(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x8B, 0x8A, 0x00, dtmdf, dst, src);
}

Inst MOV_r_mr(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x89, 0x88, 0x00, dtmdf, src, dst);
}

Inst MOV_or_r(TypeModifier dtMdf, int32 offset, uint32 reg, uint32 dst) {
    Tmpl_or_r(0x8B, 0x8A, dtMdf, offset, reg, dst);
}
// mov %src, offset(%reg)
Inst MOV_r_or(TypeModifier dtMdf, uint32 src, int32 offset, uint32 reg) {
    Tmpl_or_r(0x89, 0x88, dtMdf, offset, reg, src);
}

Inst MOV_rrs_r(TypeModifier dtMdf, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    // reg1 is the base register, reg2 is the index register
    Tmpl_rrs_r(0x8B, 0x8A, dtMdf, reg1, reg2, scale, dst);
}
Inst MOV_orrs_r(TypeModifier dtMdf, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_orrs_r(0x8B, 0x8A, dtMdf, offset, reg1, reg2, scale, dst);
}

Inst MOV_r_rrs(TypeModifier dtMdf, uint32 src, uint32 reg1, uint32 reg2, uint8 scale) {
    Tmpl_rrs_r(0x89, 0x88, dtMdf, reg1, reg2, scale, src);
}
Inst MOV_r_orrs(TypeModifier dtMdf, uint32 src, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale) {
    Tmpl_orrs_r(0x89, 0x88, dtMdf, offset, reg1, reg2, scale, src);
}
#pragma endregion

#pragma region movd and movq
Inst MOV_r_x(TypeModifier dtmdf, uint32 reg, uint32 xmm) {
    Tmpl_MOV_r_x(0x0F, 0x6E, 0xC0, dtmdf, reg, xmm);
}
Inst MOV_mr_x(TypeModifier dtmdf, uint32 reg, uint32 xmm) {
    Tmpl_MOV_r_x(0x0F, 0x6E, 0x00, dtmdf, reg, xmm);
}
Inst MOV_or_x(TypeModifier dtmdf, uint32 offset, uint32 reg, uint32 xmm) {
    Tmpl_MOV_or_x(0x0F, 0x6E, dtmdf, offset, reg, xmm);
}
Inst MOV_rrs_x(TypeModifier dtmdf, uint32 reg1, uint32 reg2, uint8 scale, uint32 xmm) {
    Tmpl_MOV_rrs_x(0x0F, 0x6E, dtmdf, reg1, reg2, scale, xmm);
}
Inst MOV_orrs_x(TypeModifier dtmdf, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale, uint32 xmm) {
    Tmpl_MOV_orrs_x(0x0F, 0x6E, dtmdf, offset, reg1, reg2, scale, xmm);
}
Inst MOV_x_r(TypeModifier dtmdf, uint32 xmm, uint32 reg) {
    Tmpl_MOV_r_x(0x0F, 0x7E, 0xC0, dtmdf, reg, xmm);
}
Inst MOV_x_mr(TypeModifier dtmdf, uint32 xmm, uint32 reg) {
    Tmpl_MOV_r_x(0x0F, 0x7E, 0x00, dtmdf, reg, xmm);
}
Inst MOV_x_or(TypeModifier dtmdf, uint32 xmm, uint32 offset, uint32 reg) {
    Tmpl_MOV_or_x(0x0F, 0x7E, dtmdf, offset, reg, xmm);
}
Inst MOV_x_rrs(TypeModifier dtmdf, uint32 xmm, uint32 reg1, uint32 reg2, uint8 scale) {
    Tmpl_MOV_rrs_x(0x0F, 0x7E, dtmdf, reg1, reg2, scale, xmm);
}
Inst MOV_x_orrs(TypeModifier dtmdf, uint32 xmm, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale) {
    Tmpl_MOV_orrs_x(0x0F, 0x7E, dtmdf, offset, reg1, reg2, scale, xmm);
}
#pragma endregion

#pragma region addss, addsd, subss, subsd, mulss, mulsd, divss, divsd, ucomiss, ucomisd
Inst ADD_x_x(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_x_x(0x0F, 0x58, dtmdf, src, dst);
}
Inst ADD_mr_x(TypeModifier dtmdf, uint32 reg, uint32 dst) {
    Tmpl_mr_x(0x0F, 0x58, dtmdf, reg, dst);
}
Inst ADD_or_x(TypeModifier dtmdf, int32 offset, uint32 reg, uint32 dst) {
    Tmpl_or_x(0x0F, 0x58, dtmdf, offset, reg, dst);
}
Inst ADD_rrs_x(TypeModifier dtmdf, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_rrs_x(0x0F, 0x58, dtmdf, reg1, reg2, scale, dst);
}
Inst ADD_orrs_x(TypeModifier dtmdf, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
     Tmpl_orrs_x(0x0F, 0x58, dtmdf, offset, reg1, reg2, scale, dst);
}
Inst SUB_x_x(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_x_x(0x0F, 0x5C, dtmdf, src, dst);
}
Inst SUB_mr_x(TypeModifier dtmdf, uint32 reg, uint32 dst) {
    Tmpl_mr_x(0x0F, 0x5C, dtmdf, reg, dst);
}
Inst SUB_or_x(TypeModifier dtmdf, int32 offset, uint32 reg, uint32 dst) {
    Tmpl_or_x(0x0F, 0x5C, dtmdf, offset, reg, dst);
}
Inst SUB_rrs_x(TypeModifier dtmdf, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_rrs_x(0x0F, 0x5C, dtmdf, reg1, reg2, scale, dst);
}
Inst SUB_orrs_x(TypeModifier dtmdf, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
     Tmpl_orrs_x(0x0F, 0x5C, dtmdf, offset, reg1, reg2, scale, dst);
}
Inst MUL_x_x(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_x_x(0x0F, 0x59, dtmdf, src, dst);
}
Inst MUL_mr_x(TypeModifier dtmdf, uint32 reg, uint32 dst) {
    Tmpl_mr_x(0x0F, 0x59, dtmdf, reg, dst);
}
Inst MUL_or_x(TypeModifier dtmdf, int32 offset, uint32 reg, uint32 dst) {
    Tmpl_or_x(0x0F, 0x59, dtmdf, offset, reg, dst);
}
Inst MUL_rrs_x(TypeModifier dtmdf, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_rrs_x(0x0F, 0x59, dtmdf, reg1, reg2, scale, dst);
}
Inst MUL_orrs_x(TypeModifier dtmdf, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
     Tmpl_orrs_x(0x0F, 0x59, dtmdf, offset, reg1, reg2, scale, dst);
}
Inst DIV_x_x(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_x_x(0x0F, 0x5E, dtmdf, src, dst);
}
Inst DIV_mr_x(TypeModifier dtmdf, uint32 reg, uint32 dst) {
    Tmpl_mr_x(0x0F, 0x5E, dtmdf, reg, dst);
}
Inst DIV_or_x(TypeModifier dtmdf, int32 offset, uint32 reg, uint32 dst) {
    Tmpl_or_x(0x0F, 0x5E, dtmdf, offset, reg, dst);
}
Inst DIV_rrs_x(TypeModifier dtmdf, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_rrs_x(0x0F, 0x5E, dtmdf, reg1, reg2, scale, dst);
}
Inst DIV_orrs_x(TypeModifier dtmdf, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_orrs_x(0x0F, 0x5E, dtmdf, offset, reg1, reg2, scale, dst);
}

Inst UCOMI_x_x(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_UCOMI_r_r(0x0F, 0x2E, 0xC0, dtmdf, src, dst);
}
Inst UCOMI_mr_x(TypeModifier dtmdf, uint32 reg, uint32 dst) {
    Tmpl_UCOMI_r_r(0x0F, 0x2E, 0x00, dtmdf, reg, dst);
}
Inst UCOMI_or_x(TypeModifier dtmdf, int32 offset, uint32 reg, uint32 dst) {
    Tmpl_UCOMI_or_r(0x0F, 0x2E, dtmdf, offset, reg, dst);
}
Inst UCOMI_rrs_x(TypeModifier dtmdf, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_UCOMI_rrs_r(0x0F, 0x2E, dtmdf, reg1, reg2, scale, dst);
}
Inst UCOMI_orrs_x(TypeModifier dtmdf, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_UCOMI_orrs_r(0x0F, 0x2E, dtmdf, offset, reg1, reg2, scale, dst);
}
#pragma endregion

#pragma region lea
Inst LEA_mr_r(uint32 src, uint32 dst) {
    Tmpl_r_r(0x8D, 0x8C, 0x00, TM_qword, src, dst);
}

Inst LEA_or_r(uint32 offset, uint32 reg, uint32 dst) {
    Tmpl_or_r(0x8D, 0x8C, TM_qword, offset, reg, dst);
}

Inst LEA_rrs_r(uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_rrs_r(0x8D, 0x8C, TM_qword, reg1, reg2, scale, dst);
}

Inst LEA_orrs_r(uint32 offset, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_orrs_r(0x8D, 0x8C, TM_qword, offset, reg1, reg2, scale, dst);
}
#pragma endregion

#pragma region add and sub
Inst ADD_r_r(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x01, 0x00, 0xC0, dtmdf, src, dst);
}
Inst ADD_mr_r(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x03, 0x02, 0x00, dtmdf, dst, src);
}
Inst ADD_r_mr(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x01, 0x00, 0x00, dtmdf, src, dst);
}
Inst ADD_i_r(TypeModifier dtmdf, uint64 imm, uint32 dst) {
    Tmpl_i32_r(0x81, 0x80, 0xC0, dtmdf, imm, dst);
}
Inst ADD_or_r(TypeModifier dtMdf, int32 offset, uint32 reg, uint32 dst) {
    Tmpl_or_r(0x03, 0x02, dtMdf, offset, reg, dst);
}
Inst ADD_r_or(TypeModifier dtMdf, uint32 src, int32 offset, uint32 reg) {
    Tmpl_or_r(0x01, 0x00, dtMdf, offset, reg, src);
}
Inst ADD_rrs_r(TypeModifier dtMdf, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_rrs_r(0x03, 0x02, dtMdf, reg1, reg2, scale, dst);
}
Inst ADD_orrs_r(TypeModifier dtMdf, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_orrs_r(0x03, 0x02, dtMdf, offset, reg1, reg2, scale, dst);
}
Inst ADD_r_rrs(TypeModifier dtMdf, uint32 src, uint32 reg1, uint32 reg2, uint8 scale) {
    Tmpl_rrs_r(0x01, 0x00, dtMdf, reg1, reg2, scale, src);
}
Inst ADD_r_orrs(TypeModifier dtMdf, uint32 src, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale) {
    Tmpl_orrs_r(0x01, 0x00, dtMdf, offset, reg1, reg2, scale, src);
}

Inst SUB_r_r(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x29, 0x28, 0xC0, dtmdf, src, dst);
}
Inst SUB_mr_r(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x2B, 0x2A, 0x00, dtmdf, dst, src);
}
Inst SUB_r_mr(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x29, 0x28, 0x00, dtmdf, src, dst);
}
Inst SUB_i_r(TypeModifier dtmdf, uint64 imm, uint32 dst) {
    Tmpl_i32_r(0x81, 0x80, 0xE8, dtmdf, imm, dst);
}
Inst SUB_or_r(TypeModifier dtMdf, int32 offset, uint32 reg, uint32 dst) {
    Tmpl_or_r(0x2B, 0x2A, dtMdf, offset, reg, dst);
}
Inst SUB_r_or(TypeModifier dtMdf, uint32 src, int32 offset, uint32 reg) {
    Tmpl_or_r(0x29, 0x28, dtMdf, offset, reg, src);
}
Inst SUB_rrs_r(TypeModifier dtMdf, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_rrs_r(0x2B, 0x2A, dtMdf, reg1, reg2, scale, dst);
}
Inst SUB_orrs_r(TypeModifier dtMdf, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_orrs_r(0x2B, 0x2A, dtMdf, offset, reg1, reg2, scale, dst);
}
Inst SUB_r_rrs(TypeModifier dtMdf, uint32 src, uint32 reg1, uint32 reg2, uint8 scale) {
    Tmpl_rrs_r(0x29, 0x28, dtMdf, reg1, reg2, scale, src);
}
Inst SUB_r_orrs(TypeModifier dtMdf, uint32 src, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale) {
    Tmpl_orrs_r(0x29, 0x28, dtMdf, offset, reg1, reg2, scale, src);
}
#pragma endregion

#pragma region and, or, and xor
Inst AND_r_r(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x21, 0x20, 0xC0, dtmdf, src, dst);
}
Inst AND_r_mr(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x21, 0x20, 0x00, dtmdf, src, dst);
}
Inst AND_mr_r(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x23, 0x22, 0x00, dtmdf, dst, src);
}
Inst AND_i_r(TypeModifier dtmdf, uint64 imm, uint32 dst) {
    Tmpl_i32_r(0x81, 0x80, 0xe0, dtmdf, imm, dst);
}
Inst AND_or_r(TypeModifier dtmdf, int32 offset, uint32 reg, uint32 dst) {
    Tmpl_or_r(0x23, 0x22, dtmdf, offset, reg, dst);
}
Inst AND_rrs_r(TypeModifier dtmdf, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_rrs_r(0x23, 0x22, dtmdf, reg1, reg2, scale, dst);
}
Inst AND_orrs_r(TypeModifier dtmdf, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_orrs_r(0x23, 0x22, dtmdf, offset, reg1, reg2, scale, dst);
}
Inst AND_r_or(TypeModifier dtmdf, uint32 src, int32 offset, uint32 reg) {
    Tmpl_or_r(0x21, 0x20, dtmdf, offset, reg, src);
}
Inst AND_r_rrs(TypeModifier dtmdf, uint32 src, uint32 reg1, uint32 reg2, uint8 scale) {
    Tmpl_rrs_r(0x21, 0x20, dtmdf, reg1, reg2, scale, src);
}
Inst AND_r_orrs(TypeModifier dtmdf, uint32 src, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale) {
    Tmpl_orrs_r(0x21, 0x20, dtmdf, offset, reg1, reg2, scale, src);
}

Inst OR_r_r(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x09, 0x08, 0xC0, dtmdf, src, dst);
}
Inst OR_r_mr(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x09, 0x08, 0x00, dtmdf, src, dst);
}
Inst OR_mr_r(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x0B, 0x0A, 0x00, dtmdf, dst, src);
}
Inst OR_i_r(TypeModifier dtmdf, uint64 imm, uint32 dst) {
    Tmpl_i32_r(0x81, 0x80, 0xc8, dtmdf, imm, dst);
}
Inst OR_or_r(TypeModifier dtmdf, int32 offset, uint32 reg, uint32 dst) {
    Tmpl_or_r(0x0B, 0x0A, dtmdf, offset, reg, dst);
}
Inst OR_rrs_r(TypeModifier dtmdf, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_rrs_r(0x0B, 0x0A, dtmdf, reg1, reg2, scale, dst);
}
Inst OR_orrs_r(TypeModifier dtmdf, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_orrs_r(0x0B, 0x0A, dtmdf, offset, reg1, reg2, scale, dst);
}
Inst OR_r_or(TypeModifier dtmdf, uint32 src, int32 offset, uint32 reg) {
    Tmpl_or_r(0x09, 0x08, dtmdf, offset, reg, src);
}
Inst OR_r_rrs(TypeModifier dtmdf, uint32 src, uint32 reg1, uint32 reg2, uint8 scale) {
    Tmpl_rrs_r(0x09, 0x08, dtmdf, reg1, reg2, scale, src);
}
Inst OR_r_orrs(TypeModifier dtmdf, uint32 src, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale) {
    Tmpl_orrs_r(0x09, 0x08, dtmdf, offset, reg1, reg2, scale, src);
}

Inst XOR_r_r(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x31, 0x30, 0xC0, dtmdf, src, dst);
}
Inst XOR_r_mr(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x31, 0x30, 0x00, dtmdf, src, dst);
}
Inst XOR_mr_r(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x33, 0x32, 0x00, dtmdf, dst, src);
}
Inst XOR_i_r(TypeModifier dtmdf, uint64 imm, uint32 dst) {
    Tmpl_i32_r(0x81, 0x80, 0xF0, dtmdf, imm, dst);
}
Inst XOR_or_r(TypeModifier dtmdf, int32 offset, uint32 reg, uint32 dst) {
    Tmpl_or_r(0x33, 0x32, dtmdf, offset, reg, dst);
}
Inst XOR_rrs_r(TypeModifier dtmdf, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_rrs_r(0x33, 0x32, dtmdf, reg1, reg2, scale, dst);
}
Inst XOR_orrs_r(TypeModifier dtmdf, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_orrs_r(0x33, 0x32, dtmdf, offset, reg1, reg2, scale, dst);
}
Inst XOR_r_or(TypeModifier dtmdf, uint32 src, int32 offset, uint32 reg) {
    Tmpl_or_r(0x31, 0x30, dtmdf, offset, reg, src);
}
Inst XOR_r_rrs(TypeModifier dtmdf, uint32 src, uint32 reg1, uint32 reg2, uint8 scale) {
    Tmpl_rrs_r(0x31, 0x30, dtmdf, reg1, reg2, scale, src);
}
Inst XOR_r_orrs(TypeModifier dtmdf, uint32 src, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale) {
    Tmpl_orrs_r(0x31, 0x30, dtmdf, offset, reg1, reg2, scale, src);
}

#pragma endregion

#pragma region imul, mul, idiv, div
Inst IMUL_r(TypeModifier dtmdf, uint32 reg) {
    Tmpl_r(0xF7, 0xF6, 0xE8, dtmdf, reg);
}

Inst IMUL_mr(TypeModifier dtmdf, uint32 reg) {
    Tmpl_r(0xF7, 0xF6, 0x20, dtmdf, reg);
}

Inst IMUL_or(TypeModifier dtmdf, int32 offset, uint32 reg) {
    Tmpl_or(0xF7, 0xF6, 0x28, dtmdf, reg, offset);
}

Inst MUL_r(TypeModifier dtmdf, uint32 reg) {
    Tmpl_r(0xF7, 0xF6, 0xE0, dtmdf, reg);
}

Inst MUL_mr(TypeModifier dtmdf, uint32 reg) {
    Tmpl_r(0xF7, 0xF6, 0x20, dtmdf, reg);
}
Inst MUL_or(TypeModifier dtmdf, int32 offset, uint32 reg) {
    Tmpl_or(0xF7, 0xF6, 0x20, dtmdf, reg, offset);
}

Inst IDIV_r(TypeModifier dtmdf, uint32 reg) {
    Tmpl_r(0xF7, 0xF6, 0xF8, dtmdf, reg);
}

Inst IDIV_mr(TypeModifier dtmdf, uint32 reg) {
    Tmpl_r(0xF7, 0xF6, 0x38, dtmdf, reg);
}

Inst IDIV_or(TypeModifier dtmdf, int32 offset, uint32 reg) {
    Tmpl_or(0xF7, 0xF6, 0x38, dtmdf, reg, offset);
}

Inst DIV_r(TypeModifier dtmdf, uint32 reg) {
    Tmpl_r(0xF7, 0xF6, 0xF0, dtmdf, reg);
}

Inst DIV_mr(TypeModifier dtmdf, uint32 reg) {
    Tmpl_r(0xF7, 0xF6, 0x30, dtmdf, reg);
}

Inst DIV_or(TypeModifier dtmdf, int32 offset, uint32 reg) {
    Tmpl_or(0xF7, 0xF6, 0x30, dtmdf, reg, offset);
}

Inst NOT_r(TypeModifier dtmdf, uint32 reg) {
    Tmpl_r(0xF7, 0xF6, 0xD0, dtmdf, reg);
}

Inst NOT_mr(TypeModifier dtmdf, uint32 reg) {
    Tmpl_r(0xF7, 0xF6, 0x10, dtmdf, reg);
}

Inst NOT_or(TypeModifier dtmdf, int32 offset, uint32 reg) {
    Tmpl_or(0xF7, 0xF6, 0x10, dtmdf, reg, offset);
}
#pragma endregion

#pragma region inc and dec
Inst INC_r(TypeModifier dtmdf, uint32 reg) { Tmpl_r(0xFF, 0xFE, 0xC0, dtmdf, reg); }
Inst INC_mr(TypeModifier dtmdf, uint32 reg) { Tmpl_r(0xFF, 0xFE, 0x00, dtmdf, reg); }
Inst INC_or(TypeModifier dtmdf, int32 offset, uint32 reg) { Tmpl_or(0xFF, 0xFE, 0x00, dtmdf, reg, offset); }

Inst DEC_r(TypeModifier dtmdf, uint32 reg) { Tmpl_r(0xFF, 0xFE, 0xC8, dtmdf, reg); }
Inst DEC_mr(TypeModifier dtmdf, uint32 reg) { Tmpl_r(0xFF, 0xFE, 0x08, dtmdf, reg); }
Inst DEC_or(TypeModifier dtmdf, int32 offset, uint32 reg) { Tmpl_or(0xFF, 0xFE, 0x08, dtmdf, reg, offset); }
#pragma endregion

#pragma region shl, shr, sal, sar
Inst SHL_i_r(TypeModifier dtmdf, uint8 imm, uint32 reg) {
    Tmpl_Shift_i_r(0xC1, 0xC0, 0xE0, dtmdf, imm, reg);
}
Inst SHL_i_mr(TypeModifier dtmdf, uint8 imm, uint32 reg) {
    Tmpl_Shift_i_r(0xC1, 0xC0, 0x20, dtmdf, imm, reg);
}
Inst SHL_CL_r(TypeModifier dtmdf, uint32 reg) {
    Tmpl_Shift_CL_r(0xD3, 0xD2, 0xE0, dtmdf, reg);
}
Inst SHL_CL_mr(TypeModifier dtmdf, uint32 reg) {
    Tmpl_Shift_CL_r(0xD3, 0xD2, 0x20, dtmdf, reg);
}
Inst SHL_CL_or(TypeModifier dtmdf, uint32 offset, uint32 reg) {
    Tmpl_Shift_CL_or(0xD3, 0xD2, 0xA0, dtmdf, offset, reg);
}

Inst SHR_i_r(TypeModifier dtmdf, uint8 imm, uint32 reg) {
    Tmpl_Shift_i_r(0xC1, 0xC0, 0xE8, dtmdf, imm, reg);
}
Inst SHR_i_mr(TypeModifier dtmdf, uint8 imm, uint32 reg) {
    Tmpl_Shift_i_r(0xC1, 0xC0, 0x28, dtmdf, imm, reg);
}
Inst SHR_CL_r(TypeModifier dtmdf, uint32 reg) {
    Tmpl_Shift_CL_r(0xD3, 0xD2, 0xE8, dtmdf, reg);
}
Inst SHR_CL_mr(TypeModifier dtmdf, uint32 reg) {
    Tmpl_Shift_CL_r(0xD3, 0xD2, 0x28, dtmdf, reg);
}
Inst SHR_CL_or(TypeModifier dtmdf, uint32 offset, uint32 reg) {
    Tmpl_Shift_CL_or(0xD3, 0xD2, 0xA8, dtmdf, offset, reg);
}

Inst SAL_i_r(TypeModifier dtmdf, uint8 imm, uint32 reg) {
    Tmpl_Shift_i_r(0xC1, 0xC0, 0xE0, dtmdf, imm, reg);
}
Inst SAL_i_mr(TypeModifier dtmdf, uint8 imm, uint32 reg) {
    Tmpl_Shift_i_r(0xC1, 0xC0, 0x20, dtmdf, imm, reg);
}
Inst SAL_CL_r(TypeModifier dtmdf, uint32 reg) {
    Tmpl_Shift_CL_r(0xD3, 0xD2, 0xE0, dtmdf, reg);
}
Inst SAL_CL_mr(TypeModifier dtmdf, uint32 reg) {
    Tmpl_Shift_CL_r(0xD3, 0xD2, 0x20, dtmdf, reg);
}
Inst SAL_CL_or(TypeModifier dtmdf, uint32 offset, uint32 reg) {
    Tmpl_Shift_CL_or(0xD3, 0xD2, 0xA0, dtmdf, offset, reg);
}

Inst SAR_i_r(TypeModifier dtmdf, uint8 imm, uint32 reg) {
    Tmpl_Shift_i_r(0xC1, 0xC0, 0xF8, dtmdf, imm, reg);
}
Inst SAR_i_mr(TypeModifier dtmdf, uint8 imm, uint32 reg) {
    Tmpl_Shift_i_r(0xC1, 0xC0, 0x38, dtmdf, imm, reg);
}
Inst SAR_CL_r(TypeModifier dtmdf, uint32 reg) {
    Tmpl_Shift_CL_r(0xD3, 0xD2, 0xF8, dtmdf, reg);
}
Inst SAR_CL_mr(TypeModifier dtmdf, uint32 reg) {
    Tmpl_Shift_CL_r(0xD3, 0xD2, 0x38, dtmdf, reg);
}
Inst SAR_CL_or(TypeModifier dtmdf, uint32 offset, uint32 reg) {
    Tmpl_Shift_CL_or(0xD3, 0xD2, 0xB8, dtmdf, offset, reg);
}
#pragma endregion

#pragma region cmp
Inst CMP_i_r(TypeModifier dtmdf, uint64 imm, uint32 dst) {
    Tmpl_i32_r(0x81, 0x80, 0xF8, dtmdf, imm, dst);
}
Inst CMP_r_r(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x39, 0x38, 0xC0, dtmdf, src, dst);
}
Inst CMP_mr_r(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x3b, 0x3a, 0x00, dtmdf, src, dst);
}
Inst CMP_r_mr(TypeModifier dtmdf, uint32 src, uint32 dst) {
    Tmpl_r_r(0x39, 0x38, 0x00, dtmdf, dst, src);
}
Inst CMP_or_r(TypeModifier dtmdf, int32 offset, uint32 reg, uint32 dst) {
    Tmpl_or_r(0x3b, 0x3a, dtmdf, offset, reg, dst);
}
Inst CMP_rrs_r(TypeModifier dtmdf, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_rrs_r(0x3b, 0x3a, dtmdf, reg1, reg2, scale, dst);
}
Inst CMP_orrs_r(TypeModifier dtmdf, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale, uint32 dst) {
    Tmpl_orrs_r(0x3b, 0x3a, dtmdf, offset, reg1, reg2, scale, dst);
}
Inst CMP_r_or(TypeModifier dtmdf, uint32 src, int32 offset, uint32 reg) {
    Tmpl_or_r(0x39, 0x38, dtmdf, offset, reg, src);
}
Inst CMP_r_rrs(TypeModifier dtmdf, uint32 src, uint32 reg1, uint32 reg2, uint8 scale) {
    Tmpl_rrs_r(0x39, 0x38, dtmdf, reg1, reg2, scale, src);
}
Inst CMP_r_orrs(TypeModifier dtmdf, uint32 src, uint32 offset, uint32 reg1, uint32 reg2, uint8 scale) {
    Tmpl_orrs_r(0x39, 0x38, dtmdf, offset, reg1, reg2, scale, src);
}
#pragma endregion

#pragma region push and pop
Inst PUSH_r(uint32 reg) {
    Inst ins;
    if (reg >= R8) {
        ins.size = 2, ins.data = malloc(2);
        ins.data[0] = 0x40 | ((reg >> 3) & 1);
        ins.data[1] = 0x50 | (reg & 0x7);
    } else ins.size = 1, ins.data = malloc(1), ins.data[0] = 0x50 | (reg & 0x7);
    return ins;
}

Inst PUSH_mr(uint32 reg) {
    Inst ins;
    int st = 0;
    if (reg >= R8) {
        ins.size = 3, ins.data = malloc(3);
        ins.data[0] = 0x40 | ((reg >> 3) & 1);
        st = 1;
    } else ins.size = 2, ins.data = malloc(2);
    ins.data[st + 0] = 0xFF;
    ins.data[st + 1] = 0x30 | (uint8)(reg & 0x7);
    return ins;
}

Inst PUSH_or(uint32 offset, uint32 reg)
{
    Inst ins;
    int st = 0;
    if (reg >= R8) {
        ins.size = 7, ins.data = malloc(7);
        ins.data[0] = 0x40 | (uint8)((reg >> 3) & 1);
        st = 1;
    } else ins.size = 6, ins.data = malloc(6);
    ins.data[st + 0] = 0xFF;
    ins.data[st + 1] = 0xB0 | (reg & 0x7);
    *(uint32 *)(ins.data + st + 2) = offset;
    return ins;
}

Inst POP_r(uint32 reg) {
    Inst ins;
    if (reg >= R8) {
        ins.size = 2, ins.data = malloc(2);
        ins.data[0] = 0x40 | ((reg >> 3) & 1);
        ins.data[1] = 0x58 | (reg & 0x7);
    } else ins.size = 1, ins.data = malloc(1), ins.data[0] = 0x58 | (reg & 0x7);
    return ins;
}

Inst POP_mr(uint32 reg) {
    Inst ins;
    int st = 0;
    if (reg >= R8) {
        Inst_init(&ins, 3), st = 1;
        ins.data[0] = 0x40 | ((reg >> 3) & 1);
    } else Inst_init(&ins, 2);
    ins.data[st + 0] = 0x8F;
    ins.data[st + 1] = 0x00 | (uint8)(reg & 0x7);
    return ins;
}

Inst POP_or(uint32 offset, uint32 reg) {
    Inst ins;
    int st = 0;
    if (reg >= R8) {
        Inst_init(&ins, 7), st = 1;
        ins.data[0] = 0x40 | (uint8)((reg >> 3) & 1);
    } else Inst_init(&ins, 6);
    ins.data[st + 0] = 0x8F;
    ins.data[st + 1] = 0x80 | (reg & 0x7);
    *(uint32 *)(ins.data + st + 2) = offset;
    return ins;
}
#pragma endregion

#pragma region jmp, call, ret and jcc
Inst JMP_r(uint32 reg) {
    Tmpl_jmp_r(0xFF, 0xE0, reg);
}
Inst JMP_i(uint32 delta) {
    Inst ins;
    Inst_init(&ins, 5);
    ins.data[0] = 0xE9;
    *(uint32 *)(ins.data + 1) = delta;
    return ins;
}

Inst JC(uint32 delta)    { Tmpl_jcc(0x82, delta); }
Inst JNC(uint32 delta)   { Tmpl_jcc(0x83, delta); }
Inst JBE(uint32 delta)   { Tmpl_jcc(0x86, delta); }
Inst JNBE(uint32 delta)  { Tmpl_jcc(0x87, delta); }
Inst JZ(uint32 delta)    { Tmpl_jcc(0x84, delta); }
Inst JNZ(uint32 delta)   { Tmpl_jcc(0x85, delta); }
Inst JL(uint32 delta)    { Tmpl_jcc(0x8C, delta); }
Inst JLE(uint32 delta)   { Tmpl_jcc(0x8E, delta); }
Inst JG(uint32 delta)    { Tmpl_jcc(0x8F, delta); }
Inst JGE(uint32 delta)   { Tmpl_jcc(0x8D, delta); }

Inst CALL(uint32 reg) { Tmpl_jmp_r(0xFF, 0xD0, reg); }

Inst LEAVE() {
    Inst ins;
    Inst_init(&ins, 1);
    ins.data[0] = 0xC9;
    return ins;
}

Inst MOV_AH_DL() {
    Inst ins;
    Inst_init(&ins, 2), ins.data[0] = 0x88, ins.data[1] = 0xE2;
    return ins;
}
Inst XOR_AH_AH() {
    Inst ins;
    Inst_init(&ins, 2), ins.data[0] = 0x30, ins.data[1] = 0xE4;
    return ins;
}

Inst RET() { Inst ins; Inst_init(&ins, 1), ins.data[0] = 0xC3; return ins; }
Inst ENDBR64() {
    Inst ins;
    Inst_init(&ins, 4);
    ins.data[0] = 0xF3, ins.data[1] = 0x0F;
    ins.data[2] = 0x1E, ins.data[3] = 0xFA;
    return ins;
}
#pragma endregion

InstList genInstList_saveReg(int isEntry, int ignoreRAX) {
    InstList list;
    InstList_init(&list);
    for (int i = RAX + ignoreRAX; i <= R15; i++) if (i != RBP && i != RSP) InstList_add(&list, PUSH_r(i), (i == RAX + ignoreRAX && isEntry));
    return list;
}

InstList genInstList_restoreReg(int isEntry, int ignoreRAX) {
    InstList list;
    InstList_init(&list);
    for (int i = R15; i >= RAX + ignoreRAX; i--) if (i != RBP && i != RSP) InstList_add(&list, POP_r(i), (i == R15 && isEntry));
    return list;
}