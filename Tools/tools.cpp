#include "tools.h"

const std::string dataTypeModifierStr[] = {"c", "b", "i16", "u16", "i32", "u32", "i64", "u64", "f32", "f64", "o", "gv0", "gv1", "gv2", "gv3", "gv4", "unknown"};
const std::string valueTypeModifierStr[] = {"mr", "r", "t", "unknown"};
const std::string idenVisibilityStr[] = {"private", "protected", "public", "unknown"};
const int dataTypeModifierNumber = 16, valueTypeModifierNumber = 3, identifierVisibilityNumber = 3;

UnionData::UnionData() { type = (DataTypeModifier)dataTypeModifierNumber, data.uint64_v = 0; }
UnionData::UnionData(DataTypeModifier type) { this->type = type, data.uint64_v = 0; }

UnionData::UnionData(uint8 dt) { type = DataTypeModifier::b, data.uint64_v = 0, data.uint8_v = dt; }
UnionData::UnionData(int8 dt) { type = DataTypeModifier::c, data.uint64_v = 0, data.int8_v = dt; }
UnionData::UnionData(uint16 dt) { type = DataTypeModifier::u16, data.uint64_v = 0, data.uint16_v = dt; }
UnionData::UnionData(int16 dt) { type = DataTypeModifier::i16, data.uint64_v = 0, data.int16_v = dt; }
UnionData::UnionData(uint32 dt) { type = DataTypeModifier::u32, data.uint64_v = 0, data.uint32_v = dt; }
UnionData::UnionData(int32 dt) { type = DataTypeModifier::i32, data.uint64_v = 0, data.int32_v = dt; }
UnionData::UnionData(uint64 dt) { type = DataTypeModifier::u64, data.uint64_v = dt; }
UnionData::UnionData(int64 dt) { type = DataTypeModifier::i64, data.uint64_v = dt; }
UnionData::UnionData(float32 dt) { type = DataTypeModifier::f32, data.uint64_v = 0, data.float32_v = dt; }
UnionData::UnionData(float64 dt) { type = DataTypeModifier::f32, data.uint64_v = 0, data.float64_v = dt; }


uint8 &UnionData::uint8_v() { return data.uint8_v; }
int8 &UnionData::int8_v() { return data.int8_v; }

uint16 &UnionData::uint16_v() { return data.uint16_v; }
int16 &UnionData::int16_v() { return data.int16_v; }

uint32 &UnionData::uint32_v() { return data.uint32_v; }
int32 &UnionData::int32_v() { return data.int32_v; }

uint64 &UnionData::uint64_v() { return data.uint64_v; }
int64 &UnionData::int64_v() { return data.int64_v; }

float32 &UnionData::float32_v() { return data.float32_v; }
float64 &UnionData::float64_v() { return data.float64_v; }

uint8 UnionData::uint8_v() const { return data.uint8_v; }
int8 UnionData::int8_v() const { return data.int8_v; }

uint16 UnionData::uint16_v() const { return data.uint16_v; }
int16 UnionData::int16_v() const { return data.int16_v; }

uint32 UnionData::uint32_v() const { return data.uint32_v; }
int32 UnionData::int32_v() const { return data.int32_v; }

uint64 UnionData::uint64_v() const { return data.uint64_v; }
int64 UnionData::int64_v() const { return data.int64_v; }

float32 UnionData::float32_v() const { return data.float32_v; }
float64 UnionData::float64_v() const { return data.float64_v; }

