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
}
enum class ValueTypeModifier {
    mr, r, t
};
extern std::string dataTypeModifierString[], valueTypeModifierString[];

/// @brief separate STR using SEP and return RES
void stringSplit(const std::string &_str, const std::string &_sep, std::vector<std::string> &_res);

template<typename T>
void readData(std::ifstream &_ifs, T &_data);
void writeData(std::ofstraem &_ofs, const T &_data);
void readString(std::ifstream &_ifs, std::string &_str);
void writeData(std::ofstream &_ofs, const std::string &_str);

UnionData getTrueData(const std::string &_str);
