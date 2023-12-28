#include "syntaxnode.h"

const uint32 IdentifierWeight = 1000000;
#pragma region Definition of methods in syntaxnode 

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

size_t SyntaxNode::getChildrenCount() const { return children.size(); }
#pragma endregion

#pragma region ExpressionNode
ExpressionNode::ExpressionNode() : SyntaxNode(SyntaxNodeType::Expression) { }
ExpressionNode::ExpressionNode(const Token &token) : SyntaxNode(SyntaxNodeType::Expression, token) { }

ExpressionNode *ExpressionNode::getContent() const { return (ExpressionNode *)at(0); }
uint32 ExpressionNode::getWeight() const { return IdentifierWeight; }
#pragma endregion

#pragma region OperatorNode
OperatorNode::OperatorNode() : ExpressionNode() { type = SyntaxNodeType::Operator; }
OperatorNode::OperatorNode(const Token &token) : ExpressionNode(token) { type = SyntaxNodeType::Operator; }

ExpressionNode *&OperatorNode::getLeft() { return (ExpressionNode *&)at(0); }
ExpressionNode *OperatorNode::getLeft() const { return (ExpressionNode *)at(0); }
ExpressionNode *&OperatorNode::getRight() { return (ExpressionNode *&)at(1); }
ExpressionNode *OperatorNode::getRight() const { return (ExpressionNode *)at(1); }

uint32 OperatorNode::getWeight() const { return getOperWeight(token.type); }
#pragma endregion

#pragma region IdentifierNode
IdentifierNode::IdentifierNode() : ExpressionNode() { type = SyntaxNodeType::Identifier, children.resize(1, nullptr); }
IdentifierNode::IdentifierNode(const Token &token) : ExpressionNode() { type = SyntaxNodeType::Identifier, children.resize(1, nullptr); }

GenericAreaNode *IdentifierNode::getGenericArea() const { return (GenericAreaNode *)at(0); }

uint32 IdentifierNode::getWeight() const { return IdentifierWeight; }
const std::string &IdentifierNode::getName() const { return name; }
void IdentifierNode::setName(const std::string &name) { this->name = name; }
size_t IdentifierNode::getParamCount() const { return getChildrenCount() - 1; }
ExpressionNode *IdentifierNode::getParam(size_t index) const { return (ExpressionNode *)at(index + 1); }

bool IdentifierNode::isFuncCall() const { return getChildrenCount() > 1; }
#pragma endregion

#pragma region GenericAreaNode
GenericAreaNode::GenericAreaNode() : SyntaxNode(SyntaxNodeType::GenericArea) {}
GenericAreaNode::GenericAreaNode(const Token &token) : SyntaxNode(SyntaxNodeType::GenericArea, token) { }

size_t GenericAreaNode::getParamCount() const { return children.size(); }

IdentifierNode *GenericAreaNode::getParam(size_t index) const { return (IdentifierNode *)at(index); }
#pragma endregion

#pragma region ConstValueNode
ConstValueNode::ConstValueNode() : ExpressionNode() { type = SyntaxNodeType::ConstValue; }
ConstValueNode::ConstValueNode(const Token &token) : ExpressionNode(token) { type = SyntaxNodeType::ConstValue; }

uint32 ConstValueNode::getWeight() const { return IdentifierWeight; }
#pragma endregion

#pragma region BlockNode
BlockNode::BlockNode() : SyntaxNode(SyntaxNodeType::Block) { }
BlockNode::BlockNode(const Token &token) : SyntaxNode(SyntaxNodeType::Block, token) { }

uint32 BlockNode::getLocalVarCount() const { return localVarCount; }
void BlockNode::setLocalVarCount(uint32 data) { localVarCount = data; }
#pragma endregion

#pragma region IfNode
IfNode::IfNode() : BlockNode() { type = SyntaxNodeType::If, children.resize(3, nullptr); }
IfNode::IfNode(const Token &token) : BlockNode(token) { type = SyntaxNodeType::If, children.resize(3, nullptr); }