UnionData UnionData::convertTo(DataTypeModifier tgType) {
    if (type == tgType || isInteger(tgType) == isInteger(type)) {
        auto res = UnionData(*this); res.type = tgType;
        return res;
    }
    if (isUnsignedInteger(type)) {
        if (type == DataTypeModifier::f32) return UnionData((float32)uint64_v());
        else return UnionData((float64)uint64_v());
    } else if (isSignedInteger(type)) {
        UnionData tmp(DataTypeModifier::i64);
        switch (type) {
            case DataTypeModifier::c: tmp.int64_v() = int8_v(); break;
            case DataTypeModifier::i16: tmp.int64_v() = int16_v(); break;
            case DataTypeModifier::i32: tmp.int64_v() = int32_v(); break;
            case DataTypeModifier::i64: tmp.int64_v() = int64_v(); break;
        }
        switch (tgType) {
            case DataTypeModifier::f32: 
                tmp.float32_v() = (float32)tmp.int64_v(); tmp.int64_v() &= (1ull << 32 - 1);
                break;
            case DataTypeModifier::f64: tmp.float64_v() = (float64)tmp.int64_v(); break;
        }
        tmp.type = tgType;
        return tmp;
    } else {
        UnionData tmp(DataTypeModifier::f64);
        if (type == DataTypeModifier::f32) tmp.float64_v() = float32_v();
        else tmp.float64_v() = float64_v();
        if (isUnsignedInteger(tgType)) return UnionData((uint64)tmp.float64_v());
        switch (type) {
            case DataTypeModifier::c: return UnionData((int8)tmp.float64_v());
            case DataTypeModifier::i16: return UnionData((int16)tmp.float64_v());
            case DataTypeModifier::i32: return UnionData((int32)tmp.float64_v());
            case DataTypeModifier::i64: return UnionData((int64)tmp.float64_v());
        }
    }
    return UnionData(DataTypeModifier::unknown);
}

DataTypeModifier getDataTypeModifier(const std::string &name) {
    for (int i = 0; i < dataTypeModifierNumber; i++) if (name == dataTypeModifierStr[i]) return (DataTypeModifier)i;
    return DataTypeModifier::unknown;
}

ValueTypeModifier getValueTypeModifier(const std::string &name) {
    for (int i = 0; i < valueTypeModifierNumber; i++) if (name == valueTypeModifierStr[i]) return (ValueTypeModifier)i;
    return ValueTypeModifier::Unknown;
}
IdenVisibility getIdenVisibility(const std::string &name) {
    for (int i = 0; i < identifierVisibilityNumber; i++) if (name == idenVisibilityStr[i]) return (IdenVisibility)i;
    return IdenVisibility::Unknown;
}

bool isInteger(DataTypeModifier dtMfr) { return dtMfr <= DataTypeModifier::u64; }
bool isUnsignedInteger(DataTypeModifier dtMfr) { return dtMfr <= DataTypeModifier::u64 && ((int)dtMfr & 1);}
bool isSignedInteger(DataTypeModifier dtMfr) { return dtMfr <= DataTypeModifier::u64 && !((int)dtMfr & 1);}
bool isReference(ValueTypeModifier vlMfr) { return vlMfr == ValueTypeModifier::MemberRef || vlMfr == ValueTypeModifier::Ref; }

void stringSplit(const std::string &str, const std::string &sep, std::vector<std::string> &res) {
    res.clear();
    std::size_t _pos = 0;
    while (_pos < str.size()) {
        std::size_t _rpos = str.find(sep, _pos);
        if (_rpos == std::string::npos) _rpos = str.size();
        res.push_back(str.substr(_pos, _rpos - _pos)), _pos = _rpos + sep.size() + 1;
    }
}

void stringSplit(const std::string &str, const char &sep, std::vector<std::string> &res) {
    size_t lastPos = str.find_first_not_of(sep, 0);
    size_t pos = str.find(sep, lastPos);
    while (lastPos != std::string::npos) {
        res.emplace_back(str.substr(lastPos, pos - lastPos));
        lastPos = str.find_first_not_of(sep, pos);
        pos = str.find(sep, lastPos);
    }
}

std::string toString(const UnionData &data) {
    static char strBuf[128];
    switch (data.type) {
        case DataTypeModifier::unknown:
            sprintf(strBuf, "<unknown data>");
            break;
        case DataTypeModifier::b:
            sprintf(strBuf, "<uint8 %#04x>\0", data.data.uint8_v);
            break;
        case DataTypeModifier::c:
            sprintf(strBuf, "<int8 %#04x>\0", data.data.int8_v);
            break;
        case DataTypeModifier::i16:
            sprintf(strBuf, "<int16 %#06x>\0", data.data.uint16_v);
            break;
        case DataTypeModifier::u16:
            sprintf(strBuf, "<uint16 %#06x>\0", data.data.int16_v);
            break;
        case DataTypeModifier::i32:
            sprintf(strBuf, "<int32 %#010x>\0", data.data.uint32_v);
            break;
        case DataTypeModifier::u32:
            sprintf(strBuf, "<uint32 %#010x>\0", data.data.int32_v);
            break;
        case DataTypeModifier::i64:
            sprintf(strBuf, "<int64 %#018x>\0", data.data.uint32_v);
            break;
        case DataTypeModifier::u64:
            sprintf(strBuf, "<uint64 %#018x>\0", data.data.int32_v);
            break;
        case DataTypeModifier::f32:
            sprintf(strBuf, "<float32 %10lf>\0", data.data.float32_v);
            break;
        case DataTypeModifier::f64:
            sprintf(strBuf, "<float64 %10lf>\0", data.data.float64_v);
            break;
        default:
            sprintf(strBuf, "<unknown data>\0");
            break;
    }
    return std::string(strBuf);
}

