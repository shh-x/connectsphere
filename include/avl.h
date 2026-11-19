#ifndef AVL_H
#define AVL_H

#include <stddef.h>

typedef struct AVLNode {
    char name[50];
    int userId;
    int height;
    struct AVLNode *left;
    struct AVLNode *right;
} AVLNode;

typedef struct {
    char rotation[3];
    int path[64];
    size_t pathCount;
} AVLMeta;

typedef void (*AVLVisitFn)(const AVLNode *node, void *context);

/* Purpose: Return node height or 0 for NULL.
   Parameters: node.
   Return: height.
   Time complexity: O(1). */
int avl_height(AVLNode *node);

/* Purpose: Insert a user into the AVL tree.
   Parameters: root, name, ID, verbose toggle.
   Return: new root.
   Time complexity: O(log n). */
AVLNode *avl_insert(AVLNode *root, const char *name, int userId, int verbose);
AVLNode *avl_insert_ex(AVLNode *root, const char *name, int userId, AVLMeta *meta);

/* Purpose: Delete a user by name.
   Parameters: root, name, verbose.
   Return: new root.
   Time complexity: O(log n). */
AVLNode *avl_delete(AVLNode *root, const char *name, int verbose);
AVLNode *avl_delete_ex(AVLNode *root, const char *name, AVLMeta *meta);

/* Purpose: Search for a name.
   Parameters: root, name.
   Return: matching node or NULL.
   Time complexity: O(log n). */
AVLNode *avl_search(AVLNode *root, const char *name);

/* Purpose: Free the complete tree.
   Parameters: root.
   Return: void.
   Time complexity: O(n). */
void avl_free(AVLNode *root);

/* Purpose: Visit nodes in sorted inorder order.
   Parameters: root, callback, callback context.
   Return: void.
   Time complexity: O(n). */
void avl_visit_inorder(const AVLNode *root, AVLVisitFn visit, void *context);

/* Purpose: Visit nodes in preorder order.
   Parameters: root, callback, callback context.
   Return: void.
   Time complexity: O(n). */
void avl_visit_preorder(const AVLNode *root, AVLVisitFn visit, void *context);

/* Purpose: Visit nodes in breadth-first order.
   Parameters: root, callback, callback context.
   Return: 1 on success, 0 on allocation failure.
   Time complexity: O(n). */
int avl_visit_level_order(const AVLNode *root, AVLVisitFn visit, void *context);

/* Purpose: Visit nodes in an inclusive case-insensitive name range.
   Parameters: root, lower/upper bounds, callback, context.
   Return: void.
   Time complexity: O(log n + k) for balanced trees. */
void avl_visit_range(const AVLNode *root, const char *lower, const char *upper,
                     AVLVisitFn visit, void *context);

/* Purpose: Visit all names with the given case-insensitive prefix.
   Parameters: root, prefix, callback, context.
   Return: void.
   Time complexity: O(n). */
void avl_visit_prefix(const AVLNode *root, const char *prefix,
                      AVLVisitFn visit, void *context);

/* Purpose: Return a node's balance factor.
   Parameters: node.
   Return: left height minus right height.
   Time complexity: O(1). */
int avl_balance_factor(const AVLNode *node);

#endif
