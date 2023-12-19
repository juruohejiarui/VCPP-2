#include "cpltree.h"

extern std::string syntaxNodeTypeString[] = {
	"Identifier", "ConstValue", "String", "GenericArea",
	"If", "Else", "While", "For", "Break", "Continue", "Return", "Block", 
	"Expression", "Operator", "MethodCall",
	"VarDef", "VarFuncDef", "FuncDef", "ClsDef", "NspDef",

	"SourceFile", "DefinitionFile",
	"Empty", "Unknown",
};
extern const int syntaxNodeTypeNumber = 23;

SyntaxNode *buildSingleNode(const TokenList &tkList, size_t st, size_t &ed);

SyntaxNode::SyntaxNode(const Token& tk) { token = tk, varCount = 0; }
SyntaxNode::SyntaxNode(SyntaxNodeType type) { this->type = type, varCount = 0; }
SyntaxNode::SyntaxNode(SyntaxNodeType type, const Token& tk) {
	this->type = type;
	this->token = tk;
	varCount = 0;
}

SyntaxNodeType SyntaxNode::getType() const { return this->type; }

const Token& SyntaxNode::getToken() const { return token; }
void SyntaxNode::setToken(const Token& tk) { token = tk; }

size_t SyntaxNode::childrenCount() const { return children.size(); }
void SyntaxNode::addChild(SyntaxNode *child) { children.push_back(child); }

SyntaxNode *SyntaxNode::operator[](size_t index) const { return children[index]; }
SyntaxNode*& SyntaxNode::operator[](size_t index) { return children[index]; }

uint32 SyntaxNode::getVarCount() const { return varCount; }

std::string ExpressionNode::toString() const { return std::string("Expression") + resultType.toString(); }

ExpressionNode::ExpressionNode(SyntaxNodeType type): SyntaxNode(type) { }

ExpressionNode::ExpressionNode(SyntaxNodeType type, const Token& tk) : SyntaxNode(type, tk) { }

const EResultType& ExpressionNode::getResultType() const { return resultType; }

bool ExpressionNode::buildNode(const TokenList &tkList, size_t st, size_t &ed) {
	ed = st;
	while (tkList[ed].type != TokenType::ExprEnd) ed++;
	return buildNode(tkList, st, ed - 1);
}

bool ExpressionNode::buildNode(const TokenList &tkList, size_t st, size_t ed) {
	if (st > ed) return true;
	// make the list of ExpressionNode
	std::vector<ExpressionNode *> list;
	bool succ = false;
	for (size_t pos = st, to = pos; pos < ed; pos = ++to) {
		const Token &firTk = tkList[pos];
		ExpressionNode *newNode = nullptr;
		switch (firTk.type) {
			case TokenType::ConstData: {
				newNode = new ConstValueNode(firTk);
				break;
			}
			case TokenType::Identifier: {
				GenericAreaNode *gaNode = nullptr;
				// get the generic area
				if (to + 1 < ed && tkList[to + 1].type == TokenType::GBrkL) {
					++to;
					gaNode = new GenericAreaNode(tkList[to]);
					succ &= gaNode->buildNode(tkList, to, tkList[to].data.uint64_v());
					to = tkList[to].data.uint64_v();
				}
				// this identifier is a function call
				if (to + 1 < ed && tkList[to + 1].type == TokenType::SBrkL) {
					newNode = new MethodCallNode(firTk);
					newNode->addChild(gaNode);
					size_t lpos = to + 2;
					to = tkList[to + 1].data.uint64_v();
					while (lpos < to) {
						size_t rpos = lpos;
						while (rpos < to && tkList[rpos].type != TokenType::Comma) {
							if (isBracketL(tkList[rpos].type)) rpos = tkList[rpos].data.uint64_v();
							rpos++;
						}
						ExpressionNode *argNode = new ExpressionNode(SyntaxNodeType::Expression, tkList[lpos]);
						succ &= argNode->buildNode(tkList, lpos, rpos - 1);
						lpos = rpos + 1;
					}
				}
				else {
					newNode = new IdentifierNode(firTk);
					newNode->addChild(gaNode);
				}
				break;
			}
			case TokenType::MBrkL: {
				OperatorNode *opNode = new OperatorNode(firTk);
				list.push_back(opNode);
				newNode = new ExpressionNode(SyntaxNodeType::Expression, tkList[pos + 1]);
				succ &= newNode->buildNode(tkList, pos + 1, firTk.data.uint64_v() - 1);
				break;
			}
			case TokenType::SBrkL: {
				newNode = new ExpressionNode(SyntaxNodeType::Expression, firTk);
				succ &= newNode->buildNode(tkList, pos + 1, firTk.data.uint64_v() - 1);
				break;
			}
			default: {
				if (isOperator(firTk.type))
					newNode = new OperatorNode(firTk);
				else {
					printError(firTk.lineId, "Invalid content of an expression : " + firTk.toString());
					succ = false;
				}
				break;
			}
		}
		list.push_back(newNode);
	}

	auto buildTree = [&] () -> ExpressionNode * {
		auto recursion = [&](auto &&self, uint32 l, uint32 r) {
			if (l > r) return (ExpressionNode *)nullptr;
			if (l == r) return list[l];
			uint32 minPos = l;
			for (uint32 i = l + 1; i <= r; i++) {
				if (list[i]->getWeight() <= list[minPos]->getWeight()) minPos = i;
			}
			ExpressionNode *root = list[minPos];
			root->addChild(self(self, l, minPos - 1));
			root->addChild(self(self, minPos + 1, r));
			return root;
		};
		return recursion(recursion, 0, list.size() - 1);
	};
	addChild(buildTree());
	return false;
}

