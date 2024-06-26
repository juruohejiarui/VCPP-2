#include "rbtree.h"
#include "memop.h"

#define parent(nd) 	((RBNode *)((nd)->unionParCol & ~3))
#define color(nd) 	((nd)->unionParCol & 1)
#define isRed(nd) 	(!color(nd))
#define isBlack(nd) (color(nd))
#define setRed(nd)		do { (nd)->unionParCol &= ~1; } while (0)
#define setBlack(nd)	do { (nd)->unionParCol |= 1; } while (0)
static inline void setParent(RBNode *node, RBNode *par) {
	node->unionParCol = (node->unionParCol & 3) | (u64)par;
}
static inline void setCol(RBNode *node, int col) {
	node->unionParCol = (node->unionParCol & ~1ul) | col;
}

static void _rotLeft(RBTree *tree, RBNode *node) {
	RBNode *right = node->right, *par = parent(node);
	if ((node->right = right->left)) setParent(right->left, node);
	right->left = node;
	setParent(right, par);
	if (par) {
		if (node == par->left) par->left = right;
		else par->right = right;
	} else tree->root = right;
	setParent(node, right);
}

static void _rotRight(RBTree *tree, RBNode *node) {
	RBNode *left = node->left, *par = parent(node);
	if ((node->right = left->right)) setParent(left->right, node);
	left->right = node;
	setParent(left, par);
	if (par) {
		if (node == par->right) par->right = left;
		else par->left = left;
	} else tree->root = left;
	setParent(node, left);
}

static void _linkNode(RBNode *node, RBNode *par) {
	node->unionParCol = (u64)par;
	node->left = node->right = 0;
}

void RBTree_init(RBTree *tree, RBTreeComparator comparator) {
	SpinLock_init(&tree->lock);
	tree->root = 0;
	tree->comparator = comparator;
}

static void _fixAfterIns(RBTree *tree, RBNode *node) {
	RBNode *par, *gPar;
	while ((par = parent(node)) && isRed(par)) {
		gPar = parent(par);
		if (par == gPar->left) {
			{
				register RBNode *uncle = gPar->right;
				if (uncle && isRed(uncle)) {
					setBlack(uncle);
					setBlack(par);
					setRed(gPar);
					node = gPar;
					continue;
				}
			}
			if (par->right == node) {
				register RBNode *tmp;
				_rotLeft(tree, par);
				tmp = par, par = node, node = tmp;
			}
			setBlack(par);
			setRed(gPar);
			_rotRight(tree, gPar);
		} else {
			{
				register RBNode *uncle = gPar->left;
				if (uncle && isRed(uncle)) {
					setBlack(uncle);
					setBlack(par);
					setBlack(gPar);
					node = gPar;
					continue;
				}
			}
			if (par->left == node) {
				register RBNode *tmp;
				_rotRight(tree, par);
				tmp = par, par = node, node = tmp;
			}
			setBlack(par);
			setRed(gPar);
			_rotLeft(tree, gPar);
		}
	}
	setBlack(tree->root);
}

void RBTree_insNode(RBTree *tree, RBNode *node) {
	if (tree == NULL || node == NULL) return ;
	SpinLock_lock(&tree->lock);
	if (tree->root == NULL) {
		tree->root = node;
		node->left = node->right = NULL;
		node->unionParCol = 0;
		return ;
	}
	RBNode *par = tree->root;
	while (1) {
		if (tree->comparator(par, node)) {
			if (par->right == NULL) {
				par->right = node;
				break;
			} else par = par->right;
		} else {
			if (par->left == NULL) {
				par->left = node;
				break;
			} else par = par->left;
		}
	}
	_linkNode(node, par);
	// rebalance
	_fixAfterIns(tree, node);
	SpinLock_unlock(&tree->lock);
}

static _fixAfterDel(RBTree *tree, RBNode *node, RBNode *par) {
	RBNode *other;
	while ((!node || isBlack(node)) && node != tree->root) {
		if (par->left == node) {
			other = par->right;
			if (isRed(other)) {
				setBlack(other);
				setRed(par);
				_rotLeft(tree, par);
				other = par->right;
			}
			if ((!other->left || isBlack(other->left))
					&& (!other->right || isBlack(other->right))) {
				setRed(other);
				node = par;
				par = parent(node);
			} else {
				
			}
		}
	}
}

void RBTree_delNode(RBTree *tree, RBNode *node) {
	if (tree == NULL || node == NULL) return ;
	SpinLock_lock(&tree->lock);
	RBNode *child, *par;
	int col;
	if (!node->left) child = node->right;
	else if (!node->right) child = node->left;
	else {
		RBNode *old = node, *left;
		node = node->right;
		while ((left = node->left) != NULL) node = left;
		if (parent(old)) {
			if (parent(old)->left == old) parent(old)->left = node;
			else parent(old)->right = node;
		} else tree->root = node;
		child = node->right;
		par = parent(node);
		col = color(node);
		if (par == old) par = node;
		else {
			if (child) setParent(child, par);
			par->left = child;
			node->right = old->right;
			setParent(old->right, node);
		}
		node->unionParCol = old->unionParCol;
		node->left = old->left;
		setParent(old->left, node);
		goto rebalance;
	}
	par = parent(node);
	col = color(node);
	if (child) setParent(child, par);
	if (par) {
		if (par->left == node) par->left = child;
		else par->right = child;
	} else tree->root = child;
	rebalance:
	if (col == RBTree_Col_Black) _fixAfterDel(tree, child, par);
	SpinLock_unlock(&tree->lock);
}
