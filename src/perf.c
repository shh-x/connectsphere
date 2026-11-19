#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include "../include/perf.h"
#include "../include/hash.h"
#include "../include/avl.h"
#include "../include/dsu.h"
#include "../include/user.h"
#include "../include/mergesort.h"

static void report_append(char *report, size_t reportSize, const char *format, ...) {
    size_t used = strlen(report);
    va_list args;
    int written;
    if (used >= reportSize) return;
    va_start(args, format);
    written = vsnprintf(report + used, reportSize - used, format, args);
    va_end(args);
    if (written < 0 || (size_t)written >= reportSize - used)
        report[reportSize - 1] = '\0';
}

static double elapsed_seconds(clock_t start) {
    return (double)(clock() - start) / (double)CLOCKS_PER_SEC;
}

static int integer_compare(const void *left, const void *right) {
    int a = *(const int *)left, b = *(const int *)right;
    return (a > b) - (a < b);
}

static void bubble_sort(int *values, size_t count) {
    size_t i, j;
    for (i = 0; i + 1 < count; i++) {
        int swapped = 0;
        for (j = 0; j + 1 < count - i; j++) {
            if (values[j] > values[j + 1]) {
                int temp = values[j];
                values[j] = values[j + 1];
                values[j + 1] = temp;
                swapped = 1;
            }
        }
        if (!swapped) break;
    }
}

static int dsu_depth(const DSU *dsu, int element) {
    int depth = 0;
    while (dsu->parent[element] != element) {
        element = dsu->parent[element];
        depth++;
    }
    return depth;
}

