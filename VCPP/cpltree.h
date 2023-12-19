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

struct IdentifierInfo;
struct VariableInfo;
struct FunctionInfo;
struct ClassInfo;
struct NamespaceInfo;
class SyntaxNode;

/**
 * @brief Array of strings representing syntax node types.
 * 
 * This array is used to map syntax node types to their corresponding string representations.
 * Each element in the array represents a syntax node type at the corresponding index.
 * 
 * @see syntaxNodeTypeString
 */
extern const std::string syntaxNodeTypeString[];
/**
 * @brief The number of syntax node types.
 *
 * This constant represents the total number of syntax node types in the program.
 * It is used to define the size of arrays or other data structures that store syntax nodes.
 */
extern const int syntaxNodeTypeNumber;

enum class IdentifierType {
    Variable, Function, VarFunction, Class, Namespace, Unknown,
};
/**
 * @brief Array of strings representing identifier types.
 *
 * This array stores the different types of identifiers as strings.
 * The index of each string corresponds to the identifier type.
 * For example, identifierTypeString[0] represents the string for the first identifier type.
 */
extern const std::string identifierTypeString[];
/**
 * @brief The number representing the type of identifier.
 */
extern const int identifierTypeNumber;

/**
 * @brief Determines the type of an identifier.
 * 
 * This function takes a string representing an identifier name and returns the type of the identifier.
 * The type can be one of the following: enum, class, struct, variable, function, or unknown.
 * 
 * @param name The name of the identifier.
 * @return The type of the identifier.
 */
IdentifierType getIdentifierType(const std::string &name);

struct EResultType {
    ClassInfo* cls;
    std::string clsName;
    int dimc;
    ValueTypeModifier valueType;
    bool isConst;
    std::vector<EResultType> genericList;

    EResultType();
    EResultType(ClassInfo* cls, int dimc = 0, ValueTypeModifier valueType = ValueTypeModifier::t, bool isConst = false);

    std::string toString() const ;
};

struct IdentifierInfo {
    IdentifierType type;
    std::string name, fullName;
    EResultType resultType;
    IdentifierVisibility visibility;
    SyntaxNode *defNode;

    IdentifierInfo();
    IdentifierInfo(const std::string &name);
    IdentifierInfo(const std::string &name, const std::string &fullName);

    virtual std::string toString() const;
};

struct VariableInfo : public IdentifierInfo {
    VariableInfo();
    VariableInfo(const std::string &name);
    VariableInfo(const std::string &name, const std::string &fullName);
    VariableInfo(const std::string &name, const std::string &fullName, const EResultType &resultType);
    std::string toString() const;
};

struct FunctionInfo : public IdentifierInfo {
    std::vector<VariableInfo *> paramList;
    FunctionInfo(bool isVarFunction = false);
    FunctionInfo(const std::string &name);
    FunctionInfo(const std::string &name, const std::string &fullName, bool isVarFunction = false);
    FunctionInfo(const std::string &name, const std::string &fullName, const EResultType &resultType, bool isVarFunction = false);

    std::string toString() const;
};

struct ClassInfo : public IdentifierInfo {
    std::map<std::string, VariableInfo *> varMap;
    std::map<std::string, FunctionInfo *> funcMap;

    ClassInfo *parent;
    std::vector<EResultType> parentGenericList;

    ClassInfo();
    ClassInfo(const std::string &name);
    ClassInfo(const std::string &name, const std::string &fullName);

    std::string toString() const;
};

struct NamespaceInfo : public IdentifierInfo {
    std::map<std::string, VariableInfo *> varMap;
    std::map<std::string, FunctionInfo *> funcMap;
    std::map<std::string, ClassInfo *> clsMap;
    std::map<std::string, NamespaceInfo *> nspMap;

    NamespaceInfo();
    NamespaceInfo(const std::string &name);
    NamespaceInfo(const std::string &name, const std::string &fullName);

    std::string toString() const;
};

#pragma region Syntax Node
/**
 * @class SyntaxNode
 * @brief Represents a node in the syntax tree.
 * 
 * The SyntaxNode class provides a base class for all nodes in the syntax tree.
 * It contains information about the node's type, associated token, and child nodes.
 * Derived classes must implement the checkEResultType() function.
 */
