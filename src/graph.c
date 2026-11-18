#include <stdio.h>
#include <stdlib.h>
#include "../include/graph.h"
#include "../include/utils.h"

int graph_list_friends(const Directory *directory, int id) {
    FriendNode *node;
    int count = 0;
    if ((size_t)id >= directory->adjacencyCapacity) return 0;
    for (node = directory->adjacency[id]; node; node = node->next) {
        User *friend = directory_find_id((Directory *)directory, node->userId);
        if (friend) {
            printf("- %s (ID %d)\n", friend->name, friend->id);
            count++;
        }
    }
    return count;
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

void graph_list_all_groups(Directory *directory) {
    GroupInfo *groups;
    size_t count, i;
    if (!graph_collect_and_rank_groups(directory, &groups, &count)) {
        printf("Unable to collect groups.\n");
        return;
    }
    for (i = 0; i < count; i++) {
        int memberCount = 0, id;
        printf("%zu. Root %d, size %d, representative %s\n",
               i + 1, groups[i].rootId, groups[i].size,
               groups[i].representativeName);
        printf("   Members:");
        for (id = 1; id < directory->dsu.capacity; id++) {
            if (directory_find_id(directory, id) &&
                dsu_find(&directory->dsu, id) == groups[i].rootId) {
                User *user = directory_find_id(directory, id);
                printf(" %s", user->name);
                memberCount++;
                if (memberCount < groups[i].size) printf(",");
            }
        }
        printf("\n");
    }
    free(groups);
}
