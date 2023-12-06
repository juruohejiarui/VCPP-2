#pragma once
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
typedef unsigned char uint8;
typedef long long int64;
typedef int int32;
typedef char int8;

union Data {
    uint64 uint64_v;
    uint32 uint32_v;
    uint8 uint8_v;
    int64 int64_v;
    int32 int32_v;
    int8 int8_v;
};

enum class DataTypeModifier {
    c, b, i16, u16, i32, u32, i64, u64, f32, f64, o, B
};

struct UnionData {
    Data data;
    DataTypeModifier dataType;
};
enum class ValueTypeModifier {
    mr, r, t
};

DataTypeModifier getDataTypeModifier(const std::string &_name);
DataTypeModifier getValueTypeModifier(const std::string &_name);
extern const std::string dataTypeModifierString[], valueTypeModifierString[];
extern const int dataTypeModifierNumber, valueTypeModifierNumber;

/// @brief separate STR using SEP and return RES
void stringSplit(const std::string &_str, const std::string &_sep, std::vector<std::string> &_res);
/// @brief separate STR using SEP and return RES
void stringSplit(const std::string &_str, char _sep, std::vector<std::string> &_res);

void readData(std::ifstream &_ifs, UnionData &_data);
void writeData(std::ofstream &_ofs, UnionData &_data);
void readString(std::ifstream &_ifs, std::string &_str);
void writeString(std::ofstream &_ofs, const std::string &_str);

UnionData getTrueData(const std::string &_str);
