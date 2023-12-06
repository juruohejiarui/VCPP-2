#include "tools.h"

extern const std::string dataTypeModifierString[] = {"c", "b", "i16", "u16", "i32", "u32", "i64", "u64", "f32", "f64", "o", "B"};
extern const std::string valueTypeModifierString[];

void stringSplit(const std::string &_str, const std::string &_sep, std::vector<std::string> &_res) {
    _res.clear();
    std::size_t _pos = 0;
    while (_pos < _str.size()) {
        std::size_t _rpos = _str.find(_sep, _pos);
        if (_rpos == std::string::npos) _rpos = _str.size();
        _res.push_back(_str.substr(_pos, _rpos - _pos)), _pos = _rpos + _sep.size() + 1;
    }
}