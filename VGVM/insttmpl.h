#pragma once

#include <math.h>

#define setInstSize_or(ins, offset, baseSize) \
    do { \
        if (abs(offset) >= (1 << 8)) modRM = 0x80, Inst_init(&ins, (baseSize) + 4), *(int32 *)(ins.data + (baseSize)) = offset; \
        else modRM = 0x40, Inst_init(&ins, (baseSize) + 1), *(int8 *)(ins.data + (baseSize)) = (int8)offset; \
    } while(0);

#pragma region basic template
#define Tmpl_r_r(opcode_other, opcode_8bit, modRM, dtmdf, src, dst) \
    Inst ins; \
    int st = 0; uint8 opcode = (opcode_other); \
    switch (dtmdf) { \
        case TM_none: \
            Inst_init(&ins, 3), st = 1; \
            ins.data[0] = 0x40 | ((uint8)(((src) >> 3) & 1) << 2) | (uint8)(((dst) >> 3) & 1); \
            opcode = (opcode_8bit); \
            break; \
        case TM_word: \
            if (src >= R8 || dst >= R8) { \
                Inst_init(&ins, 4), st = 2; \
                ins.data[1] = 0x40 | ((uint8)(((src) >> 3) & 1) << 2) | (uint8)(((dst) >> 3) & 1); \
            } else Inst_init(&ins, 3), st = 1; \
            ins.data[0] = 0x66; \
            break; \
        case TM_dword: \
            if (src >= R8 || dst >= R8) { \
                Inst_init(&ins, 3), st = 1; \
                ins.data[0] = 0x40 | ((uint8)(((src) >> 3) & 1) << 2) | (uint8)(((dst) >> 3) & 1); \
            } else Inst_init(&ins, 2); \
            break; \
        case TM_qword: \
            Inst_init(&ins, 3), st = 1; \
            ins.data[0] = 0x48 | ((uint8)(((src) >> 3) & 1) << 2) | (uint8)(((dst) >> 3) & 1); \
            break; \
    } \
    ins.data[st + 0] = opcode; \
    ins.data[st + 1] = modRM | (uint8)((((src) & 0x7) << 3) | ((dst) & 0x7)); \
    return ins; \

#define Tmpl_or_r(opcode_other, opcode_8bit, dtmdf, offset, reg, dst) \
    Inst ins; \
    int st = 0; uint8 opcode = (opcode_other), modRM; \
    switch (dtmdf) { \
        case TM_none: \
            setInstSize_or(ins, offset, 3); \
            st = 1; \
            ins.data[0] = 0x40 | ((uint8)(((dst) >> 3) & 1) << 2) | (uint8)(((reg) >> 3) & 1); \
            opcode = (opcode_8bit); \
            break; \
        case TM_word: \
            if (reg >= R8 || dst >= R8) { \
                setInstSize_or(ins, offset, 4); st = 2; \
                ins.data[1] = 0x40 | ((uint8)(((dst) >> 3) & 1) << 2) | (uint8)(((reg) >> 3) & 1); \
            } else { setInstSize_or(ins, offset, 3); st = 1; } \
            ins.data[0] = 0x66; \
            break; \
        case TM_dword: \
            if (reg >= R8 || dst >= R8) { \
                setInstSize_or(ins, offset, 3); st = 1; \
                ins.data[0] = 0x40 | ((uint8)(((dst) >> 3) & 1) << 2) | (uint8)(((reg) >> 3) & 1); \
            } else setInstSize_or(ins, offset, 2); \
            break; \
        case TM_qword: \
            setInstSize_or(ins, offset, 3); st = 1; \
            ins.data[0] = 0x48 | ((uint8)(((dst) >> 3) & 1) << 2) | (uint8)(((reg) >> 3) & 1); \
            break; \
    } \
    ins.data[st + 0] = opcode; \
    ins.data[st + 1] = modRM | (uint8)((((dst) & 0x7) << 3) | ((reg) & 0x7)); \
    *(uint32 *)&ins.data[st + 2] = offset; \
    return ins;

