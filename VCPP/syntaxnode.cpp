#include "syntaxnode.h"
#include "constexprcalc.h"

const uint32 IdentifierWeight = 1000000;
#pragma region Definition of methods in syntaxnode 

const std::string syntaxNodeTypeString[] = {
    "Expression", "Identifier", "ConstValue", "Operator", "GenericArea",
    "Block", "If", "While", "Switch","Case", "For", "Continue", "Break", "Return",
    "VarDef", "FuncDef", "VarFuncDef", "ClsDef", "NspDef",
    "Using",
    "SourceRoot", "SymbolRoot",
    "Error", "Empty", "Unknown", 
};
const int32 syntaxNodeTypeNumber = 23;

#pragma region SyntaxNode
SyntaxNode::SyntaxNode(SyntaxNodeType type) { parent = nullptr, this->type = type, localVarCount = 0; }
SyntaxNode::SyntaxNode(SyntaxNodeType type, const Token &token) : type(type), token(token) { parent = nullptr, localVarCount = 0; }

SyntaxNode::~SyntaxNode() {
    if (parent != nullptr) parent->removeChild(this);
    for (auto child : children) if (child != nullptr) delete child;
}

Token &SyntaxNode::getToken() { return token; }
const Token &SyntaxNode::getToken() const { return token; }
void SyntaxNode::setToken(const Token &token) { this->token = token; }

SyntaxNodeType SyntaxNode::getType() const { return type; }

SyntaxNode *&SyntaxNode::operator[](int index) { return children[index]; }
SyntaxNode *SyntaxNode::operator[](int index) const { return children[index]; }
SyntaxNode *&SyntaxNode::get(int index) { return children[index]; }
SyntaxNode *SyntaxNode::get(int index) const { return children[index]; }

void SyntaxNode::addChild(SyntaxNode *child) { 
    if (child != nullptr) child->parent = this;
    children.push_back(child);
}

void SyntaxNode::clearChildren() { 
    for (auto child : children) if (child != nullptr) child->parent = nullptr;
    children.clear(); 
}
void SyntaxNode::removeChild(size_t index) {
    if (children[index] != nullptr) children[index]->parent = nullptr;
    children.erase(children.begin() + index); 
}
void SyntaxNode::removeChild(SyntaxNode *child) { 
    auto iter = std::find(children.begin(), children.end(), child);
    if (iter == children.end()) return ;
    (*iter)->parent = nullptr;
    children.erase(iter); 
}
void SyntaxNode::insertChild(size_t index, SyntaxNode *child) { 
    child->parent = this;
    children.insert(children.begin() + index, child); 
}
void SyntaxNode::replaceChild(size_t index, SyntaxNode *child) { 
    if (children[index] != nullptr) children[index]->parent = nullptr;
    child->parent = this, children[index] = child; 
}
void SyntaxNode::replaceChild(SyntaxNode *oldChild, SyntaxNode *newChild) { 
    auto iter = std::find(children.begin(), children.end(), oldChild);
    if (iter != children.end()) {
        (*iter)->parent = nullptr;
        newChild->parent = this, (*iter) = newChild;
    }
}

SyntaxNode *SyntaxNode::getParent() const { return parent; }

size_t SyntaxNode::getChildrenCount() const { return children.size(); }
std::string SyntaxNode::toString() const { return syntaxNodeTypeString[(int)type]; }

uint32 SyntaxNode::getLocalVarCount() const { return localVarCount; }
#pragma endregion

#pragma region ExpressionNode
ExpressionNode::ExpressionNode() : SyntaxNode(SyntaxNodeType::Expression) { }
ExpressionNode::ExpressionNode(const Token &token) : SyntaxNode(SyntaxNodeType::Expression, token) { }

ExpressionNode *ExpressionNode::getContent() const { return (ExpressionNode *)get(0); }
uint32 ExpressionNode::getWeight() const { return IdentifierWeight; }
#pragma endregion

#pragma region OperatorNode
OperatorNode::OperatorNode() : ExpressionNode() { type = SyntaxNodeType::Operator; }
OperatorNode::OperatorNode(const Token &token) : ExpressionNode(token) { type = SyntaxNodeType::Operator; }

ExpressionNode *&OperatorNode::getLeft() { return (ExpressionNode *&)get(0); }
ExpressionNode *OperatorNode::getLeft() const { return (ExpressionNode *)get(0); }
ExpressionNode *&OperatorNode::getRight() { return (ExpressionNode *&)get(1); }
ExpressionNode *OperatorNode::getRight() const { return (ExpressionNode *)get(1); }

uint32 OperatorNode::getWeight() const { return getOperWeight(token.type); }
std::string OperatorNode::toString() const {
    return syntaxNodeTypeString[(int)type] + " " + tokenTypeString[(int)token.type] + " weight = " + std::to_string(getWeight());
}
#pragma endregion

#pragma region IdentifierNode
IdentifierNode::IdentifierNode() : ExpressionNode() {
    type = SyntaxNodeType::Identifier, children.resize(2, nullptr); 
    get(1) = new ConstValueNode();
    get(1)->getToken().type = TokenType::ConstData;
    get(1)->getToken().data.type = DataTypeModifier::u32;
    get(1)->getToken().data.uint32_v() = 0u;
}
IdentifierNode::IdentifierNode(const Token &token) : ExpressionNode() {
    type = SyntaxNodeType::Identifier, children.resize(2, nullptr); 
    this->token.lineId = token.lineId;
    get(1) = new ConstValueNode();
    get(1)->getToken().type = TokenType::ConstData;
    get(1)->getToken().data.type = DataTypeModifier::u32;
    get(1)->getToken().data.uint32_v() = 0u;
}

