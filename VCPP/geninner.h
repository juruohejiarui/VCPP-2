#pragma once
#include "gen.h"

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

/// @brief Get the data type modifier using "etype" PS: this function will ignore the member "dimc" of "etype"
/// @param etype the expression type
/// @return the data type modifier
DataTypeModifier getDtMdf(const ExprType &etype);

struct LocalVarFrame {
private:
    std::map<std::string, VariableInfo *> varMap;
    uint32 varCount;
    LocalVarFrame *prev;
public:
    LocalVarFrame(LocalVarFrame *prev = nullptr, uint32 varCount = 0);
    
    void writeCleanVCode();

    VariableInfo *getVar(const std::string &name);
    /// @brief insert the variable, modify the offset of this variable and return whether it is successful
    /// @param vInfo the information of this variable
    /// @return whether it is successful
    bool insertVar(VariableInfo *vInfo);

    uint32 getVarCount() const;

    LocalVarFrame *getPrev();

    void clear(bool writeVCode = false);
};

void locVarStkInit();
LocalVarFrame *locVarStkTop();
void locVarStkPop(bool writeVCode = false);
void locVarStkPush();

typedef std::tuple<bool, ExprType> ETChkRes;

/// @brief the information for calling a variable
typedef std::tuple<VariableInfo *, ExprType> VarCallInfo;
/// @brief the information for calling a function
typedef std::tuple<FunctionInfo *, ExprType, GTableData> FuncCallInfo;

/// @brief Find a variable and get the expression type of it under the current symbol environment
/// @param name the name/path of this variable
/// @return <the pointer to this variable, the expression type of this variale>
VarCallInfo findVar(const std::string &name);

/// @brief Find a function and get the information of it
/// @param name the name/path of this function
/// @param params the list of params
/// @return <the pointer to this function, the return value of this function, the GTableData for the function call> 
FuncCallInfo findFunc(const std::string &name, const std::vector<ExprType> &params);

/// @brief This function will check the expression type of an expression and handle all the syntax candies in this expression 
/// @param node the root of the expression
/// @return <If this expression is valid, the result of this expression>
ETChkRes chkEType(ExpressionNode *node);

const ExprType &getEType(ExpressionNode *node);
const FuncCallInfo &getFuncCallInfo(IdentifierNode *node);
const VarCallInfo &getVarCallInfo(IdentifierNode *node);
const size_t getStrId(ConstValueNode *node);
const std::string getString(size_t id);
const std::vector<std::string> &getStrList();

void initOperCandy();
/// @brief Load the syntax candies for operator
/// @return if it is successful
bool updOperCandy();
VariableInfo *findOperCandy(const std::string &name, const ExprType &expr1, const ExprType &expr2);

DataTypeModifier getRealDtMdf(const ExprType &etype);