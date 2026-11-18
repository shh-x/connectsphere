#ifndef AVL_H
#define AVL_H

typedef struct AVLNode {
    char name[50];
    int userId;
    int height;
    struct AVLNode *left;
    struct AVLNode *right;
} AVLNode;

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

/* Purpose: Delete a user by name.
   Parameters: root, name, verbose.
   Return: new root.
   Time complexity: O(log n). */
AVLNode *avl_delete(AVLNode *root, const char *name, int verbose);

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

/* Purpose: Print tree traversals.
   Parameters: root.
   Return: void.
   Time complexity: O(n). */
void avl_inorder(AVLNode *root);
void avl_preorder(AVLNode *root);
void avl_level_order(AVLNode *root);

/* Purpose: Print names in an inclusive range.
   Parameters: root, lower, upper.
   Return: void.
   Time complexity: O(n) worst, output-sensitive average. */
void avl_range(AVLNode *root, const char *lower, const char *upper);

/* Purpose: Print names with a prefix.
   Parameters: root, prefix.
   Return: void.
   Time complexity: O(n). */
void avl_prefix(AVLNode *root, const char *prefix);

/* Purpose: Print balance factors and tree shape.
   Parameters: root.
   Return: void.
   Time complexity: O(n). */
void avl_display(AVLNode *root);

#endif
