#include "tools.h"

const std::string dataTypeModifierString[] = {"c", "b", "i16", "u16", "i32", "u32", "i64", "u64", "f32", "f64", "o", "B", "unknown"};
const std::string valueTypeModifierString[] = {"rm", "m", "t", "unknown"};
const int dataTypeModifierNumber = 12, valueTypeModifierNumber = 3;

UnionData::UnionData() { type = (DataTypeModifier)dataTypeModifierNumber; }
UnionData::UnionData(DataTypeModifier _type) { type = _type; }

DataTypeModifier getDataTypeModifier(const std::string &_name)
{
    for (int i = 0; i < dataTypeModifierNumber; i++) if (_name == dataTypeModifierString[i]) return (DataTypeModifier)i;
    return DataTypeModifier::unknown;
}

ValueTypeModifier getValueTypeModifier(const std::string &_name)
{
    for (int i = 0; i < valueTypeModifierNumber; i++) if (_name == dataTypeModifierString[i]) return (ValueTypeModifier)i;
    return ValueTypeModifier::unknown;
}

void stringSplit(const std::string &_str, const std::string &_sep, std::vector<std::string> &_res)
{
    _res.clear();
    std::size_t _pos = 0;
    while (_pos < _str.size()) {
        std::size_t _rpos = _str.find(_sep, _pos);
        if (_rpos == std::string::npos) _rpos = _str.size();
        _res.push_back(_str.substr(_pos, _rpos - _pos)), _pos = _rpos + _sep.size() + 1;
    }
}

void stringSplit(const std::string &_str, const char &_sep, std::vector<std::string> &_res) {
    size_t lastPos = _str.find_first_not_of(_sep, 0);
    size_t pos = _str.find(_sep, lastPos);
    while (lastPos != std::string::npos) {
        _res.emplace_back(_str.substr(lastPos, pos - lastPos));
        lastPos = _str.find_first_not_of(_sep, pos);
        pos = _str.find(_sep, lastPos);
    }
}

void readData(std::ifstream &_ifs, UnionData &_data) {
    switch (_data.type) {
        case DataTypeModifier::c:
            _ifs.read((char *)&_data.data.int8_v, sizeof(int8));
            break;
        case DataTypeModifier::b:
            _ifs.read((char *)&_data.data.uint8_v, sizeof(uint8));
            break;
        case DataTypeModifier::i16:
            _ifs.read((char *)&_data.data.int16_v, sizeof(int16));
            break;
        case DataTypeModifier::u16:
            _ifs.read((char *)&_data.data.uint16_v, sizeof(uint16));
            break;
        case DataTypeModifier::i32:
            _ifs.read((char *)&_data.data.int32_v, sizeof(int32));
            break;
        case DataTypeModifier::u32:
            _ifs.read((char *)&_data.data.uint32_v, sizeof(uint32));
            break;
        case DataTypeModifier::i64:
            _ifs.read((char *)&_data.data.int64_v, sizeof(int64));
            break;
        case DataTypeModifier::u64:
            _ifs.read((char *)&_data.data.uint64_v, sizeof(uint64));
            break;
        case DataTypeModifier::f32:
            _ifs.read((char *)&_data.data.float32_v, sizeof(float32));
            break;
        case DataTypeModifier::f64:
            _ifs.read((char *)&_data.data.float64_v, sizeof(float64));
            break;
    }
}

void writeData(std::ofstream &_ofs, const UnionData &_data) {
    switch (_data.type) {
        case DataTypeModifier::c:
            _ofs.write((char *)&_data.data.int8_v, sizeof(int8));
            break;
        case DataTypeModifier::b:
            _ofs.write((char *)&_data.data.uint8_v, sizeof(uint8));
            break;
        case DataTypeModifier::i16:
            _ofs.write((char *)&_data.data.int16_v, sizeof(int16));
            break;
        case DataTypeModifier::u16:
            _ofs.write((char *)&_data.data.uint16_v, sizeof(uint16));
            break;
        case DataTypeModifier::i32:
            _ofs.write((char *)&_data.data.int32_v, sizeof(int32));
            break;
        case DataTypeModifier::u32:
            _ofs.write((char *)&_data.data.uint32_v, sizeof(uint32));
            break;
        case DataTypeModifier::i64:
            _ofs.write((char *)&_data.data.int64_v, sizeof(int64));
            break;
        case DataTypeModifier::u64:
            _ofs.write((char *)&_data.data.uint64_v, sizeof(uint64));
            break;
        case DataTypeModifier::f32:
            _ofs.write((char *)&_data.data.float32_v, sizeof(float32));
            break;
        case DataTypeModifier::f64:
            _ofs.write((char *)&_data.data.float64_v, sizeof(float64));
            break;
    }
}
void readString(std::ifstream &_ifs, std::string &_str) {
    UnionData _data;
    _data.type = DataTypeModifier::c, _str.clear();
    while (readData(_ifs, _data), _data.data.int8_v != '\0')
        _str.push_back(_data.data.int8_v);
}
void writeString(std::ofstream &_ofs, const std::string &_str) {
    _ofs.write(_str.c_str(), sizeof(int8) * (_str.size() + 1));
}