int32 ExpressionNode::getWeight() const {
	return 10000000;
}

IdentifierNode::IdentifierNode() : ExpressionNode(SyntaxNodeType::Identifier) { id = 0; }

IdentifierNode::IdentifierNode(const Token &tk) : ExpressionNode(SyntaxNodeType::Identifier, tk) { name = tk.dataStr; id = 0; }

std::string IdentifierNode::toString() const { return tokenTypeString[(int)type] + " " + name + " " + std::to_string(id) + " " + resultType.toString(); }

const uint64 &IdentifierNode::getId() const { return id; }

void IdentifierNode::setId(uint64 id) { this->id = id; }

const std::string &IdentifierNode::getName() const { return name; }
void IdentifierNode::setName(const std::string &name) { this->name = name; }

bool IdentifierNode::buildNode(const TokenList &tkList, size_t st, size_t &ed) {
	name = tkList[st].dataStr;
	bool succ = true;
	if (st + 1 < tkList.size() && tkList[st + 1].type == TokenType::GBrkL) {
		GenericAreaNode *gaNode = new GenericAreaNode(tkList[st + 1]);
		succ = gaNode->buildNode(tkList, st + 1, tkList[st + 1].data.uint64_v());
		addChild(gaNode);
		ed = tkList[st + 1].data.uint64_v();
	}
	return succ;
}

OperatorNode::OperatorNode() : ExpressionNode(SyntaxNodeType::Operator) { }

OperatorNode::OperatorNode(TokenType opType) : ExpressionNode(SyntaxNodeType::Operator) { token.type = opType; }
OperatorNode::OperatorNode(const Token &tk) : ExpressionNode(SyntaxNodeType::Operator, tk) { }

TokenType OperatorNode::getOpType() const { return token.type; }

int32 OperatorNode::getWeight() const { return getOperWeight(token.type); }

ConstValueNode::ConstValueNode() : ExpressionNode(SyntaxNodeType::ConstValue) { }

ConstValueNode::ConstValueNode(const UnionData &data) : ExpressionNode(SyntaxNodeType::ConstValue) {
	this->token.type = TokenType::ConstData;
	this->token.data = data;
}

ConstValueNode::ConstValueNode(const Token &tk) : ExpressionNode(SyntaxNodeType::ConstValue, tk) { }

UnionData ConstValueNode::data() const { return token.data; }
UnionData &ConstValueNode::data() { return token.data; }

std::string ConstValueNode::toString() const {
	return tokenTypeString[(int)type] + " " + token.toString() + " " + resultType.toString();
}

GenericAreaNode::GenericAreaNode() : ExpressionNode(SyntaxNodeType::GenericArea) { }
GenericAreaNode::GenericAreaNode(const Token &tk) : ExpressionNode(SyntaxNodeType::GenericArea, tk) { }

bool GenericAreaNode::buildNode(const TokenList &tkList, size_t st, size_t ed) {
	if (st > ed) return true;
	size_t pos = st + 1;
	while (pos < ed) {
		size_t rpos = pos;
		while (rpos < ed && tkList[rpos].type != TokenType::Comma) {
			if (isBracketL(tkList[rpos].type)) rpos = tkList[rpos].data.uint64_v();
			rpos++;
		}
		ExpressionNode *argNode = new ExpressionNode(SyntaxNodeType::Expression, tkList[pos]);
		bool succ = argNode->buildNode(tkList, pos, rpos - 1);
		pos = rpos + 1;
		addChild(argNode);
	}
	return false;
}

