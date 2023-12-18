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


SyntaxNode::SyntaxNode(SyntaxNodeType type) {
	this->type = type;
}

SyntaxNode::SyntaxNode(SyntaxNodeType type, const Token& tk) {
	this->type = type;
	this->token = tk;
}

SyntaxNodeType SyntaxNode::getType() const {
	return this->type;
}

const Token& SyntaxNode::getToken() const {
	return token;
}

void SyntaxNode::setToken(const Token& tk) {
	token = tk;
}

size_t SyntaxNode::childrenCount() const {
	return children.size();
}

void SyntaxNode::addChild(SyntaxNode *child) {
	children.push_back(child);
}

SyntaxNode* SyntaxNode::operator[](size_t index) const {
	return children[index];
}

SyntaxNode*& SyntaxNode::operator[](size_t index) {
	return children[index];
}

std::string ExpressionNode::toString() const
{
	return std::string("Expression") + resultType.toString();
}

ExpressionNode::ExpressionNode(SyntaxNodeType type): SyntaxNode(type) {
}

ExpressionNode::ExpressionNode(SyntaxNodeType type, const Token& tk) : SyntaxNode(type, tk) {
}

const EResultType& ExpressionNode::getResultType() const {
	return resultType;
}

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
	return false;
}

int32 ExpressionNode::getWeight() const {
	return 10000000;
}

IdentifierNode::IdentifierNode() : ExpressionNode(SyntaxNodeType::Identifier) {
	id = 0;
}

IdentifierNode::IdentifierNode(const Token &tk) : ExpressionNode(SyntaxNodeType::Identifier, tk) {
	name = tk.dataStr;
	id = 0;
}

std::string IdentifierNode::toString() const {
    return tokenTypeString[(int)type] + " " + name + " " + std::to_string(id) + " " + resultType.toString();
}

const uint64 &IdentifierNode::getId() const {
    return id;
}