UnionData getUnionData(const std::string &_str) {
    bool hasDot = (_str.find('.') != std::string::npos);
    UnionData data;
    int base = 10, sign = 1, pos = 0;
    if (_str[0] == '-') sign = -1, pos = 1;
    // handler float
    if (hasDot != 0) {
        if (_str.back() == 'f') data.type = DataTypeModifier::f32, data.data.float32_v = 0.0f;
        else data.type = DataTypeModifier::f64, data.data.float64_v = 0.0;
        int i;
        for (i = pos; _str[i] != '.'; i++) {
            if (data.type == DataTypeModifier::f32) 
                data.data.float32_v = data.data.float32_v * 10 + toDigtal(_str[i]);
            else data.data.float64_v = data.data.float64_v * 10 + toDigtal(_str[i]);
        }
        i++; float64 base2 = 1.0 / base;
        for (; _str[i] != 'f' || i < _str.size(); i++, base2 /= base)
            if (data.type == DataTypeModifier::f32) 
                data.data.float32_v = data.data.float32_v + toDigtal(_str[i]) * base2;
            else data.data.float64_v = data.data.float64_v + toDigtal(_str[i]) * base2;
        if (data.type == DataTypeModifier::f32)
            data.data.float32_v *= sign;
        else data.data.float64_v *= sign;
    // handler integer
    } else {
        // check the base
        if (_str.size() - pos > 1) {
            if (_str[pos] == '0') {
                if (_str.size() - pos > 2 && _str[pos + 1] == 'x') base = 16, pos++;
                else base = 8, pos++;
            }
        }
        int endPos = _str.size();
        if (_str[endPos - 1] == 'u') {
            endPos--;
            if (_str[endPos - 1] == 'l') data.type = DataTypeModifier::u64, data.data.uint64_v = 0, endPos--;
            else if (_str[endPos - 1] == 's') data.type = DataTypeModifier::u16, data.data.uint16_v = 0, endPos--;
            else if (_str[endPos - 1] == 'i') data.type = DataTypeModifier::u32, data.data.uint32_v = 0, endPos--;
            else data.type = DataTypeModifier::u32, data.data.uint32_v = 0;
        } else {
            if (_str[endPos - 1] == 'l') data.type = DataTypeModifier::i64, data.data.int64_v = 0, endPos--;
            else if (_str[endPos - 1] == 's') data.type = DataTypeModifier::i16, data.data.int16_v = 06, endPos--;
            else if (_str[endPos - 1] == 'i') data.type =DataTypeModifier::i32, data.data.int32_v = 0, endPos--;
            else data.type = DataTypeModifier::i32, data.data.int64_v = 0;
        }
        data.data.uint64_v;
        for (int i = 0; i < endPos; i++) 
            data.data.uint64_v = data.data.uint64_v * base + toDigtal(_str.size());
        switch (data.type) {
            case DataTypeModifier::i64:
                data.data.int64_v = sign * (int64)data.data.uint64_v; break;
            case DataTypeModifier::u32:
                data.data.uint32_v = sign * data.data.uint64_v; break;
            case DataTypeModifier::i32:
                data.data.int32_v = sign * (int32)data.data.uint64_v; break;
            case DataTypeModifier::u16:
                data.data.uint16_v = sign * data.data.uint64_v; break;
            case DataTypeModifier::i16:
                data.data.int16_v = sign * (int16)data.data.uint64_v; break;
        }
    }
    return data;
}

std::string getString(const std::string &_str, int _start, int &_end) {
    std::string _res;
    for (_end = _start + 1; _end < _str.size() && _str[_end] != '\"'; _end++) {
        char newChar = _str[_end];
        if (_str[_end] == '\\') {
            _end++;
            switch (_str[_end]) {
                case 'n': newChar = '\n'; break;
                case 'r': newChar = '\r'; break;
                case 'b': newChar = '\b'; break;
                case 't': newChar = '\t'; break;
                case '0': newChar = '\0'; break;
                default: newChar = _str[_end]; break;
            }
        }
        _res.push_back(newChar);
    }
    return _res;
}