ConditionNode::ConditionNode() : SyntaxNode(SyntaxNodeType::If) { }
ConditionNode::ConditionNode(const Token &tk) : SyntaxNode(SyntaxNodeType::If, tk) { }

bool ConditionNode::buildNode(const TokenList &tkList, size_t st, size_t &ed) {
	ed = st;
	size_t pos = st + 1;
	if (tkList[pos].type != TokenType::SBrkL) {
		printError(tkList[pos + 1].lineId, "Invalid content of a condition expression " + tkList[pos].toString());
		return false;
	}
	auto condNode = new ExpressionNode(SyntaxNodeType::Expression, tkList[pos]);
	bool succ = condNode->buildNode(tkList, pos + 1, tkList[pos].data.uint64_v() - 1);
	pos = tkList[pos].data.uint64_v() + 1;
	auto succNode = buildSingleNode(tkList, pos, ed);
	if (succNode == nullptr) return false;
	addChild(succNode);
	varCount = succNode->getVarCount();
	pos = ed + 1;
	if (tkList[pos].type == TokenType::Else) {
		auto failNode = buildSingleNode(tkList, pos + 1, ed);
		if (failNode == nullptr) return false;
		else {
			addChild(failNode);
			varCount = std::max(varCount, failNode->getVarCount());
		}
	} else addChild(nullptr);
	return succ;
}

ExpressionNode *ConditionNode::getCondNode() const { return (ExpressionNode *)children[0]; }
SyntaxNode *ConditionNode::getSuccNode() const { return children[1]; }
SyntaxNode *ConditionNode::getFailNode() const { return children[2]; }

WhileNode::WhileNode() : SyntaxNode(SyntaxNodeType::While) { }
WhileNode::WhileNode(const Token &tk) : SyntaxNode(SyntaxNodeType::While, tk) { }

ExpressionNode *WhileNode::getCondNode() const { return (ExpressionNode *)children[0]; }
SyntaxNode *WhileNode::getContentNode() const { return children[1]; }

bool WhileNode::buildNode(const TokenList &tkList, size_t st, size_t &ed) {
	ed = st;
	size_t pos = st + 1;
	if (tkList[pos].type != TokenType::SBrkL) {
		printError(tkList[pos + 1].lineId, "Invalid content of a condition expression " + tkList[pos].toString());
		return false;
	}
	auto condNode = new ExpressionNode(SyntaxNodeType::Expression, tkList[pos]);
	bool succ = condNode->buildNode(tkList, pos + 1, tkList[pos].data.uint64_v() - 1);
	pos = tkList[pos].data.uint64_v() + 1;
	auto contentNode = buildSingleNode(tkList, pos, ed);
	if (contentNode == nullptr) return false;
	else varCount = contentNode->getVarCount();
	addChild(condNode);
	addChild(contentNode);
	return succ;
}

ForNode::ForNode() : SyntaxNode(SyntaxNodeType::For) { }
ForNode::ForNode(const Token &tk) : SyntaxNode(SyntaxNodeType::For, tk) { }

SyntaxNode *ForNode::getInitNode() const { return children[0]; }
ExpressionNode *ForNode::getCondNode() const { return (ExpressionNode *)children[1]; }
ExpressionNode *ForNode::getStepNode() const { return (ExpressionNode *)children[2]; }
SyntaxNode *ForNode::getContentNode() const { return children[3]; }

bool ForNode::buildNode(const TokenList &tkList, size_t st, size_t &ed) {
	ed = tkList[st + 1].data.uint64_v();
	size_t pos = st + 2, rpos = pos;
	bool succ = true;
	auto initNode = buildSingleNode(tkList, pos, rpos);
	if (initNode == nullptr) succ = false;
	else varCount = initNode->getVarCount();
	pos = ++rpos;
	while (tkList[rpos].type != TokenType::ExprEnd) rpos++;
	auto condNode = new ExpressionNode(SyntaxNodeType::Expression, tkList[pos]);
	succ &= condNode->buildNode(tkList, pos, rpos - 1);
	auto stepNode = new ExpressionNode(SyntaxNodeType::Expression, tkList[rpos + 1]);
	succ &= stepNode->buildNode(tkList, rpos + 1, ed - 1);
	addChild(initNode), addChild(condNode), addChild(stepNode);
	auto contentNode = buildSingleNode(tkList, ed + 1, ed);
	if (contentNode == nullptr) succ = false;
	addChild(contentNode);
	varCount += contentNode->getVarCount();
	return succ;
}

