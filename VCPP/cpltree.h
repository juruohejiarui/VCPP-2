#pragma once
#include "tokenlist.h"

enum class SyntaxNodeType {
    Identifier, ConstValue, String, GenericArea,
    If, Else, While, For, Break, Continue, Return, Block, 
    Expression, Operator, MethodCall,
    VarDef, VarFuncDef, FuncDef, ClsDef, NspDef,

    SourceFile, DefinitionFile,
    Empty, Unknown,
};

extern std::string syntaxNodeTypeString[];
extern const int syntaxNodeTypeNumber;

class ClassInfo {
    std::string name, fullName;
};
struct EResultType {
    ClassInfo* cls;
    int dimc;
    ValueTypeModifier valueType;
    bool isConst;
    std::vector<EResultType> genericList;

    EResultType();
    EResultType(ClassInfo* cls, int dimc = 0, ValueTypeModifier valueType = ValueTypeModifier::t, bool isConst = false);

    std::string toString() const ;
};

#pragma region Syntax Node
class SyntaxNode {
protected:
    SyntaxNodeType type;
    Token token;

    std::vector<SyntaxNode*> children;
public:
    SyntaxNode(SyntaxNodeType type);
    SyntaxNode(SyntaxNodeType type, const Token& tk);
    virtual std::string toString() const = 0;

    SyntaxNodeType getType() const;

    const Token &getToken() const;
    void setToken(const Token &tk);

    size_t childrenCount() const;
    void addChild(SyntaxNode *child);
    SyntaxNode *operator [] (size_t index) const;
    SyntaxNode *&operator [] (size_t index);

    /// @brief build this node and its children using the token list, and the start point is ST, then return the end point in ED
    /// @param tkList the token list
    /// @param st the start point 
    /// @param ed the end point
    /// @return if it is successful to build this syntax node
    virtual bool buildNode(const TokenList &tkList, size_t st, size_t &ed) = 0;
    virtual bool checkEResultType() = 0;
};

/// @brief the syntax node of an expression
class ExpressionNode : public SyntaxNode {
protected:
    EResultType resultType;
public:
    virtual std::string toString() const;
    ExpressionNode(SyntaxNodeType type);
    ExpressionNode(SyntaxNodeType type, const Token &tk);
    const EResultType &getResultType() const ;

    bool buildNode(const TokenList &tkList, size_t st, size_t &ed);
    bool buildNode(const TokenList &tkList, size_t st, size_t ed);

    int32 getWeight() const;
    virtual bool checkEResultType();
};
/// @brief the syntax node of an identifier
class IdentifierNode : public ExpressionNode {
private:
    std::string name;
    uint64 id;
public:
    IdentifierNode();
    IdentifierNode(const Token &tk);
    
    std::string toString() const;
    
    void setId(uint64 id);
    const uint64 &getId() const;
    
    void setName(const std::string &name);
    const std::string &getName() const;

    virtual bool checkEResultType();
};

class MethodCallNode : public IdentifierNode {
public:
    MethodCallNode();
    MethodCallNode(const Token &tk);
    virtual bool buildNode(const TokenList &tkList, size_t st, size_t &ed);
    
    bool checkEResultType();
};

/// @brief the syntax node of an operator
class OperatorNode : public ExpressionNode {
public:
    std::string toString() const;

    OperatorNode();
    OperatorNode(TokenType opType);
    OperatorNode(const Token &tk);
    
    TokenType getOperator() const;
    int32 getWeight() const;

    bool checkEResultType();
};

class ConstValueNode : public ExpressionNode {
public:
    std::string toString() const;

    ConstValueNode();
    ConstValueNode(const UnionData &data);
    ConstValueNode(const Token &tk);
    
    UnionData &data();

    bool checkEResultType();
};

class GenericAreaNode : public ExpressionNode {
public:
    std::string toString() const;

    GenericAreaNode();
    GenericAreaNode(const Token &tk);
    
    bool buildNode(const TokenList &tkList, size_t st, size_t ed);
    bool checkEResultType();
};

class ConditionNode : public SyntaxNode {
public:
    std::string toString() const;

    ConditionNode();
    ConditionNode(const Token &tk);

    const ExpressionNode &getCondNode() const;
    const SyntaxNode &getSuccNode() const;
    const SyntaxNode &getFailNode() const;

    bool buildNode(const TokenList &tkList, size_t st, size_t &ed);
    bool checkEResultType();
};

class WhileNode : public SyntaxNode {
public:
    std::string toString() const;

    WhileNode();
    WhileNode(const Token &tk);
    
    const ExpressionNode &getCondNode() const;
    const SyntaxNode &getContentNode() const;

    bool buildNode(const TokenList &tkList, size_t st, size_t &ed);
    bool checkEResultType();
};

class ForNode : public SyntaxNode {
public:
    std::string toString() const;

    ForNode();
    ForNode(const Token &tk);

    const SyntaxNode &getInitNode() const;
    const ExpressionNode &getCondNode() const;
    const ExpressionNode &getModifyNode() const;
    const SyntaxNode &getContentNode() const;

    bool buildNode(const TokenList &tkList, size_t st, size_t &ed);
    bool checkEResultType();
};

class VarDefNode : public SyntaxNode {
private:
    IdentifierVisibility visibility;
public:
    std::string toString() const;

    VarDefNode();
    VarDefNode(const Token &tk);
    
    bool buildNode(const TokenList &tkList, size_t st, size_t &ed);
    IdentifierVisibility getVisibility() const;
    bool checkEResultType();
};

class FuncDefNode : public SyntaxNode {
private:
    IdentifierVisibility visibility;
public:
    std::string toString() const;

    FuncDefNode();
    FuncDefNode(const Token& tk);

    bool buildNode(const TokenList& tkList, size_t st, size_t& ed);
    IdentifierVisibility getVisibility() const;
    bool checkEResultType();
};

class ClsDefNode : public SyntaxNode {
private:
    IdentifierVisibility visibility;
public:
    std::string toString() const;

    ClsDefNode();
    ClsDefNode(const Token& tk);

    bool buildNode(const TokenList& tkList, size_t st, size_t& ed);
    IdentifierVisibility getVisibility() const;
    bool checkEResultType();
};
class NspDefNode : public SyntaxNode {
public:
    std::string toString() const;

    NspDefNode();
    NspDefNode(const Token& tk);

    bool buildNode(const TokenList& tkList, size_t st, size_t& ed);
    bool checkEResultType();
};

class BlockNode : public SyntaxNode {
public:
    std::string toString() const;

    BlockNode();
    BlockNode(const Token& tk);

    bool buildNode(const TokenList& tkList, size_t st, size_t& ed);
    bool checkEResultType();
};

#pragma endregion
