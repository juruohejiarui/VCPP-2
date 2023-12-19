#include "cpltree.h"

bool ExpressionNode::checkEResultType() {
    if (childrenCount() == 0) return true;
    return children[0]->checkEResultType();
}