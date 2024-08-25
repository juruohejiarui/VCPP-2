#include "rbtree.h"
#include "memop.h"
#include "../includes/log.h"

#define parent(nd) 	((RBNode *)((nd)->unionParCol & ~3ul))
#define color(nd) 	((nd)->unionParCol & 1)
#define isRed(nd) 	(!color(nd))
#define isBlack(nd) (color(nd))
#define setRed(nd)		do { (nd)->unionParCol &= ~1ull; } while (0)
#define setBlack(nd)	do { (nd)->unionParCol |= 1ull; } while (0)
static __always_inline__ void setParent(RBNode *node, RBNode *par) {
	node->unionParCol = (node->unionParCol & 3) | (u64)par;
}
static __always_inline__ void setCol(RBNode *node, int col) {
	node->unionParCol = (node->unionParCol & ~1ul) | col;
}

static void _rotLeft(RBTree *tree, RBNode *node) {
	RBNode *right = node->right;
	if ((node->right = right->left) != NULL) setParent(right->left, node);
	right->left = node;
	setParent(right, parent(node));
	if (parent(right) != NULL) {
		if (node == parent(node)->left) parent(node)->left = right;
		else parent(node)->right = right;
	} else tree->root = right;
	setParent(node, right);
}

static void _rotRight(RBTree *tree, RBNode *node) {
	RBNode *left = node->left;
	if ((node->left = left->right) != NULL) setParent(left->right, node);
	left->right = node;
	setParent(left, parent(node));
	if (parent(node) != NULL) {
		if (node == parent(node)->right) parent(node)->right = left;
		else parent(node)->left = left;
	} else tree->root = left;
	setParent(node, left);
}

static void _linkNode(RBNode **src, RBNode *node, RBNode *par) {
	node->unionParCol = (u64)par;
	setRed(node);
	node->left = node->right = NULL;
	*src = node;
}

static void _debug(RBNode *node, int dep) {
	if (dep >= 5) while (1) IO_hlt();
	if (node == NULL) { printk(WHITE, BLACK, "\n"); return ; }
	for (int i = 0; i < dep; i++) printk(WHITE, BLACK, "  ");
	printk(YELLOW, BLACK, "%c %#018lx\n", color(node) ? 'B' : 'R', node);
	if (node->left == NULL && node->right == NULL) return ;
	_debug(node->left, dep + 1);
	_debug(node->right, dep + 1);
}

void RBTree_debug(RBTree *tree) {
	if (tree == NULL) return ;
	SpinLock_lock(&tree->lock);
	_debug(tree->root, 0);
	SpinLock_unlock(&tree->lock);
}

void RBTree_init(RBTree *tree, RBTreeComparator comparator) {
	SpinLock_init(&tree->lock);
	tree->root = NULL;
	tree->comparator = comparator;
}

