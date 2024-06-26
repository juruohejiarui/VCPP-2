#ifndef __LIB_RB_TREE_H__
#define __LIB_RB_TREE_H__

#include "ds.h"
#include "spinlock.h"

#define RBTree_Color_Red 0
#define RBTree_Color_Black 1

typedef struct _tmpNode {
	struct _tmpNode *left, *right;
	u64 unionParentCol;
	u64 val;
	List head;
} __attribute__ ((aligned(sizeof(long)))) RBNode;

typedef struct {
	RBNode *root;
	SpinLock lock;
} RBTree;

void RBTree_init(RBTree *tree);
RBNode *RBTree_get(RBTree *tree, u64 val);
// get the node with the minimum value
RBNode *RBTree_getMin(RBTree *tree);
List *RBTree_getMinListEle(RBTree *tree);
// get the node with maximum value
RBNode *RBTree_getMax(RBTree *tree);
void RBTree_insert(RBTree *tree, u64 val, List *listEle);
// del the first listEle of the node with VAL
void RBTree_del(RBTree *tree, u64 val);
void RBTree_delNode(RBTree *tree, RBNode *node);

void RBTree_debug(RBTree *tree);

#endif