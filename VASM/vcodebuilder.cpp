#include "vcodebuilder.h"

CommandInfo::CommandInfo(Command _command) {
    this->command = _command;
}

VASMPackage::VASMPackage() {
    type = 0;
    vcodeSize = mainAddr = globalMemory = 0;
}

void VASMPackage::write(const std::string &_path) const {
    std::ofstream ofs(_path, std::ios::binary);
    UnionData data(DataTypeModifier::b);
    data.data.uint8_v = type, writeData(ofs, data);
    if (type == 1) {
        data.type = DataTypeModifier::u32;
        data.data.uint32_v = mainAddr;
        writeData(ofs, data);
    } else {
        writeString(ofs, definition);
        data.type = DataTypeModifier::u32;
        data.data.uint32_v = exposeMap.size();
        writeData(ofs, data);
        for (auto &pir : exposeMap) {
            writeString(ofs, pir.first);
            data.type = DataTypeModifier::u32, data.data.uint32_v = pir.second;
            writeData(ofs, data);
        }
    }
    data.type = DataTypeModifier::u32, data.data.uint32_v = relyList.size();
    writeData(ofs, data);
    for (auto &path : relyList) writeString(ofs, path);

    data.type = DataTypeModifier::u32, data.data.uint32_v = externMap.size();
    writeData(ofs, data);
    std::vector< std::pair<uint32, std::string> > externList;
    for (auto &iden : externMap) externList.push_back(std::make_pair(iden.second, iden.first));
    std::sort(externList.begin(), externList.end());
    for (auto pir : externList) writeString(ofs, pir.second);

    data.type = DataTypeModifier::u32, data.data.uint32_v = stringList.size();
    writeData(ofs, data);
    for (auto &str : stringList) writeString(ofs, str);

    data.type = DataTypeModifier::u64, data.data.uint64_v = globalMemory;
    writeData(ofs, data);
    data.type = DataTypeModifier::u32, data.data.uint32_v = vcodeSize;
    writeData(ofs, data);

    for (auto &info : commandList) {
        uint32 vcode = getVCode(info.command);
        data.type = DataTypeModifier::u32;
        for (auto arg : info.argument) {
            auto relt = arg.type;
            arg.type = DataTypeModifier::u64;
            writeData(ofs, arg);
            arg.type = relt;
        }
    }
}

bool VASMPackage::generate(const std::string &_src_path, bool _ignore_hint) {
    bool succ = true;
    std::ifstream ifs(_src_path);
    if (!ifs.good()) {
        printMessage(("Unable to read file " + _src_path + "\n").c_str(), MessageType::Error);
        return false;
    }
    int lineId = 0;
    while (!ifs.eof()) {
        std::string line;
        std::getline(ifs, line);
        lineId++;
        succ &= generateLine(line, lineId, _ignore_hint);
    }
    return succ;
}

bool VASMPackage::generateLine(const std::string &_line, int _line_id, bool _ignore_hintd) {
    int pos = 0, rpos;
    // skip the separate character in the front of the line
    while (pos < _line.size() && isSeparator(_line[pos])) pos++;
    // it is an empty line
    if (pos == _line.size()) return true;
    // this line is a pretreat command
    std::vector<std::string> lst;
    for (rpos = pos; pos < _line.size(); pos = ++rpos) {
        if (isSeparator(_line[pos])) continue;
        std::string newPart;
        if (_line[pos] == '\"') {
            newPart = getString(_line, pos, rpos);
            if (rpos == _line.size()) {
                printError(_line_id, "Invalid string");
                return false;
            }
        } else if (isNumber(_line[pos])) {
            while (rpos + 1 < _line.size() && (isLetter(_line[rpos + 1]) || isNumber(_line[rpos + 1]) || _line[rpos + 1] == '.')) rpos++;
            newPart = _line.substr(pos, rpos - pos + 1);
        } else if (isLetter(_line[pos]) || _line[pos] == '@') {
            while (rpos + 1 < _line.size() && (isNumber(_line[rpos + 1]) || isLetter(_line[rpos + 1]) || _line[rpos + 1] == '@'))
                rpos++;
            newPart = _line.substr(pos, rpos - pos + 1);
        } else if (_line[pos] == '#') {
            newPart = '#';
        }
        lst.push_back(newPart);
    }

    // this line is pretreat line
    if (lst[0] == "#") {
        PretreatCommand cmd = getPretreatCommand(lst[1]);
        if (cmd == PretreatCommand::unknown) {
            printError(_line_id, "Invalid pretreat command " + lst[1]);
            return false;
        }
        switch (cmd) {
            case PretreatCommand::DEF:
                definition += lst[1], definition.push_back('\n'); break;
            case PretreatCommand::EXPOSE:
                exposeMap[lst[1]] = 0; break;
            case PretreatCommand::EXTERN:
                if (!externMap.count(lst[1])) 
                    externMap.insert(std::make_pair(lst[1], externMap.size()));
                break;
            case PretreatCommand::RELY:
                relyList.push_back(lst[1]);
                break;
            case PretreatCommand::GLOMEM:
                globalMemory += getUnionData(lst[1]).data.uint64_v;
                break;
            case PretreatCommand::HINT:
                hints.push_back(std::make_pair(vcodeSize, lst[1]));
                break;
            case PretreatCommand::LABEL:
                if (labelOffset.count(lst[1])) {
                    printError(_line_id, "Multiple definition of label " + lst[1]);
                    return false;
                }
                labelOffset[lst[1]] = vcodeSize;
                break;
        }
    } else {
        std::vector<std::string> prt;
        stringSplit(lst[1], '_', prt);
        TCommand tcmd = getTCommand(prt.back());
        if (tcmd == TCommand::unknown) {
            printError(_line_id, "Invalid command name " + prt.back());
            return false;
        }
        if (getCommand(lst[1]) == Command::unknown) {
             printError(_line_id, "Invalid command name " + lst[1]);
            return false;
        }
        switch (tcmd) {
            case TCommand::mov:
            case TCommand::addmov:
            case TCommand::submov:
            case TCommand::mulmov:
            case TCommand::divmov:
            case TCommand::andmov:
            case TCommand::ormov:
            case TCommand::xormov:
            case TCommand::shlmov:
            case TCommand::shrmov:
            case TCommand::modmov:
            case TCommand::add:
            case TCommand::sub:
            case TCommand::mul:
            case TCommand::div:
            case TCommand::_and:
            case TCommand::_or:
            case TCommand::_xor:
                break;
        }
    }
    return true;
}