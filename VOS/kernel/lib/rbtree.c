#include "rbtree.h"
#include "memop.h"
#include "../includes/memory.h"
#include "../includes/log.h"

#define _parent(node) ((RBNode *)((node)->unionParentCol & ~3))
#define _col(node) ((node)->unionParentCol & 3)
#define _isRed(node) (!_col(node))
#define _isBlack(node) (!_col(node))
#define _setRed(node) do { (node)->unionParentCol &= ~1; } while (0)
#define _setBlack(node) do { (node)->unionParentCol |= 1; } while (0)

static inline void _setParent(RBNode *node, RBNode *parent) {
	node->unionParentCol = (node->unionParentCol & 3) | (u64)parent;
}
static inline void _setCol(RBNode *node, int col) {
	node->unionParentCol = (node->unionParentCol & ~3) | col;
}

static RBNode *_newNode(i64 val, RBNode *parent) {
	// printk(WHITE, BLACK, "rbtree/_newNode: ");
	RBNode *node = (RBNode *)kmalloc(sizeof(RBNode), 0);
	memset(node, 0, sizeof(RBNode));
	List_init(&node->head);
	node->val = val;
	node->unionParentCol = (u64)parent | 0;
}
void RBTree_init(RBTree *tree) {
	tree->root = NULL;
	SpinLock_init(&tree->lock);
}

RBNode *RBTree_get(RBTree *tree, u64 val) {
	SpinLock_lock(&tree->lock);
	if (tree == NULL || tree->root == NULL) {
		SpinLock_unlock(&tree->lock);
		return NULL;
	}
	RBNode *cur = tree->root;
	while (cur != NULL && cur->val != val)
		if (cur->val < val) cur = cur->right;
		else cur = cur->left;
	SpinLock_unlock(&tree->lock);
	return cur;
}

static RBNode *_getNode(RBNode *node, u64 val) {
	while (node != NULL && node->val != val) {
		if (node->val < val) node = node->right;
		else node = node->left;
	}
	return node;
}

static RBNode *_getMin(RBNode *node) {
	while (node->left != NULL) node = node->left;
	return node;
}

RBNode *RBTree_getMin(RBTree *tree) {
	SpinLock_lock(&tree->lock);
	RBNode *res = tree == NULL || tree->root == NULL ? NULL : _getMin(tree->root);
	SpinLock_unlock(&tree->lock);
	return res;
}

List *RBTree_getMinListEle(RBTree *tree) {
	SpinLock_lock(&tree->lock);
    RBNode *node = RBTree_getMin(tree);
	List *res = List_isEmpty(&node->head) ? NULL : node->head.next;
	SpinLock_unlock(&tree->lock);
	return res;
}

static RBNode *_getMax(RBNode *node) {
	while (node->right != NULL) node = node->right;
	return node;
}

RBNode *RBTree_getMax(RBTree *tree) {
	SpinLock_lock(&tree->lock);
	RBNode *res = tree == NULL || tree->root == NULL ? NULL : _getMax(tree->root);
	SpinLock_unlock(&tree->lock);
	return res;
}

static void _rotLeft(RBTree *tree, RBNode *node) {
	RBNode *r = node->right, *parent = _parent(node);
	if ((node->right = r->left) != NULL) _setParent(r->left, node);
	r->left = node;
	_setParent(r, parent);
	if (parent != NULL) {
		if (node == parent->left) parent->left = r;
		else parent->right = r;
	} else tree->root = r;
	_setParent(node, r);
}

static void _rotRight(RBTree *tree, RBNode *node) {
	RBNode *l = node->left, *parent = _parent(node);
	if ((node->left = l->right) != NULL) _setParent(l->right, node);
	l->right = node;
	_setParent(l, parent);
	if (parent != NULL) {
		if (node == parent->right) parent->right = l;
		else parent->left = l;
	} else tree->root = l;
	_setParent(node, l);
}

static void _fixAfterIns(RBTree *tree, RBNode *node) {
	RBNode *parent, *gParent;
	while ((parent = _parent(node)) != NULL && _isRed(parent)) {
		gParent = _parent(parent);
		if (parent == gParent->left) {
			{
				register RBNode *uncle = gParent->right;
				if (uncle != NULL && _isRed(uncle)) {
					_setBlack(uncle); _setBlack(parent);
					_setRed(gParent);
					continue;
				}
			}
			if (parent->right == node) {
				register RBNode *tmp;
				_rotLeft(tree, parent);
				tmp = parent, parent = node, node = tmp;
			}
			_setBlack(parent); _setRed(gParent);
			_rotRight(tree, gParent);
		} else {
			{
				register RBNode *uncle = gParent->left;
				if (uncle != NULL && _isRed(uncle)) {
					_setBlack(uncle); _setBlack(parent);
					_setRed(gParent);
					continue;
				}
			}
			if (parent->left == node) {
				register RBNode *tmp;
				_rotLeft(tree, parent);
				tmp = parent, parent = node, node = tmp;
			}
			_setBlack(parent); _setRed(gParent);
			_rotLeft(tree, gParent);
		}
	}
	_setBlack(tree->root);
}