class SyntaxNode {
protected:
    uint32 varCount;
    SyntaxNodeType type; /**< The type of the syntax node. */
    Token token; /**< The associated token. */

    std::vector<SyntaxNode*> children; /**< The child nodes of the syntax node. */
public:
    /**
     * @brief Constructs a SyntaxNode object with the given token.
     * @param tk The associated token.
     */
    SyntaxNode(const Token& tk);

    /**
     * @brief Constructs a SyntaxNode object with the given type.
     * @param type The type of the syntax node.
     */
    SyntaxNode(SyntaxNodeType type);

    /**
     * @brief Constructs a SyntaxNode object with the given type and token.
     * @param type The type of the syntax node.
     * @param tk The associated token.
     */
    SyntaxNode(SyntaxNodeType type, const Token& tk);

    /**
     * @brief Converts the syntax node to a string representation.
     * @return The string representation of the syntax node.
     */
    virtual std::string toString() const;

    /**
     * @brief Gets the type of the syntax node.
     * @return The type of the syntax node.
     */
    SyntaxNodeType getType() const;

    /**
     * @brief Gets the associated token.
     * @return The associated token.
     */
    const Token &getToken() const;

    /**
     * @brief Sets the associated token.
     * @param tk The associated token.
     */
    void setToken(const Token &tk);

    /**
     * @brief Gets the number of child nodes.
     * @return The number of child nodes.
     */
    size_t childrenCount() const;

    /**
     * @brief Adds a child node to the syntax node.
     * @param child The child node to add.
     */
    void addChild(SyntaxNode *child);

    /**
     * @brief Gets the child node at the specified index.
     * @param index The index of the child node.
     * @return The child node at the specified index.
     */
    SyntaxNode *operator [] (size_t index) const;

    /**
     * @brief Gets a reference to the child node at the specified index.
     * @param index The index of the child node.
     * @return A reference to the child node at the specified index.
     */
    SyntaxNode *&operator [] (size_t index);

    /**
     * @brief Gets the number of variables.
     *
     * This function returns the count of variables in the tree.
     *
     * @return The number of variables.
     */
    uint32 getVarCount() const;

    /**
     * @brief Builds the syntax node from a list of tokens.
     * @param tkList The list of tokens.
     * @param st The starting index in the token list.
     * @param ed The ending index in the token list (updated by the function).
     * @return True if the syntax node was successfully built, false otherwise.
     */
    virtual bool buildNode(const TokenList &tkList, size_t st, size_t &ed) = 0;

    /**
     * @brief Checks the expected result type of the syntax node.
     * @return True if the expected result type is valid, false otherwise.
     */
    virtual bool checkEResultType() = 0;
};

/**
 * @class ExpressionNode
 * @brief Represents a node in the syntax tree that represents an expression.
 * 
 * This class is derived from the SyntaxNode class and provides additional functionality
 * specific to expression nodes. It stores the result type of the expression and provides
 * methods for building the node from a list of tokens and checking the result type.
 */
class ExpressionNode : public SyntaxNode {
protected:
    EResultType resultType; /**< The result type of the expression. */
public:
    /**
     * @brief Converts the expression node to a string representation.
     * @return The string representation of the expression node.
     */
    virtual std::string toString() const;

    /**
     * @brief Constructs an expression node with the specified syntax node type.
     * @param type The syntax node type of the expression node.
     */
    ExpressionNode(SyntaxNodeType type);

    /**
     * @brief Constructs an expression node with the specified syntax node type and token.
     * @param type The syntax node type of the expression node.
     * @param tk The token associated with the expression node.
     */
    ExpressionNode(SyntaxNodeType type, const Token &tk);

    /**
     * @brief Gets the result type of the expression.
     * @return The result type of the expression.
     */
    const EResultType &getResultType() const;

    /**
     * @brief Builds the expression node from a list of tokens within the specified range.
     * @param tkList The list of tokens.
     * @param st The start index of the range.
     * @param ed The end index of the range (updated by the method).
     * @return True if the node was successfully built, false otherwise.
     */
    bool buildNode(const TokenList &tkList, size_t st, size_t &ed);

