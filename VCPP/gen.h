#pragma once
#include "idensys.h"
#include "../VASM/commandlist.h"

bool setOutputStream(const std::string &path);
void closeOutputStream();

void indentInc();
void indentDec();
int32 getIndentDep();

void writeVCode(std::string text);
void writeVCode(Command tcmd);
void writeVCode(Command tcmd, const UnionData &data);
void writeVCode(Command tcmd, const UnionData &data1, const UnionData &data2);
void writeVCode(Command tcmd, const std::string &str);
void writeVCode(const std::string &cmdStr, const std::string &str);

struct LocalVarFrame {
private:
    std::map<std::string, VariableInfo *> varMap;
    uint32 varCount;
    LocalVarFrame *prev;
public:
    LocalVarFrame(LocalVarFrame *prev = nullptr, uint32 varCount = 0);
    
    void writeCleanVCode();

    VariableInfo *getVar(const std::string &name);
    bool insertVar(VariableInfo *vInfo);

    uint32 getVarCount() const;

    LocalVarFrame *getPrev();

    void clear(bool writeVCode = false);
};

void locVarStkInit();
LocalVarFrame *locVarStkTop();
void locVarStkPop(bool writeVCode = false);
void locVarStkPush();

/// @brief Find a variable and get the expression type of it under the current symbol environment
/// @param name the name/path of this variable
/// @return <the pointer to this variable, the expression type of this variale>
std::tuple<VariableInfo *, ExprType> findVar(const std::string &name);

/// @brief Find a function and get the information of it
/// @param name the name/path of this function
/// @param params the list of params
/// @return <the pointer to this function, the return value of this function, the GTableData for the function call> 
std::tuple<FunctionInfo *, ExprType, GTableData> findFunc(const std::string &name, const std::vector<ExprType> &params);

bool generateVCode(const std::string &vasmPath);
bool generateSymbol(const std::string &vdtPath, const std::string &defPath);
