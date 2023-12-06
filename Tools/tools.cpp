#include "tools.h"

extern const std::string dataTypeModifierString[] = {"c", "b", "i16", "u16", "i32", "u32", "i64", "u64", "f32", "f64", "o", "B", "unknown"};
extern const std::string valueTypeModifierString[] = {"rm", "m", "t", "unknown"};

DataTypeModifier getDataTypeModifier(const std::string &_name)
{
    for (int i = 0; i < dataTypeModifierNumber; i++) if (_name == dataTypeModifierString[i]) return (DataTypeModifier)i;
    return (DataTypeModifier)dataTypeModifierNumber; 
}

ValueTypeModifier getValueTypeModifier(const std::string &_name)
{
    for (int i = 0; i < valueTypeModifierNumber; i++) if (_name == dataTypeModifierString[i]) return (ValueTypeModifier)i;
    return (ValueTypeModifier)dataTypeModifierNumber; 
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