#define Tmpl_rrs_r(opcode_other, opcode_8bit, dtmdf, reg1, reg2, scale, dst) \
    Inst ins; \
    int st = 0; uint8 opcode = (opcode_other); \
    switch (dtmdf) { \
        case TM_none: \
            Inst_init(&ins, 4), st = 1; \
            ins.data[0] = 0x40 | ((uint8)(((dst) >> 3) & 1) << 2) | ((uint8)(((reg2) >> 3) & 1) << 1) | (uint8)(((reg1) >> 3) & 1); \
            opcode = (opcode_8bit); \
            break; \
        case TM_word: \
            if (reg1 >= R8 || reg2 >= R8 || dst >= R8) { \
                Inst_init(&ins, 5), st = 2; \
                ins.data[1] = 0x40 | ((uint8)(((dst) >> 3) & 1) << 2) | ((uint8)(((reg2) >> 3) & 1) << 1) | (uint8)(((reg1) >> 3) & 1); \
            } else Inst_init(&ins, 4), st = 1; \
            ins.data[0] = 0x66; \
            break; \
        case TM_dword: \
            if (reg1 >= R8 || reg2 >= R8 || dst >= R8) { \
                Inst_init(&ins, 4), st = 1; \
                ins.data[0] = 0x40 | ((uint8)(((dst) >> 3) & 1) << 2) | ((uint8)(((reg2) >> 3) & 1) << 1) | (uint8)(((reg1) >> 3) & 1); \
            } else Inst_init(&ins, 3); \
            break; \
        case TM_qword: \
            Inst_init(&ins, 4), st = 1; \
            ins.data[0] = 0x48 | ((uint8)(((dst) >> 3) & 1) << 2) | ((uint8)(((reg2) >> 3) & 1) << 1) | (uint8)(((reg1) >> 3) & 1); \
            break; \
    } \
    ins.data[st + 0] = opcode; \
    ins.data[st + 1] = 0x00 | (uint8)((((dst) & 0x7) << 3)) | 0x4; \
    ins.data[st + 2] = (fakeLog2(scale) << 6) | ((uint8)(((reg2) & 0x7) << 3)) | ((uint8)((reg1) & 0x7)); \
    return ins;
#define Tmpl_orrs_r(opcode_other, opcode_8bit, dtmdf, offset, reg1, reg2, scale, dst) \
    Inst ins; \
    int st = 0; uint8 opcode = (opcode_other); \
    switch (dtmdf) { \
        case TM_none: \
            Inst_init(&ins, 8), st = 1; \
            ins.data[0] = 0x40 | ((uint8)(((dst) >> 3) & 1) << 2) | ((uint8)(((reg2) >> 3) & 1) << 1) | (uint8)(((reg1) >> 3) & 1); \
            opcode = (opcode_8bit); \
            break; \
        case TM_word: \
            if (reg1 >= R8 || reg2 >= R8 || dst >= R8) { \
                Inst_init(&ins, 9), st = 2; \
                ins.data[1] = 0x40 | ((uint8)(((dst) >> 3) & 1) << 2) | ((uint8)(((reg2) >> 3) & 1) << 1) | (uint8)(((reg1) >> 3) & 1); \
            } else Inst_init(&ins, 8), st = 1; \
            ins.data[0] = 0x66; \
            break; \
        case TM_dword: \
            if (reg1 >= R8 || reg2 >= R8 || dst >= R8) { \
                Inst_init(&ins, 8), st = 1; \
                ins.data[0] = 0x40 | ((uint8)(((dst) >> 3) & 1) << 2) | ((uint8)(((reg2) >> 3) & 1) << 1) | (uint8)(((reg1) >> 3) & 1); \
            } else Inst_init(&ins, 7); \
            break; \
        case TM_qword: \
            Inst_init(&ins, 8), st = 1; \
            ins.data[0] = 0x48 | ((uint8)(((dst) >> 3) & 1) << 2) | ((uint8)(((reg2) >> 3) & 1) << 1) | (uint8)(((reg1) >> 3) & 1); \
            break; \
    } \
    ins.data[st + 0] = opcode; \
    ins.data[st + 1] = 0x80 | (uint8)((((dst) & 0x7) << 3)) | 0x4; \
    ins.data[st + 2] = (fakeLog2(scale) << 6) | ((uint8)(((reg2) & 0x7) << 3)) | ((uint8)((reg1) & 0x7)); \
    *(uint32 *)&ins.data[st + 3] = offset; \
    return ins;