ExpressionNode *IfNode::getCondNode() const { return (ExpressionNode *)at(0); }
void IfNode::setCondNode(ExpressionNode *node) { at(0) = node; }
SyntaxNode *IfNode::getSuccNode() const { return at(1); }
void IfNode::setSuccNode(SyntaxNode *node) { at(1) = node; }
SyntaxNode *IfNode::getFailNode() const { return at(2); }
void IfNode::setFailNode(SyntaxNode *node) { at(2) = node; }
#pragma endregion

#pragma region LoopNode
LoopNode::LoopNode(SyntaxNodeType type) : BlockNode() { this->type = type, children.resize(4, nullptr); }
LoopNode::LoopNode(SyntaxNodeType type, const Token &token) : BlockNode(token) { this->type = type, children.resize(4, nullptr); }

SyntaxNode *LoopNode::getInitNode() const { return at(0); }
void LoopNode::setInitNode(SyntaxNode *node) { at(0) = node; }
ExpressionNode *LoopNode::getCondNode() const { return (ExpressionNode *)at(1); }
void LoopNode::setCondNode(ExpressionNode *node) { at(1) = node; }
ExpressionNode *LoopNode::getStepNode() const { return (ExpressionNode *)at(2); }
void LoopNode::setStepNode(ExpressionNode *node) { at(2) = node; }
SyntaxNode *LoopNode::getContent() const { return at(3); }
void LoopNode::setContent(SyntaxNode *node) { at(3) = node; }
#pragma endregion

#pragma region ControlNode
ControlNode::ControlNode(SyntaxNodeType type) : SyntaxNode(type) {
    if (type == SyntaxNodeType::Return) children.resize(1, nullptr);
}
ControlNode::ControlNode(SyntaxNodeType type, const Token &token) : SyntaxNode(type, token) {
    if (type == SyntaxNodeType::Return) children.resize(1, nullptr);
}
ExpressionNode *ControlNode::getContent() const {
    if (type != SyntaxNodeType::Return) return nullptr;
    return (ExpressionNode *)at(0);
}
#pragma endregion

#pragma region VarDefNode
VarDefNode::VarDefNode() : BlockNode() { type = SyntaxNodeType::VarDef; }
VarDefNode::VarDefNode(const Token &token) : BlockNode(token) { type = SyntaxNodeType::VarDef; }

IdentifierVisibility VarDefNode::getVisibility() const { return visibility; }
void VarDefNode::setVisibility(IdentifierVisibility visibility) { this->visibility = visibility; }

std::tuple<IdentifierNode *, IdentifierNode *, ExpressionNode *> VarDefNode::getVariable(size_t index) const {
    return std::make_tuple((IdentifierNode *)at(index * 3), (IdentifierNode *)at(index * 3 + 1), (ExpressionNode *)at(index * 3 + 2));
}
#pragma endregion

#pragma region FuncDef
FuncDefNode::FuncDefNode() : BlockNode() { type = SyntaxNodeType::FuncDef; }
FuncDefNode::FuncDefNode(const Token &token) : BlockNode(token) { type = SyntaxNodeType::FuncDef; }

IdentifierVisibility FuncDefNode::getVisibility() const { return visibility; }
void FuncDefNode::setVisibility(IdentifierVisibility visibility) { this->visibility = visibility; }

IdentifierNode *FuncDefNode::getNameNode() const { return (IdentifierNode *)at(0); }
size_t FuncDefNode::getParamCount() const { return (getChildrenCount() - 3) / 2; }
std::pair<IdentifierNode *, IdentifierNode *> FuncDefNode::getParam(size_t index) const {
    return std::make_pair((IdentifierNode *)at(index * 2 + 1), (IdentifierNode *)at(index * 2 + 2));
}
IdentifierNode *FuncDefNode::getResNode() const { return (IdentifierNode *)at(getChildrenCount() - 2); }
SyntaxNode *FuncDefNode::getContent() const { return at(getChildrenCount()); }
#pragma endregion

