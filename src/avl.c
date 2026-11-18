#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/avl.h"
#include "../include/utils.h"

int avl_height(AVLNode *node) { return node ? node->height : 0; }

static AVLNode *new_node(const char *name, int userId) {
    AVLNode *node = (AVLNode *)malloc(sizeof(AVLNode));
    if (!node) return NULL;
    snprintf(node->name, sizeof(node->name), "%s", name);
    node->userId = userId;
    node->height = 1;
    node->left = node->right = NULL;
    return node;
}

static void update_height(AVLNode *node) {
    int lh = avl_height(node->left), rh = avl_height(node->right);
    node->height = (lh > rh ? lh : rh) + 1;
}

static int balance(AVLNode *node) {
    return avl_height(node->left) - avl_height(node->right);
}

static AVLNode *rotate_left(AVLNode *x) {
    AVLNode *y = x->right;
    AVLNode *t = y->left;
    y->left = x;
    x->right = t;
    update_height(x);
    update_height(y);
    return y;
}

static AVLNode *rotate_right(AVLNode *y) {
    AVLNode *x = y->left;
    AVLNode *t = x->right;
    x->right = y;
    y->left = t;
    update_height(y);
    update_height(x);
    return x;
}

AVLNode *avl_insert(AVLNode *root, const char *name, int userId, int verbose) {
    int bf;
    if (!root) return new_node(name, userId);
    if (name_compare(name, root->name) < 0)
        root->left = avl_insert(root->left, name, userId, verbose);
    else if (name_compare(name, root->name) > 0)
        root->right = avl_insert(root->right, name, userId, verbose);
    else
        return root;
    update_height(root);
    bf = balance(root);
    if (bf > 1 && name_compare(name, root->left->name) < 0) {
        if (verbose) printf("Rotation: LL on %s\n", root->name);
        return rotate_right(root);
    }
    if (bf < -1 && name_compare(name, root->right->name) > 0) {
        if (verbose) printf("Rotation: RR on %s\n", root->name);
        return rotate_left(root);
    }
    if (bf > 1 && name_compare(name, root->left->name) > 0) {
        if (verbose) printf("Rotation: LR on %s\n", root->name);
        root->left = rotate_left(root->left);
        return rotate_right(root);
    }
    if (bf < -1 && name_compare(name, root->right->name) < 0) {
        if (verbose) printf("Rotation: RL on %s\n", root->name);
        root->right = rotate_right(root->right);
        return rotate_left(root);
    }
    return root;
}

static AVLNode *minimum_node(AVLNode *node) {
    while (node && node->left) node = node->left;
    return node;
}

AVLNode *avl_delete(AVLNode *root, const char *name, int verbose) {
    int bf;
    if (!root) return NULL;
    if (name_compare(name, root->name) < 0)
        root->left = avl_delete(root->left, name, verbose);
    else if (name_compare(name, root->name) > 0)
        root->right = avl_delete(root->right, name, verbose);
    else {
        if (!root->left || !root->right) {
            AVLNode *child = root->left ? root->left : root->right;
            free(root);
            return child;
        }
        AVLNode *successor = minimum_node(root->right);
        snprintf(root->name, sizeof(root->name), "%s", successor->name);
        root->userId = successor->userId;
        root->right = avl_delete(root->right, successor->name, verbose);
    }
    update_height(root);
    bf = balance(root);
    if (bf > 1 && balance(root->left) >= 0) {
        if (verbose) printf("Rotation: LL on delete at %s\n", root->name);
        return rotate_right(root);
    }
    if (bf > 1 && balance(root->left) < 0) {
        if (verbose) printf("Rotation: LR on delete at %s\n", root->name);
        root->left = rotate_left(root->left);
        return rotate_right(root);
    }
    if (bf < -1 && balance(root->right) <= 0) {
        if (verbose) printf("Rotation: RR on delete at %s\n", root->name);
        return rotate_left(root);
    }
    if (bf < -1 && balance(root->right) > 0) {
        if (verbose) printf("Rotation: RL on delete at %s\n", root->name);
        root->right = rotate_right(root->right);
        return rotate_left(root);
    }
    return root;
}

AVLNode *avl_search(AVLNode *root, const char *name) {
    while (root) {
        int cmp = name_compare(name, root->name);
        if (cmp == 0) return root;
        root = cmp < 0 ? root->left : root->right;
    }
    return NULL;
}

void avl_free(AVLNode *root) {
    if (!root) return;
    avl_free(root->left);
    avl_free(root->right);
    free(root);
}

void avl_inorder(AVLNode *root) {
    if (!root) return;
    avl_inorder(root->left);
    printf("%s (ID %d)\n", root->name, root->userId);
    avl_inorder(root->right);
}

void avl_preorder(AVLNode *root) {
    if (!root) return;
    printf("%s (ID %d, h=%d, bf=%d)\n", root->name, root->userId,
           root->height, balance(root));
    avl_preorder(root->left);
    avl_preorder(root->right);
}

void avl_level_order(AVLNode *root) {
    AVLNode **queue;
    size_t head = 0, tail = 0, capacity = 64;
    if (!root) return;
    queue = (AVLNode **)calloc(capacity, sizeof(AVLNode *));
    if (!queue) return;
    queue[tail++] = root;
    while (head < tail) {
        AVLNode *node = queue[head++];
        printf("%s (ID %d, h=%d)\n", node->name, node->userId, node->height);
        if (node->left) {
            if (tail >= capacity) {
                size_t i;
                AVLNode **larger = (AVLNode **)realloc(queue, capacity * 2 * sizeof(AVLNode *));
                if (!larger) break;
                queue = larger;
                for (i = capacity; i < capacity * 2; i++) queue[i] = NULL;
                capacity *= 2;
            }
            queue[tail++] = node->left;
        }
        if (node->right) {
            if (tail >= capacity) {
                size_t i;
                AVLNode **larger = (AVLNode **)realloc(queue, capacity * 2 * sizeof(AVLNode *));
                if (!larger) break;
                queue = larger;
                for (i = capacity; i < capacity * 2; i++) queue[i] = NULL;
                capacity *= 2;
            }
            queue[tail++] = node->right;
        }
    }
    free(queue);
}

void avl_range(AVLNode *root, const char *lower, const char *upper) {
    if (!root) return;
    if (name_compare(lower, root->name) < 0) avl_range(root->left, lower, upper);
    if (name_compare(root->name, lower) >= 0 &&
        name_compare(root->name, upper) <= 0)
        printf("%s (ID %d)\n", root->name, root->userId);
    if (name_compare(upper, root->name) > 0) avl_range(root->right, lower, upper);
}

void avl_prefix(AVLNode *root, const char *prefix) {
    if (!root) return;
    avl_prefix(root->left, prefix);
    if (strlen(root->name) >= strlen(prefix)) {
        char sample[51];
        snprintf(sample, sizeof(sample), "%.*s", (int)strlen(prefix), root->name);
        if (name_compare(sample, prefix) == 0)
        printf("%s (ID %d)\n", root->name, root->userId);
    }
    avl_prefix(root->right, prefix);
}

void avl_display(AVLNode *root) {
    if (!root) return;
    avl_display(root->left);
    printf("%s (ID %d): height=%d, balance=%d\n",
           root->name, root->userId, root->height, balance(root));
    avl_display(root->right);
}