#define Tmpl_i32_r(opcode_other, opcode_8bit, rmMod, dtmdf, imm, reg) \
    Inst ins; \
    int st = 0; uint8 opcode = (opcode_other); \
    switch (dtmdf) { \
        case TM_none: \
            Inst_init(&ins, 4), st = 1; \
            ins.data[0] = 0x40 | (uint8)(((reg) >> 3) & 1); \
            opcode = (opcode_8bit); \
            ins.data[st + 2] = imm; \
            break; \
        case TM_word: \
            if (reg >= R8) { \
                Inst_init(&ins, 6), st = 2; \
                ins.data[1] = 0x40 | (uint8)(((reg) >> 3) & 1); \
            } else Inst_init(&ins, 5), st = 1; \
            ins.data[0] = 0x66; \
            *(uint16 *)&ins.data[st + 2] = (uint16)imm; \
            break; \
        case TM_dword: \
            if (reg >= R8) { \
                Inst_init(&ins, 7), st = 1; \
                ins.data[0] = 0x40 | (uint8)(((reg) >> 3) & 1); \
            } else Inst_init(&ins, 6); \
            *(uint32 *)&ins.data[st + 2] = imm; \
            break; \
        case TM_qword: \
            Inst_init(&ins, 7), st = 1; \
            ins.data[0] = 0x48 | (uint8)(((reg) >> 3) & 1); \
            *(uint64 *)&ins.data[st + 2] = imm; \
            break; \
    } \
    ins.data[st + 0] = opcode; \
    ins.data[st + 1] = rmMod | (uint8)((reg) & 0x7); \
    return ins;
#pragma endregion

#pragma region template for movd and movq
#define Tmpl_MOV_r_x(opcode, suf, modRM, dtmdf, reg, xmm) \
    Inst ins; \
    int st = 1; \
    switch (dtmdf) { \
        case TM_float32: \
            if (reg >= R8 || xmm >= XMM8) \
                Inst_init(&ins, 5), st = 2, \
                ins.data[1] = 0x40 | (uint8)((reg >> 3) & 1) | (uint8)(((xmm >> 3) & 1) << 2); \
            else Inst_init(&ins, 4); \
            break; \
        case TM_float64: \
            Inst_init(&ins, 5), st = 2; \
            ins.data[1] = 0x48 | (uint8)((reg >> 3) & 1) | (uint8)(((xmm >> 3) & 1) << 2); \
            break; \
    } \
    ins.data[0] = 0x66, ins.data[st + 0] = (opcode); \
    ins.data[st + 1] = (suf); \
    ins.data[st + 2] = (modRM) | ((uint8)(xmm & 0x7) << 3) | (reg & 0x7); \
    return ins;
#define Tmpl_MOV_or_x(opcode, suf, dtmdf, offset, reg, xmm) \
    Inst ins; \
    int st = 1; \
    switch (dtmdf) { \
        case TM_float32: \
            if (reg >= R8 || xmm >= XMM8) \
                Inst_init(&ins, 9), st = 2, \
                ins.data[1] = 0x40 | (uint8)((reg >> 3) & 1) | (uint8)(((xmm >> 3) & 1) << 2); \
            else Inst_init(&ins, 8); \
            break; \
        case TM_float64: \
            Inst_init(&ins, 9), st = 2; \
            ins.data[1] = 0x48 | (uint8)((reg >> 3) & 1) | (uint8)(((xmm >> 3) & 1) << 2); \
            break; \
    } \
    ins.data[0] = 0x66, ins.data[st + 0] = (opcode); \
    ins.data[st + 1] = (suf); \
    ins.data[st + 2] = 0x80 | ((uint8)(xmm & 0x7) << 3) | (reg & 0x7); \
    *(uint32 *)&ins.data[st + 3] = (offset); \
    return ins;

