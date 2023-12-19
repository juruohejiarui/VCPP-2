#include "cpltree.h"

const std::string syntaxNodeTypeString[] = {
	"Identifier", "ConstValue", "String", "GenericArea",
	"If", "Else", "While", "For", "Break", "Continue", "Return", "Block", 
	"Expression", "Operator", "MethodCall",
	"VarDef", "VarFuncDef", "FuncDef", "ClsDef", "NspDef",

	"SourceFile", "DefinitionFile",
	"Empty", "Unknown",
};
const int syntaxNodeTypeNumber = 23;

/**
 * @brief Builds a single syntax node from a given token list.
 * 
 * This function takes a token list, a starting index, and a reference to an ending index.
 * It constructs a single syntax node using the tokens in the token list, starting from the
 * specified starting index. The ending index is updated to point to the last token used in
 * constructing the syntax node.
 * 
 * @param tkList The token list from which to build the syntax node.
 * @param st The starting index in the token list.
 * @param ed A reference to the ending index, which will be updated by the function.
 * @return A pointer to the constructed syntax node.
 */
SyntaxNode *buildSingleNode(const TokenList &tkList, size_t st, size_t &ed);

#pragma region SyntaxNode
SyntaxNode::SyntaxNode(const Token& tk) { token = tk, varCount = 0; }
SyntaxNode::SyntaxNode(SyntaxNodeType type) { this->type = type, varCount = 0; }
SyntaxNode::SyntaxNode(SyntaxNodeType type, const Token& tk) {
	this->type = type;
	this->token = tk;
	varCount = 0;
}
SyntaxNode::~SyntaxNode() { for (auto child : children) if (child != nullptr) delete child; }

SyntaxNodeType SyntaxNode::getType() const { return this->type; }

const Token& SyntaxNode::getToken() const { return token; }
void SyntaxNode::setToken(const Token& tk) { token = tk; }

size_t SyntaxNode::childrenCount() const { return children.size(); }
void SyntaxNode::addChild(SyntaxNode *child) { children.push_back(child), child->setParent(this); }
SyntaxNode *SyntaxNode::operator[](size_t index) const { return children[index]; }
SyntaxNode*& SyntaxNode::operator[](size_t index) { return children[index]; }
SyntaxNode *SyntaxNode::getParent() const { return parent; }
void SyntaxNode::setParent(SyntaxNode *parent) { this->parent = parent; }

uint32 SyntaxNode::getVarCount() const { return varCount; }
#pragma endregion

#pragma region ExpressionNode
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
#pragma endregion

#pragma region IdentifierNode
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
	} else ed = st;
	return succ;
}
#pragma endregion

#pragma region MethodCallNode
MethodCallNode::MethodCallNode() : IdentifierNode() { type = SyntaxNodeType::MethodCall; }
MethodCallNode::MethodCallNode(const Token &tk) : IdentifierNode(tk) { type = SyntaxNodeType::MethodCall; }

std::string MethodCallNode::toString() const { return tokenTypeString[(int)type] + " " + getName() + " " + resultType.toString(); }
#pragma endregion

#pragma region OperatorNode
OperatorNode::OperatorNode() : ExpressionNode(SyntaxNodeType::Operator) { }

OperatorNode::OperatorNode(TokenType opType) : ExpressionNode(SyntaxNodeType::Operator) { token.type = opType; }
OperatorNode::OperatorNode(const Token &tk) : ExpressionNode(SyntaxNodeType::Operator, tk) { }

TokenType OperatorNode::getOpType() const { return token.type; }

int32 OperatorNode::getWeight() const { return getOperWeight(token.type); }
#pragma endregion

#pragma region ConstValueNode
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
#pragma endregion

#pragma region GenericAreaNode
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
#pragma endregion

#pragma region ConditionNode
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
#pragma endregion

#pragma region WhileNode
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
#pragma endregion

#pragma region ForNode
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
#pragma endregion

#pragma region ControlNode
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
#pragma endregion

#pragma region BlockNode
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
#pragma endregion

#pragma region VarDefNode
VarDefNode::VarDefNode() : SyntaxNode(SyntaxNodeType::VarDef) { }
VarDefNode::VarDefNode(const Token &tk) : SyntaxNode(SyntaxNodeType::VarDef, tk) { }

std::string VarDefNode::toString() const { return identifierVisibilityString[(int)visibility] + " " + syntaxNodeTypeString[(int)type]; }

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
		pos = rpos + 1;
	}
	return succ;
}
#pragma endregion

