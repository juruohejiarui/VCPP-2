#pragma once
#include "tokenlist.h"

enum class SyntaxNodeType {
    Identifier, ConstValue, String, GenericArea,
    If, Else, While, For, Break, Continue, Return, Block, 
    Expression, Operator, MethodCall,
    VarDef, VarFuncDef, FuncDef, ClsDef, NspDef, Using,
    SourceFile, DefinitionFile,
    Empty, Unknown,
};

struct IdentifierInfo;
struct ClassInfo;

/// @brief structure that stores the result type of an expression
struct ExprResType {
    ClassInfo* cls; // pointer to the class
    std::string clsName; // name of the class (used when cls == nullptr)
    int dimc; // number of dimensions
    
    ValueTypeModifier valueType; // value type modifier
    std::vector<ExprResType> genericParams; // parameters for generic classes in cls

    /**
     * @brief Default constructor for ExprResType.
     * 
     * This constructor initializes an instance of ExprResType.
     */
    ExprResType();
    /**
     * @brief Constructs an ExprResType object.
     * 
     * @param cls Pointer to the ClassInfo object.
     * @param dimc Number of dimensions.
     * @param valueType Value type modifier.
     * @param isConst Flag indicating if the object is const.
     */
    ExprResType(ClassInfo* cls, int dimc = 0, ValueTypeModifier valueType = ValueTypeModifier::t, bool isConst = false);

    /**
     * @brief Converts the expression result type to the parent type.
     * 
     * This function converts the expression result type to the type of its parent.
     * 
     * @return The converted expression result type.
     */
    ExprResType convertToParent() const;

    /**
     * Converts the object to a string representation.
     *
     * @return The string representation of the object.
     */
    std::string toString() const ;

    /**
     * Checks if the result type is generic , e.g a generic variable, a generic array, etc.
     *
     * @return true if the object is generic, false otherwise.
     */
    bool isGeneric() const;

    /**
     * @brief Checks if the object is a reference.
     * 
     * @return true if the object is a reference, false otherwise.
     */
    bool isRef() const;

    /**
     * @brief Checks if the current expression result is equal to another expression result.
     * 
     * @param other The other expression result to compare with.
     * @return true if the expression results are equal, false otherwise.
     */
    bool equalTo(const ExprResType &other) const;
};

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

#pragma region Syntax Node
/**
 * @class SyntaxNode
 * @brief Represents a node in the syntax tree.
 * 
 * The SyntaxNode class provides a base class for all nodes in the syntax tree.
 * It contains information about the node's type, associated token, and child nodes.
 * Derived classes must implement the checkExprResType() function.
 */
class SyntaxNode {
protected:
    uint32 varCount;
    SyntaxNodeType type; /**< The type of the syntax node. */
    Token token; /**< The associated token. */

    std::vector<SyntaxNode*> children; /**< The child nodes of the syntax node. */
    SyntaxNode *parent; /**< The parent node of the syntax node. */
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

    ~SyntaxNode();

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
     * @brief Gets the parent node of the syntax node.
     * @return The parent node of the syntax node.
     */
    SyntaxNode *getParent() const;
    /**
     * @brief Sets the parent node of the syntax node.
     * @param parent The parent node to set.
     */
    void setParent(SyntaxNode *parent);

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
    virtual bool checkExprResType() = 0;
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
    ExprResType resultType; /**< The result type of the expression. */
public:
    /**
     * @brief Converts the expression node to a string representation.
     * @return The string representation of the expression node.
     */
    virtual std::string toString() const override;

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
    const ExprResType &getResultType() const;

    /**
     * @brief Builds the expression node from a list of tokens within the specified range.
     * @param tkList The list of tokens.
     * @param st The start index of the range.
     * @param ed The end index of the range (updated by the method).
     * @return True if the node was successfully built, false otherwise.
     */
    bool buildNode(const TokenList &tkList, size_t st, size_t &ed) override;

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
    virtual bool checkExprResType() override;
};