#define Tmpl_MOV_rrs_x(opcode, suf, dtmdf, reg1, reg2, scale, xmm) \
    Inst ins; \
    int st = 1; \
    switch (dtmdf) { \
        case TM_float32: \
            if (reg1 >= R8 || reg2 >= R8 || xmm >= XMM8) { \
                Inst_init(&ins, 6), st = 2; \
                ins.data[1] = 0x40 | (uint8)(((xmm >> 3) & 1) << 2) | (uint8)(((reg2 >> 3) & 1) << 1) | (uint8)((reg1 >> 3) & 1); \
            } else Inst_init(&ins, 5); \
            break; \
        case TM_float64: \
            Inst_init(&ins, 6), st = 2; \
            ins.data[1] = 0x48 | (uint8)(((xmm >> 3) & 1) << 2) | (uint8)(((reg2 >> 3) & 1) << 1) | (uint8)((reg1 >> 3) & 1); \
            break; \
    } \
    ins.data[0] = 0x66, ins.data[st + 0] = (opcode); \
    ins.data[st + 1] = (suf); \
    ins.data[st + 2] = 0x00 | ((uint8)(xmm & 0x7) << 3) | 0x4; \
    ins.data[st + 3] = (fakeLog2(scale) << 6) | (uint8)(((reg2) & 0x7) << 3) | (reg1 & 0x7); \
    return ins;
    
#define Tmpl_MOV_orrs_x(opcode, suf, dtmdf, offset, reg1, reg2, scale, xmm) \
    Inst ins; \
    int st = 1; \
    switch (dtmdf) { \
        case TM_float32: \
            if (reg1 >= R8 || reg2 >= R8 || xmm >= XMM8) { \
                Inst_init(&ins, 10), st = 2; \
                ins.data[1] = 0x40 | (uint8)(((xmm >> 3) & 1) << 2) | (uint8)(((reg2 >> 3) & 1) << 1) | (uint8)((reg1 >> 3) & 1); \
            } else Inst_init(&ins, 9); \
            break; \
        case TM_float64: \
            Inst_init(&ins, 10), st = 2; \
            ins.data[1] = 0x48 | (uint8)(((xmm >> 3) & 1) << 2) | (uint8)(((reg2 >> 3) & 1) << 1) | (uint8)((reg1 >> 3) & 1); \
            break; \
    } \
    ins.data[0] = 0x66; \
    ins.data[st + 0] = (opcode); \
    ins.data[st + 1] = (suf); \
    ins.data[st + 2] = 0x80 | ((uint8)(xmm & 0x7) << 3) | 0x4; \
    ins.data[st + 3] = (fakeLog2(scale) << 6) | (uint8)(((reg2) & 0x7) << 3) | (reg1 & 0x7); \
    *(uint32 *)&ins.data[st + 4] = offset; \
    return ins;
#pragma endregion

#pragma region template for addss, addsd, subss, subsd, mulss, mulsd, divss, divsd
#define Tmpl_x_x(opcode, suf, dtmdf, xmm1, xmm2) \
    Inst ins; int st = 1; \
    uint8 pfx = (dtmdf == TM_float32 ? 0xF3 : 0xF2); \
    if (xmm1 >= XMM8 || xmm2 >= XMM8) { \
        Inst_init(&ins, 5), st = 2; \
        ins.data[1] = 0x40 | (uint8)(((xmm2 >> 3) & 1) << 2) | (uint8)((xmm1 >> 3) & 1); \
    } else Inst_init(&ins, 4); \
    ins.data[0] = pfx, ins.data[st + 0] = opcode, ins.data[st + 1] = suf; \
    ins.data[st + 2] = 0xC0 | (uint8)((xmm2 & 0x7) << 3) | (uint8)((xmm1 & 0x7)); \
    return ins;

#define Tmpl_mr_x(opcode, suf, dtmdf, reg, dst) \
    Inst ins; int st = 1; \
    uint8 pfx = (dtmdf == TM_float32 ? 0xF3 : 0xF2); \
    if (reg >= XMM8 || dst >= XMM8) { \
        Inst_init(&ins, 5), st = 2; \
        ins.data[1] = 0x40 | (uint8)(((dst >> 3) & 1) << 2) | (uint8)((reg >> 3) & 1); \
    } else Inst_init(&ins, 4); \
    ins.data[0] = pfx, ins.data[st + 0] = opcode, ins.data[st + 1] = suf; \
    ins.data[st + 2] = 0x00 | (uint8)((dst & 0x7) << 3) | (uint8)((reg & 0x7)); \
    return ins;

