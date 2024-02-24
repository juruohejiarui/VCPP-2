#pragma once
#include <sstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <stack>
#include <queue>
#include <cmath>
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
    c, b, i16, u16, i32, u32, i64, u64, f32, f64, o, gv0, gv1, gv2, gv3, gv4, unknown,
};

struct UnionData {
    Data data;
    DataTypeModifier type;
    UnionData();
    UnionData(DataTypeModifier type);

    UnionData(uint8 dt);
    UnionData(int8 dt);
    UnionData(uint16 dt);
    UnionData(int16 dt);
    UnionData(uint32 dt);
    UnionData(int32 dt);
    UnionData(uint64 dt);
    UnionData(int64 dt);
    UnionData(float32 dt);
    UnionData(float64 dt);

    uint8 &uint8_v();
    int8 &int8_v();

    uint16 &uint16_v();
    int16 &int16_v();

    uint32 &uint32_v();
    int32 &int32_v();

    uint64 &uint64_v();
    int64 &int64_v();

    float32 &float32_v();
    float64 &float64_v();

    uint8 uint8_v() const;
    int8 int8_v() const;

    uint16 uint16_v() const;
    int16 int16_v() const;

    uint32 uint32_v() const;
    int32 int32_v() const;

    uint64 uint64_v() const;
    int64 int64_v() const;

    float32 float32_v() const;
    float64 float64_v() const;
};


enum class ValueTypeModifier {
    MemberRef, Ref, TrueValue, Unknown,
};

enum class IdenVisibility {
    Private, Protected, Public, Unknown
};
DataTypeModifier getDataTypeModifier(const std::string &name);
ValueTypeModifier getValueTypeModifier(const std::string &name);
IdenVisibility getIdenVisibility(const std::string &name);
extern const std::string dataTypeModifierStr[], valueTypeModifierStr[], idenVisibilityStr[];
extern const int dataTypeModifierNumber, valueTypeModifierNumber, identifierVisibilityNumber;

bool isInteger(DataTypeModifier dtMfr);
bool isReference(ValueTypeModifier vlMfr);

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

/// @brief read data from a binary file, the size of the data relies on data.type which should be set before calling this function
/// @param ifs the stream of the binary file
/// @param data the data
void readData(std::ifstream &ifs, UnionData &data);
/// @brief write the data to a binary file, the size of the data relies on data.type which should be set before calling this function
/// @param ofs the stream of the binary file
/// @param data the data
void writeData(std::ofstream &ofs, const UnionData &data);
void readString(std::ifstream &ifs, std::string &str);
void writeString(std::ofstream &ofs, const std::string &str);

UnionData getUnionData(const std::string &str);
std::string getString(const std::string &str, size_t st, size_t &ed);
std::string toCodeString(const std::string &str);

std::string toString(const UnionData &data);
std::string toString(uint64 data, int base);

#define isLetter(ch) (('a' <= (ch) && (ch) <= 'z') || ('A' <= (ch) && (ch) <= 'Z') || (ch) == '_')
#define isNumber(ch) ('0' <= (ch) && (ch) <= '9')
#define toDigtal(ch) (('a' <= (ch) && ch <= 'f') ? ((ch) - 'a' + 10) : (('A' <= (ch) && (ch) <= 'F') ? ((ch) - 'A' + 10) : (ch) - '0'))
#define isSeparator(ch) ((ch) == ' ' || (ch) == '\t' || (ch) == '\n' || (ch) == '\r') 

/// @brief This function can align the value to multiples of base
/// @param value the initial value
/// @param base the base
/// @return the value after aligned
uint64 alignTo(uint64 value, uint64 base);