static void _fixAfterIns(RBTree *tree, RBNode *node) {
	RBNode *par, *gPar;
	while ((par = parent(node)) != NULL && isRed(par)) {
		gPar = parent(par);
		if (par == gPar->left) {
			{
				register RBNode *uncle = gPar->right;
				if (uncle != NULL && isRed(uncle)) {
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
				if (uncle != NULL && isRed(uncle)) {
					setBlack(uncle);
					setBlack(par);
					setRed(gPar);
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
		node->unionParCol = 1;
		SpinLock_unlock(&tree->lock);
		return ;
	}
	RBNode **src = &tree->root, *lst = NULL;
	while (*src) {
		lst = *src;
		if (tree->comparator(node, lst))
			src = &(*src)->left;
		else src = &(*src)->right;
	}
	_linkNode(src, node, lst);
	// rebalance
	_fixAfterIns(tree, node);
	SpinLock_unlock(&tree->lock);
}

static void _fixAfterDel(RBTree *tree, RBNode *node, RBNode *par) {
	RBNode *other;
	while ((node == NULL || isBlack(node)) && node != tree->root) {
		if (par->left == node) {
			other = par->right;
			if (isRed(other)) {
				setBlack(other);
				setRed(par);
				_rotLeft(tree, par);
				other = par->right;
			}
			if ((other->left == NULL || isBlack(other->left))
					&& (other->right == NULL || isBlack(other->right))) {
				setRed(other);
				node = par;
				par = parent(node);
			} else {
				if (other->right == NULL || isBlack(other->right)) {
					register RBNode *o_left;
					if ((o_left = other->left)) setBlack(o_left);
					setRed(other);
					_rotRight(tree, other);
					other = par->right;
				}
				setCol(other, color(par));
				setBlack(par);
				if (other->right) setBlack(other->right);
				_rotLeft(tree, par);
				node = tree->root;
				break;
			}
		} else {
			other = par->left;
			if (isRed(other)) {
				setBlack(other);
				setRed(par);
				_rotRight(tree, par);
				other = par->left;
			}
			if ((other->left == NULL || isBlack(other->left))
					&& (other->right == NULL || isBlack(other->right))) {
				setRed(other);
				node = par;
				par = parent(node);
			} else {
				if (other->left == NULL || isBlack(other->left)) {
					if (other->right) setBlack(other->right);
					setRed(other);
					_rotLeft(tree, other);
					other = par->left;
				}
				setCol(other, color(par));
				setBlack(par);
				if (other->left) setBlack(other->left);
				_rotRight(tree, par);
				node = tree->root;
				break;
			}
		}
	}
	if (node != NULL) setBlack(node);
}

void RBTree_delNode(RBTree *tree, RBNode *node) {
	if (tree == NULL || node == NULL) return ;
	SpinLock_lock(&tree->lock);
	RBNode *child, *par;
	int col;
	if (node->left == NULL) child = node->right;
	else if (node->right == NULL) child = node->left;
	else {
		RBNode *old = node, *left;
		node = node->right;
		while ((left = node->left) != NULL) node = left;
		child = node->right;
		par = parent(node);
		col = color(node);
		if (child) setParent(child, par);
		if (par) {
			if (par->left == node) par->left = child;
			else par->right = child;
		} else tree->root = child;
		if (parent(node) == old) par = node;
		node->unionParCol = old->unionParCol;
		node->left = old->left;
		node->right = old->right;
		if (parent(old)) {
			if (parent(old)->left == old)
				parent(old)->left = node;
			else parent(old)->right = node;
		} else tree->root = node;
		setParent(old->left, node);
		if (old->right) setParent(old->right, node);
		goto rebalance;
	}
	par = parent(node);
	col = color(node);
	if (child != NULL) setParent(child, par);
	if (par != NULL) {
		if (par->left == node) par->left = child;
		else par->right = child;
	} else tree->root = child;

	if (node == tree->root) tree->root = NULL;

	rebalance:
	if (col == RBTree_Col_Black) _fixAfterDel(tree, child, par);
	SpinLock_unlock(&tree->lock);
}

RBNode *RBTree_getMin(RBTree *tree) {
	if (tree == NULL || tree->root == NULL) return NULL;
	SpinLock_lock(&tree->lock);
	RBNode *res = tree->root;
	while (res->left) res = res->left;
	SpinLock_unlock(&tree->lock);
	return res;
}

RBNode *RBTree_getMax(RBTree *tree) {
	if (tree == NULL || tree->root == NULL) return NULL;
	SpinLock_lock(&tree->lock);
	RBNode *res = tree->root;
	while (res->right) res = res->right;
	SpinLock_unlock(&tree->lock);
	return res;
}

RBNode *RBTree_getNext(RBTree *tree, RBNode *node) {
	if (tree == NULL || tree->root == NULL) return NULL;
	SpinLock_lock(&tree->lock);
	RBNode *par;
	if (node->right) {
		node = node->right;
		while (node->left) node = node->left;
		SpinLock_unlock(&tree->lock);
		return node;
	}
	while ((par = parent(node)) && node == par->right)
		node = par;
	SpinLock_unlock(&tree->lock);
	return node;
}