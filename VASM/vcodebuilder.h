#pragma once
#include "../Tools/tools.h"
#include "commandlist.h"

struct CommandInfo {
    Command command;
    std::vector<UnionData> argument;

    CommandInfo(Command _command = Command::none);
};

struct VCodePackage {
    std::vector<CommandInfo> commandList;
    uint32 vcodeSize, mainAddr;
    uint8 type;
    std::map<std::string, uint32_t> labelOffset;
    std::vector< std::pair<uint32_t, std::string> > hints;
    std::vector<std::string> stringList;
    std::map<std::string, uint32> exposeMap;
    std::vector<std::string> relyList;
    std::vector<std::string> externList;
    uint64_t globalMemory;
    std::string definition;

    VCodePackage();

    void write(const std::string &_path);
    /// @brief read the package from the file and return if it is successful
    /// @param _src_path the path of the file
    /// @return 
    bool read(const std::string &_path);
    /// @brief generate this package using the source and return if it is successful
    /// @param _src_path the path of the source
    /// @return 
    bool generate(const std::string &_src_path, bool _ignore_hint = false);

    /// @brief merge two package and return if it is successful.
    /// @param _dst the destination
    /// @param _src the source
    /// @return 
    static bool merge(VCodePackage &_dst, VCodePackage &_src);

private:
    bool generateLine(const std::string &line, bool _ignore_hint = false);
};