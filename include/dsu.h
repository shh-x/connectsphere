#ifndef DSU_H
#define DSU_H

#include "hash.h"

typedef struct DSU {
    int *parent;
    int *rank;
    int *size;
    int capacity;
} DSU;

/* Purpose: Initialize/resize arrays for positive IDs.
   Parameters: DSU, maximum ID capacity.
   Return: 1 on success, 0 on failure.
   Time complexity: O(capacity). */
int dsu_init(DSU *dsu, int capacity);

/* Purpose: Resize DSU storage to a larger maximum ID.
   Parameters: DSU, new capacity.
   Return: 1 on success, 0 on failure.
   Time complexity: O(old + new capacity). */
int dsu_resize(DSU *dsu, int newCapacity);

/* Purpose: Free DSU arrays.
   Parameters: DSU.
   Return: void.
   Time complexity: O(1). */
void dsu_free(DSU *dsu);

/* Purpose: Reset every existing element as its own group.
   Parameters: DSU.
   Return: void.
   Time complexity: O(capacity). */
void dsu_reset(DSU *dsu);

/* Purpose: Find the root with path compression.
   Parameters: DSU, element.
   Return: root.
   Time complexity: Near O(alpha(n)); without compression O(n). */
int dsu_find(DSU *dsu, int element);

/* Purpose: Find a root and record the path before compression.
   Parameters: DSU, element, path buffer, buffer capacity, path length output.
   Return: root or -1 for invalid elements.
   Time complexity: Near O(alpha(n)) with compression. */
int dsu_find_trace(DSU *dsu, int element, int *path,
                   int pathCapacity, int *pathLength);

/* Purpose: Find the root without path compression.
   Parameters: DSU, element.
   Return: root.
   Time complexity: O(n) worst. */
int dsu_find_no_compression(const DSU *dsu, int element);

/* Purpose: Merge two sets using union by rank.
   Parameters: DSU, first, second.
   Return: root or -1 for invalid elements.
   Time complexity: Near O(alpha(n)); without rank O(n). */
int dsu_union(DSU *dsu, int first, int second);

/* Purpose: Merge sets without rank or compression.
   Parameters: DSU, first, second.
   Return: root or -1.
   Time complexity: O(n) worst. */
int dsu_union_naive(DSU *dsu, int first, int second);

/* Purpose: Return the number of elements in the root component.
   Parameters: DSU, element.
   Return: component size.
   Time complexity: Near O(alpha(n)). */
int dsu_group_size(DSU *dsu, int element);

/* Purpose: Count current distinct components.
   Parameters: DSU.
   Return: component count.
   Time complexity: O(capacity). */
int dsu_count_groups(const DSU *dsu);

#endif