#pragma region VarFuncDefNode
VarFuncDefNode::VarFuncDefNode() : FuncDefNode() { type = SyntaxNodeType::VarFuncDef; }
VarFuncDefNode::VarFuncDefNode(const Token &token) : FuncDefNode(token) { type = SyntaxNodeType::VarFuncDef; }
#pragma endregion

#pragma region ClsDefNode
ClsDefNode::ClsDefNode() : SyntaxNode(SyntaxNodeType::ClsDef) { children.resize(2, nullptr); }
ClsDefNode::ClsDefNode(const Token &token) : SyntaxNode(SyntaxNodeType::ClsDef, token) { children.resize(2, nullptr); }

IdentifierVisibility ClsDefNode::getVisibility() const { return visibility; }
void ClsDefNode::setVisibility(IdentifierVisibility visibility) { this->visibility = visibility; }

size_t ClsDefNode::getFieldCount() const { return fieldIndex.size(); }
size_t ClsDefNode::getFuncCount() const { return funcIndex.size(); }
size_t ClsDefNode::getVarFuncCount() const { return varFuncIndex.size(); }
VarDefNode *ClsDefNode::getFieldDef(size_t index) const { return (VarDefNode *)at(fieldIndex[index]); }
FuncDefNode *ClsDefNode::getFuncDef(size_t index) const { return (FuncDefNode *)at(funcIndex[index]); }
VarFuncDefNode *ClsDefNode::getVarFuncDef(size_t index) const { return (VarFuncDefNode *)at(varFuncIndex[index]); }

IdentifierNode *ClsDefNode::getNameNode() const { return (IdentifierNode *)at(0); }
void ClsDefNode::setNameNode(IdentifierNode *node) { at(0) = node; }
IdentifierNode *ClsDefNode::getBaseNode() const { return (IdentifierNode *)at(1); }
void ClsDefNode::setBaseNode(IdentifierNode *node) { at(1) = node; }

void ClsDefNode::addChild(SyntaxNode *node) {
    switch (node->getType()) {
        case SyntaxNodeType::VarDef:
            fieldIndex.push_back(getChildrenCount());
            break;
        case SyntaxNodeType::FuncDef:
            funcIndex.push_back(getChildrenCount());
            break;
        case SyntaxNodeType::VarFuncDef:
            varFuncIndex.push_back(getChildrenCount());
            break;
    }
    SyntaxNode::addChild(node);
}
#pragma endregion

#pragma region NspDefNode
NspDefNode::NspDefNode() : SyntaxNode(SyntaxNodeType::NspDef) { children.resize(1, nullptr); }
NspDefNode::NspDefNode(const Token &token) : SyntaxNode(SyntaxNodeType::NspDef, token) { children.resize(1, nullptr); }

size_t NspDefNode::getVarCount() const { return varIndex.size(); }
size_t NspDefNode::getFuncCount() const { return funcIndex.size(); }
size_t NspDefNode::getClsCount() const { return clsIndex.size(); }
VarDefNode *NspDefNode::getVarDef(size_t index) const { return (VarDefNode *)at(varIndex[index]); }
FuncDefNode *NspDefNode::getFuncDef(size_t index) const { return (FuncDefNode *)at(funcIndex[index]); }
ClsDefNode *NspDefNode::getClsDef(size_t index) const { return (ClsDefNode *)at(clsIndex[index]); }

IdentifierNode *NspDefNode::getNameNode() const { return (IdentifierNode *)at(0); }
void NspDefNode::setNameNode(IdentifierNode *node) { at(0) = node; }

void NspDefNode::addChild(SyntaxNode *node) {
    switch (node->getType()) {
        case SyntaxNodeType::VarDef:
            varIndex.push_back(getChildrenCount());
            break;
        case SyntaxNodeType::FuncDef:
            funcIndex.push_back(getChildrenCount());
            break;
        case SyntaxNodeType::ClsDef:
            clsIndex.push_back(getChildrenCount());
            break;
    }
    SyntaxNode::addChild(node);
}
#pragma endregion

#pragma endregion
