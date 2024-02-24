#pragma once
#include "../Tools/tools.h"
#include "tokenlist.h"

extern const uint32 IdentifierWeight;

enum class SyntaxNodeType {
    Expression, Identifier, ConstValue, Operator, GenericArea,
    Block, If, While, For, Continue, Break, Return,
    VarDef, FuncDef, VarFuncDef, ClsDef, NspDef,
    Using,
    SourceRoot, SymbolRoot,
    Error, Empty, Unknown,
};

class SyntaxNode {
protected:
    Token token;
    std::vector<SyntaxNode *> children;
    SyntaxNodeType type;
    uint32 localVarCount;
    SyntaxNode *parent;
public:
    SyntaxNode(SyntaxNodeType type);
    SyntaxNode(SyntaxNodeType type, const Token &token);
    virtual ~SyntaxNode();

    Token &getToken();
    const Token &getToken() const;
    void setToken(const Token &token);

    SyntaxNodeType getType() const;

    
    SyntaxNode *&operator [] (int index);
    SyntaxNode *operator [] (int index) const;

    SyntaxNode *&get(int index);
    SyntaxNode *get(int index) const;

    virtual void addChild(SyntaxNode *child);

    size_t getChildrenCount() const;

    /// @brief clear the children of this node PS: this function will not delete the children
    void clearChildren();
    /// @brief remove the child at INDEX PS: this function will not delete the child
    /// @param index 
    void removeChild(size_t index);
    /// @brief remove the child CHILD PS: this function will not delete the child
    /// @param child
    void removeChild(SyntaxNode *child);
    void insertChild(size_t index, SyntaxNode *child);
    /// @brief replace the child at INDEX with CHILD PS: this function will not delete the old child
    void replaceChild(size_t index, SyntaxNode *child);
    /// @brief replace the child OLDCHILD with NEWCHILD PS: this function will not delete the old child
    void replaceChild(SyntaxNode *oldChild, SyntaxNode *newChild);

    SyntaxNode *getParent() const;

    virtual std::string toString() const;

    uint32 getLocalVarCount() const;
};

class ExpressionNode : public SyntaxNode {
public:
    ExpressionNode();
    ExpressionNode(const Token &token);

    ExpressionNode *getContent() const;

    virtual uint32 getWeight() const;
};

class OperatorNode : public ExpressionNode {
public:
    OperatorNode();
    OperatorNode(const Token &token);

    ExpressionNode *&getLeft();
    ExpressionNode *getLeft() const;
    ExpressionNode *&getRight();
    ExpressionNode *getRight() const;
    
    uint32 getWeight() const override;

    std::string toString() const override;
};

class GenericAreaNode;
class ConstValueNode;

class IdentifierNode : public ExpressionNode {
private:
    std::string name;
public:
    IdentifierNode();
    IdentifierNode(const Token &token);

    GenericAreaNode *getGenericArea() const ;
    void setGenericArea(GenericAreaNode *node);
    uint32 getDimc() const ;
    void setDimc(uint32 dimc);

    uint32 getWeight() const override;

    const std::string &getName() const;
    void setName(const std::string &name);

    size_t getParamCount() const;
    ExpressionNode *getParam(size_t index) const;

    bool isFuncCall() const;

    std::string toString() const override;
};

class GenericAreaNode : public SyntaxNode {
public:
    GenericAreaNode();
    GenericAreaNode(const Token &token);

    size_t getParamCount() const;
    IdentifierNode *getParam(size_t index) const;
};

class ConstValueNode : public ExpressionNode {
public:
    ConstValueNode();
    ConstValueNode(const Token &token);

    uint32 getWeight() const override;

    std::string toString() const override;
};

class BlockNode : public SyntaxNode {
public:
    BlockNode();
    BlockNode(const Token &token);

    void setLocalVarCount(uint32 data);

    std::string toString() const override;
};

class IfNode : public BlockNode {
public:
    IfNode();
    IfNode(const Token &token);

    ExpressionNode *getCondNode() const;
    void setCondNode(ExpressionNode *node);
    SyntaxNode *getSuccNode() const;
    void setSuccNode(SyntaxNode *node);
    SyntaxNode *getFailNode() const;
    void setFailNode(SyntaxNode *node);
};

