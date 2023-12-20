#include "cpltree.h"

bool ExpressionNode::checkExprResType() {
    if (childrenCount() && children[0] != nullptr) return children[0]->checkExprResType();
    return true;
}

bool IdentifierNode::checkExprResType() {
    return true;
}

bool MethodCallNode::checkExprResType() {
    return true;
}

bool OperatorNode::checkExprResType() {
    return true;
}

bool ConstValueNode::checkExprResType() {
    return true;
}

bool GenericAreaNode::checkExprResType() {
    return true;
}

bool ConditionNode::checkExprResType() {
    return true;
}

bool WhileNode::checkExprResType() {
    return true;
}

bool ForNode::checkExprResType() {
    return true;
}

bool ControlNode::checkExprResType() {
    return true;
}

bool BlockNode::checkExprResType() {
    return true;
}

bool VarDefNode::checkExprResType() {
    return true;
}

bool FuncDefNode::checkExprResType() {
    return true;
}

bool ClsDefNode::checkExprResType() {
    return true;
}

bool NspDefNode::checkExprResType() {
    return true;
}