std::string toString(uint64 data, int base) {
    std::string res;
    if (!data) res.append("0");
    else {
        while (data) {
            uint64 t = data % base; char ch;
            if (t >= 10) ch = t - 10 + 'A';
            else ch = t + '0';
            res.push_back(ch);
            data /= base;
        }
    }
    std::reverse(res.begin(), res.end());
    if (base == 16) return "0x" + res;
    return res;
}

uint64 alignTo(uint64 value, uint64 base) {
    return (uint64)ceil(1.0 * value / base) * base;
}

void readData(std::ifstream &ifs, UnionData &data) {
    switch (data.type) {
        case DataTypeModifier::c:
            ifs.read((char *)&data.data.int8_v, sizeof(int8));
            break;
        case DataTypeModifier::b:
            ifs.read((char *)&data.data.uint8_v, sizeof(uint8));
            break;
        case DataTypeModifier::i16:
            ifs.read((char *)&data.data.int16_v, sizeof(int16));
            break;
        case DataTypeModifier::u16:
            ifs.read((char *)&data.data.uint16_v, sizeof(uint16));
            break;
        case DataTypeModifier::i32:
            ifs.read((char *)&data.data.int32_v, sizeof(int32));
            break;
        case DataTypeModifier::u32:
            ifs.read((char *)&data.data.uint32_v, sizeof(uint32));
            break;
        case DataTypeModifier::i64:
            ifs.read((char *)&data.data.int64_v, sizeof(int64));
            break;
        case DataTypeModifier::u64:
            ifs.read((char *)&data.data.uint64_v, sizeof(uint64));
            break;
        case DataTypeModifier::f32:
            ifs.read((char *)&data.data.float32_v, sizeof(float32));
            break;
        case DataTypeModifier::f64:
            ifs.read((char *)&data.data.float64_v, sizeof(float64));
            break;
    }
}

void writeData(std::ofstream &ofs, const UnionData &data) {
    switch (data.type) {
        case DataTypeModifier::c:
            ofs.write((char *)&data.data.int8_v, sizeof(int8));
            break;
        case DataTypeModifier::b:
            ofs.write((char *)&data.data.uint8_v, sizeof(uint8));
            break;
        case DataTypeModifier::i16:
            ofs.write((char *)&data.data.int16_v, sizeof(int16));
            break;
        case DataTypeModifier::u16:
            ofs.write((char *)&data.data.uint16_v, sizeof(uint16));
            break;
        case DataTypeModifier::i32:
            ofs.write((char *)&data.data.int32_v, sizeof(int32));
            break;
        case DataTypeModifier::u32:
            ofs.write((char *)&data.data.uint32_v, sizeof(uint32));
            break;
        case DataTypeModifier::i64:
            ofs.write((char *)&data.data.int64_v, sizeof(int64));
            break;
        case DataTypeModifier::u64:
            ofs.write((char *)&data.data.uint64_v, sizeof(uint64));
            break;
        case DataTypeModifier::f32:
            ofs.write((char *)&data.data.float32_v, sizeof(float32));
            break;
        case DataTypeModifier::f64:
            ofs.write((char *)&data.data.float64_v, sizeof(float64));
            break;
    }
}
void readString(std::ifstream &ifs, std::string &str) {
    char ch;
    while (ifs.read(&ch, sizeof(char)), ch != '\0')
        str.push_back(ch);
}
void writeString(std::ofstream &ofs, const std::string &str) {
    ofs.write(str.c_str(), sizeof(int8) * (str.size() + 1));
}

