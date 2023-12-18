#include "tokenlist.h"

enum class SyntaxNodeType {
    Identifier, ConstValue, String, GenericArea,
    If, Else, While, For, Break, Continue, Return, Block, 
    Expression, Operator, MethodCall,
    VarDef, VarFuncDef, FuncDef, ClsDef, NspDef,

    SourceFile, DefinitionFile,
    Empty, Unknown,
};

class EResultType {
    
};

#pragma region Syntax Node
class SyntaxNode {
protected:
    SyntaxNodeType type;
    Token token;

    SyntaxNode(SyntaxNodeType type);
    SyntaxNode(SyntaxNodeType type, const Token &tk);

    std::vector<SyntaxNodeType*> children;
public:
    virtual std::string toString() const = 0;

    SyntaxNodeType getType() const;

    const Token &getToken() const;
    void setToken(const Token &tk);

    size_t childrenCount() const;
    const SyntaxNode &operator [] (size_t index) const;
    SyntaxNode &operator [] (size_t index);

    /// @brief build this node and its children using the token list, and the start point is ST, then return the end point in ED
    /// @param tkList the token list
    /// @param st the start point 
    /// @param ed the end point
    /// @return if it is successful to build this syntax node
    virtual bool buildNode(const TokenList &tkList, size_t st, size_t &ed) = 0;
};

/// @brief the syntax node of an expression
class ExpressionNode : public SyntaxNode {
protected:
    EResultType resultType;
public:
    ExpressionNode(SyntaxNodeType *type);
    ExpressionNode(SyntaxNodeType *type, const Token &tk);
    const EResultType &getResultType();

    virtual bool buildNode(const TokenList &tkList, size_t st, size_t &ed);

    virtual uint32 getWeight() const;
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

    virtual bool buildNode(const TokenList &tkList, size_t st, size_t &ed);
};

class MethodCallNode : public IdentifierNode {
private:
    MethodCallNode();
    MethodCallNode(const Token &tk);
    virtual bool buildNode(const TokenList &tkList, size_t st, size_t &ed);
};

/// @brief the syntax node of an operator
class OperatorNode : public ExpressionNode {
public:
    std::string toString() const;

    OperatorNode();
    OperatorNode(TokenType opType);
    OperatorNode(const Token &tk);
    
    TokenType getOperator() const;
    uint32 getWeight() const;

    bool buildNode(const TokenList &tkList, size_t st, size_t &ed);
};

class ConstValueNode : public ExpressionNode {
public:
    std::string toString() const;

    ConstValueNode();
    ConstValueNode(const UnionData &data);
    ConstValueNode(const Token &tk);
    
    UnionData &data();

    bool buildNode(const TokenList &tkList, size_t st, size_t &ed);
};

class GenericAreaNode : public ExpressionNode {
public:
    std::string toString() const;

    GenericAreaNode();
    GenericAreaNode(const Token &tk);
    
    bool buildNode(const TokenList &tkList, size_t st, size_t &ed);
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
};

class WhileNode : public SyntaxNode {
public:
    std::string toString() const;

    WhileNode();
    WhileNode(const Token &tk);
    
    const ExpressionNode &getCondNode() const;
    const SyntaxNode &getContentNode() const;

    bool buildNode(const TokenList &tkList, size_t st, size_t &ed);
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
};

class VarDefNode : public SyntaxNode {
public:
    std::string toString() const;

    VarDefNode();
    VarDefNode(const Token &tk);
    
    bool buildNode(const TokenList &tkList, size_t st, size_t &ed);
};

#pragma endregion