class LoopNode : public BlockNode {
public:
    LoopNode(SyntaxNodeType type);
    LoopNode(SyntaxNodeType type, const Token &token);

    SyntaxNode *getInitNode() const;
    void setInitNode(SyntaxNode *node);
    ExpressionNode *getCondNode() const;
    void setCondNode(ExpressionNode *node);
    ExpressionNode *getStepNode() const;
    void setStepNode(ExpressionNode *node);
    SyntaxNode *getContent() const;
    void setContent(SyntaxNode *node);
};

class ControlNode : public SyntaxNode {
public:
    ControlNode(SyntaxNodeType type);
    ControlNode(SyntaxNodeType type, const Token &token);

    ExpressionNode *getContent() const;
};
class VarDefNode : public BlockNode {
private:
    IdenVisibility visibility;
public:
    VarDefNode();
    VarDefNode(const Token &token);

    IdenVisibility getVisibility() const;
    void setVisibility(IdenVisibility visibility);

    /// @brief get the info of the index-th definition
    /// @param index 
    /// @return a tuple that represents the name, the type, and the expression for initialization
    std::tuple<IdentifierNode *, IdentifierNode *, ExpressionNode *> getVariable(size_t index) const ;
};

class FuncDefNode : public BlockNode {
private:
    IdenVisibility visibility;
public:
    FuncDefNode();
    FuncDefNode(const Token &token);

    IdenVisibility getVisibility() const;
    void setVisibility(IdenVisibility visibility);

    IdentifierNode *getNameNode() const ;
    size_t getParamCount() const;
    std::pair<IdentifierNode *, IdentifierNode *> getParam(size_t index) const;
    IdentifierNode *getResNode() const;
    SyntaxNode *getContent() const;
};

class VarFuncDefNode : public FuncDefNode {
public:
    VarFuncDefNode();
    VarFuncDefNode(const Token &token);
};

class ClsDefNode : public SyntaxNode {
private:
    IdenVisibility visibility;
    std::vector<size_t> fieldIndex, funcIndex, varFuncIndex;
public:
    ClsDefNode();
    ClsDefNode(const Token &token);

    IdenVisibility getVisibility() const;
    void setVisibility(IdenVisibility visibility); 

    size_t getFieldCount() const;
    size_t getFuncCount() const;
    size_t getVarFuncCount() const;

    VarDefNode *getFieldDef(size_t index) const;
    FuncDefNode *getFuncDef(size_t index) const;
    VarFuncDefNode *getVarFuncDef(size_t index) const;

    IdentifierNode *getNameNode() const;
    void setNameNode(IdentifierNode *node);
    IdentifierNode *getBaseNode() const;
    void setBaseNode(IdentifierNode *node);

    void addChild(SyntaxNode *child) override;
};

class NspDefNode : public SyntaxNode {
private:
    std::vector<size_t> clsIndex, varIndex, funcIndex;
public:
    NspDefNode();
    NspDefNode(const Token &token);

    size_t getVarCount() const;
    size_t getFuncCount() const;
    size_t getClsCount() const;
    
    VarDefNode *getVarDef(size_t index) const;
    FuncDefNode *getFuncDef(size_t index) const;
    ClsDefNode *getClsDef(size_t index) const;

    IdentifierNode *getNameNode() const;
    void setNameNode(IdentifierNode *node);

    void addChild(SyntaxNode *child) override;
};

class UsingNode : public SyntaxNode {
private:
    std::string path;
public:
    UsingNode();
    UsingNode(const Token &token);

    const std::string getPath() const;
    void setPath(const std::string &path);

    std::string toString() const override;
};

class RootNode : public SyntaxNode {
private:
    std::string path;
    std::vector<size_t> usingIndex, defIndex;
public:
    RootNode(SyntaxNodeType type);
    
    size_t getUsingCount() const;
    size_t getDefCount() const;

    UsingNode *getUsing(size_t index) const;
    SyntaxNode *getDef(size_t index) const;

    void addChild(SyntaxNode *node) override;
};

typedef std::vector<RootNode *> RootList;

SyntaxNode *buildNode(const TokenList &tkList, size_t l, size_t &r);
RootNode *buildRootNode(SyntaxNodeType type, const TokenList &tkList);

void debugPrintTree(SyntaxNode *node, int dep = 0);