#include "syntaxnode.h"

#pragma region SyntaxNode
SyntaxNode::SyntaxNode(SyntaxNodeType type) { this->type = type, localVarCount = 0; }
SyntaxNode::SyntaxNode(SyntaxNodeType type, const Token &token) : type(type), token(token) { localVarCount = 0; }

SyntaxNode::~SyntaxNode() { for (auto child : children) if (child != nullptr) delete child; }

Token &SyntaxNode::getToken() { return token; }
const Token &SyntaxNode::getToken() const { return token; }
void SyntaxNode::setToken(const Token &token) { this->token = token; }

SyntaxNodeType SyntaxNode::getType() const { return type; }

SyntaxNode *&SyntaxNode::operator[](int index) { return children[index]; }
SyntaxNode *SyntaxNode::operator[](int index) const { return children[index]; }
SyntaxNode *&SyntaxNode::at(int index) { return children[index]; }
SyntaxNode *SyntaxNode::at(int index) const { return children[index]; }

void SyntaxNode::addChild(SyntaxNode *child) { children.push_back(child); }
#pragma endregion

ExpressionNode::ExpressionNode() : SyntaxNode(SyntaxNodeType::Expression) { }
ExpressionNode::ExpressionNode(const Token &token) : SyntaxNode(SyntaxNodeType::Expression, token) { }

ExpressionNode *ExpressionNode::getContent() const { return (ExpressionNode *)at(0); }
uint32 ExpressionNode::getWeight() const { return 100000000; }

OperatorNode::OperatorNode() : ExpressionNode() { type = SyntaxNodeType::Operator; }
