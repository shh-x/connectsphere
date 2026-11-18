#include <assert.h>
#include "../include/mergesort.h"

int main(void) {
    User users[4] = {
        {1, "One", 30, "Pune", 2},
        {2, "Two", 20, "Pune", 4},
        {3, "Three", 20, "Pune", 1},
        {4, "Four", 30, "Pune", 4}
    };
    assert(merge_sort_users(users, 4, compare_user_age));
    assert(users[0].age == 20 && users[1].age == 20);
    assert(merge_sort_users(users, 4, compare_user_friend_count));
    assert(users[0].friendCount >= users[1].friendCount);
    return 0;
}