#define Tmpl_or_x(opcode, suf, dtmdf, offset, reg, dst) \
    Inst ins; int st = 1; \
    uint8 pfx = (dtmdf == TM_float32 ? 0xF3 : 0xF2), modRM; \
    if (reg >= XMM8 || dst >= XMM8) { \
        setInstSize_or(ins, offset, 5); st = 2; \
        ins.data[1] = 0x40 | (uint8)(((dst >> 3) & 1) << 2) | (uint8)((reg >> 3) & 1); \
    } else setInstSize_or(ins, offset, 4); \
    ins.data[0] = pfx, ins.data[st + 0] = opcode, ins.data[st + 1] = suf; \
    ins.data[st + 2] = 0x80 | (uint8)((dst & 0x7) << 3) | (uint8)((reg & 0x7)); \
    return ins;

#define Tmpl_rrs_x(opcode, suf, dtmdf, reg1, reg2, scale, dst) \
    Inst ins; int st = 1; \
    uint8 pfx = (dtmdf == TM_float32 ? 0xF3 : 0xF2); \
    if (reg1 >= R8 || reg2 >= R8 || dst >= XMM8) { \
        Inst_init(&ins, 6); st = 2; \
        ins.data[1] = 0x40 | (uint8)(((dst >> 3) & 1) << 2) | (uint8)(((reg2 >> 3) & 1) << 1) | (uint8)((reg1 >> 3) & 1); \
    } else Inst_init(&ins, 5); \
    ins.data[0] = pfx, ins.data[st + 0] = opcode, ins.data[st + 1] = suf; \
    ins.data[st + 2] = 0x00 | (uint8)((dst & 0x7) << 3) | 0x04; \
    ins.data[st + 3] = (fakeLog2(scale) << 6) | (uint8)(((reg2) & 0x7) << 3) | (uint8)(reg1 & 0x7); \
    return ins;

#define Tmpl_orrs_x(opcode, suf, dtmdf, offset, reg1, reg2, scale, dst) \
    Inst ins; int st = 1; \
    uint8 pfx = (dtmdf == TM_float32 ? 0xF3 : 0xF2); \
    if (reg1 >= R8 || reg2 >= R8 || dst >= XMM8) { \
        Inst_init(&ins, 10), st = 2; \
        ins.data[1] = 0x40 | (uint8)(((dst >> 3) & 1) << 2) | (uint8)(((reg2 >> 3) & 1) << 1) | (uint8)((reg1 >> 3) & 1); \
    } else Inst_init(&ins, 9); \
    ins.data[0] = pfx, ins.data[st + 0] = opcode, ins.data[st + 1] = suf; \
    ins.data[st + 2] = 0x80 | (uint8)((dst & 0x7) << 3) | 0x04; \
    ins.data[st + 3] = (fakeLog2(scale) << 6) | (uint8)(((reg2) & 0x7) << 3) | (uint8)(reg1 & 0x7); \
    *(uint32 *)&ins.data[st + 4] = offset; \
    return ins;

#pragma endregion

#pragma region template for mul, div, imul, idiv
#define Tmpl_r(opcode_other, other_8bit, rmMod, dtmdf, reg) \
    Inst ins; \
    int st = 0; uint8 opcode = (opcode_other); \
    switch (dtmdf) { \
        case TM_none: \
            Inst_init(&ins, 3), st = 1; \
            ins.data[0] = 0x40 | (uint8)(((reg) >> 3) & 1); \
            opcode = (other_8bit); \
            break; \
        case TM_word: \
            if (reg >= R8) { \
                Inst_init(&ins, 4), st = 2; \
                ins.data[1] = 0x40 | (uint8)(((reg) >> 3) & 1); \
            } else Inst_init(&ins, 3), st = 1; \
            ins.data[0] = 0x66; \
            break; \
        case TM_dword: \
            if (reg >= R8) { \
                Inst_init(&ins, 3), st = 1; \
                ins.data[0] = 0x40 | (uint8)(((reg) >> 3) & 1); \
            } else Inst_init(&ins, 2); \
            break; \
        case TM_qword: \
            Inst_init(&ins, 3), st = 1; \
            ins.data[0] = 0x48 | (uint8)(((reg) >> 3) & 1); \
            break; \
    } \
    ins.data[st + 0] = opcode; \
    ins.data[st + 1] = rmMod | (uint8)((reg) & 0x7); \
    return ins;

