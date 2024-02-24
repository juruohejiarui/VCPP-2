#include "geninner.h"

static std::ofstream oStream;

bool setOutputStream(const std::string &path) {
    oStream = std::ofstream(path, std::ios::out);
    return oStream.good();
}

void closeOutputStream() { oStream.close(); }

static int32 indent;
void indentInc() { indent++; }
void indentDec() { indent = std::max(0, --indent); }
int32 getIndentDep() { return indent; }

void writeVCode(std::string text) {
    oStream << getIndent(getIndentDep()) << text << std::endl;
}
void writeVCode(Command tcmd) { writeVCode(commandString[(int)tcmd]); }
void writeVCode(Command tcmd, const UnionData &data) { writeVCode(commandString[(int)tcmd] + " " + toString(data.uint64_v(), 16)); }
void writeVCode(Command tcmd, const UnionData &data1, const UnionData &data2) {
    writeVCode(commandString[(int)tcmd] + " " + toString(data1.uint64_v(), 16) + " " + toString(data2.uint64_v(), 16));
}
void writeVCode(Command tcmd, const std::string &str) { writeVCode(commandString[(int)tcmd] + " " + str); }
void writeVCode(const std::string &cmdStr, const std::string &str) { writeVCode(cmdStr + " " + str); }

DataTypeModifier getDtMdf(const ExprType &etype) {
    if (!isBasicCls(etype.cls)) return DataTypeModifier::o;
    else if (etype.cls == int8Cls) return DataTypeModifier::c;
    else if (etype.cls == uint8Cls) return DataTypeModifier::b;
    else if (etype.cls == int16Cls) return DataTypeModifier::i16;
    else if (etype.cls == uint16Cls) return DataTypeModifier::u16;
    else if (etype.cls == int32Cls) return DataTypeModifier::i32;
    else if (etype.cls == uint32Cls) return DataTypeModifier::u32;
    else if (etype.cls == int64Cls) return DataTypeModifier::i64;
    else if (etype.cls == uint64Cls) return DataTypeModifier::u64;
    else if (etype.cls == float32Cls) return DataTypeModifier::f32;
    else if (etype.cls == float64Cls) return DataTypeModifier::f64;
    
    return DataTypeModifier::unknown;
}