struct GenericAreaNode;
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
    int dimc;
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
     * @brief Represents a node of identifier.
     *
     * This class represents a node of identifier. It stores the token and name associated with the identifier.
     *
     * @param tk The token associated with the identifier.
     * @param name The name of the identifier.
     */
    IdentifierNode(const Token &tk, const std::string &name);


    /**
     * @brief Converts the IdentifierNode to a string representation.
     * @return The string representation of the IdentifierNode.
     */
    std::string toString() const override;
    
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
     * @brief Get the dimension count.
     * 
     * @return int The dimension count.
     */
    int getDimc() const;
    /**
     * @brief Retrieves the GenericAreaNode associated with this object.
     * 
     * @return A pointer to the GenericAreaNode.
     */
    GenericAreaNode *getGenericArea() const;

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
    bool buildNode(const TokenList &tkList, size_t st, size_t &ed) override;

    /**
     * @brief Checks if the expression result type is valid.
     * @return True if the expression result type is valid, false otherwise.
     */
    virtual bool checkExprResType() override;
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
     * @brief Represents a method call node.
     *
     * This class represents a node in the syntax tree that represents a method call.
     * It stores the token and name of the method being called.
     *
     * @param tk The token associated with the method call.
     * @param name The name of the method being called.
     */
    MethodCallNode(const Token &tk, const std::string &name);
    /**
     * @brief Checks the expected result type of the method call.
     * @return True if the expected result type is valid, false otherwise.
     */
    bool checkExprResType() override;

    /**
     * @brief Converts the MethodCallNode to a string representation.
     * @return The string representation of the MethodCallNode.
     */
    std::string toString() const override;
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
    std::string toString() const override;

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

    bool buildNode(const TokenList &tkList, size_t st, size_t &ed) override;

    /**
     * @brief Checks if the operator node has a valid expression result type.
     * 
     * @return True if the expression result type is valid, false otherwise.
     */
    bool checkExprResType() override;
};

class ConstValueNode : public ExpressionNode {
public:
    /**
     * @brief Returns a string representation of the ConstValueNode.
     * 
     * @return std::string The string representation of the ConstValueNode.
     */
    std::string toString() const override;
    
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
    bool checkExprResType() override;
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
    std::string toString() const override;

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
     * @brief Get the parameter at the specified index.
     * 
     * @param index The index of the parameter to retrieve.
     * @return Pointer to the IdentifierNode representing the parameter.
     */
    IdentifierNode *getParam(size_t index) const;
    
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
    bool checkExprResType() override;
};

/**
 * @brief Represents a node in the syntax tree that represents a condition.
 * 
 * This class inherits from SyntaxNode and provides methods to manipulate and access the condition node.
 */
class ConditionNode : public SyntaxNode {
public:
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
    bool buildNode(const TokenList &tkList, size_t st, size_t &ed) override;

    /**
     * @brief Checks the expression result type of the ConditionNode object.
     * @return True if the expression result type is valid, false otherwise.
     */
    bool checkExprResType() override;
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
    bool buildNode(const TokenList &tkList, size_t st, size_t &ed) override;

    /**
     * @brief Checks the result type of the expression in the while loop.
     * @return True if the result type is valid, false otherwise.
     */
    bool checkExprResType() override;
};

/**
 * @brief Represents a For loop node in the syntax tree.
 */
class ForNode : public SyntaxNode {
public:
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
    bool buildNode(const TokenList &tkList, size_t st, size_t &ed) override;

    /**
     * @brief Checks the expression result type of the ForNode.
     * @return True if the expression result type is valid, false otherwise.
     */
    bool checkExprResType() override;
};

/**
 * @brief Represents a control node in the syntax tree.
 * 
 * This class inherits from SyntaxNode and provides additional functionality
 * specific to control nodes.
 */
class ControlNode : public SyntaxNode {
public:
    /**
     * @brief Converts the control node to a string representation.
     * 
     * @return The string representation of the control node.
     */
    std::string toString() const override;

    /**
     * @brief Constructs a control node with the specified type.
     * 
     * @param type The type of the control node.
     */
    ControlNode(SyntaxNodeType type);

    /**
     * @brief Constructs a control node with the specified token.
     * 
     * @param tk The token to initialize the control node with.
     */
    ControlNode(const Token &tk);

    /**
     * @brief Builds the control node from a list of tokens.
     * 
     * @param tkList The list of tokens.
     * @param st The starting index in the token list.
     * @param ed The ending index in the token list (updated by the function).
     * @return True if the control node was successfully built, false otherwise.
     */
    bool buildNode(const TokenList &tkList, size_t st, size_t &ed) override;

