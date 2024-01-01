#pragma once
#include "syntaxnode.h"

enum class IdentifierRegion {
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

    std::vector<ExprType> genericParams;
    
    ExprType();
    ExprType(const std::string &clsName, int dimc = 0, ValueTypeModifier vtMdf = ValueTypeModifier::t);
    
    /// @brief initialize this expression type using the information from NODE
    /// @param node the source of information
    ExprType(IdentifierNode *node);

    uint64 getSize() const;
    bool isObject() const;

    ExprType convertToBase() const;
    /// @brief Set the member "cls" using the member "clsName" and the "cls" in "genericParams"
    /// @return if it is successful
    bool setCls();

    bool operator == (const ExprType &other) const;
    bool operator != (const ExprType &other) const;

    std::string toString() const;
    std::string toDebugString() const;
};

ExprType substitute(const ExprType &target, ClassInfo *cls, const ExprType &clsImpl);
typedef std::map<ClassInfo *, ExprType> GenericSubstitutionMap;

GenericSubstitutionMap makeSubtitutionMap(const std::vector<ClassInfo *> &gClsList, const std::vector<ExprType> &gParamList);
/// @brief merge the <key and value pair> in SRC into DST, if there are same generic classes in both map, then the param of it will be the one in SRC
/// @param dst the destination
/// @param src the source
void mergeSubtitutionMap(GenericSubstitutionMap &dst, const GenericSubstitutionMap &src);

struct VariableInfo {
    std::string name, fullName;
    ExprType type;
    FunctionInfo *blgFunc;
    ClassInfo *blgCls;
    NamespaceInfo *blgNsp;
    IdentifierVisibility visibility;

    RootNode *blgRoot;

    uint64 offset;

    VariableInfo();

    IdentifierNode *getNameNode() const;
    void setNameNode(IdentifierNode *nameNode);
    IdentifierNode *getTypeNode() const;
    void setTypeNode(IdentifierNode *typeNode);
    ExpressionNode *getInitNode() const;
    void setInitNode(ExpressionNode *initNode);

    IdentifierRegion getRegion() const;
private:
    IdentifierNode *nameNode, *typeNode;
    ExpressionNode *initNode;
};

struct FunctionInfo {
    std::string name, nameWithParam, fullName;
    std::vector<ClassInfo *> genericClasses;
    std::vector<VariableInfo *> params;
    ExprType resType;
    IdentifierVisibility visibility;

    ClassInfo *blgCls;
    NamespaceInfo *blgNsp;
    RootNode *blgRoot;
    
    FuncDefNode *defNode;

    uint64 offset;

    FunctionInfo();

    FuncDefNode *getDefNode() const;
    /// @brief This function will update the defNode, and the information that stored in defNode excepts the content.
    /// @param defNode 
    void setDefNode(FuncDefNode *defNode);

    IdentifierRegion getRegion() const;

    /// @brief This function will check whether the param list satisfy the type requirements of 
    /// this function and return the result and the expression type of the return value of this function.
    /// @param gsMap the subtitution map of generic class in the generic list
    /// @param paramList the param list
    /// @return (Whether the param list satisfy the requirements, the expression type of the return value)
    std::pair<bool, ExprType> satisfy(const GenericSubstitutionMap &gsMap, const std::vector<ExprType> &paramList);
};

typedef std::vector<FunctionInfo *> FunctionList;

struct ClassInfo {
    std::string name, fullName;
    ClassInfo *baseCls;
    std::vector<ClassInfo *> genericClasses;
    /// @brief the params for every generic classes in base class
    std::vector<ExprType> genericParams;

    std::map<std::string, VariableInfo *> fieldMap;
    std::map<std::string, FunctionList> functionMap;

    IdentifierVisibility visibility;

    uint64 size, dep;

    NamespaceInfo *blgNsp;
    RootNode *blgRoot;

    ClsDefNode *defNode;

    bool isGeneric;

    std::vector<ClassInfo *> derivedClasses;

    ClassInfo();
};

struct NamespaceInfo {
    std::string name, fullName;
    
    std::map<std::string, VariableInfo *> varMap;
    std::map<std::string, FunctionList> functionMap;
    std::map<std::string, ClassInfo *> clsMap;
    std::map<std::string, NamespaceInfo *> nspMap;

    NamespaceInfo *blgNsp;
    
    std::vector<std::pair<NspDefNode *, RootNode * > > defList;

    NamespaceInfo();
};

extern NamespaceInfo *rootNsp;
extern ClassInfo *basicCls, *int8Cls, *uint8Cls, *int16Cls, *uint16Cls, *int32Cls, *uint32Cls, *int64Cls, *uint64Cls;

bool isBasicCls(ClassInfo *cls);

FunctionInfo *getCurFunc();
void setCurFunc(FunctionInfo *func);
ClassInfo *getCurCls();
void setCurCls(ClassInfo *cls);
NamespaceInfo *getCurNsp();
void setCurNsp(NamespaceInfo *nsp);
RootNode *getCurRoot();
bool setCurRoot(RootNode *node);

ClassInfo *findCls(const std::string &path);
FunctionInfo *findFunc(const std::string &path);
VariableInfo *findVar(const std::string &path);

bool buildIdenSystem(const RootList &roots);

void debugPrintNspStruct(NamespaceInfo *nsp, int dep = 0);