GenericAreaNode *IdentifierNode::getGenericArea() const { return (GenericAreaNode *)get(0); }
void IdentifierNode::setGenericArea(GenericAreaNode *node) { get(0) = node; }

uint32 IdentifierNode::getDimc() const { return get(1)->getToken().data.uint32_v(); }
void IdentifierNode::setDimc(uint32 dimc) { get(1)->getToken().data.uint32_v() = dimc; }

uint32 IdentifierNode::getWeight() const { return IdentifierWeight; }
const std::string &IdentifierNode::getName() const { return name; }
void IdentifierNode::setName(const std::string &name) { this->name = name; }
size_t IdentifierNode::getParamCount() const { return getChildrenCount() - 2; }
ExpressionNode *IdentifierNode::getParam(size_t index) const { return (ExpressionNode *)get(index + 2); }

bool IdentifierNode::isFuncCall() const { return getChildrenCount() > 2; }
std::string IdentifierNode::toString() const { return syntaxNodeTypeString[(int)type] + " " + name; }
#pragma endregion

#pragma region GenericAreaNode
GenericAreaNode::GenericAreaNode() : SyntaxNode(SyntaxNodeType::GenericArea) {}
GenericAreaNode::GenericAreaNode(const Token &token) : SyntaxNode(SyntaxNodeType::GenericArea, token) { }

size_t GenericAreaNode::getParamCount() const { return children.size(); }

IdentifierNode *GenericAreaNode::getParam(size_t index) const { return (IdentifierNode *)get(index); }
#pragma endregion

#pragma region ConstValueNode
ConstValueNode::ConstValueNode() : ExpressionNode() { type = SyntaxNodeType::ConstValue; }
ConstValueNode::ConstValueNode(const Token &token) : ExpressionNode(token) { type = SyntaxNodeType::ConstValue; }

uint32 ConstValueNode::getWeight() const { return IdentifierWeight; }
std::string ConstValueNode::toString() const { return syntaxNodeTypeString[(int)type] + " " + token.toString(); }
#pragma endregion

#pragma region BlockNode
BlockNode::BlockNode() : SyntaxNode(SyntaxNodeType::Block) { }
BlockNode::BlockNode(const Token &token) : SyntaxNode(SyntaxNodeType::Block, token) { }

void BlockNode::setLocalVarCount(uint32 data) { localVarCount = data; }
std::string BlockNode::toString() const { return syntaxNodeTypeString[(int)type] + " varCount = " + std::to_string(getLocalVarCount()); }
#pragma endregion

#pragma region IfNode
IfNode::IfNode() : BlockNode() { type = SyntaxNodeType::If, children.resize(3, nullptr); }
IfNode::IfNode(const Token &token) : BlockNode(token) { type = SyntaxNodeType::If, children.resize(3, nullptr); }

ExpressionNode *IfNode::getCondNode() const { return (ExpressionNode *)get(0); }
void IfNode::setCondNode(ExpressionNode *node) { get(0) = node; }
SyntaxNode *IfNode::getSuccNode() const { return get(1); }
void IfNode::setSuccNode(SyntaxNode *node) { get(1) = node; }
SyntaxNode *IfNode::getFailNode() const { return get(2); }
void IfNode::setFailNode(SyntaxNode *node) { get(2) = node; }
#pragma endregion

#pragma region LoopNode
LoopNode::LoopNode(SyntaxNodeType type) : BlockNode() { this->type = type, children.resize(4, nullptr); }
LoopNode::LoopNode(SyntaxNodeType type, const Token &token) : BlockNode(token) { this->type = type, children.resize(4, nullptr); }

SyntaxNode *LoopNode::getInitNode() const { return get(0); }
void LoopNode::setInitNode(SyntaxNode *node) { get(0) = node; }
ExpressionNode *LoopNode::getCondNode() const { return (ExpressionNode *)get(1); }
void LoopNode::setCondNode(ExpressionNode *node) { get(1) = node; }
ExpressionNode *LoopNode::getStepNode() const { return (ExpressionNode *)get(2); }
void LoopNode::setStepNode(ExpressionNode *node) { get(2) = node; }
SyntaxNode *LoopNode::getContent() const { return get(3); }
void LoopNode::setContent(SyntaxNode *node) { get(3) = node; }
#pragma endregion

#pragma region CaseNode and SwitchNode
CaseNode::CaseNode() : SyntaxNode(SyntaxNodeType::Case) {
    children.resize(1, nullptr);
    isDefault = false;
}

CaseNode::CaseNode(const Token &token) : SyntaxNode(SyntaxNodeType::Case, token) {
    children.resize(1, nullptr);
    isDefault = false;
}

bool CaseNode::isDefaultCase() const { return isDefault; }
void CaseNode::setDefaultCase(bool isDefault) { this->isDefault = isDefault; }
ExpressionNode *CaseNode::getCondNode() const { return isDefault ? nullptr : (ExpressionNode *)get(0); }

SwitchNode::SwitchNode() {
    type = SyntaxNodeType::Switch;
}

SwitchNode::SwitchNode(const Token &token) {
    type = SyntaxNodeType::Switch;
    setToken(token);
}

size_t SwitchNode::getCaseCount() const {
    return caseIndex.size();
}

