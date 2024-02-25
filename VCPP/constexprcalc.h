#pragma once
#include "syntaxnode.h"

bool isConstExpr(ExpressionNode *expr);
ConstValueNode *calcConstExpr(ExpressionNode *expr);