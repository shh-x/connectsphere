#ifndef GRAPH_H
#define GRAPH_H

#include "user.h"
#include "mergesort.h"

/* Purpose: Collect immediate friend IDs.
   Parameters: directory, user ID, output IDs/count.
   Return: 1 on success, 0 on invalid user or allocation failure.
   Time complexity: O(degree), O(degree) output space. */
int graph_get_friends(const Directory *directory, int id, int **friendIds,
                      size_t *count);

/* Purpose: Count mutual friends and optionally print mutual IDs.
   Parameters: directory, first, second.
   Return: mutual-friend count.
   Time complexity: O(degree1 + degree2). */
int graph_mutual_count(const Directory *directory, int first, int second);

/* Purpose: Build the ranked community table.
   Parameters: directory, output array, output count.
   Return: 1 on success, 0 on allocation failure.
   Time complexity: O(V + E + G log G). */
int graph_collect_and_rank_groups(Directory *directory,
                                  GroupInfo **groups, size_t *count);

/* Purpose: Collect IDs belonging to a DSU component.
   Parameters: directory, user ID, output IDs/count.
   Return: 1 on success, 0 for absent user or allocation failure.
   Time complexity: O(V alpha(V)), O(V) output space. */
int graph_get_group_members(Directory *directory, int id, int **memberIds,
                            size_t *count);

#endif
