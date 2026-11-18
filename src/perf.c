#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../include/perf.h"
#include "../include/hash.h"
#include "../include/avl.h"
#include "../include/dsu.h"
#include "../include/user.h"
#include "../include/mergesort.h"

typedef struct BSTNode { int value; struct BSTNode *left, *right; } BSTNode;

static BSTNode *bst_insert(BSTNode *root, int value) {
    if (!root) {
        root = (BSTNode *)malloc(sizeof(BSTNode));
        if (!root) return NULL;
        root->value = value;
        root->left = root->right = NULL;
    } else if (value < root->value)
        root->left = bst_insert(root->left, value);
    else
        root->right = bst_insert(root->right, value);
    return root;
}

static void bst_free(BSTNode *root) {
    if (!root) return;
    bst_free(root->left); bst_free(root->right); free(root);
}

static int bst_height(BSTNode *root) {
    int lh, rh;
    if (!root) return 0;
    lh = bst_height(root->left);
    rh = bst_height(root->right);
    return 1 + (lh > rh ? lh : rh);
}

static void bubble_sort(int *array, size_t count) {
    size_t i, j;
    for (i = 0; i < count; i++)
        for (j = 0; j + 1 < count - i; j++)
            if (array[j] > array[j + 1]) {
                int temp = array[j];
                array[j] = array[j + 1];
                array[j + 1] = temp;
            }
}