UnionData getUnionData(const std::string &str) {
    bool hasDot = (str.find('.') != std::string::npos);
    UnionData data(DataTypeModifier::unknown);
    int base = 10, sign = 1, pos = 0;
    if (str[0] == '-') sign = -1, pos = 1;
    // handler float
    if (hasDot != 0) {
        if (str.back() == 'f') data.type = DataTypeModifier::f32, data.data.float32_v = 0.0f;
        else data.type = DataTypeModifier::f64, data.data.float64_v = 0.0;
        int i;
        for (i = pos; str[i] != '.'; i++) {
            if (data.type == DataTypeModifier::f32) 
                data.data.float32_v = data.data.float32_v * 10 + toDigtal(str[i]);
            else data.data.float64_v = data.data.float64_v * 10 + toDigtal(str[i]);
        }
        i++; float64 base2 = 1.0 / base;
        for (; str[i] != 'f' && i < str.size(); i++, base2 /= base)
            if (data.type == DataTypeModifier::f32) 
                data.data.float32_v = data.data.float32_v + (float32)toDigtal(str[i]) * (float32)base2;
            else data.data.float64_v = data.data.float64_v + (float64)toDigtal(str[i]) * base2;
        if (data.type == DataTypeModifier::f32)
            data.data.float32_v *= sign;
        else data.data.float64_v *= sign;
    // handler integer
    } else {
        // check the base
        if (str.size() - pos > 1) {
            if (str[pos] == '0') {
                if (str.size() - pos > 2 && str[pos + 1] == 'x') base = 16, pos += 2;
                else base = 8, pos++;
            }
        }
        size_t endPos = str.size();
        if (str[endPos - 1] == 'u') {
            endPos--;
            if (str[endPos - 1] == 'l') data.type = DataTypeModifier::u64, data.data.uint64_v = 0, endPos--;
            else if (str[endPos - 1] == 's') data.type = DataTypeModifier::u16, data.data.uint16_v = 0, endPos--;
            else if (str[endPos - 1] == 'i') data.type = DataTypeModifier::u32, data.data.uint32_v = 0, endPos--;
            else data.type = DataTypeModifier::u32, data.data.uint32_v = 0;
        } else {
            if (str[endPos - 1] == 'l') data.type = DataTypeModifier::i64, data.data.int64_v = 0, endPos--;
            else if (str[endPos - 1] == 's') data.type = DataTypeModifier::i16, data.data.int16_v = 0, endPos--;
            else if (str[endPos - 1] == 'i') data.type =DataTypeModifier::i32, data.data.int32_v = 0, endPos--;
            else data.type = DataTypeModifier::i32, data.data.int32_v = 0;
        }
        data.data.uint64_v = 0;
        for (int i = pos; i < endPos; i++) 
            data.data.uint64_v = data.data.uint64_v * base + toDigtal(str[i]);
        switch (data.type) {
            case DataTypeModifier::i64:
                data.data.int64_v = sign * (int64)data.data.uint64_v; break;
            case DataTypeModifier::u32:
                data.data.uint32_v = sign * (uint32)data.data.uint64_v; break;
            case DataTypeModifier::i32:
                data.data.int32_v = sign * (int32)data.data.uint64_v; break;
            case DataTypeModifier::u16:
                data.data.uint16_v = sign * (uint16)data.data.uint16_v; break;
            case DataTypeModifier::i16:
                data.data.int16_v = sign * (int16)data.data.uint64_v; break;
        }
    }
    return data;
}

std::string getString(const std::string &str, size_t st, size_t &ed) {
    std::string res;
    for (ed = st + 1; ed < str.size() && str[ed] != '\"'; ed++) {
        char newChar = str[ed];
        if (str[ed] == '\\') {
            ed++;
            switch (str[ed]) {
                case 'n': newChar = '\n'; break;
                case 'r': newChar = '\r'; break;
                case 'b': newChar = '\b'; break;
                case 't': newChar = '\t'; break;
                case '0': newChar = '\0'; break;
                default: newChar = str[ed]; break;
            }
        }
        res.push_back(newChar);
    }
    return res;
}

std::string toCodeString(const std::string &str) {
    std::string res = "\"";
    for (int i = 0; i < str.size(); i++) {
        char ch = str[i];
        switch (ch) {
            case '\n': res.append("\\n"); break;
            case '\r': res.append("\\r"); break;
            case '\b': res.append("\\b"); break;
            case '\t': res.append("\\t"); break;
            case '\0': res.append("\\0"); break;
            case '\\': res.append("\\\\"); break;
            case '\"': res.append("\\\""); break;
            default: res.push_back(ch); break;
        }
    }
    res.append("\"");
    return res;
}

std::string getRealString(const std::string &str) {
    
}