#pragma region FuncDefNode
FuncDefNode::FuncDefNode() : SyntaxNode(SyntaxNodeType::FuncDef) { }
FuncDefNode::FuncDefNode(const Token &tk) : SyntaxNode(SyntaxNodeType::FuncDef, tk) {
	if (tk.type == TokenType::VarFuncDef) type = SyntaxNodeType::VarFuncDef;
	else type = SyntaxNodeType::FuncDef;
}

std::string FuncDefNode::toString() const { return identifierTypeString[(int)visibility] + " " + syntaxNodeTypeString[(int)type] + " " + name; }
IdentifierVisibility FuncDefNode::getVisibility() const { return visibility; }
std::string FuncDefNode::getName() const { return name; }

bool FuncDefNode::buildNode(const TokenList &tkList, size_t st, size_t &ed) {
	bool succ = true;
	varCount = 0;
	if (st > 0 && isVisibility(tkList[st - 1].type)) visibility = ::getVisibility(tkList[st - 1].type);

	// get the function name
	ed = st + 1;
	name = tkList[ed].dataStr;
	
	// get the param list
	size_t pos = ed + 2;
	ed = tkList[ed + 1].data.uint64_v();
	while (pos < ed) {
		size_t rpos = pos;
		IdentifierNode *idNode = new IdentifierNode(tkList[pos]), *typeNode = nullptr;
		if (tkList[pos + 1].type != TokenType::TypeHint) {
			succ = false;
			printError(tkList[pos + 1].lineId, "Invalid parameter definition " + tkList[pos + 1].toString());
		}
		typeNode = new IdentifierNode(tkList[pos + 2]);
		typeNode->buildNode(tkList, pos + 2, rpos);
		pos = rpos + 1;
		addChild(idNode), addChild(typeNode);
	}
	// get the return type
	ed++;
	IdentifierNode *retType = nullptr;
	if (tkList[ed].type != TokenType::TypeHint) {
		printError(tkList[ed].lineId, "Invalid return type definition " + tkList[ed].toString());
		succ = false;
	} else {
		retType = new IdentifierNode(tkList[ed + 1]);
		succ &= retType->buildNode(tkList, ed + 1, ed);
	}
	addChild(retType);
	ed++;
	// get the content
	auto contentNode = buildSingleNode(tkList, ed, ed);
	if (contentNode == nullptr) succ = false;
	else varCount += contentNode->getVarCount();
	addChild(contentNode);

	return succ;
}
#pragma endregion

#pragma region ClsDefNode
ClsDefNode::ClsDefNode() : SyntaxNode(SyntaxNodeType::ClsDef) { }
ClsDefNode::ClsDefNode(const Token &tk) : SyntaxNode(SyntaxNodeType::ClsDef, tk) { name = tk.dataStr; }

std::string ClsDefNode::toString() const { return identifierTypeString[(int)visibility] + " " + syntaxNodeTypeString[(int)type] + " " + name; }
std::string ClsDefNode::getName() const { return name; }
IdentifierVisibility ClsDefNode::getVisibility() const { return visibility; }

bool ClsDefNode::buildNode(const TokenList &tkList, size_t st, size_t& ed) {
	bool succ = true;
	varCount = 0;
	if (st > 0 && isVisibility(tkList[st - 1].type)) visibility = ::getVisibility(tkList[st - 1].type);
	// get the class name
	ed = st + 1;
	name = tkList[ed].dataStr;
	// get the base class
	ed++;
	IdentifierNode *baseClass = nullptr;
	if (tkList[ed].type == TokenType::TypeHint) {
		baseClass = new IdentifierNode(tkList[ed + 1]);
		succ &= baseClass->buildNode(tkList, ed + 1, ed);
		ed++;
	}
	addChild(baseClass);

	// get the content
	size_t pos = ed + 1;
	ed = tkList[ed].data.uint64_v();
	while (pos < ed) {
		size_t rpos = pos;
		switch (tkList[pos].type) {
			case TokenType::VarDef: {
				VarDefNode *varDefNode = new VarDefNode(tkList[pos]);
				succ &= varDefNode->buildNode(tkList, pos, rpos);
				addChild(varDefNode);
				break;
			}
			case TokenType::FuncDef: {
				FuncDefNode *funcDefNode = new FuncDefNode(tkList[pos]);
				succ &= funcDefNode->buildNode(tkList, pos, rpos);
				addChild(funcDefNode);
				break;
			}
			default:
				if (!isVisibility(tkList[pos].type)) {
					printError(tkList[pos].lineId, "Invalid content of a class definition " + tkList[pos].toString());
					succ = false;
				}
				break;
		}
		pos = rpos + 1;
	}
	return succ;
}
#pragma endregion