// instruction offset(%reg)
#define Tmpl_or(opcode_other, other_8bit, rmMod, dtmdf, reg, offset) \
    Inst ins; \
    int st = 0; uint8 opcode = (opcode_other), modRM; \
    switch (dtmdf) { \
        case TM_none: \
            setInstSize_or(ins, offset, 3); st = 1; \
            ins.data[0] = 0x40 | (uint8)(((reg) >> 3) & 1); \
            opcode = (other_8bit); \
            break; \
        case TM_word: \
            if (reg >= R8) { \
                setInstSize_or(ins, offset, 4); st = 2; \
                ins.data[1] = 0x40 | (uint8)(((reg) >> 3) & 1); \
            } else { setInstSize_or(ins, offset, 3); st = 1; } \
            ins.data[0] = 0x66; \
            break; \
        case TM_dword: \
            if (reg >= R8) { \
                setInstSize_or(ins, offset, 3); st = 1; \
                ins.data[0] = 0x40 | (uint8)(((reg) >> 3) & 1); \
            } else setInstSize_or(ins, offset, 2); \
            break; \
        case TM_qword: \
            setInstSize_or(ins, offset, 3); st = 1; \
            ins.data[0] = 0x48 | (uint8)(((reg) >> 3) & 1); \
            break; \
    } \
    ins.data[st + 0] = opcode; \
    ins.data[st + 1] = rmMod | modRM | (uint8)((reg) & 0x7); \
    return ins;
#pragma endregion

#pragma region template for ucomiss and ucomisd
#define Tmpl_UCOMI_r_r(opcode, suf, modRM, dtmdf, src, dst) \
    Inst ins; int st = 0; \
    switch (dtmdf) { \
        case TM_float32: \
            if (src >= XMM8 || dst >= XMM8) \
                Inst_init(&ins, 4), st = 1, \
                ins.data[0] = 0x40 | (uint8)(((dst >> 3) & 1) << 2) | (uint8)((src >> 3) & 1); \
            else Inst_init(&ins, 3); \
            break; \
        case TM_float64: \
            if (src >= XMM8 || dst >= XMM8) \
                Inst_init(&ins, 5), st = 2, \
                ins.data[1] = 0x40 | (uint8)(((dst >> 3) & 1) << 2) | (uint8)((src >> 3) & 1); \
            else Inst_init(&ins, 4), st = 1; \
            ins.data[0] = 0x66; \
            break; \
    } \
    ins.data[st + 0] = opcode, ins.data[st + 1] = suf; \
    ins.data[st + 2] = modRM | (uint8)((dst & 0x7) << 3) | (uint8)(src & 0x7); \
    return ins;

#define Tmpl_UCOMI_or_r(opcode, suf, dtmdf, offset, src, dst) \
    Inst ins; int st = 0; \
    uint8 modRM; \
    switch (dtmdf) { \
        case TM_float32: \
            if (src >= XMM8 || dst >= XMM8) { \
                setInstSize_or(ins, offset, 4); st = 1, \
                ins.data[0] = 0x40 | (uint8)(((dst >> 3) & 1) << 2) | (uint8)((src >> 3) & 1); \
            } else setInstSize_or(ins, offset, 3); \
            break; \
        case TM_float64: \
            if (src >= XMM8 || dst >= XMM8) { \
                setInstSize_or(ins, offset, 5); st = 2, \
                ins.data[1] = 0x40 | (uint8)(((dst >> 3) & 1) << 2) | (uint8)((src >> 3) & 1); \
            } else { setInstSize_or(ins, offset, 4); st = 1; } \
            ins.data[0] = 0x66; \
            break; \
    } \
    ins.data[st + 0] = opcode, ins.data[st + 1] = suf; \
    ins.data[st + 2] = modRM | (uint8)((dst & 0x7) << 3) | (uint8)(src & 0x7); \
    return ins;

