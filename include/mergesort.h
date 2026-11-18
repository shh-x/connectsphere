#ifndef MERGESORT_H
#define MERGESORT_H

#include <stddef.h>
#include "user.h"

typedef struct GroupInfo {
    int rootId;
    int size;
    char representativeName[50];
} GroupInfo;

typedef int (*UserComparator)(const User *a, const User *b);

/* Purpose: Sort an array of users stably using top-down merge sort.
   Parameters: array, count, comparator.
   Return: 1 on success, 0 on allocation failure.
   Time complexity: O(n log n) time, O(n) auxiliary space. */
int merge_sort_users(User *array, size_t count, UserComparator comparator);

/* Purpose: Sort community groups by descending size and ascending root ID.
   Parameters: array, count.
   Return: 1 on success, 0 on allocation failure.
   Time complexity: O(n log n) time, O(n) auxiliary space. */
int merge_sort_groups(GroupInfo *array, size_t count);

/* Purpose: Compare users by age ascending.
   Parameters: two users.
   Return: comparator sign.
   Time complexity: O(1). */
int compare_user_age(const User *a, const User *b);

/* Purpose: Compare users by friend count descending.
   Parameters: two users.
   Return: comparator sign.
   Time complexity: O(1). */
int compare_user_friend_count(const User *a, const User *b);

#endif