CaseNode *SwitchNode::getCase(size_t index) const {
    return (CaseNode *)children[caseIndex[index]];
}
void SwitchNode::addCaseIndex(size_t index) { caseIndex.push_back(index); }
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
    return (ExpressionNode *)get(0);
}
#pragma endregion

#pragma region VarDefNode
VarDefNode::VarDefNode() : BlockNode() { type = SyntaxNodeType::VarDef, visibility = IdenVisibility::Unknown; }
VarDefNode::VarDefNode(const Token &token) : BlockNode(token) { type = SyntaxNodeType::VarDef, visibility = IdenVisibility::Unknown; }

IdenVisibility VarDefNode::getVisibility() const { return visibility; }
void VarDefNode::setVisibility(IdenVisibility visibility) { this->visibility = visibility; }

std::tuple<IdentifierNode *, IdentifierNode *, ExpressionNode *> VarDefNode::getVariable(size_t index) const {
    return std::make_tuple((IdentifierNode *)get(index * 3), (IdentifierNode *)get(index * 3 + 1), (ExpressionNode *)get(index * 3 + 2));
}
#pragma endregion

#pragma region FuncDef
FuncDefNode::FuncDefNode() : BlockNode() { type = SyntaxNodeType::FuncDef, visibility = IdenVisibility::Unknown; }
FuncDefNode::FuncDefNode(const Token &token) : BlockNode(token) { type = SyntaxNodeType::FuncDef, visibility = IdenVisibility::Unknown; }

IdenVisibility FuncDefNode::getVisibility() const { return visibility; }
void FuncDefNode::setVisibility(IdenVisibility visibility) { this->visibility = visibility; }

IdentifierNode *FuncDefNode::getNameNode() const { return (IdentifierNode *)get(0); }
size_t FuncDefNode::getParamCount() const { return (getChildrenCount() - 3) / 3; }
std::tuple<IdentifierNode *, IdentifierNode *, ConstValueNode *> FuncDefNode::getParam(size_t index) const {
    return std::make_tuple((IdentifierNode *)get(index * 3 + 1), (IdentifierNode *)get(index * 3 + 2), (ConstValueNode *)get(index * 3 + 3));
}
IdentifierNode *FuncDefNode::getResNode() const { return (IdentifierNode *)get(getChildrenCount() - 2); }
SyntaxNode *FuncDefNode::getContent() const { return get(getChildrenCount() - 1); }
#pragma endregion

#pragma region VarFuncDefNode
VarFuncDefNode::VarFuncDefNode() : FuncDefNode() { type = SyntaxNodeType::VarFuncDef; }
VarFuncDefNode::VarFuncDefNode(const Token &token) : FuncDefNode(token) { type = SyntaxNodeType::VarFuncDef; }
#pragma endregion

#pragma region ClsDefNode
ClsDefNode::ClsDefNode() : SyntaxNode(SyntaxNodeType::ClsDef) { children.resize(2, nullptr), visibility = IdenVisibility::Unknown; }
ClsDefNode::ClsDefNode(const Token &token) : SyntaxNode(SyntaxNodeType::ClsDef, token) { children.resize(2, nullptr), visibility = IdenVisibility::Unknown; }

IdenVisibility ClsDefNode::getVisibility() const { return visibility; }
void ClsDefNode::setVisibility(IdenVisibility visibility) { this->visibility = visibility; }

size_t ClsDefNode::getFieldCount() const { return fieldIndex.size(); }
size_t ClsDefNode::getFuncCount() const { return funcIndex.size(); }
size_t ClsDefNode::getVarFuncCount() const { return varFuncIndex.size(); }
VarDefNode *ClsDefNode::getFieldDef(size_t index) const { return (VarDefNode *)get(fieldIndex[index]); }
FuncDefNode *ClsDefNode::getFuncDef(size_t index) const { return (FuncDefNode *)get(funcIndex[index]); }
VarFuncDefNode *ClsDefNode::getVarFuncDef(size_t index) const { return (VarFuncDefNode *)get(varFuncIndex[index]); }

IdentifierNode *ClsDefNode::getNameNode() const { return (IdentifierNode *)get(0); }
void ClsDefNode::setNameNode(IdentifierNode *node) { get(0) = node; }
IdentifierNode *ClsDefNode::getBaseNode() const { return (IdentifierNode *)get(1); }
void ClsDefNode::setBaseNode(IdentifierNode *node) { get(1) = node; }

