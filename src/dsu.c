#include <stdlib.h>
#include "../include/dsu.h"

int dsu_init(DSU *dsu, int capacity) {
    int i;
    if (!dsu || capacity < 1) return 0;
    dsu->parent = (int *)malloc((size_t)capacity * sizeof(int));
    dsu->rank = (int *)calloc((size_t)capacity, sizeof(int));
    dsu->size = (int *)malloc((size_t)capacity * sizeof(int));
    dsu->capacity = capacity;
    if (!dsu->parent || !dsu->rank || !dsu->size) {
        free(dsu->parent); free(dsu->rank); free(dsu->size);
        dsu->parent = dsu->rank = dsu->size = NULL;
        dsu->capacity = 0;
        return 0;
    }
    for (i = 0; i < capacity; i++) {
        dsu->parent[i] = i;
        dsu->rank[i] = 0;
        dsu->size[i] = i == 0 ? 0 : 1;
    }
    return 1;
}

int dsu_resize(DSU *dsu, int newCapacity) {
    int *parent, *rank, *size;
    int i;
    if (!dsu || newCapacity <= dsu->capacity) return dsu && dsu->capacity >= newCapacity;
    parent = (int *)realloc(dsu->parent, (size_t)newCapacity * sizeof(int));
    if (!parent) return 0;
    dsu->parent = parent;
    rank = (int *)realloc(dsu->rank, (size_t)newCapacity * sizeof(int));
    if (!rank) return 0;
    dsu->rank = rank;
    size = (int *)realloc(dsu->size, (size_t)newCapacity * sizeof(int));
    if (!size) return 0;
    dsu->size = size;
    for (i = dsu->capacity; i < newCapacity; i++) {
        dsu->parent[i] = i;
        dsu->rank[i] = 0;
        dsu->size[i] = 1;
    }
    dsu->capacity = newCapacity;
    return 1;
}

void dsu_free(DSU *dsu) {
    if (!dsu) return;
    free(dsu->parent); free(dsu->rank); free(dsu->size);
    dsu->parent = dsu->rank = dsu->size = NULL;
    dsu->capacity = 0;
}

void dsu_reset(DSU *dsu) {
    int i;
    if (!dsu) return;
    for (i = 0; i < dsu->capacity; i++) {
        dsu->parent[i] = i;
        dsu->rank[i] = 0;
        dsu->size[i] = i == 0 ? 0 : 1;
    }
}

int dsu_find(DSU *dsu, int element) {
    int root;
    if (!dsu || element <= 0 || element >= dsu->capacity) return -1;
    root = element;
    while (dsu->parent[root] != root) root = dsu->parent[root];
    while (element != root) {
        int next = dsu->parent[element];
        dsu->parent[element] = root;
        element = next;
    }
    return root;
}

int dsu_find_no_compression(const DSU *dsu, int element) {
    if (!dsu || element <= 0 || element >= dsu->capacity) return -1;
    while (dsu->parent[element] != element) element = dsu->parent[element];
    return element;
}

int dsu_union(DSU *dsu, int first, int second) {
    int ra = dsu_find(dsu, first);
    int rb = dsu_find(dsu, second);
    if (ra < 0 || rb < 0) return -1;
    if (ra == rb) return ra;
    if (dsu->rank[ra] < dsu->rank[rb]) { int t = ra; ra = rb; rb = t; }
    dsu->parent[rb] = ra;
    dsu->size[ra] += dsu->size[rb];
    dsu->size[rb] = 0;
    if (dsu->rank[ra] == dsu->rank[rb]) dsu->rank[ra]++;
    return ra;
}

int dsu_union_naive(DSU *dsu, int first, int second) {
    int ra = dsu_find_no_compression(dsu, first);
    int rb = dsu_find_no_compression(dsu, second);
    if (ra < 0 || rb < 0) return -1;
    if (ra == rb) return ra;
    dsu->parent[rb] = ra;
    dsu->size[ra] += dsu->size[rb];
    dsu->size[rb] = 0;
    return ra;
}

int dsu_group_size(DSU *dsu, int element) {
    int root = dsu_find(dsu, element);
    if (root < 0) return 0;
    return dsu->size[root];
}

int dsu_count_groups(const DSU *dsu) {
    int i, count = 0;
    if (!dsu) return 0;
    for (i = 1; i < dsu->capacity; i++)
        if (dsu->parent[i] == i && dsu->size[i] > 0) count++;
    return count;
}
