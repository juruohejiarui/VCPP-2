#include "gen.h"

static std::ofstream oStream;

bool setOutputStream(const std::string &path) {
    oStream = std::ofstream(path, std::ios::out);
    return oStream.good();
}

void closeOutputStream() { oStream.close(); }

static int32 indent;
void indentInc() { indent++; }
void indentDec() { indent = std::max(0, indent--); }
int32 getIndentDep() { return indent; }

void writeVCode(std::string text) {
    oStream << getIndent(getIndentDep()) << text << std::endl;
}
void writeVCode(Command tcmd) { writeVCode(tCommandString[(int)tcmd]); }
void writeVCode(Command tcmd, const UnionData &data) { writeVCode(tCommandString[(int)tcmd] + " " + toString(data.uint64_v(), 16)); }
void writeVCode(Command tcmd, const UnionData &data1, const UnionData &data2) {
    writeVCode(tCommandString[(int)tcmd] + " " + toString(data1.uint64_v(), 16) + " " + toString(data2.uint64_v(), 16));
}
void writeVCode(Command tcmd, const std::string &str) { writeVCode(tCommandString[(int)tcmd] + " " + str); }
void writeVCode(const std::string &cmdStr, const std::string &str) { writeVCode(cmdStr + " " + str); }