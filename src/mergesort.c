#include <stdlib.h>
#include "../include/mergesort.h"

static void merge_users(User *array, size_t left, size_t middle, size_t right,
                        User *temporary, UserComparator comparator) {
    size_t i = left, j = middle, k = left;
    while (i < middle && j < right) {
        if (comparator(&array[i], &array[j]) <= 0)
            temporary[k++] = array[i++];
        else
            temporary[k++] = array[j++];
    }
    while (i < middle) temporary[k++] = array[i++];
    while (j < right) temporary[k++] = array[j++];
    for (i = left; i < right; i++) array[i] = temporary[i];
}

static int sort_users_range(User *array, size_t left, size_t right,
                            User *temporary, UserComparator comparator) {
    if (right - left > 1) {
        size_t middle = left + (right - left) / 2;
        if (!sort_users_range(array, left, middle, temporary, comparator)) return 0;
        if (!sort_users_range(array, middle, right, temporary, comparator)) return 0;
        merge_users(array, left, middle, right, temporary, comparator);
    }
    return 1;
}

int merge_sort_users(User *array, size_t count, UserComparator comparator) {
    User *temporary;
    if (count < 2) return 1;
    temporary = (User *)malloc(count * sizeof(User));
    if (!temporary) return 0;
    sort_users_range(array, 0, count, temporary, comparator);
    free(temporary);
    return 1;
}

int compare_user_age(const User *a, const User *b) {
    if (a->age != b->age) return a->age < b->age ? -1 : 1;
    return a->id < b->id ? -1 : (a->id > b->id);
}

int compare_user_friend_count(const User *a, const User *b) {
    if (a->friendCount != b->friendCount)
        return a->friendCount > b->friendCount ? -1 : 1;
    return a->id < b->id ? -1 : (a->id > b->id);
}

static int group_better(const GroupInfo *a, const GroupInfo *b) {
    if (a->size != b->size) return a->size > b->size;
    return a->rootId < b->rootId;
}

static void merge_groups(GroupInfo *array, size_t left, size_t middle, size_t right,
                         GroupInfo *temporary) {
    size_t i = left, j = middle, k = left;
    while (i < middle && j < right) {
        if (group_better(&array[i], &array[j]))
            temporary[k++] = array[i++];
        else
            temporary[k++] = array[j++];
    }
    while (i < middle) temporary[k++] = array[i++];
    while (j < right) temporary[k++] = array[j++];
    for (i = left; i < right; i++) array[i] = temporary[i];
}

static void sort_groups(GroupInfo *array, size_t left, size_t right, GroupInfo *temporary) {
    if (right - left > 1) {
        size_t middle = left + (right - left) / 2;
        sort_groups(array, left, middle, temporary);
        sort_groups(array, middle, right, temporary);
        merge_groups(array, left, middle, right, temporary);
    }
}

int merge_sort_groups(GroupInfo *array, size_t count) {
    GroupInfo *temporary;
    if (count < 2) return 1;
    temporary = (GroupInfo *)malloc(count * sizeof(GroupInfo));
    if (!temporary) return 0;
    sort_groups(array, 0, count, temporary);
    free(temporary);
    return 1;
}
