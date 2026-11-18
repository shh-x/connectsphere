#ifndef GRAPH_H
#define GRAPH_H

#include "user.h"
#include "mergesort.h"

/* Purpose: List a user's immediate friends.
   Parameters: directory, user ID.
   Return: number printed.
   Time complexity: O(degree). */
int graph_list_friends(const Directory *directory, int id);

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

/* Purpose: Print all distinct friend groups.
   Parameters: directory.
   Return: void.
   Time complexity: O(V + E). */
void graph_list_all_groups(Directory *directory);

#endif