    /**
     * @brief Checks the expected result type of the control node.
     * 
     * @return True if the expected result type is valid, false otherwise.
     */
    bool checkExprResType() override;
};

class BlockNode : public SyntaxNode {
public:
    /**
     * @brief Converts the BlockNode object to a string representation.
     * @return The string representation of the BlockNode object.
     */
    std::string toString() const override;

    /**
     * @brief Default constructor for BlockNode.
     */
    BlockNode();

    /**
     * @brief Constructor for BlockNode with a specified token.
     * @param tk The token to initialize the BlockNode with.
     */
    BlockNode(const Token &tk);

    /**
     * @brief Builds the BlockNode from a list of tokens.
     * @param tkList The list of tokens.
     * @param st The starting index in the token list.
     * @param ed The ending index in the token list (updated by the function).
     * @return True if the BlockNode was successfully built, false otherwise.
     */
    bool buildNode(const TokenList &tkList, size_t st, size_t &ed) override;

    /**
     * @brief Checks the expected result type of the BlockNode.
     * @return True if the expected result type is valid, false otherwise.
     */
    bool checkExprResType() override;
};

class VarDefNode : public SyntaxNode {
private:
    IdentifierVisibility visibility;
public:
    /**
     * @brief Default constructor for VarDefNode.
     */
    VarDefNode();

    /**
     * @brief Constructor for VarDefNode with a token.
     * @param tk The token to initialize the VarDefNode with.
     */
    VarDefNode(const Token &tk);
    
    /**
     * @brief Get the visibility of the VarDefNode.
     * @return The visibility of the VarDefNode.
     */
    IdentifierVisibility getVisibility() const;

    /**
     * @brief Build the VarDefNode from a list of tokens.
     * @param tkList The list of tokens to build the VarDefNode from.
     * @param st The starting index of the tokens to build the VarDefNode from.
     * @param ed The ending index of the tokens to build the VarDefNode from (updated by the function).
     * @return True if the VarDefNode was successfully built, false otherwise.
     */
    bool buildNode(const TokenList &tkList, size_t st, size_t &ed) override;

    /**
     * @brief Check the expression result type of the VarDefNode.
     * @return True if the expression result type is valid, false otherwise.
     */
    bool checkExprResType() override;

    /**
     * @brief Convert the VarDefNode to a string representation.
     * @return The string representation of the VarDefNode.
     */
    std::string toString() const override;
};

/**
 * @class FuncDefNode
 * @brief Represents a syntax node for a function definition.
 * 
 * This class inherits from SyntaxNode and provides functionality to represent a function definition in the syntax tree.
 */
class FuncDefNode : public SyntaxNode {
private:
    std::string name; /**< The name of the function. */
    IdentifierVisibility visibility; /**< The visibility of the function. */
public:
    /**
     * @brief Converts the FuncDefNode to a string representation.
     * @return The string representation of the FuncDefNode.
     */
    std::string toString() const override;

    /**
     * @brief Default constructor for FuncDefNode.
     */
    FuncDefNode();

    /**
     * @brief Constructor for FuncDefNode.
     * @param tk The token representing the function definition.
     */
    FuncDefNode(const Token& tk);

    /**
     * @brief Gets the visibility of the function.
     * @return The visibility of the function.
     */
    IdentifierVisibility getVisibility() const;

    /**
     * @brief Gets the name of the function.
     * @return The name of the function.
     */
    std::string getName() const;

    /**
     * @brief Builds the FuncDefNode from a list of tokens.
     * @param tkList The list of tokens.
     * @param st The starting index in the token list.
     * @param ed The ending index in the token list.
     * @return True if the node was successfully built, false otherwise.
     */
    bool buildNode(const TokenList& tkList, size_t st, size_t& ed) override;

    /**
     * @brief Checks the expected result type of the function.
     * @return True if the expected result type is valid, false otherwise.
     */
    bool checkExprResType() override;
};

class ClsDefNode : public SyntaxNode {
private:
    std::string name;
    IdentifierVisibility visibility;
public:
    std::string toString() const override;

    ClsDefNode();
    ClsDefNode(const Token& tk);

    std::string getName() const;
    IdentifierVisibility getVisibility() const;

    bool buildNode(const TokenList& tkList, size_t st, size_t& ed) override;