void RBTree_insert(RBTree *tree, u64 val, List *listEle) {
	SpinLock_lock(&tree->lock);
	List_init(listEle);
	if (tree->root == NULL) {
		tree->root = _newNode(val, NULL);
		_setBlack((RBNode *)tree->root);
		List_insBehind(listEle, &tree->root->head);
		SpinLock_unlock(&tree->lock);
		return ;
	}
	RBNode *cur = (RBNode *)tree->root;
	while (1) {
		if (cur->val == val) {
			List_insBehind(listEle, &cur->head);
			break;
		} else if (cur->val < val) {
			if (cur->right == NULL) cur->right = _newNode(val, cur);
			cur = cur->right;
		} else {
			if (cur->left == NULL) cur->left = _newNode(val, cur);
			cur = cur->left;
		}
	}
	_setRed(cur);
	_fixAfterIns(tree, cur);
	SpinLock_unlock(&tree->lock);
}

void _fixAfterDel(RBTree *tree, RBNode *node, RBNode *parent) {
	RBNode *other;
	while ((node == NULL || _isBlack(node)) && node != tree->root) {
		if (parent->left == node) {
			other = parent->right;
			if (_isRed(other)) {
				_setBlack(other); _setRed(parent);
				_rotLeft(tree, parent);
				other = parent->right;
			}
			if ((other->left == NULL || _isBlack(other->left))
					&& (other->right == NULL || _isBlack(other->right))) {
				_setRed(other);
				node = parent, parent = _parent(node);
			} else {
				if (other->right == NULL || _isBlack(other->right)) {
					_setBlack(other->left); _setRed(other);
					_rotRight(tree, other);
					other = parent->right;
				}
				_setCol(other, _col(parent));
				_setBlack(parent);
				_setBlack(other->right);
				_rotLeft(tree, other->right);
				node = tree->root;
				break;
			}
		} else {
			other = parent->left;
			if (_isRed(other)) {
				_setBlack(other); _setRed(parent);
				_rotRight(tree, parent);
				other = parent->left;
			}
			if ((other->left == NULL || _isBlack(other->left))
					&& (other->right == NULL || _isBlack(other->right))) {
				_setRed(other);
				node = parent, parent = _parent(node);
			} else {
				if (other->left == NULL || _isBlack(other->left)) {
					_setBlack(other->right); _setRed(other);
					_rotLeft(tree, other);
					other = parent->left;
				}
				_setCol(other, _col(parent));
				_setBlack(parent);
				_setBlack(other->left);
				_rotRight(tree, parent);
				node = tree->root;
				break;
			}
		}
	}
	if (node) _setBlack(node);
}

static void _delNode(RBTree *tree, RBNode *node) {
	RBNode *child, *parent;
	int col;
	if (!node->left) child = node->right;
	else if (!node->right) child = node->left;
	else {
		RBNode *old = node, *left;
		node = node->right;
		while ((left = node->left) != NULL) node = left;
		// modify the .left or .right of the parent of OLD
		if (_parent(old) != NULL) {
			if (_parent(old)->left == old) _parent(old)->left = node;
			else _parent(old)->right = node;
		} else tree->root = node;
		child = node->right;
		parent = _parent(node);
		col = _col(node);
		// NODE is a leave node or the node with only right child 
		if (parent == old) parent = node;
		else {
			if (child != NULL) _setParent(child, parent);
			parent->left = child;
			node->right = old->right;
			_setParent(old->right, node);
		}
		node->unionParentCol = old->unionParentCol;
		node->left = old->left;
		_setParent(old->left, node);
		
		if (old == tree->root) tree->root = NULL;
		kfree(old);

		goto color;
	}
	// replace the NODE with its only child
	parent = _parent(node);
	col = _col(node);
	if (child) _setParent(child, parent);
	if (parent) {
		if (parent->left == node) parent->left = child;
		else parent->right = child;
	} else tree->root = child;

	if (node == tree->root) tree->root = NULL;
	kfree(node);
	
	color:
	if (col == RBTree_Color_Black) _fixAfterDel(tree, child, parent);
}

void RBTree_del(RBTree *tree, u64 val) {
	SpinLock_lock(&tree->lock);
	RBNode *node = _getNode(tree->root, val);
	if (node == NULL) { SpinLock_unlock(&tree->lock); return ; }
	List_del(node->head.next);
	if (!List_isEmpty(&node->head)) {
		SpinLock_unlock(&tree->lock);
		return ;
	}
	_delNode(tree, node);
	SpinLock_unlock(&tree->lock);
}

void RBTree_delNode(RBTree *tree, RBNode *node) {
	SpinLock_lock(&tree->lock);
	_delNode(tree, node);
	SpinLock_unlock(&tree->lock);
}

void _debug(RBNode *node, u64 dep) {
	if (node == NULL) return ;
	for (int i = 0; i < dep; i++) printk(RED, BLACK, "\t");
	printk(WHITE, BLACK, "%ld col: %d [", node->val, _col(node));
	for (List *ele = node->head.next; ele != &node->head; ele = ele->next)
		printk(WHITE, BLACK, " %#018lx", ele);
	printk(WHITE, BLACK, "]\n");
	_debug(node->left, dep + 1);
	_debug(node->right, dep + 1);
}

void RBTree_debug(RBTree *tree) {
	_debug(tree->root, 1);
}