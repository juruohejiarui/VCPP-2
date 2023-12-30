#pragma once
#include "syntaxnode.h"

enum class IdentifierRegion {
    Local, Member, Global
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

    ExprType convertToBase() const;
};

ExprType substitute(const ExprType &target, ClassInfo *cls, const ExprType &clsImpl);

struct VariableInfo {
    std::string name, fullName;
    ExprType type;
    FunctionInfo *blgFunc;
    ClassInfo *blgCls;
    NamespaceInfo *blgNsp;


    RootNode *blgRoot;

    uint64 offset;

    VariableInfo();

    IdentifierNode *getNameNode() const;
    void setNameNode(IdentifierNode *nameNode);
    IdentifierNode *getTypeNode() const;
    void setTypeNode(IdentifierNode *typeNode);

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

    ClassInfo *blgCls;
    NamespaceInfo *blgNsp;
    RootNode *blgRoot;
    
    FuncDefNode *defNode;

    FunctionInfo();

    IdentifierRegion getRegion() const;

    bool equalTo(const FunctionInfo &target) const;
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

    uint64 size;

    NamespaceInfo *blgNsp;
    RootNode *blgRoot;

    ClassInfo();
};

struct NamespaceInfo {
    std::string name, fullName;
    
    std::map<std::string, VariableInfo *> varMap;
    std::map<std::string, FunctionList> functionMap;
    std::map<std::string, ClassInfo *> clsMap;
    std::map<std::string, NamespaceInfo *> nspMap;

    NamespaceInfo *blgNsp;
    RootNode *blgRoot;

    NamespaceInfo();
};

bool isBasicCls(ClassInfo *cls);

FunctionInfo *getCurFunc();
void switchCurFunc(FunctionInfo *func);
ClassInfo *getCurCls();
void switchCurCls(ClassInfo *cls);
NamespaceInfo *getCurNsp();
void switchCurNsp(NamespaceInfo *nsp);
RootNode *getCurRoot();
void switchCurRoot(RootNode *node);

ClassInfo *findCls(const std::string &path);
FunctionInfo *findFunc(const std::string &path);
VariableInfo *findVar(const std::string &path);

bool buildIdenSystem(const RootList &roots);