    bool checkExprResType() override;
};
class NspDefNode : public SyntaxNode {
private:
    std::string name;
public:
    std::string toString() const override;

    NspDefNode();
    NspDefNode(const Token& tk);

    std::string getName() const;

    bool buildNode(const TokenList& tkList, size_t st, size_t& ed) override;
    bool checkExprResType() override;
};

class UsingNode : public SyntaxNode {
private:
    std::string path;
public:
    /**
     * @brief Returns a string representation of the UsingNode.
     * 
     * @return std::string The string representation of the UsingNode.
     */
    std::string toString() const override;

    /**
     * @brief Default constructor for UsingNode.
     */
    UsingNode();

    /**
     * @brief Constructor for UsingNode with a given token.
     * 
     * @param tk The token to initialize the UsingNode with.
     */
    UsingNode(const Token &tk);

    std::string getPath() const;
    void setPath(const std::string &path);

    /**
     * @brief Builds the UsingNode from a list of tokens.
     * 
     * @param tkList The list of tokens.
     * @param st The starting index in the token list.
     * @param ed The ending index in the token list (updated by the function).
     * @return bool True if the UsingNode was successfully built, false otherwise.
     */
    bool buildNode(const TokenList &tkList, size_t st, size_t &ed) override;
};
#pragma endregion

#pragma region Identifier Info
/// @brief The type of identifier.
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

/// @brief The information of an identifier
struct IdentifierInfo {
    IdentifierType type;
    std::string name, fullName;
    ExprResType resType;
    IdentifierVisibility visibility;
    SyntaxNode *defNode;

    IdentifierInfo();
    IdentifierInfo(SyntaxNode *defNode);

    virtual ~IdentifierInfo();

    virtual std::string toString() const = 0;
};

/// @brief The information of a variable
struct VariableInfo : public IdentifierInfo {
    uint64 offset;
    VariableInfo();
    VariableInfo(IdentifierNode *nameNode, IdentifierNode *typeNode);

    ~VariableInfo();

    std::string toString() const override;
};

/// @brief The information of a function (or a variable function)
struct FunctionInfo : public IdentifierInfo {
    uint64 offset;
    std::vector<VariableInfo *> params;
    std::vector<ClassInfo *> genericClasses;
    FunctionInfo(bool isVarFunction = false);
    FunctionInfo(FuncDefNode *defNode, bool isVarFunction = false);

    ~FunctionInfo();

    std::string toString() const override;
};

/// @brief The information of a class (or a generic identifier)
struct ClassInfo : public IdentifierInfo {
    std::map<std::string, VariableInfo *> varMap;
    std::map<std::string, std::vector<FunctionInfo *> > funcMap;
    std::vector<TokenList> requiredOperators;

    ClassInfo *parent;
    // the generic class of this class
    std::vector<ClassInfo *> genericClasses;
    // the generic type for each generic class in genericClasses in parent
    std::vector<ExprResType> genericParams;

    bool isGenericIdentifier;

    ClassInfo();
    ClassInfo(ClsDefNode *defNode);
    ClassInfo(const std::string &name, const std::string &fullName);

    ~ClassInfo();

    std::string toString() const override;
};

/// @brief The information of a namespace
struct NamespaceInfo : public IdentifierInfo {
    std::map<std::string, VariableInfo *> varMap;
    std::map<std::string, std::vector<FunctionInfo *> > funcMap;
    std::map<std::string, ClassInfo *> clsMap;
    std::map<std::string, NamespaceInfo *> nspMap;

    NamespaceInfo();
    NamespaceInfo(NspDefNode *defNode);

    ~NamespaceInfo();

    std::string toString() const;
};
#pragma endregion

/**
 * Builds the type system using the given syntax tree roots.
 *
 * @param roots The vector of syntax tree roots.
 * @return True if the type system was successfully built, false otherwise.
 */
bool buildTypeSystem(const std::vector<SyntaxNode *> &roots);

/**
 * @brief Checks the result type of an expression.
 * 
 * This function takes a vector of SyntaxNode pointers representing the roots of an expression tree
 * and checks the result type of the expression. It returns true if the result type is valid, and false otherwise.
 * 
 * @param roots The vector of SyntaxNode pointers representing the roots of the expression tree.
 * @return true if the result type is valid, false otherwise.
 */
bool CheckExprResType(const std::vector<SyntaxNode *> &roots);