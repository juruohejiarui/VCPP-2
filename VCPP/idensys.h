#pragma once
#include "syntaxnode.h"

enum class IdenRegion {
    Local, Member, Global, Unknown,
};
class VariableInfo;
class FunctionInfo;
class ClassInfo;
class NamespaceInfo;

struct ExprType {
    ClassInfo *cls;
    std::string clsName;
    int dimc;
    ValueTypeModifier vtMdf;

    std::vector<ExprType> generParams;
    
    ExprType();
    ExprType(const std::string &clsName, int dimc = 0, ValueTypeModifier vtMdf = ValueTypeModifier::TrueValue);
    
    /// @brief initialize this expression type using the information from NODE
    /// @param node the source of information
    ExprType(IdentifierNode *node);

    ExprType(ClassInfo *cls);

    uint64 getSize() const;
    /// @brief this function helps to check whether this expression type represents an object. PS : generic classes will be treat as objects
    /// @return the result of checking
    bool isObject() const;

    ExprType convertToBase() const;
    /// @brief Set the member "cls" using the member "clsName" and the "cls" in "genericParams"
    /// @return if it is successful
    bool setCls();

    bool operator == (const ExprType &other) const;
    bool operator != (const ExprType &other) const;

    std::string toString() const;
    std::string toVtdString() const;
    std::string toDebugString() const;
};
typedef std::map<ClassInfo *, ExprType> GenerSubstMap;
ExprType subst(const ExprType &target, ClassInfo *cls, const ExprType &clsImpl);
ExprType subst(const ExprType &target, const GenerSubstMap &gsMap);
std::vector<ExprType> subst(const std::vector<ExprType> &target, const GenerSubstMap &gsMap);

struct GTableData {
    ExprType etype[5];
    uint8 size;
    ExprType &operator [] (uint8 index);
    const ExprType &operator [] (uint8 index) const;
    void insert(const std::vector<ClassInfo *> gClsList, const GenerSubstMap &gsMap);
    GTableData();
};

GenerSubstMap makeSubstMap(const std::vector<ClassInfo *> &gClsList, const std::vector<ExprType> &gParamList);
/// @brief merge the <key and value pair> in SRC into DST, if there are same generic classes in both map, then the param of it will be the one in SRC
/// @param dst the destination
/// @param src the source
void mergeSubstMap(GenerSubstMap &dst, const GenerSubstMap &src);

struct VariableInfo {
    std::string name, fullName;
    ExprType type;
    FunctionInfo *blgFunc;
    ClassInfo *blgCls;
    NamespaceInfo *blgNsp;
    IdenVisibility visibility;

    RootNode *blgRoot;

    uint64 offset;

    VariableInfo();

    IdentifierNode *getNameNode() const;
    void setNameNode(IdentifierNode *nameNode);
    IdentifierNode *getTypeNode() const;
    void setTypeNode(IdentifierNode *typeNode);
    ExpressionNode *getInitNode() const;
    void setInitNode(ExpressionNode *initNode);

    IdenRegion getRegion() const;
private:
    IdentifierNode *nameNode, *typeNode;
    ExpressionNode *initNode;
};

struct FunctionInfo {
    std::string name, nameWithParam, fullName;
    std::vector<ClassInfo *> generCls;
    std::vector<VariableInfo *> params;
    std::vector<ConstValueNode *> defaultVals;
    ExprType resType;
    IdenVisibility visibility;

    ClassInfo *blgCls;
    NamespaceInfo *blgNsp;
    RootNode *blgRoot;
    
    FuncDefNode *defNode;

    uint64 offset;

    FunctionInfo();

    FuncDefNode *getDefNode() const;
    /// @brief This function will update the defNode, and the information that stored in defNode excepts the content.
    /// @param defNode 
    bool setDefNode(FuncDefNode *defNode);

    IdenRegion getRegion() const;

    /// @brief This function will check whether the param list satisfy the type requirements of 
    /// this function and return the result and the expression type of the return value of this function.
    /// @param gsMap the subtitution map of generic class in the generic list
    /// @param paramList the param list
    /// @return <Whether the param list satisfy the requirements, the expression type of the return value, GTableData>
    std::tuple<bool, ExprType, GTableData> satisfy(const GenerSubstMap &gsMap, const std::vector<ExprType> &paramList);
};

typedef std::vector<FunctionInfo *> FunctionList;

struct ClassInfo {
    std::string name, fullName;
    ClassInfo *baseCls;
    std::vector<ClassInfo *> generCls;
    /// @brief the params for every generic classes in base class
    std::vector<ExprType> generParams;

    std::map<std::string, VariableInfo *> fieldMap;
    std::map<std::string, FunctionList> funcMap;

    IdenVisibility visibility;

    uint64 size, dep;

    NamespaceInfo *blgNsp;
    RootNode *blgRoot;

    ClsDefNode *defNode;

    bool isGeneric;

    std::vector<ClassInfo *> derivedCls;

    ClassInfo();
};

/// @brief Check if bsCls is the base class of dCls
/// @param bsCls ...
/// @param dCls ...
/// @return the result
bool isBaseCls(ClassInfo *bsCls, ClassInfo *dCls);

struct NamespaceInfo {
    std::string name, fullName;
    
    std::map<std::string, VariableInfo *> varMap;
    std::map<std::string, FunctionList> funcMap;
    std::map<std::string, ClassInfo *> clsMap;
    std::map<std::string, NamespaceInfo *> nspMap;

    NamespaceInfo *blgNsp;
    
    std::vector<std::pair<NspDefNode *, RootNode * > > defList;

    NamespaceInfo();
};

extern NamespaceInfo *rootNsp;
extern ClassInfo *basicCls, *int8Cls, *uint8Cls, *int16Cls, *uint16Cls, *int32Cls, *uint32Cls, *int64Cls, *uint64Cls, *float32Cls, *float64Cls, *objectCls, *voidCls;

bool isBasicCls(ClassInfo *cls);

#pragma region Symbol Search
FunctionInfo *getCurFunc();
void setCurFunc(FunctionInfo *func);
ClassInfo *getCurCls();
void setCurCls(ClassInfo *cls);
NamespaceInfo *getCurNsp();
void setCurNsp(NamespaceInfo *nsp);
RootNode *getCurRoot();
bool setCurRoot(RootNode *node);
const std::vector<NamespaceInfo *> &getUsingList();

ClassInfo *findCls(const std::string &path);
#pragma endregion

/// @brief This function can build the structures of classes, functions and variables 
/// @param roots the roots of contain the structures
/// @return <if it is successful, the usage of global memory>
std::pair<bool, uint64> buildIdenSystem(const RootList &roots);

bool isIntegerCls(ClassInfo *cls);
bool isFloatCls(ClassInfo *cls);

void debugPrintNspStruct(NamespaceInfo *nsp, int dep = 0);

