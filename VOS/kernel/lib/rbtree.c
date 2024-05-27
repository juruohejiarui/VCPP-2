#include "rbtree.h"
#include "memop.h"

typedef struct _tmpNode {
	struct _tmpNode *left, *right;
	u64 unionParentCol;
	i64 val;
	List head;
} __attribute__ ((aligned(sizeof(long)))) _Node;

#define _parent(node) ((_Node *)((node)->unionParentCol & ~3))
#define _col(node) ((node)->unionParentCol & 3)
#define _isRed(node) (!_col(node))
#define _isBlack(node) (!_col(node))
#define _setRed(node) do { (node)->unionParentCol &= ~1; } while (0)
#define _setBlack(node) do { (node)->unionParentCol |= 1; } while (0)

static inline void _setParent(_Node *node, _Node *parent) {
	node->unionParentCol = (node->unionParentCol & 3) | (u64)parent;
}
static inline void _setCol(_Node *node, int col) {
	node->unionParentCol = (node->unionParentCol & ~3) | col;
}

static _Node *_newNode(i64 val, _Node *parent) {
	_Node *node = (_Node *)kmalloc(sizeof(_Node), 0);
	memset(node, 0, sizeof(_Node));
	List_init(&node->head);
	node->val = val;
	node->unionParentCol = (u64)parent | 2;
}
void RBTree_init(RBTree *tree) { tree->root = NULL; }

List *RBTree_get(RBTree *tree, i64 val) {
	if (tree == NULL || tree->root == NULL) return NULL;
	_Node *cur = tree->root;
	while (cur != NULL && cur->val != val)
		if (cur->val < val) cur = cur->right;
		else cur = cur->left;
	return cur == NULL ? NULL : &cur->head;
}

static List *_getMin(_Node *node) {
	while (node->left != NULL) node = node->left;
	return &node->head;
}

List *RBTree_getMin(RBTree *tree) {
	return tree == NULL || tree->root == NULL ? NULL : _getMin(tree->root);
}

static List *_getMax(_Node *node) {
	while (node->right != NULL) node = node->right;
	return &node->head;
}

List *RBTree_getMax(RBTree *tree) {
	return tree == NULL || tree->root == NULL ? NULL : _getMax(tree->root);
}

static void _rotLeft(RBTree *tree, _Node *node) {
	_Node *r = node->right, *parent = _parent(node);
	if ((node->right = r->left) != NULL) _setParent(r->left, node);
	r->left = node;
	_setParent(r, parent);
	if (parent != NULL) {
		if (node == parent->left) parent->left = r;
		else parent->right = r;
	} else tree->root = r;
	_setParent(node, r);
}

static void _rotRight(RBTree *tree, _Node *node) {
	_Node *l = node->left, *parent = _parent(node);
	if ((node->left = l->right) != NULL) _setParent(l->right, node);
	l->right = node;
	_setParent(l, parent);
	if (parent != NULL) {
		if (node == parent->right) parent->right = l;
		else parent->left = l;
	} else tree->root = l;
	_setParent(node, l);
}

static void _fixAfterIns(RBTree *tree, _Node *node) {
	_Node *parent, *gParent;
	while ((parent = _parent(node)) != NULL && _isRed(parent)) {
		gParent = _parent(parent);
		if (parent == gParent->left) {
			
		}
	}
}

void RBTree_insert(RBTree *tree, i64 val, List *listEle) {
	if (tree->root == NULL) {
		tree->root = _newNode(val, NULL);
		_setBlack((_Node *)tree->root);
		List_insBehind(listEle, &((_Node *)tree->root)->head);
		return ;
	}
	_Node *cur = (_Node *)tree->root;
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
	if (_col(cur) != 2) return ;
	_setRed(cur);
	_fixAfterIns(tree, cur);
}