#pragma once
#include "../Tools/tools.h"
#include "commandlist.h"

struct CommandInfo {
    Command command;
    uint32 vcode, offset;
    std::vector<UnionData> argument;
    std::string argumentString;
    CommandInfo(Command _command = Command::none);
};

struct VASMPackage {
    std::vector<CommandInfo> commandList;
    uint32 vcodeSize, mainAddr;
    std::map<std::string, uint64> labelOffset;
    std::vector< std::pair<uint32, std::string> > hints;
    std::vector<std::string> stringList;
    /// @brief the map of exposed identifier : identifier -> offset
    std::map<std::string, uint32> exposeMap;
    std::vector<std::string> relyList;
    /// @brief the map of extern identifier : identifier -> id(before compiled) / (rely | offset)(after compiled)
    std::map<std::string, uint64> externMap;
    uint64 globalMemory;

    VASMPackage();
    /// @brief generate this package using the source and return if it is successful
    /// @param _src_path the path of the source
    /// @return 
    bool generate(const std::string &_src_path, bool _ignore_hint = false);

private:
    /// @brief generate command for one line and return whether it is successful
    /// @param line 
    /// @param _ignore_hint 
    /// @return 
    bool generateLine(const std::string &_line, int _line_id, bool _ignore_hint = false);
};

struct ClassTypeData;
struct FieldTypeData {

};
struct VariableTypeData {

};
struct MethodTypeData {

};
struct ClassTypeData {
    std::string name;
    uint64 offset;
    std::map<std::string, VariableTypeData *> fields;
    std::map<std::string, 
};
struct NamespaceTypeData {
    std::string name;
    std::map<std::string, NamespaceTypeData *> children;
    std::map<std::string, MethodTypeData *> methods;
    std::map<std::string, VariableTypeData *> variables;
    std::map<std::string, ClassTypeData *> classes;
};
struct DataTypePackage {
    NamespaceTypeData root;
};

struct VOBJPackage {
    VASMPackage vasmPackage;
    DataTypePackage dataTypePackage;
    std::string definition;
};
bool buildVObj(uint8 type, const std::string &_vasm_path, const std::string &_tdt_path, const std::string &_def_path, VOBJPackage &_vobj_pkg, const std::vector<std::string> &_rely_list);