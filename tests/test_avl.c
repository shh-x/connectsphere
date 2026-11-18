#include <assert.h>
#include <stddef.h>
#include "../include/avl.h"

int main(void) {
    AVLNode *root = NULL;
    root = avl_insert(root, "Charlie", 3, 0);
    root = avl_insert(root, "Alice", 1, 0);
    root = avl_insert(root, "Bob", 2, 0);
    assert(avl_search(root, "Bob") != NULL);
    root = avl_delete(root, "Alice", 0);
    assert(avl_search(root, "Alice") == NULL);
    avl_free(root);
    return 0;
}
