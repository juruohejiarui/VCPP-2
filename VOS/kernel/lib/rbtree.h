#ifndef __LIB_RB_TREE_H__
#define __LIB_RB_TREE_H__

#include "ds.h"

#define RBTree_Color_Red 0
#define RBTree_Color_Black 1

typedef struct _tmpNode {
	struct _tmpNode *left, *right;
	u64 unionParentCol;
	i64 val;
	List head;
} __attribute__ ((aligned(sizeof(long)))) RBNode;

typedef struct {
	RBNode *root;
} RBTree;

void RBTree_init(RBTree *tree);
RBTree *RBTree_get(RBTree *tree, i64 val);
// get the node with minimum value of subtree T(ST), (starts from the root if ST==NULL)
RBTree *RBTree_getMin(RBTree *tree);
// get the node with maximum value of subtree T(ST), (starts from the root if ST==NULL)
RBTree *RBTree_getMax(RBTree *tree);
void RBTree_insert(RBTree *tree, i64 val, List *listEle);
void RBTree_del(RBTree *tree, i64 val);

#endif