#define Tmpl_UCOMI_rrs_r(opcode, suf, dtmdf, reg1, reg2, scale, dst) \
    Inst ins; int st = 0; \
    switch (dtmdf) { \
        case TM_float32: \
            if (reg1 >= XMM8 || reg2 >= XMM8 || dst >= XMM8) \
                Inst_init(&ins, 5), st = 1, \
                ins.data[0] = 0x40 | (uint8)(((dst >> 3) & 1) << 2) | (uint8)(((reg2 >> 3) & 1) << 1) | (uint8)((reg1 >> 3) & 1); \
            else Inst_init(&ins, 4); \
            break; \
        case TM_float64: \
            if (reg1 >= XMM8 || reg2 >= XMM8 || dst >= XMM8) \
                Inst_init(&ins, 6), st = 2, \
                ins.data[1] = 0x40 | (uint8)(((dst >> 3) & 1) << 2) | (uint8)(((reg2 >> 3) & 1) << 1) | (uint8)((reg1 >> 3) & 1); \
            else Inst_init(&ins, 5), st = 1; \
            ins.data[0] = 0x66; \
            break; \
    } \
    ins.data[st + 0] = opcode, ins.data[st + 1] = suf; \
    ins.data[st + 2] = 0x80 | (uint8)((dst & 0x7) << 3) | 0x4; \
    ins.data[st + 3] = (uint8)(fakeLog2(scale) << 6) | (uint8)((reg2 & 0x7) << 3) | (uint8)(reg1 & 0x7); \
    return ins;

#define Tmpl_UCOMI_orrs_r(opcode, suf, dtmdf, offset, reg1, reg2, scale, dst) \
    Inst ins; int st = 0; \
    switch (dtmdf) { \
        case TM_float32: \
            if (reg1 >= XMM8 || reg2 >= XMM8 || dst >= XMM8) \
                Inst_init(&ins, 9), st = 1, \
                ins.data[0] = 0x40 | (uint8)(((dst >> 3) & 1) << 2) | (uint8)(((reg2 >> 3) & 1) << 1) | (uint8)((reg1 >> 3) & 1); \
            else Inst_init(&ins, 8); \
            break; \
        case TM_float64: \
            if (reg1 >= XMM8 || reg2 >= XMM8 || dst >= XMM8) \
                Inst_init(&ins, 10), st = 2, \
                ins.data[1] = 0x40 | (uint8)(((dst >> 3) & 1) << 2) | (uint8)(((reg2 >> 3) & 1) << 1) | (uint8)((reg1 >> 3) & 1); \
            else Inst_init(&ins, 9), st = 1; \
            ins.data[0] = 0x66; \
            break; \
    } \
    ins.data[st + 0] = opcode, ins.data[st + 1] = suf; \
    ins.data[st + 2] = 0x80 | (uint8)((dst & 0x7) << 3) | 0x4; \
    ins.data[st + 3] = (uint8)(fakeLog2(scale) << 6) | (uint8)((reg2 & 0x7) << 3) | (uint8)(reg1 & 0x7); \
    *(uint32 *)&ins.data[st + 4] = offset; \
    return ins;
#pragma endregion

