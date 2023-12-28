#pragma once
#include "../Tools/tools.h"
#include "tokenlist.h"
enum SyntaxNodeType {
    Expression, Identifier, ConstValue, Operator, GenericArea,
    Block, If, While, For, Continue, Break, Return,
    VarDef, FuncDef, VarFuncDef, ClsDef, NspDef,
    Error, Unknown, 
};

class SyntaxNode {
protected:
    Token token;
    std::vector<SyntaxNode *> children;
    SyntaxNodeType type;
    uint32 localVarCount;
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

    SyntaxNode *&at(int index);
    SyntaxNode *at(int index) const;

    void addChild(SyntaxNode *child);
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

    ExpressionNode *getLeft();
    ExpressionNode *getRight();
    
    uint32 getWeight() const override;
};

class GenericAreaNode;

class IdentifierNode : public ExpressionNode {
public:
    IdentifierNode();
    IdentifierNode(const Token &token);

    GenericAreaNode *getGenericArea();

    uint32 getWeight() const override;
};

class GenericAreaNode : public SyntaxNode {
public:
    GenericAreaNode();
    GenericAreaNode(const Token &token);

    size_t getGenericCount() const;
    IdentifierNode *getIdentifier(size_t index) const;
};

class ConstValueNode : public ExpressionNode {
public:
    ConstValueNode();
    ConstValueNode(const Token &token);

    uint32 getWeight() const override;
};

class BlockNode : public SyntaxNode {
public:
    BlockNode();
    BlockNode(const Token &token);

    uint32 getLocalVarCount() const;
};

class IfNode : public BlockNode {
public:
    IfNode();
    IfNode(const Token &token);

    ExpressionNode *getCondNode() const;
    SyntaxNode *getSuccNode() const;
    SyntaxNode *getFailNode() const;
};

class LoopNode : public BlockNode {
public:
    LoopNode(SyntaxNodeType type);
    LoopNode(SyntaxNodeType type, const Token &token);

    SyntaxNode *getInitNode() const;
    ExpressionNode *getCondNode() const;
    ExpressionNode *getStepNode() const;
    SyntaxNode *getContent() const;
};

class ControlNode : public SyntaxNode {
public:
    ControlNode(SyntaxNodeType type);
    ControlNode(SyntaxNodeType type, const Token &token);

    ExpressionNode *getContent() const;
};
class VarDefNode : public BlockNode {
private:
    IdentifierVisibility visibility;
public:
    VarDefNode();
    VarDefNode(const Token &token);

    IdentifierVisibility getVisibility() const;
    void setVisibility(IdentifierVisibility visibility);

    /// @brief get the info of the index-th definition
    /// @param index 
    /// @return a tuple that represents the name, the type, and the expression for initialization
    std::tuple<IdentifierNode *, IdentifierNode *, ExpressionNode *> getInfo(size_t index) const ;
};

class FuncDefNode : public BlockNode {
private:
    IdentifierVisibility visibility;
public:
    FuncDefNode();
    FuncDefNode(const Token &token);

    IdentifierVisibility getVisibility() const;
    void setVisibility(IdentifierVisibility visibility);

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

SyntaxNode *buildNode(const TokenList &tkList, size_t l, size_t &r);