    /**
     * @brief Builds the expression node from a list of tokens within the specified range.
     * @param tkList The list of tokens.
     * @param st The start index of the range.
     * @param ed The end index of the range.
     * @return True if the node was successfully built, false otherwise.
     */
    bool buildNode(const TokenList &tkList, size_t st, size_t ed);

    /**
     * @brief Gets the weight of the expression node.
     * @return The weight of the expression node.
     */
    int32 getWeight() const;

    /**
     * @brief Checks the result type of the expression.
     * @return True if the result type is valid, false otherwise.
     */
    virtual bool checkEResultType();
};
/**
 * @class IdentifierNode
 * @brief Represents a node in the abstract syntax tree that represents an identifier.
 * 
 * This class inherits from ExpressionNode and provides functionality to store and retrieve
 * the name and ID of the identifier.
 */
class IdentifierNode : public ExpressionNode {
private:
    std::string name; ///< The name of the identifier.
    uint64 id; ///< The ID of the identifier.
public:
    /**
     * @brief Default constructor for IdentifierNode.
     */
    IdentifierNode();

    /**
     * @brief Constructor for IdentifierNode that takes a token.
     * @param tk The token representing the identifier.
     */
    IdentifierNode(const Token &tk);

    /**
     * @brief Converts the IdentifierNode to a string representation.
     * @return The string representation of the IdentifierNode.
     */
    std::string toString() const;
    
    /**
     * @brief Sets the ID of the identifier.
     * @param id The ID to set.
     */
    void setId(uint64 id);

    /**
     * @brief Gets the ID of the identifier.
     * @return The ID of the identifier.
     */
    const uint64 &getId() const;
    
    /**
     * @brief Sets the name of the identifier.
     * @param name The name to set.
     */
    void setName(const std::string &name);

    /**
     * @brief Gets the name of the identifier.
     * @return The name of the identifier.
     */
    const std::string &getName() const;

    /**
     * @brief Builds a node in the tree using the given token list.
     * 
     * This function takes a token list, starting index, and an ending index as parameters.
     * It builds a node in the tree using the tokens from the token list within the specified range.
     * The ending index is updated to the index of the last token used in building the node.
     * 
     * @param tkList The token list to build the node from.
     * @param st The starting index of the token list.
     * @param ed The ending index of the token list (updated by the function).
     * @return true if the node is successfully built, false otherwise.
     */
    bool buildNode(const TokenList &tkList, size_t st, size_t &ed);

    /**
     * @brief Checks if the expression result type is valid.
     * @return True if the expression result type is valid, false otherwise.
     */
    virtual bool checkEResultType();
};

/**
 * @brief Represents a method call node.
 * 
 * This class inherits from IdentifierNode and represents a node that represents a method call.
 * It provides functionality to check the expected result type of the method call.
 */
class MethodCallNode : public IdentifierNode {
public:
    /**
     * @brief Default constructor for MethodCallNode.
     */
    MethodCallNode();

    /**
     * @brief Constructor for MethodCallNode.
     * @param tk The token associated with the method call.
     */
    MethodCallNode(const Token &tk);
    
    /**
     * @brief Checks the expected result type of the method call.
     * @return True if the expected result type is valid, false otherwise.
     */
    bool checkEResultType();
};

/**
 * @brief Represents a node in the expression tree that represents an operator.
 * 
 * This class inherits from ExpressionNode and provides additional functionality
 * specific to operator nodes.
 */
class OperatorNode : public ExpressionNode {
public:
    /**
     * @brief Converts the operator node to a string representation.
     * 
     * @return The string representation of the operator node.
     */
    std::string toString() const;

    /**
     * @brief Default constructor for OperatorNode.
     */
    OperatorNode();

    /**
     * @brief Constructor for OperatorNode with a specified operator type.
     * 
     * @param opType The type of the operator.
     */
    OperatorNode(TokenType opType);

    /**
     * @brief Constructor for OperatorNode with a specified token.
     * 
     * @param tk The token representing the operator.
     */
    OperatorNode(const Token &tk);
    
    /**
     * @brief Gets the type of the operator.
     * 
     * @return The type of the operator.
     */
    TokenType getOpType() const;

