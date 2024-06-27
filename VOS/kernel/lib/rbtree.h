#ifndef __LIB_RB_TREE_H__
#define __LIB_RB_TREE_H__

#include "ds.h"
#include "spinlock.h"

#define RBTree_Color_Red 0
#define RBTree_Color_Black 1

typedef struct RBNode {
	u64 unionParCol;
	struct RBNode *left, *right;
} __attribute__((aligned(sizeof(long)))) RBNode;

#define RBTree_Col_Red		0
#define RBTree_Col_Black	1

// the comparator for RBTree, return 1 if a < b, 0 if a > b; invalid for a == b
typedef int (*RBTreeComparator)(RBNode *a, RBNode *b);
typedef struct {
	RBNode *root;
	SpinLock lock;
	RBTreeComparator comparator;
} RBTree;

void RBTree_debug(RBTree *tree);

void RBTree_init(RBTree *tree, RBTreeComparator comparator);

void RBTree_insNode(RBTree *tree, RBNode *node);
void RBTree_delNode(RBTree *tree, RBNode *node);

RBNode *RBTree_getMin(RBTree *tree);
RBNode *RBTree_getMax(RBTree *tree);

RBNode *RBTree_getNext(RBTree *tree, RBNode *node);

#endif