ControlNode::ControlNode(SyntaxNodeType type) : SyntaxNode(type) { }
ControlNode::ControlNode(const Token &tk) : SyntaxNode(tk) {
	if (tk.type == TokenType::Break) type = SyntaxNodeType::Break;
	else if (tk.type == TokenType::Continue) type = SyntaxNodeType::Continue;
	else if (tk.type == TokenType::Return) type = SyntaxNodeType::Return;
	else type = SyntaxNodeType::Unknown; 
}

std::string ControlNode::toString() const { 
	if (type == SyntaxNodeType::Break || type == SyntaxNodeType::Continue) return syntaxNodeTypeString[(int)type];
	else if (type == SyntaxNodeType::Return) return syntaxNodeTypeString[(int)type] + " " + ((ExpressionNode *)children[0])->toString();
	else return "Unknown";
}

bool ControlNode::buildNode(const TokenList &tkList, size_t st, size_t &ed) {
	ed = st;
	bool succ = true;
	switch (type) {
		case SyntaxNodeType::Break:
			ed = st + 1;
			break;
		case SyntaxNodeType::Continue:
			ed = st + 1;
			break;
		case SyntaxNodeType::Return: {
			if (tkList[st + 1].type == TokenType::ExprEnd) {
				addChild(nullptr);
				ed = st + 1;
			} else {
				while (tkList[ed].type != TokenType::ExprEnd) ed++;
				auto exprNode = new ExpressionNode(SyntaxNodeType::Expression, tkList[st + 1]);
				succ = exprNode->buildNode(tkList, st + 1, ed - 1);
				addChild(exprNode);
			}
			break;
		}
		default:
			succ = false;
			break;
	}
	return succ;
}

BlockNode::BlockNode() : SyntaxNode(SyntaxNodeType::Block) { }
BlockNode::BlockNode(const Token &tk) : SyntaxNode(SyntaxNodeType::Block, tk) { }

std::string BlockNode::toString() const { return syntaxNodeTypeString[(int)type]; }

bool BlockNode::buildNode(const TokenList &tkList, size_t st, size_t &ed) {
	ed = tkList[st].data.uint64_v();
	bool succ = true;
	size_t pos = st + 1;
	uint32 tmpVarCount = 0;
	while (pos < ed) {
		size_t rpos = pos;
		auto childNode = buildSingleNode(tkList, pos, rpos);
		if (childNode == nullptr) succ = false;
		else {
			if (childNode->getType() == SyntaxNodeType::VarDef) tmpVarCount += childNode->getVarCount();
			else varCount = std::max(varCount, tmpVarCount + childNode->getVarCount());
		}
		pos = rpos + 1;
		addChild(childNode);
	}
	varCount = std::max(varCount, tmpVarCount);
	return succ;
}

VarDefNode::VarDefNode() : SyntaxNode(SyntaxNodeType::VarDef) { }
VarDefNode::VarDefNode(const Token &tk) : SyntaxNode(SyntaxNodeType::VarDef, tk) { }

std::string VarDefNode::toString() const { return syntaxNodeTypeString[(int)type]; }

bool VarDefNode::buildNode(const TokenList &tkList, size_t st, size_t &ed) {
	ed = st;
	bool succ = true;
	varCount = 0;
	if (st > 0 && isVisibility(tkList[st - 1].type)) 
		visibility = ::getVisibility(tkList[st - 1].type);
	while (tkList[ed].type != TokenType::ExprEnd) ed++;
	for (size_t pos = st + 1; pos < ed; pos++) {
		size_t rpos = pos;
		while (rpos < ed && tkList[rpos].type != TokenType::Comma) {
			if (isBracketL(tkList[rpos].type)) rpos = tkList[rpos].data.uint64_v();
			rpos++;
		}
		IdentifierNode *idNode = new IdentifierNode(tkList[pos]), *typeNode = nullptr;
		ExpressionNode *initNode = nullptr;
		varCount++;
		if (rpos > pos + 1 && tkList[pos + 1].type == TokenType::TypeHint) {
			typeNode = new IdentifierNode(tkList[pos + 2]);
			size_t typeHintTo = pos + 2;
			succ &= typeNode->buildNode(tkList, pos + 2, typeHintTo);
			pos = typeHintTo + 1;
		}
		size_t assignPos = pos + 1;
		while (assignPos < rpos && tkList[assignPos].type != TokenType::Assign) assignPos++;
		if (assignPos < rpos) {
			initNode = new ExpressionNode(SyntaxNodeType::Expression, tkList[assignPos + 1]);
			succ &= initNode->buildNode(tkList, assignPos + 1, rpos - 1);
		}
		addChild(idNode), addChild(typeNode), addChild(initNode);
		pos = rpos;
	}
	return succ;
}