#pragma region template for shift operator 
#define Tmpl_Shift_i_r(opcode_other, opcode_8bit, modRM, dtmdf, imm, reg) \
    Inst ins; \
    int st = 0; uint8 opcode = (opcode_other); \
    switch (dtmdf) { \
        case TM_none: \
            Inst_init(&ins, 4), st = 1, \
            ins.data[0] = 0x40 | (uint8)((reg >> 3) & 1); \
            opcode = (opcode_8bit); \
            break; \
        case TM_word: \
            if (reg >= R8) \
                Inst_init(&ins, 5), st = 2, \
                ins.data[1] = 0x40 | (uint8)((reg >> 3) & 1); \
            else Inst_init(&ins, 4), st = 1; \
            ins.data[0] = 0x66; \
            break; \
         case TM_dword: \
            if (reg >= R8) \
                Inst_init(&ins, 4), st = 1, \
                ins.data[0] = 0x40 | (uint8)((reg >> 3) & 1); \
            else  Inst_init(&ins, 3); \
            break; \
        case TM_qword: \
            Inst_init(&ins, 4), st = 1; \
            ins.data[0] = 0x48 | (uint8)((reg >> 3) & 1); \
            break; \
    } \
    ins.data[st + 0] = opcode; \
    ins.data[st + 1] = modRM | (reg & 0x7); \
    ins.data[st + 2] = imm; \
    return ins;

#define Tmpl_Shift_CL_r(opcode_other, opcode_8bit, modRM, dtmdf, reg) \
    Inst ins; \
    int st = 0; uint8 opcode = (opcode_other); \
    switch (dtmdf) { \
        case TM_none: \
            Inst_init(&ins, 3), st = 1, \
            ins.data[0] = 0x40 | (uint8)((reg >> 3) & 1); \
            opcode = (opcode_8bit); \
            break; \
        case TM_word: \
            if (reg >= R8) \
                Inst_init(&ins, 4), st = 2, \
                ins.data[1] = 0x40 | (uint8)((reg >> 3) & 1); \
            else Inst_init(&ins, 3), st = 1; \
            ins.data[0] = 0x66; \
            break; \
         case TM_dword: \
            if (reg >= R8) \
                Inst_init(&ins, 3), st = 1, \
                ins.data[0] = 0x40 | (uint8)((reg >> 3) & 1); \
            else Inst_init(&ins, 2); \
            break; \
        case TM_qword: \
            Inst_init(&ins, 3), st = 1; \
            ins.data[0] = 0x48 | (uint8)((reg >> 3) & 1); \
            break; \
    } \
    ins.data[st + 0] = opcode; \
    ins.data[st + 1] = modRM | (reg & 0x7); \
    return ins;

#define Tmpl_Shift_CL_or(opcode_other, opcode_8bit, modRM, dtmdf, offset, reg) \
    Inst ins; \
    int st = 0; uint8 opcode = (opcode_other); \
    switch (dtmdf) { \
        case TM_none: \
            Inst_init(&ins, 7), st = 1, \
            ins.data[0] = 0x40 | (uint8)((reg >> 3) & 1); \
            opcode = (opcode_8bit); \
            break; \
        case TM_word: \
            if (reg >= R8) \
                Inst_init(&ins, 8), st = 2, \
                ins.data[1] = 0x40 | (uint8)((reg >> 3) & 1); \
            else Inst_init(&ins, 7), st = 1; \
            ins.data[0] = 0x66; \
            break; \
         case TM_dword: \
            if (reg >= R8) \
                Inst_init(&ins, 7), st = 1, \
                ins.data[0] = 0x40 | (uint8)((reg >> 3) & 1); \
            else Inst_init(&ins, 6); \
            break; \
        case TM_qword: \
            Inst_init(&ins, 7), st = 1; \
            ins.data[0] = 0x48 | (uint8)((reg >> 3) & 1); \
            break; \
    } \
    ins.data[st + 0] = opcode; \
    ins.data[st + 1] = modRM | (reg & 0x7); \
    *(uint32 *)(ins.data + st + 2) = offset; \
    return ins;
#pragma endregion

#define Tmpl_jcc(opcode, delta) \
    Inst ins; \
    Inst_init(&ins, 6); \
    ins.data[0] = 0x0F; \
    ins.data[1] = (opcode); \
    *(uint32 *)&ins.data[2] = (delta); \
    return ins;

#define Tmpl_jmp_r(opcode, modRM, reg) \
    Inst ins; \
    int st = 0; \
    if (reg >= R8) { \
        Inst_init(&ins, 3), st = 1; \
        ins.data[0] = 0x40 | ((reg >> 3) & 1); \
    } else Inst_init(&ins, 2); \
    ins.data[st + 0] = opcode; \
    ins.data[st + 1] = modRM | (reg & 0x7); \
    return ins;