#pragma region NspDefNode
NspDefNode::NspDefNode() : SyntaxNode(SyntaxNodeType::NspDef) { }
NspDefNode::NspDefNode(const Token &tk) : SyntaxNode(SyntaxNodeType::NspDef, tk) { name = tk.dataStr; }

std::string NspDefNode::toString() const { return syntaxNodeTypeString[(int)type] + name; }
std::string NspDefNode::getName() const { return name; }

bool NspDefNode::buildNode(const TokenList &tkList, size_t st, size_t &ed) {
	// get the namespace name
	ed = st + 1;
	name = tkList[ed].dataStr;
	bool succ = true;
	// get the content
	size_t pos = ed + 2;
	ed = tkList[ed + 1].data.uint64_v();
	while (pos < ed) {
		size_t rpos = pos;
		switch (tkList[pos].type) {
			case TokenType::VarDef: {
				VarDefNode *varDefNode = new VarDefNode(tkList[pos]);
				succ &= varDefNode->buildNode(tkList, pos, rpos);
				addChild(varDefNode);
				break;
			}
			case TokenType::FuncDef: {
				FuncDefNode *funcDefNode = new FuncDefNode(tkList[pos]);
				succ &= funcDefNode->buildNode(tkList, pos, rpos);
				addChild(funcDefNode);
				break;
			}
			case TokenType::ClsDef: {
				ClsDefNode *clsDefNode = new ClsDefNode(tkList[pos]);
				succ &= clsDefNode->buildNode(tkList, pos, rpos);
				addChild(clsDefNode);
				break;
			}
			default:
				if (!isVisibility(tkList[pos].type)) {
					printError(tkList[pos].lineId, "Invalid content of a namespace definition " + tkList[pos].toString());
					succ = false;
				}
				break;
		}
		pos = rpos + 1;
	}
	return succ;
}
#pragma endregion

SyntaxNode *buildSingleNode(const TokenList &tkList, size_t st, size_t &ed) {
	const Token &firTk = tkList[st];
	bool succ = true;
	SyntaxNode *newNode = nullptr;
	switch (firTk.type) {
		case TokenType::If: {
			newNode = new ConditionNode(firTk);
			succ = ((ConditionNode *)newNode)->buildNode(tkList, st, ed);
			break;
		}
		case TokenType::While: {
			newNode = new WhileNode(firTk);
			succ = ((WhileNode *)newNode)->buildNode(tkList, st, ed);
			break;
		}
		case TokenType::For: {
			newNode = new ForNode(firTk);
			succ = ((ForNode *)newNode)->buildNode(tkList, st, ed);
			break;
		}
		case TokenType::Break:
		case TokenType::Continue:
		case TokenType::Return: {
			newNode = new ControlNode(firTk);
			succ = ((ControlNode *)newNode)->buildNode(tkList, st, ed);
			break;
		}
		case TokenType::LBrkL: {
			newNode = new BlockNode(firTk);
			succ = ((BlockNode *)newNode)->buildNode(tkList, st, ed);
			break;
		}
		case TokenType::VarDef: {
			newNode = new VarDefNode(firTk);
			succ = ((VarDefNode *)newNode)->buildNode(tkList, st, ed);
			break;
		}
		case TokenType::FuncDef: {
			newNode = new FuncDefNode(firTk);
			succ = ((FuncDefNode *)newNode)->buildNode(tkList, st, ed);
			break;
		}
		case TokenType::ClsDef: {
			newNode = new ClsDefNode(firTk);
			succ = ((ClsDefNode *)newNode)->buildNode(tkList, st, ed);
			break;
		}
		case TokenType::ExprEnd: break;
		default: {
			// it is an expression node
			newNode = new ExpressionNode(SyntaxNodeType::Expression, firTk);
			while (tkList[ed].type != TokenType::ExprEnd) ed++;
			succ = ((ExpressionNode *)newNode)->buildNode(tkList, st, ed - 1);
			break;
		}
	}
	if (!succ) {
		delete newNode;
		return nullptr;
	}
	return newNode;
}