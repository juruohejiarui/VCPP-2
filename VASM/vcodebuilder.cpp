#include "vcodebuilder.h"

CommandInfo::CommandInfo(Command _command) {
    this->command = _command;
}

VCodePackage::VCodePackage() {
    type = 0;
    vcodeSize = mainAddr = globalMemory = 0;
}

void VCodePackage::write(const std::string &_path) {
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

    data.type = DataTypeModifier::u32, data.data.uint32_v = externList.size();
    writeData(ofs, data);
    for (auto &iden : externList) writeString(ofs, iden);

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

bool VCodePackage::generate(const std::string &_src_path, bool _ignore_hint) {
    bool succ = false;

}