    /**
     * @brief Gets the weight of the operator.
     * 
     * @return The weight of the operator.
     */
    int32 getWeight() const;

    /**
     * @brief Checks if the operator node has a valid expression result type.
     * 
     * @return True if the expression result type is valid, false otherwise.
     */
    bool checkEResultType();
};

class ConstValueNode : public ExpressionNode {
public:
    /**
     * @brief Returns a string representation of the ConstValueNode.
     * 
     * @return std::string The string representation of the ConstValueNode.
     */
    std::string toString() const;
    
    /**
     * @brief Default constructor for ConstValueNode.
     */
    ConstValueNode();
    
    /**
     * @brief Constructor for ConstValueNode that takes a UnionData object.
     * 
     * @param data The UnionData object to initialize the ConstValueNode with.
     */
    ConstValueNode(const UnionData &data);
    
    /**
     * @brief Constructor for ConstValueNode that takes a Token object.
     * 
     * @param tk The Token object to initialize the ConstValueNode with.
     */
    ConstValueNode(const Token &tk);
    
    /**
     * @brief Returns a reference to the UnionData object of the ConstValueNode.
     * 
     * @return UnionData& A reference to the UnionData object of the ConstValueNode.
     */
    UnionData &data();
    
    /**
     * @brief Returns a copy of the UnionData object of the ConstValueNode.
     * 
     * @return UnionData A copy of the UnionData object of the ConstValueNode.
     */
    UnionData data() const;

    /**
     * @brief Checks if the expression result type of the ConstValueNode is valid.
     * 
     * @return bool True if the expression result type is valid, false otherwise.
     */
    bool checkEResultType();
};

/**
 * @brief Represents a generic area node.
 * 
 * This class inherits from ExpressionNode and provides functionality to represent a generic area node.
 */
class GenericAreaNode : public ExpressionNode {
public:
    /**
     * @brief Converts the generic area node to a string representation.
     * @return The string representation of the generic area node.
     */
    std::string toString() const;

    /**
     * @brief Default constructor for the generic area node.
     */
    GenericAreaNode();

    /**
     * @brief Constructor for the generic area node.
     * @param tk The token associated with the generic area node.
     */
    GenericAreaNode(const Token &tk);
    
    /**
     * @brief Builds the generic area node from a list of tokens.
     * @param tkList The list of tokens.
     * @param st The starting index of the tokens to consider.
     * @param ed The ending index of the tokens to consider.
     * @return True if the generic area node is successfully built, false otherwise.
     */
    bool buildNode(const TokenList &tkList, size_t st, size_t ed);

    /**
     * @brief Checks the expression result type of the generic area node.
     * @return True if the expression result type is valid, false otherwise.
     */
    bool checkEResultType();
};

/**
 * @brief Represents a node in the syntax tree that represents a condition.
 * 
 * This class inherits from SyntaxNode and provides methods to manipulate and access the condition node.
 */
class ConditionNode : public SyntaxNode {
public:
    /**
     * @brief Converts the ConditionNode object to a string representation.
     * @return The string representation of the ConditionNode object.
     */
    std::string toString() const;

    /**
     * @brief Default constructor for ConditionNode.
     */
    ConditionNode();

    /**
     * @brief Constructor for ConditionNode that takes a Token object.
     * @param tk The Token object to initialize the ConditionNode with.
     */
    ConditionNode(const Token &tk);

    /**
     * @brief Gets the condition node of the ConditionNode object.
     * @return A pointer to the ExpressionNode representing the condition.
     */
    ExpressionNode *getCondNode() const;

    /**
     * @brief Gets the successor node of the ConditionNode object.
     * @return A pointer to the SyntaxNode representing the successor.
     */
    SyntaxNode *getSuccNode() const;

    /**
     * @brief Gets the failure node of the ConditionNode object.
     * @return A pointer to the SyntaxNode representing the failure.
     */
    SyntaxNode *getFailNode() const;

    /**
     * @brief Builds the ConditionNode object from a list of tokens.
     * @param tkList The list of tokens to build the ConditionNode from.
     * @param st The starting index in the token list.
     * @param ed The ending index in the token list (updated by the function).
     * @return True if the node was successfully built, false otherwise.
     */
    bool buildNode(const TokenList &tkList, size_t st, size_t &ed);

