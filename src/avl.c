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

int avl_balance_factor(const AVLNode *node) {
    return node ? avl_height(node->left) - avl_height(node->right) : 0;
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

static void record_path(AVLMeta *meta, int userId) {
    if (meta && meta->pathCount < sizeof(meta->path) / sizeof(meta->path[0]))
        meta->path[meta->pathCount++] = userId;
}

static void record_rotation(AVLMeta *meta, const char *rotation) {
    if (meta && meta->rotation[0] == '\0')
        snprintf(meta->rotation, sizeof(meta->rotation), "%s", rotation);
}

AVLNode *avl_insert_ex(AVLNode *root, const char *name, int userId, AVLMeta *meta) {
    int bf;
    if (!root) return new_node(name, userId);
    record_path(meta, root->userId);
    if (name_compare(name, root->name) < 0)
        root->left = avl_insert_ex(root->left, name, userId, meta);
    else if (name_compare(name, root->name) > 0)
        root->right = avl_insert_ex(root->right, name, userId, meta);
    else
        return root;
    update_height(root);
    bf = balance(root);
    if (bf > 1 && name_compare(name, root->left->name) < 0) {
        record_rotation(meta, "LL");
        return rotate_right(root);
    }
    if (bf < -1 && name_compare(name, root->right->name) > 0) {
        record_rotation(meta, "RR");
        return rotate_left(root);
    }
    if (bf > 1 && name_compare(name, root->left->name) > 0) {
        record_rotation(meta, "LR");
        root->left = rotate_left(root->left);
        return rotate_right(root);
    }
    if (bf < -1 && name_compare(name, root->right->name) < 0) {
        record_rotation(meta, "RL");
        root->right = rotate_right(root->right);
        return rotate_left(root);
    }
    return root;
}

AVLNode *avl_insert(AVLNode *root, const char *name, int userId, int verbose) {
    (void)verbose;
    return avl_insert_ex(root, name, userId, NULL);
}

static AVLNode *minimum_node(AVLNode *node) {
    while (node && node->left) node = node->left;
    return node;
}

AVLNode *avl_delete_ex(AVLNode *root, const char *name, AVLMeta *meta) {
    int bf;
    if (!root) return NULL;
    record_path(meta, root->userId);
    if (name_compare(name, root->name) < 0)
        root->left = avl_delete_ex(root->left, name, meta);
    else if (name_compare(name, root->name) > 0)
        root->right = avl_delete_ex(root->right, name, meta);
    else {
        if (!root->left || !root->right) {
            AVLNode *child = root->left ? root->left : root->right;
            free(root);
            return child;
        }
        AVLNode *successor = minimum_node(root->right);
        snprintf(root->name, sizeof(root->name), "%s", successor->name);
        root->userId = successor->userId;
        root->right = avl_delete_ex(root->right, successor->name, meta);
    }
    update_height(root);
    bf = balance(root);
    if (bf > 1 && balance(root->left) >= 0) {
        record_rotation(meta, "LL");
        return rotate_right(root);
    }
    if (bf > 1 && balance(root->left) < 0) {
        record_rotation(meta, "LR");
        root->left = rotate_left(root->left);
        return rotate_right(root);
    }
    if (bf < -1 && balance(root->right) <= 0) {
        record_rotation(meta, "RR");
        return rotate_left(root);
    }
    if (bf < -1 && balance(root->right) > 0) {
        record_rotation(meta, "RL");
        root->right = rotate_right(root->right);
        return rotate_left(root);
    }
    return root;
}

AVLNode *avl_delete(AVLNode *root, const char *name, int verbose) {
    (void)verbose;
    return avl_delete_ex(root, name, NULL);
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

void avl_visit_inorder(const AVLNode *root, AVLVisitFn visit, void *context) {
    if (!root || !visit) return;
    avl_visit_inorder(root->left, visit, context);
    visit(root, context);
    avl_visit_inorder(root->right, visit, context);
}

void avl_visit_preorder(const AVLNode *root, AVLVisitFn visit, void *context) {
    if (!root || !visit) return;
    visit(root, context);
    avl_visit_preorder(root->left, visit, context);
    avl_visit_preorder(root->right, visit, context);
}

int avl_visit_level_order(const AVLNode *root, AVLVisitFn visit, void *context) {
    const AVLNode **queue;
    size_t head = 0, tail = 0, capacity = 64;
    if (!root || !visit) return 1;
    queue = (const AVLNode **)malloc(capacity * sizeof(AVLNode *));
    if (!queue) return 0;
    queue[tail++] = root;
    while (head < tail) {
        const AVLNode *node = queue[head++];
        visit(node, context);
        if (node->left) {
            if (tail >= capacity) {
                const AVLNode **larger =
                    (const AVLNode **)realloc(queue, capacity * 2 * sizeof(AVLNode *));
                if (!larger) { free(queue); return 0; }
                queue = larger;
                capacity *= 2;
            }
            queue[tail++] = node->left;
        }
        if (node->right) {
            if (tail >= capacity) {
                const AVLNode **larger =
                    (const AVLNode **)realloc(queue, capacity * 2 * sizeof(AVLNode *));
                if (!larger) { free(queue); return 0; }
                queue = larger;
                capacity *= 2;
            }
            queue[tail++] = node->right;
        }
    }
    free(queue);
    return 1;
}

void avl_visit_range(const AVLNode *root, const char *lower, const char *upper,
                     AVLVisitFn visit, void *context) {
    if (!root || !visit) return;
    if (name_compare(lower, root->name) < 0)
        avl_visit_range(root->left, lower, upper, visit, context);
    if (name_compare(root->name, lower) >= 0 &&
        name_compare(root->name, upper) <= 0) visit(root, context);
    if (name_compare(upper, root->name) > 0)
        avl_visit_range(root->right, lower, upper, visit, context);
}

void avl_visit_prefix(const AVLNode *root, const char *prefix,
                      AVLVisitFn visit, void *context) {
    if (!root || !visit) return;
    avl_visit_prefix(root->left, prefix, visit, context);
    if (strlen(root->name) >= strlen(prefix)) {
        char sample[51];
        snprintf(sample, sizeof(sample), "%.*s", (int)strlen(prefix), root->name);
        if (name_compare(sample, prefix) == 0)
            visit(root, context);
    }
    avl_visit_prefix(root->right, prefix, visit, context);
}