static int int_compare(const void *a, const void *b) {
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

static double elapsed_seconds(clock_t start) {
    return (double)(clock() - start) / (double)CLOCKS_PER_SEC;
}

static int max_depth_dsu(const DSU *dsu) {
    int i, maxDepth = 0;
    for (i = 1; i < dsu->capacity; i++) {
        int depth = 0, x = i;
        while (dsu->parent[x] != x) { x = dsu->parent[x]; depth++; }
        if (depth > maxDepth) maxDepth = depth;
    }
    return maxDepth;
}

void perf_run_all(void) {
    static const int sizes[] = {1000, 10000, 50000};
    size_t s;
    printf("=== ConnectSphere Performance Analysis ===\n");
    printf("Lookup (hash / AVL / linear) seconds for synthetic names/IDs\n");
    printf("N       Hash-avg   AVL        Linear\n");
    for (s = 0; s < 3; s++) {
        int n = sizes[s], i;
        HashTable idHash;
        AVLNode *root = NULL;
        int *linear = (int *)malloc((size_t)n * sizeof(int));
        clock_t start;
        double hashTime, avlTime, linearTime;
        if (!linear) continue;
        for (i = 0; i < n; i++) linear[i] = i + 1;
        hash_init(&idHash, HASH_BY_ID, (size_t)n);
        for (i = 0; i < n; i++) hash_insert(&idHash, i + 1, "Synthetic", &linear[i]);
        for (i = 0; i < n; i++) {
            char name[20];
            snprintf(name, sizeof(name), "User%d", i);
            root = avl_insert(root, name, i + 1, 0);
        }
        start = clock();
        for (i = 0; i < n; i++) (void)hash_search_id(&idHash, n - i);
        hashTime = elapsed_seconds(start);
        start = clock();
        for (i = 0; i < n; i++) {
            char name[20];
            snprintf(name, sizeof(name), "User%d", n - i - 1);
            (void)avl_search(root, name);
        }
        avlTime = elapsed_seconds(start);
        start = clock();
        for (i = 0; i < n; i++) {
            int target = n - i;
            int j;
            for (j = 0; j < n; j++) if (linear[j] == target) break;
        }
        linearTime = elapsed_seconds(start);
        printf("%-7d %-10.6f %-10.6f %-10.6f\n", n, hashTime, avlTime, linearTime);
        avl_free(root);
        hash_free(&idHash);
        free(linear);
    }

    printf("\nSorting (merge / bubble / qsort) seconds\n");
    printf("N       Merge      Bubble     qsort\n");
    {
        static const int sortSizes[] = {1000, 10000};
        size_t si;
        for (si = 0; si < 2; si++) {
            int n = sortSizes[si], i;
            int *mergeArray = (int *)malloc((size_t)n * sizeof(int));
            int *bubbleArray = (int *)malloc((size_t)n * sizeof(int));
            int *qsortArray = (int *)malloc((size_t)n * sizeof(int));
            User *users = (User *)malloc((size_t)n * sizeof(User));
            clock_t start;
            double mergeTime, bubbleTime, qsortTime;
            if (!mergeArray || !bubbleArray || !qsortArray || !users) {
                free(mergeArray); free(bubbleArray); free(qsortArray); free(users);
                continue;
            }
            for (i = 0; i < n; i++) {
                mergeArray[i] = bubbleArray[i] = qsortArray[i] = n - i;
                users[i].id = i + 1;
                users[i].age = n - i;
            }
            start = clock();
            merge_sort_users(users, (size_t)n, compare_user_age);
            mergeTime = elapsed_seconds(start);
            start = clock();
            bubble_sort(bubbleArray, (size_t)n);
            bubbleTime = elapsed_seconds(start);
            start = clock();
            qsort(qsortArray, (size_t)n, sizeof(int), int_compare);
            qsortTime = elapsed_seconds(start);
            printf("%-7d %-10.6f %-10.6f %-10.6f\n", n,
                   mergeTime, bubbleTime, qsortTime);
            free(mergeArray); free(bubbleArray); free(qsortArray); free(users);
        }
    }

    printf("\nUnion-Find optimized vs naive\n");
    printf("N       Optimized depth  Naive depth  Optimized time  Naive time\n");
    {
        static const int unionSizes[] = {1000, 10000, 50000};
        size_t ui;
        for (ui = 0; ui < 3; ui++) {
            int n = unionSizes[ui], i;
            DSU optimized, naive;
            clock_t start;
            double optimizedTime, naiveTime;
            dsu_init(&optimized, n + 1);
            dsu_init(&naive, n + 1);
            start = clock();
            for (i = 1; i < n; i++) dsu_union(&optimized, i, i + 1);
            optimizedTime = elapsed_seconds(start);
            start = clock();
            for (i = 1; i < n; i++) dsu_union_naive(&naive, i, i + 1);
            naiveTime = elapsed_seconds(start);
            printf("%-7d %-16d %-12d %-15.6f %.6f\n", n,
                   max_depth_dsu(&optimized), max_depth_dsu(&naive),
                   optimizedTime, naiveTime);
            dsu_free(&optimized); dsu_free(&naive);
        }
    }

    printf("\nAVL height vs log2(n) and sorted-input unbalanced BST\n");
    printf("N       AVL height  log2 bound  BST height\n");
    {
        static const int treeSizes[] = {1000, 10000, 50000};
        size_t ti;
        for (ti = 0; ti < 3; ti++) {
            int n = treeSizes[ti], i;
            AVLNode *root = NULL;
            BSTNode *bst = NULL;
            char name[20];
            for (i = 0; i < n; i++) {
                snprintf(name, sizeof(name), "Name%d", i);
                root = avl_insert(root, name, i + 1, 0);
            }
            for (i = 0; i < n; i++) bst = bst_insert(bst, i);
            {
                double bound = 0.0;
                int x;
                for (x = n; x > 1; x /= 2) bound++;
                printf("%-7d %-11d %-11.2f %-11d\n", n,
                       avl_height(root), bound, bst_height(bst));
            }
            avl_free(root); bst_free(bst);
        }
    }
    printf("\nConclusion: Hash and AVL lookup remain practical; linear search degrades\n");
    printf("quadratically for repeated searches. Merge sort is predictable O(n log n),\n");
    printf("while bubble sorting is poor. Union optimizations reduce tree depth and\n");
    printf("AVL height remains logarithmic where a sorted unbalanced BST degenerates.\n");
}