    /**
     * @brief Checks the expression result type of the ConditionNode object.
     * @return True if the expression result type is valid, false otherwise.
     */
    bool checkEResultType();
};

/**
 * @class WhileNode
 * @brief Represents a while loop node in the syntax tree.
 * 
 * The WhileNode class is a derived class of SyntaxNode and represents a while loop in the syntax tree.
 * It provides methods for getting the condition node and content node of the while loop, as well as
 * building the node from a token list and checking the result type of the expression.
 */
class WhileNode : public SyntaxNode {
public:
    /**
     * @brief Converts the while loop node to a string representation.
     * @return The string representation of the while loop node.
     */
    std::string toString() const;

    /**
     * @brief Default constructor for WhileNode.
     */
    WhileNode();

    /**
     * @brief Constructor for WhileNode with a token.
     * @param tk The token associated with the while loop node.
     */
    WhileNode(const Token &tk);
    
    /**
     * @brief Gets the condition node of the while loop.
     * @return A pointer to the condition node.
     */
    ExpressionNode *getCondNode() const;

    /**
     * @brief Gets the content node of the while loop.
     * @return A pointer to the content node.
     */
    SyntaxNode *getContentNode() const;

    /**
     * @brief Builds the while loop node from a token list.
     * @param tkList The token list.
     * @param st The starting index in the token list.
     * @param ed The ending index in the token list (updated by the function).
     * @return True if the node was successfully built, false otherwise.
     */
    bool buildNode(const TokenList &tkList, size_t st, size_t &ed);

    /**
     * @brief Checks the result type of the expression in the while loop.
     * @return True if the result type is valid, false otherwise.
     */
    bool checkEResultType();
};

/**
 * @brief Represents a For loop node in the syntax tree.
 */
class ForNode : public SyntaxNode {
public:
    /**
     * @brief Converts the ForNode to a string representation.
     * @return The string representation of the ForNode.
     */
    std::string toString() const;

    /**
     * @brief Default constructor for ForNode.
     */
    ForNode();

    /**
     * @brief Constructor for ForNode with a given token.
     * @param tk The token associated with the ForNode.
     */
    ForNode(const Token &tk);

    /**
     * @brief Gets the initialization node of the ForNode.
     * @return The initialization node.
     */
    SyntaxNode *getInitNode() const;

    /**
     * @brief Gets the condition node of the ForNode.
     * @return The condition node.
     */
    ExpressionNode *getCondNode() const;

    /**
     * @brief Gets the step node of the ForNode.
     * @return The step node.
     */
    ExpressionNode *getStepNode() const;

    /**
     * @brief Gets the content node of the ForNode.
     * @return The content node.
     */
    SyntaxNode *getContentNode() const;

    /**
     * @brief Builds the ForNode from a list of tokens.
     * @param tkList The list of tokens.
     * @param st The starting index in the token list.
     * @param ed The ending index in the token list (updated by the function).
     * @return True if the ForNode was successfully built, false otherwise.
     */
    bool buildNode(const TokenList &tkList, size_t st, size_t &ed);

    /**
     * @brief Checks the expression result type of the ForNode.
     * @return True if the expression result type is valid, false otherwise.
     */
    bool checkEResultType();
};

class ControlNode : public SyntaxNode {
public:
    std::string toString() const;

    ControlNode(SyntaxNodeType type);
    ControlNode(const Token &tk);

    bool buildNode(const TokenList &tkList, size_t st, size_t &ed);
    bool checkEResultType();
};

class BlockNode : public SyntaxNode {
public:
    std::string toString() const;

    BlockNode();
    BlockNode(const Token &tk);

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
    
    IdentifierVisibility getVisibility() const;

    bool buildNode(const TokenList &tkList, size_t st, size_t &ed);
    bool checkEResultType();
};

class FuncDefNode : public SyntaxNode {
private:
    IdentifierVisibility visibility;
public:
    std::string toString() const;

    FuncDefNode();
    FuncDefNode(const Token& tk);

    IdentifierVisibility getVisibility() const;

    bool buildNode(const TokenList& tkList, size_t st, size_t& ed);
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
#pragma endregion
