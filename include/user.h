#ifndef USER_H
#define USER_H

#include <stddef.h>
#include "hash.h"
#include "avl.h"
#include "dsu.h"

typedef struct FriendNode {
    int userId;
    struct FriendNode *next;
} FriendNode;

typedef struct User {
    int id;
    char name[50];
    int age;
    char city[40];
    int friendCount;
} User;

typedef struct Directory {
    User *users;
    size_t userCount;
    size_t userCapacity;
    FriendNode **adjacency;
    size_t adjacencyCapacity;
    HashTable *idHash;
    HashTable *nameHash;
    AVLNode *avlRoot;
    DSU dsu;
} Directory;

/* Purpose: Initialize an empty directory.
   Parameters: directory pointer.
   Return: 1 on success, 0 on allocation failure.
   Time complexity: O(1) plus table initialization. */
int directory_init(Directory *directory);

/* Purpose: Release every allocation owned by a directory.
   Parameters: directory pointer.
   Return: void.
   Time complexity: O(V + E). */
void directory_free(Directory *directory);

/* Purpose: Find a user by ID.
   Parameters: directory, ID.
   Return: user pointer or NULL.
   Time complexity: Average O(1), worst O(n). */
User *directory_find_id(Directory *directory, int id);

/* Purpose: Find a user by case-insensitive name.
   Parameters: directory, name.
   Return: user pointer or NULL.
   Time complexity: Average O(1), worst O(n). */
User *directory_find_name(Directory *directory, const char *name);

/* Purpose: Check whether an ID or name is already used.
   Parameters: directory, ID, name.
   Return: 1 if duplicate exists, otherwise 0.
   Time complexity: Average O(1), worst O(n). */
int directory_duplicate(Directory *directory, int id, const char *name);

/* Purpose: Add a user consistently to all structures.
   Parameters: directory and user fields.
   Return: 1 on success, 0 on validation/allocation failure.
   Time complexity: O(log V) amortized plus table operations. */
int directory_add_user(Directory *directory, int id, const char *name,
                       int age, const char *city);

/* Purpose: Delete a user and all incident friendships.
   Parameters: directory and user ID.
   Return: 1 on success, 0 when absent.
   Time complexity: O(V + E + N). */
int directory_delete_user(Directory *directory, int id);

/* Purpose: Update a user's name, age, and city.
   Parameters: directory, ID, and new fields.
   Return: 1 on success, 0 on failure.
   Time complexity: O(log V) plus hash operations. */
int directory_update_user(Directory *directory, int id, const char *name,
                          int age, const char *city);

/* Purpose: Add an undirected friendship.
   Parameters: directory and the two IDs.
   Return: 1 on success, 0 on invalid/duplicate/self edge.
   Time complexity: O(log V) amortized plus hash/DSU operations. */
int directory_add_friendship(Directory *directory, int first, int second);

/* Purpose: Remove an undirected friendship.
   Parameters: directory and the two IDs.
   Return: 1 on success, 0 if absent.
   Time complexity: O(V + E). */
int directory_remove_friendship(Directory *directory, int first, int second);

/* Purpose: Test whether an adjacency edge exists.
   Parameters: directory and endpoint IDs.
   Return: 1 if present, otherwise 0.
   Time complexity: O(degree). */
int directory_are_friends(Directory *directory, int first, int second);

#endif
