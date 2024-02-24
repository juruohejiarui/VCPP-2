#pragma once
#include "../Tools/tools.h"
#include "commandlist.h"

struct CommandInfo {
    Command command;
    uint32 vcode, offset, lineId;
    std::vector<UnionData> argument;
    std::string argumentString;
    CommandInfo(Command _command = Command::none, uint32 lineId = 0);
};

struct VASMPackage {
    std::vector<CommandInfo> commandList;
    uint64 vcodeSize, mainAddr;
    std::map<std::string, uint64> labelOffset;
    std::vector< std::pair<uint32, std::string> > hints;
    std::vector<std::string> strList;
    /// @brief the map of exposed identifier : identifier -> offset
    // std::map<std::string, uint32> exposeMap;
    // std::vector<std::string> relyList;
    /// @brief the map of extern identifier : identifier -> id(before compiled) / (rely | offset)(after compiled)
    // std::map<std::string, uint64> externMap;
    uint64 globalMemory;

    VASMPackage();
    /// @brief generate this package using the source and return if it is successful
    /// @param srcPath the path of the source
    /// @return 
    bool generate(const std::string &srcPath, bool ignoreHint = false);

private:
    /// @brief generate command for one line and return whether it is successful
    /// @param line 
    /// @param ignoreHint 
    /// @return 
    bool generateLine(const std::string &line, int lineId, bool ignoreHint = false);
};

struct ClassTypeData;
struct VariableTypeData {
    IdenVisibility visibility;
    std::string name;
    std::string type;

    uint64 offset;
};
struct FunctionTypeData {
    std::string name, labelName;
    IdenVisibility visibility;
    uint8 gtableSize;
    uint64 offset;
    std::string resType;
    std::vector<std::string> argTypes;
};
struct ClassTypeData {
    std::string name, baseClsName;
    IdenVisibility visibility;
    uint64 size;
    uint8 gtableSize;
    uint64 gtableOffset;

    std::vector<VariableTypeData *> varList;
    std::vector<FunctionTypeData *> funcList;

    ClassTypeData();
    ~ClassTypeData();
};
struct NamespaceTypeData {
    std::string name;
    std::vector<VariableTypeData *> varList;
    std::vector<FunctionTypeData *> funcList;
    std::vector<ClassTypeData *> clsList;
    std::vector<NamespaceTypeData *> nspList;

    ~NamespaceTypeData();
};
struct DataTypePackage {
    NamespaceTypeData *root;
    DataTypePackage();
    ~DataTypePackage();

    bool generate(const std::string &filePath);
};

struct VOBJPackage {
    uint8 type;
    VASMPackage vasmPackage;
    DataTypePackage dataTypePackage;
    std::string definition;
    
    /// @brief read the content of the vobj file
    /// @param path 
    /// @return if it is successful
    bool read(const std::string &path);
};
bool buildVObj( uint8 type,
                const std::string &vasmPath, 
                const std::string &typeDataPath,
                const std::string &defPath, 
                const std::vector<std::string> &relyList,
                const std::string &target);