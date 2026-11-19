#include <stdio.h>
#include <stdlib.h>
#include "../include/graph.h"
#include "../include/utils.h"

int graph_get_friends(const Directory *directory, int id, int **friendIds,
                      size_t *count) {
    FriendNode *node;
    size_t found = 0, capacity = 0;
    int *result = NULL;
    if (!directory || !friendIds || !count || id <= 0 ||
        (size_t)id >= directory->adjacencyCapacity ||
        !directory_find_id((Directory *)directory, id)) return 0;
    *friendIds = NULL;
    *count = 0;
    for (node = directory->adjacency[id]; node; node = node->next) {
        if (directory_find_id((Directory *)directory, node->userId)) {
            if (found == capacity) {
                size_t nextCapacity = capacity ? capacity * 2 : 8;
                int *larger = (int *)realloc(result, nextCapacity * sizeof(int));
                if (!larger) { free(result); return 0; }
                result = larger;
                capacity = nextCapacity;
            }
            result[found++] = node->userId;
        }
    }
    *friendIds = result;
    *count = found;
    return 1;
}

int graph_mutual_count(const Directory *directory, int first, int second) {
    FriendNode *node;
    int count = 0;
    if ((size_t)first >= directory->adjacencyCapacity ||
        (size_t)second >= directory->adjacencyCapacity) return 0;
    for (node = directory->adjacency[first]; node; node = node->next) {
        FriendNode *candidate = directory->adjacency[second];
        while (candidate && candidate->userId != node->userId) candidate = candidate->next;
        if (candidate) count++;
    }
    return count;
}

int graph_collect_and_rank_groups(Directory *directory,
                                  GroupInfo **groups, size_t *count) {
    int i;
    size_t found = 0;
    GroupInfo *result;
    *count = 0;
    *groups = NULL;
    result = (GroupInfo *)calloc(directory->dsu.capacity, sizeof(GroupInfo));
    if (!result) return 0;
    for (i = 1; i < directory->dsu.capacity; i++) {
        int root;
        if (!directory_find_id(directory, i)) continue;
        root = dsu_find(&directory->dsu, i);
        if (root != i) continue;
        result[found].rootId = root;
        result[found].size = dsu_group_size(&directory->dsu, i);
        {
            User *representative = directory_find_id(directory, i);
            snprintf(result[found].representativeName,
                     sizeof(result[found].representativeName),
                     "%s", representative ? representative->name : "");
        }
        found++;
    }
    if (found && !merge_sort_groups(result, found)) {
        free(result);
        return 0;
    }
    *groups = result;
    *count = found;
    return 1;
}

int graph_get_group_members(Directory *directory, int id, int **memberIds,
                            size_t *count) {
    int root, current;
    size_t found = 0;
    int *members;
    if (!directory || !memberIds || !count || !directory_find_id(directory, id))
        return 0;
    *memberIds = NULL;
    *count = 0;
    members = (int *)malloc(directory->userCount * sizeof(int));
    if (directory->userCount && !members) return 0;
    root = dsu_find(&directory->dsu, id);
    for (current = 1; current < directory->dsu.capacity; current++)
        if (directory_find_id(directory, current) &&
            dsu_find(&directory->dsu, current) == root)
            members[found++] = current;
    *memberIds = members;
    *count = found;
    return 1;
}
