#pragma once
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <stack>
#include <queue>
#include <map>
#include <set>

#include "message.h"

typedef unsigned long long uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef long long int64;
typedef int int32;
typedef short int16;
typedef char int8;
typedef float float32;
typedef double float64;

union Data {
    uint64 uint64_v;
    uint32 uint32_v;
    uint16 uint16_v;
    uint8 uint8_v;
    int64 int64_v;
    int32 int32_v;
    int16 int16_v;
    int8 int8_v;

    float32 float32_v;
    float64 float64_v;
};

enum class DataTypeModifier {
    c, b, i16, u16, i32, u32, i64, u64, f32, f64, o, B, unknown,
};

struct UnionData {
    Data data;
    DataTypeModifier type;
    UnionData();
    UnionData(DataTypeModifier type);
};


enum class ValueTypeModifier {
    mr, r, t, unknown,
};

enum class IdentifierVisibility {
    Private, Public, Protected, unknown
};
DataTypeModifier getDataTypeModifier(const std::string &name);
ValueTypeModifier getValueTypeModifier(const std::string &name);
IdentifierVisibility getIdentifierVisibility(const std::string &name);
extern const std::string dataTypeModifierString[], valueTypeModifierString[], identifierVisibilityString[];
extern const int dataTypeModifierNumber, valueTypeModifierNumber, identifierVisibilityNumber;

bool isInteger(DataTypeModifier dtMfr);
bool isRefereence(ValueTypeModifier vlMfr);

/// @brief separate STR using SEP and return RES
/// @param str 
/// @param sep 
/// @param res 
void stringSplit(const std::string &str, const std::string &sep, std::vector<std::string> &res);
/// @brief separate STR using SEP and return RES
/// @param str 
/// @param sep 
/// @param res 
void stringSplit(const std::string &str, const char &sep, std::vector<std::string> &res);

void readData(std::ifstream &ifs, UnionData &data);
void writeData(std::ofstream &ofs, const UnionData &data);
void readString(std::ifstream &ifs, std::string &str);
void writeString(std::ofstream &ofs, const std::string &str);

UnionData getUnionData(const std::string &str);
std::string getString(const std::string &str, int st, int &ed);

std::string toString(const UnionData &data);

#define isLetter(ch) (('a' <= (ch) && (ch) <= 'z') || ('A' <= (ch) && (ch) <= 'Z') || (ch) == '_')
#define isNumber(ch) ('0' <= (ch) && (ch) <= '9')
#define toDigtal(ch) (('a' <= (ch) && ch <= 'f') ? ((ch) - 'a' + 10) : (('A' <= (ch) && (ch) <= 'F') ? ((ch) - 'A' + 10) : (ch) - '0'))
#define isSeparator(ch) ((ch) == ' ' || (ch) == '\t' || (ch) == '\n' || (ch) == '\r') 