void ClsDefNode::addChild(SyntaxNode *node) {
    if (node == nullptr) {
        SyntaxNode::addChild(node);
        return ;
    }
    switch (node->getType()) {
        case SyntaxNodeType::VarDef:
            fieldIndex.push_back(getChildrenCount());
            break;
        case SyntaxNodeType::VarFuncDef:
        case SyntaxNodeType::FuncDef:
            funcIndex.push_back(getChildrenCount());
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
VarDefNode *NspDefNode::getVarDef(size_t index) const { return (VarDefNode *)get(varIndex[index]); }
FuncDefNode *NspDefNode::getFuncDef(size_t index) const { return (FuncDefNode *)get(funcIndex[index]); }
ClsDefNode *NspDefNode::getClsDef(size_t index) const { return (ClsDefNode *)get(clsIndex[index]); }

IdentifierNode *NspDefNode::getNameNode() const { return (IdentifierNode *)get(0); }
void NspDefNode::setNameNode(IdentifierNode *node) { get(0) = node; }

void NspDefNode::addChild(SyntaxNode *node) {
    if (node == nullptr) {
        SyntaxNode::addChild(node);
        return ;
    }
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

#pragma region UsingNode
UsingNode::UsingNode() : SyntaxNode(SyntaxNodeType::Using) { }
UsingNode::UsingNode(const Token &token) : SyntaxNode(SyntaxNodeType::Using, token) { }

const std::string UsingNode::getPath() const { return path; }
void UsingNode::setPath(const std::string &path) { this->path = path; }
std::string UsingNode::toString() const { return syntaxNodeTypeString[(int)type] + " path = " + path; }
#pragma endregion

#pragma region RootNode
RootNode::RootNode(SyntaxNodeType type) : SyntaxNode(type) { }

size_t RootNode::getUsingCount() const { return usingIndex.size(); }
size_t RootNode::getDefCount() const { return defIndex.size(); }

UsingNode *RootNode::getUsing(size_t index) const { return (UsingNode *)get(usingIndex[index]); }
SyntaxNode *RootNode::getDef(size_t index) const { return get(defIndex[index]); }

void RootNode::addChild(SyntaxNode *node) {
    if (node == nullptr) return ;
    switch (node->getType()) {
        case SyntaxNodeType::Using:
            usingIndex.push_back(getChildrenCount());
            break;
        case SyntaxNodeType::ClsDef:
        case SyntaxNodeType::VarDef:
        case SyntaxNodeType::FuncDef:
        case SyntaxNodeType::NspDef:
            defIndex.push_back(getChildrenCount());
            break;
    }
    SyntaxNode::addChild(node);
}
#pragma endregion

#pragma endregion

#pragma region build expression

ExpressionNode *buildExpression(const TokenList &tkList, size_t l, size_t r);

IdentifierNode *buildIdentifier(const TokenList &tkList, size_t l, size_t &r) {
    IdentifierNode *node = new IdentifierNode(tkList[l]);
    std::string name = tkList[l].dataStr;
    while (l + 1 < tkList.size() && tkList[l + 1].type == TokenType::GetChild) {
        name.push_back('.');
        name.append(tkList[l + 2].dataStr);
        l += 2;
    }
    r = l;
    node->setName(name);
    // have generic area
    if (l + 1 < tkList.size() && tkList[l + 1].type == TokenType::GBrkL) {
        r = tkList[l + 1].data.uint64_v();
        node->setGenericArea(new GenericAreaNode());
        for (size_t pos = l + 2, rpos = pos; pos < r; pos = ++rpos) {
            IdentifierNode *param = buildIdentifier(tkList, pos, rpos);
            if (param != nullptr && param->isFuncCall()) {
                printError(param->getToken().lineId, "Param(s) in generic area cannot be function call");
                delete node;
                return nullptr;
            }
            if (param != nullptr && param->getDimc()) {
                printError(param->getToken().lineId, "Param(s) in generic area cannot be an array");
                delete node;
                return nullptr;
            }
            node->getGenericArea()->addChild(param);
            if (rpos + 1 != r && tkList[rpos + 1].type != TokenType::Comma) {
                printError(tkList[rpos + 1].lineId, "Invalid syntax in generic area");
                delete node;
                return nullptr;
            }
            rpos++;
        }
    }
    l = r;
    // is a function call
    if (l < tkList.size() && tkList[l + 1].type == TokenType::SBrkL) {
        r = tkList[l + 1].data.uint64_v();
        for (size_t pos = l + 2, rpos = pos; pos < r; pos = ++rpos) {
            // get the range of tokens that represents one param
            while (rpos < r && tkList[rpos].type != TokenType::Comma) {
                if (isBracketL(tkList[rpos].type)) rpos = tkList[rpos].data.uint64_v();
                rpos++; 
            }
            node->addChild(buildExpression(tkList, pos, rpos - 1));
        }
        // add a null pointer to a function call without params
        if (node->getChildrenCount() == 2) node->addChild(nullptr);
    } else if (l < tkList.size() && tkList[l + 1].type == TokenType::MBrkL) {
        uint32 dimc = 0;
        while (tkList[r + 1].type == TokenType::MBrkL && tkList[r + 1].data.uint64_v() == r + 2)
            dimc++, r += 2;
        node->setDimc(dimc);
    }
    return node;
}

ConstValueNode *buildConstValue(const TokenList &tkList, size_t l, size_t &r) {
    r = l;
    return new ConstValueNode(tkList[l]);
}
ExpressionNode *buildExpression(const TokenList &tkList, size_t l, size_t r) {
    if (r < l) return nullptr;
    std::vector<ExpressionNode *> nodes;
    for (size_t pos = l, rpos = l; pos <= r; pos = ++rpos) {
        const auto &fir = tkList[pos];
        ExpressionNode *newNode = nullptr;
        switch (fir.type) {
            case TokenType::Identifier:
                newNode = buildIdentifier(tkList, pos, rpos);
                break;
            case TokenType::String:
            case TokenType::ConstData:
                newNode = buildConstValue(tkList, pos, rpos);
                break;
            case TokenType::SBrkL:
                newNode = buildExpression(tkList, pos + 1, fir.data.uint64_v() - 1);
                rpos = fir.data.uint64_v();
                break;
            case TokenType::MBrkL: {
                auto operNode = new OperatorNode(fir);
                nodes.push_back(operNode);
                newNode = buildExpression(tkList, pos + 1, fir.data.uint64_v() - 1);
                rpos = fir.data.uint64_v();
                break;
            }
            default:
                if (isOperator(fir.type)) newNode = new OperatorNode(fir);
                break;
        }
        if (newNode != nullptr) nodes.push_back(newNode);
    }
    auto getRoot = [&tkList, &nodes]() -> ExpressionNode * {
        auto recursion = [&tkList, &nodes] (auto &&self, int l, int r) -> ExpressionNode * {
            if (l > r) return nullptr;
            if (l == r) return nodes[l];
            uint32 mn = IdentifierWeight + 1; int mnp = nodes.size();
            for (int p = l; p <= r; p++) {
                uint32 w = nodes[p]->getWeight();
                if (w <= mn) mn = w, mnp = p;
            }
            auto &node = nodes[mnp];
            node->addChild(self(self, l, mnp - 1));
            node->addChild(self(self, mnp + 1, r));
            return node;
        };
        return recursion(recursion, 0, nodes.size() - 1);
    };
    ExpressionNode *node = new ExpressionNode(tkList[l]);
    node->addChild(getRoot());
    return node;
}
#pragma endregion

#pragma region build block
BlockNode *buildBlock(const TokenList &tkList, size_t l, size_t &r) {
    BlockNode *node = new BlockNode(tkList[l]);
    r = tkList[l].data.uint64_v();
    uint32 tmpVarCnt = 0;
    for (size_t pos = l + 1, rpos = pos; pos < r; pos = ++rpos) {
        SyntaxNode *child = buildNode(tkList, pos, rpos);
        node->addChild(child);

        // update local variable count
        if (child == nullptr) continue;
        switch (child->getType()) {
            case SyntaxNodeType::VarDef: {
                auto varDef = (VarDefNode *)child;
                tmpVarCnt += varDef->getLocalVarCount();
                node->setLocalVarCount(std::max(node->getLocalVarCount(), tmpVarCnt));
                break;
            }
            case SyntaxNodeType::If:
            case SyntaxNodeType::While:
            case SyntaxNodeType::For:
            case SyntaxNodeType::Block: {
                auto subBlock = (BlockNode *)child;
                node->setLocalVarCount(std::max(node->getLocalVarCount(), tmpVarCnt + subBlock->getLocalVarCount()));
                break;
            }
        }
    }
    node->setLocalVarCount(std::max(node->getLocalVarCount(), tmpVarCnt));
    return node;
}
#pragma endregion

#pragma region build if, while, for, case, switch
IfNode *buildIf(const TokenList &tkList, size_t l, size_t &r) {
    IfNode *node = new IfNode();
    r = l;
    if (tkList[l + 1].type != TokenType::SBrkL) {
        printError(tkList[l + 1].lineId, "Invalid content in \"if\".");
        delete node;
        return nullptr;
    }
    ExpressionNode *condNode = buildExpression(tkList, l + 2, tkList[l + 1].data.uint64_v() - 1);
    node->setCondNode(condNode);
    r = tkList[l + 1].data.uint64_v();

    SyntaxNode *succNode = buildNode(tkList, r + 1, r), *failNode = nullptr;
    node->setSuccNode(succNode);
    if (succNode != nullptr) 
        node->setLocalVarCount(succNode->getLocalVarCount());

    if (tkList[r + 1].type == TokenType::Else) {
        failNode = buildNode(tkList, r + 2, r);
        node->setFailNode(failNode);
        if (failNode != nullptr) 
            node->setLocalVarCount(std::max(node->getLocalVarCount(), failNode->getLocalVarCount()));
    }
    return node;
}

LoopNode *buildWhile(const TokenList &tkList, size_t l, size_t &r) {
    LoopNode *node = new LoopNode(SyntaxNodeType::While);
    r = l;
    if (tkList[l + 1].type != TokenType::SBrkL) {
        printError(tkList[l + 1].lineId, "Invalid content in \"while\".");
        delete node;
        return nullptr;
    }
    ExpressionNode *condNode = buildExpression(tkList, l + 2, tkList[l + 1].data.uint64_v() - 1);
    node->setCondNode(condNode);
    r = tkList[l + 1].data.uint64_v();

    SyntaxNode *content = buildNode(tkList, r + 1, r);
    node->setContent(content);
    node->setLocalVarCount(content->getLocalVarCount());

    return node;
}

LoopNode *buildFor(const TokenList &tkList, size_t l, size_t &r) {
    LoopNode *node = new LoopNode(SyntaxNodeType::For);
    r = l;
    if (tkList[l + 1].type != TokenType::SBrkL) {
        printError(tkList[l + 1].lineId, "Invalid content in \"for\".");
        delete node;
        return nullptr;
    }
    size_t pos = l + 2;
    SyntaxNode *initNode = nullptr, *content = nullptr;
    ExpressionNode *condNode = nullptr, *stepNode = nullptr;
    initNode = buildNode(tkList, pos, pos);
    if (initNode != nullptr && initNode->getType() != SyntaxNodeType::Expression && initNode->getType() != SyntaxNodeType::VarDef) {
        printError(tkList[l + 1].lineId, "Initialization part of \"for\" must be definition of variables or expression");
        delete node;
        return nullptr;
    }
    node->setInitNode(initNode);
    size_t to = pos + 1;
    while (tkList[to].type != TokenType::ExprEnd) to++;
    condNode = buildExpression(tkList, pos + 1, to - 1);
    stepNode = buildExpression(tkList, to + 1, tkList[l + 1].data.uint64_v() - 1);
    r = tkList[l + 1].data.uint64_v();
    content = buildNode(tkList, r + 1, r);
    node->setCondNode(condNode), node->setStepNode(stepNode), node->setContent(content);

    uint32 varCount = 0;
    if (initNode != nullptr) varCount += initNode->getLocalVarCount();
    if (content != nullptr) varCount += content->getLocalVarCount();
    node->setLocalVarCount(varCount);
    return node;
}

CaseNode *buildCase(const TokenList &tkList, size_t l, size_t &r) {
    CaseNode *node = new CaseNode(tkList[l]);
    r = l + 1;
    if (tkList[r].type == TokenType::Default) {
        node->setDefaultCase(true);
    } else {
        if (tkList[r].type != TokenType::SBrkL) {
            printError(tkList[r].lineId, "Invalid content in \"case\".");
            delete node;
            return nullptr;
        }
        size_t pos = tkList[r].data.uint64_v();
        ExpressionNode *condNode = buildExpression(tkList, r + 1, pos - 1);
        if (condNode == nullptr) { delete node; return nullptr; }
        node->addChild(condNode);
        r = pos;
    }
    return node;
}

SwitchNode *buildSwitch(const TokenList &tkList, size_t l, size_t &r) {
    SwitchNode *node = new SwitchNode(tkList[l]);
    r = l + 1;
    if (tkList[r].type != TokenType::SBrkL) {
        printError(tkList[r].lineId, "Invalid content in \"switch\".");
        delete node;
        return nullptr;
    }
    // get condition expression
    size_t pos = tkList[r].data.uint64_v();
    ExpressionNode *condNode = buildExpression(tkList, r + 1, pos - 1);
    if (condNode == nullptr) { delete node; return nullptr; }
    node->addChild(condNode);
    r = pos + 1;
    if (tkList[r].type != TokenType::LBrkL) {
        printError(tkList[r].lineId, "Invalid content in \"switch\".");
        delete node;
        return nullptr;
    }
    // get childrens
    uint32 varDefCnt = 0;
    for (pos = r + 1; pos < tkList[r].data.uint64_v(); pos++) {
        SyntaxNode *child = buildNode(tkList, pos, pos, true);
        if (child == nullptr) { delete node; return nullptr; }
        node->addChild(child);
        // update the local variable count
        switch (child->getType()) {
            case SyntaxNodeType::Case:
                node->addCaseIndex(node->getChildrenCount() - 1);
                break;
            case SyntaxNodeType::VarDef :
                varDefCnt += child->getLocalVarCount();
                node->setLocalVarCount(std::max(node->getLocalVarCount(), varDefCnt));
                break;
            case SyntaxNodeType::Block:
            case SyntaxNodeType::If:
            case SyntaxNodeType::While:
            case SyntaxNodeType::For:
                node->setLocalVarCount(std::max(node->getLocalVarCount(), varDefCnt + child->getLocalVarCount()));
                break;
        }
    }
    node->setLocalVarCount(std::max(node->getLocalVarCount(), varDefCnt));
    return node;
}
#pragma endregion

#pragma region build control
ControlNode *buildControl(const TokenList &tkList, size_t l, size_t &r) {
    SyntaxNodeType type;
    if (tkList[l].type == TokenType::Break) type = SyntaxNodeType::Break;
    else if (tkList[l].type == TokenType::Continue) type = SyntaxNodeType::Continue;
    else if (tkList[l].type == TokenType::Return) type = SyntaxNodeType::Return;
    ControlNode *node = new ControlNode(type, tkList[l]);
    r = l + 1;
    if (type == SyntaxNodeType::Return) {
        while (tkList[r].type != TokenType::ExprEnd) r++;
        node->get(0) = buildExpression(tkList, l + 1, r - 1);
    }
    return node;
}
#pragma endregion

#pragma region build definition
VarDefNode *buildVarDef(const TokenList &tkList, size_t l, size_t &r) {
    VarDefNode *node = new VarDefNode();
    if (l && isVisibility(tkList[l - 1].type)) node->setVisibility(getVisibility(tkList[l - 1].type));
    r = l + 1;
    uint32 varCount = 0;
    IdentifierNode *defaultTypeNode = nullptr;
    if (tkList[r].type == TokenType::TypeHint) {
        defaultTypeNode = buildIdentifier(tkList, r + 1, r);
        r++;
    }
    while (tkList[r].type != TokenType::ExprEnd) {
        IdentifierNode *nameNode = new IdentifierNode(tkList[l]), *typeNode = nullptr;
        ExpressionNode *initNode = nullptr;
        nameNode->setName(tkList[r].dataStr);
        size_t to = r;
        if (tkList[to + 1].type == TokenType::TypeHint) {
            if (defaultTypeNode == nullptr) typeNode = buildIdentifier(tkList, to + 2, to);
            else {
                printError(tkList[to + 1].lineId, "It is invalid to add type hint for a signle variable when there is a global type hint.");
                delete node;
                return nullptr;
            }
        } else typeNode = defaultTypeNode;
        if (tkList[to + 1].type == TokenType::Assign) {
            size_t to2 = to + 1;
            while (tkList[to2].type != TokenType::Comma && tkList[to2].type != TokenType::ExprEnd) {
                if (isBracketL(tkList[to2].type)) to2 = tkList[to2].data.uint64_v();
                to2++;
            }
            initNode = buildExpression(tkList, to + 2, to2 - 1);
            to = to2;

            
        } else to++;
        if (initNode == nullptr && nameNode == nullptr) {
            printError(nameNode->getToken().lineId, "Definition of \"" + nameNode->getName() + "\" is empty.");
            delete nameNode, delete node;
            return nullptr;
        }
        node->addChild(nameNode), node->addChild(typeNode), node->addChild(initNode);
        varCount++;
        r = to;
        if (tkList[r].type == TokenType::ExprEnd) break;
        else {
            if (tkList[r].type != TokenType::Comma) {
                printError(tkList[r].lineId, "Invalid content of variable definition");
                delete node;
                return nullptr;
            }
            r++;
        }
    }
    node->setLocalVarCount(varCount);
    return node;
}

FuncDefNode *buildFuncDef(const TokenList &tkList, size_t l, size_t &r) {
    FuncDefNode *node;
    if (tkList[l].type == TokenType::VarFuncDef) node = new VarFuncDefNode();
    else node = new FuncDefNode();
    if (l > 0 && isVisibility(tkList[l - 1].type)) node->setVisibility(getVisibility(tkList[l - 1].type));
    IdentifierNode *nameNode = new IdentifierNode(tkList[l + 1]);
    nameNode->setName(tkList[l + 1].dataStr);
    r = l + 2;
    GenericAreaNode *gArea = nullptr;
    // have generic area
    if (tkList[r].type == TokenType::GBrkL) {
        gArea = new GenericAreaNode(tkList[r]);
        nameNode->setGenericArea(gArea);
        for (size_t pos = r + 1, rpos; pos < tkList[r].data.uint64_v(); pos = ++rpos) {
            IdentifierNode *gParam = buildIdentifier(tkList, pos, rpos);
            if (gParam != nullptr && (gParam->getGenericArea() != nullptr || gParam->isFuncCall())) {
                printError(tkList[pos].lineId, "Invalid syntax of definition of generic identifier.");
                delete gParam, delete nameNode, delete node;
                return nullptr;
            }
            nameNode->getGenericArea()->addChild(gParam);
            rpos++; 
        }
        r = tkList[r].data.uint64_v() + 1;
    }
    node->addChild(nameNode);
    // get the params
    if (tkList[r].type != TokenType::SBrkL) {
        printError(tkList[r].lineId, "Invalid content of definition of function.");
        delete node;
        return nullptr;
    }
    for (size_t pos = r + 1, rpos = pos; pos < tkList[r].data.uint64_v(); pos = ++rpos) {
        IdentifierNode *pNameNode = new IdentifierNode(tkList[pos]), *pTypeNode = nullptr;
        ConstValueNode *pValNode = nullptr;
        pNameNode->setName(tkList[pos].dataStr);
        if (tkList[pos + 1].type != TokenType::TypeHint) {
            printError(tkList[pos + 1].lineId, "Invalid content of param definition of function \"" + nameNode->getName() + "\"");
            delete pNameNode, delete node;
            return nullptr;
        }
        pTypeNode = buildIdentifier(tkList, pos + 2, rpos);
        node->addChild(pNameNode), node->addChild(pTypeNode);
        // found default value expression for this param
        if (tkList[rpos + 1].type == TokenType::Assign) {
            size_t st = (rpos += 2);
            while (tkList[rpos].type != TokenType::Comma && rpos < tkList[r].data.uint64_v()) {
                if (isBracketL(tkList[rpos].type)) rpos = tkList[rpos].data.uint64_v();
                rpos++;
            }
            ExpressionNode *constexprTree = buildExpression(tkList, st, rpos - 1);
            if (!isConstExpr(constexprTree)) {
                printError(tkList[st].lineId, "The default value of function param must be constant.");
                delete constexprTree, delete node;
                return nullptr;
            }
            pValNode = calcConstExpr(constexprTree);
        }
        node->addChild(pValNode);
        rpos++;
    }
    r = tkList[r].data.uint64_v();
    // get return type
    if (tkList[r + 1].type != TokenType::TypeHint) {
        printError(tkList[r + 1].lineId, "Invalid content of definition funcion");
        delete node;
        return nullptr;
    }
    IdentifierNode *retNode = buildIdentifier(tkList, r + 2, r);
    SyntaxNode *content = buildNode(tkList, r + 1, r);
    node->addChild(retNode), node->addChild(content);
    node->setLocalVarCount(node->getParamCount() + (content != nullptr ? content->getLocalVarCount() : 0)); 
    return node;
}

ClsDefNode *buildClsDef(const TokenList &tkList, size_t l, size_t &r) {
    ClsDefNode *node = new ClsDefNode(tkList[l]);
    if (l > 0 && isVisibility(tkList[l - 1].type)) node->setVisibility(getVisibility(tkList[l - 1].type));
    IdentifierNode *nameNode = buildIdentifier(tkList, l + 1, r);
    if (nameNode == nullptr || nameNode->isFuncCall()) {
        printError(node->getToken().lineId, "Invalid syntax of definition of class");
        if (nameNode != nullptr) delete nameNode;
        delete node;
        return nullptr;
    }
    node->setNameNode(nameNode);
    IdentifierNode *baseNode = nullptr;
    if (tkList[r + 1].type == TokenType::TypeHint) {
        baseNode = buildIdentifier(tkList, r + 2, r);
        if (baseNode != nullptr && baseNode->isFuncCall()) {
            printError(baseNode->getToken().lineId, "Invalid syntax of definition of base class");
            delete baseNode, delete node;
            return nullptr;
        }
    }
    node->setBaseNode(baseNode);
    // just a declaration
    if (tkList[r + 1].type == TokenType::ExprEnd) r++, node->addChild(new SyntaxNode(SyntaxNodeType::Empty));
    else if (tkList[r + 1].type == TokenType::LBrkL) {
        for (size_t pos = r + 2, rpos = pos; pos < tkList[r + 1].data.uint64_v(); pos = ++rpos) {
            switch (tkList[pos].type) {
                case TokenType::FuncDef:
                case TokenType::VarFuncDef:
                    node->addChild(buildFuncDef(tkList, pos, rpos));
                    break;
                case TokenType::VarDef:
                    node->addChild(buildVarDef(tkList, pos, rpos));
                    break;
                default:
                    if (!isVisibility(tkList[pos].type))
                        printError(tkList[pos].lineId, "Invalid content of definition of class \"" + node->getNameNode()->getName() + "\"");
                    break;
            }
        }
        r = tkList[r + 1].data.uint64_v();
    } else {
        printError(tkList[r + 1].lineId, "Invalid content of definition of class \"" + node->getNameNode()->getName() + "\"");
        delete node;
    }
    return node;
}

NspDefNode *buildNspDef(const TokenList &tkList, size_t l, size_t &r) {
    NspDefNode *node = new NspDefNode(tkList[l]);
    IdentifierNode *nameNode = buildIdentifier(tkList, l + 1, r);
    node->setNameNode(nameNode);
    if (tkList[r + 1].type != TokenType::LBrkL) {
        printError(tkList[r + 1].lineId, "Invalid content of definition of namespace \"" + nameNode->getName() + "\"");
        delete node;
        return nullptr;
    }
    for (size_t pos = r + 2, rpos = pos; pos < tkList[r + 1].data.uint64_v(); pos = ++rpos) {
        switch (tkList[pos].type) {
            case TokenType::VarDef:
                node->addChild(buildVarDef(tkList, pos, rpos));
                break;
            case TokenType::FuncDef:
                node->addChild(buildFuncDef(tkList, pos, rpos));
                break;
            case TokenType::ClsDef:
                node->addChild(buildClsDef(tkList, pos, rpos));
                break;
            default:
                if (!isVisibility(tkList[pos].type))
                    printError(tkList[r + 1].lineId, "Invalid content of definition of namespace \"" + nameNode->getName() + "\"");
                break;
        }
    }
    return node;
}
#pragma endregion

#pragma region build using
UsingNode *buildUsing(const TokenList &tkList, size_t l, size_t &r) {
    UsingNode *node = new UsingNode(tkList[l]);
    std::string path = tkList[l + 1].dataStr;
    r = l + 1;
    while (tkList[r + 1].type == TokenType::GetChild) {
        path.push_back('.');
        path.append(tkList[r + 2].dataStr);
        r += 2;
    }
    if (tkList[r + 1].type != TokenType::ExprEnd) {
        printError(tkList[r + 1].lineId, "Invalid syntax of using");
        delete node;
        return nullptr;
    }
    node->setPath(path);
    return node;
}
#pragma endregion

SyntaxNode *buildNode(const TokenList &tkList, size_t l, size_t &r, bool isInSwitch) {
    SyntaxNode *node = nullptr;
    r = l;
    switch (tkList[l].type) {
        case TokenType::ExprEnd:
            node = new SyntaxNode(SyntaxNodeType::Empty);
            break;
        case TokenType::LBrkL:
            node = buildBlock(tkList, l, r);
            break;
        case TokenType::VarDef:
            node = buildVarDef(tkList, l, r);
            break;
        case TokenType::If:
            node = buildIf(tkList, l, r);
            break;
        case TokenType::For:
            node = buildFor(tkList, l, r);
            break;
        case TokenType::While:
            node = buildWhile(tkList, l, r);
            break;
        case TokenType::Switch:
            node = buildSwitch(tkList, l, r);
            break;
        case TokenType::Case:
            if (isInSwitch) node = buildCase(tkList, l, r);
            else printError(tkList[l].lineId, "Invalid syntax of \"case\".");
            break;
        case TokenType::Break:
        case TokenType::Continue:
        case TokenType::Return:
            node = buildControl(tkList, l, r);
            break;
        case TokenType::FuncDef:
        case TokenType::VarFuncDef:
            node = buildFuncDef(tkList, l, r);
            break;
        case TokenType::ClsDef:
            node = buildClsDef(tkList, l, r);
            break;
        case TokenType::NspDef:
            node = buildNspDef(tkList, l, r);
            break;
        case TokenType::Using:
            node = buildUsing(tkList, l, r);
            break;
        default:
            if (isVisibility(tkList[l].type)) break;
            r = l;
            while (tkList[r].type != TokenType::ExprEnd) r++;
            node = buildExpression(tkList, l, r - 1);
            break;
    }
    return node;
}

RootNode *buildRootNode(SyntaxNodeType type, const TokenList &tkList) {
    auto node = new RootNode(type);
    for (size_t pos = 0, rpos = 0; pos < tkList.size(); pos = ++rpos)
        node->addChild(buildNode(tkList, pos, rpos));
    return node;
}

void debugPrintTree(SyntaxNode *node, int dep) {
    if (node == nullptr) {
        std::cout << getIndent(dep) << "<NULL>\n";
        return ;
    }
    std::cout << getIndent(dep) << node->toString() << std::endl;
    for (size_t i = 0; i < node->getChildrenCount(); i++) debugPrintTree(node->get(i), dep + 1);
}