int perf_run_for_size(int n, char *report, size_t reportSize) {
    int i, queries, sortCount;
    HashTable idHash = {NULL, 0, 0, HASH_BY_ID};
    AVLNode *avl = NULL;
    int *linear = NULL, *bubble = NULL, *baseline = NULL;
    User *users = NULL;
    DSU optimized, naive;
    clock_t start;
    double hashTime, avlTime, linearTime, mergeTime, bubbleTime, qsortTime;
    double optimizedTime, naiveTime;
    int optimizedDepth, naiveDepth;
    double logBound = 0.0;
    if (!report || reportSize == 0 || (n != 1000 && n != 10000 && n != 50000))
        return 0;
    report[0] = '\0';
    report_append(report, reportSize,
                  "Performance analysis for N=%d\n"
                  "lookup uses up to 1000 matching queries; sorting uses the same "
                  "dataset capped at 10000 for the quadratic baseline.\n",
                  n);
    linear = (int *)malloc((size_t)n * sizeof(int));
    if (!linear) goto allocation_failure;
    if (!hash_init(&idHash, HASH_BY_ID, (size_t)n)) goto allocation_failure;
    for (i = 0; i < n; i++) {
        char name[32];
        linear[i] = i + 1;
        snprintf(name, sizeof(name), "User-%05d", i + 1);
        avl = avl_insert(avl, name, i + 1, 0);
        if (!avl) goto allocation_failure;
        if (!hash_insert(&idHash, i + 1, name, &linear[i])) goto allocation_failure;
    }
    queries = n < 1000 ? n : 1000;
    start = clock();
    for (i = 0; i < queries; i++) (void)hash_search_id(&idHash, n - i);
    hashTime = elapsed_seconds(start);
    start = clock();
    for (i = 0; i < queries; i++) {
        char name[32];
        snprintf(name, sizeof(name), "User-%05d", n - i);
        (void)avl_search(avl, name);
    }
    avlTime = elapsed_seconds(start);
    start = clock();
    for (i = 0; i < queries; i++) {
        int j, target = n - i;
        for (j = 0; j < n && linear[j] != target; j++) { }
    }
    linearTime = elapsed_seconds(start);
    report_append(report, reportSize,
                  "\nLookup (%d queries): hash=%.6fs AVL=%.6fs linear=%.6fs\n",
                  queries, hashTime, avlTime, linearTime);
    avl_free(avl);
    avl = NULL;
    hash_free(&idHash);
    free(linear);
    linear = NULL;

    sortCount = n < 10000 ? n : 10000;
    bubble = (int *)malloc((size_t)sortCount * sizeof(int));
    baseline = (int *)malloc((size_t)sortCount * sizeof(int));
    users = (User *)calloc((size_t)sortCount, sizeof(User));
    if (!bubble || !baseline || !users) goto allocation_failure;
    for (i = 0; i < sortCount; i++) {
        bubble[i] = baseline[i] = sortCount - i;
        users[i].id = i + 1;
        users[i].age = sortCount - i;
    }
    start = clock();
    if (!merge_sort_users(users, (size_t)sortCount, compare_user_age))
        goto allocation_failure;
    mergeTime = elapsed_seconds(start);
    start = clock();
    bubble_sort(bubble, (size_t)sortCount);
    bubbleTime = elapsed_seconds(start);
    start = clock();
    qsort(baseline, (size_t)sortCount, sizeof(int), integer_compare);
    qsortTime = elapsed_seconds(start);
    report_append(report, reportSize,
                  "Sorting (same %d records): merge=%.6fs bubble=%.6fs qsort baseline=%.6fs\n",
                  sortCount, mergeTime, bubbleTime, qsortTime);
    free(users); users = NULL;
    free(bubble); bubble = NULL;
    free(baseline); baseline = NULL;

    if (!dsu_init(&optimized, n + 1)) goto allocation_failure;
    if (!dsu_init(&naive, n + 1)) {
        dsu_free(&optimized);
        goto allocation_failure;
    }
    start = clock();
    for (i = n - 1; i >= 1; i--) dsu_union(&optimized, i, i + 1);
    optimizedTime = elapsed_seconds(start);
    start = clock();
    for (i = n - 1; i >= 1; i--) dsu_union_naive(&naive, i, i + 1);
    naiveTime = elapsed_seconds(start);
    optimizedDepth = dsu_depth(&optimized, n);
    naiveDepth = dsu_depth(&naive, n);
    report_append(report, reportSize,
                  "Union-Find (%d unions): optimized=%.6fs depth=%d; "
                  "naive=%.6fs depth=%d\n",
                  n - 1, optimizedTime, optimizedDepth, naiveTime, naiveDepth);
    dsu_free(&optimized);
    dsu_free(&naive);

    if (!hash_init(&idHash, HASH_BY_ID, 101)) goto allocation_failure;
    for (i = 0; i < n; i++) {
        char name[32];
        snprintf(name, sizeof(name), "AVL-%05d", i + 1);
        avl = avl_insert(avl, name, i + 1, 0);
        if (!avl) {
            hash_free(&idHash);
            goto allocation_failure;
        }
    }
    for (i = n; i > 1; i /= 2) logBound++;
    report_append(report, reportSize,
                  "AVL height=%d; floor(log2(N))=%.0f; sorted unbalanced BST height=%d "
                  "(theoretical chain, not built quadratically).\n",
                  avl_height(avl), logBound, n);
    avl_free(avl);
    hash_free(&idHash);
    return report[reportSize - 1] == '\0';

allocation_failure:
    avl_free(avl);
    if (idHash.buckets) hash_free(&idHash);
    free(linear);
    free(bubble);
    free(baseline);
    free(users);
    snprintf(report, reportSize, "Performance analysis failed: allocation error.");
    return 0;
}

int perf_run_all(char *report, size_t reportSize) {
    static const int sizes[] = {1000, 10000, 50000};
    char section[8192];
    size_t i;
    if (!report || reportSize == 0) return 0;
    report[0] = '\0';
    for (i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
        if (!perf_run_for_size(sizes[i], section, sizeof(section))) return 0;
        report_append(report, reportSize, "%s%s\n",
                      i ? "\n" : "", section);
    }
    return 1;
}
