#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/user.h"
#include "../include/utils.h"

static void rebuild_dsu(Directory *directory) {
    size_t i;
    dsu_reset(&directory->dsu);
    for (i = 1; i < (size_t)directory->dsu.capacity; i++)
        directory->dsu.size[i] = 0;
    for (i = 0; i < directory->userCount; i++)
        directory->dsu.size[directory->users[i].id] = 1;
    for (i = 0; i < directory->adjacencyCapacity; i++) {
        FriendNode *node = directory->adjacency[i];
        while (node) {
            if ((int)i < node->userId)
                dsu_union(&directory->dsu, (int)i, node->userId);
            node = node->next;
        }
    }
}

static int ensure_adjacency(Directory *directory, int id) {
    if (id < 0) return 0;
    if ((size_t)id >= directory->adjacencyCapacity) {
        size_t newCapacity = directory->adjacencyCapacity ? directory->adjacencyCapacity : 101;
        FriendNode **newAdj;
        while (newCapacity <= (size_t)id) newCapacity *= 2;
        newAdj = (FriendNode **)realloc(directory->adjacency,
                                        newCapacity * sizeof(FriendNode *));
        if (!newAdj) return 0;
        directory->adjacency = newAdj;
        while (directory->adjacencyCapacity < newCapacity) {
            directory->adjacency[directory->adjacencyCapacity++] = NULL;
        }
    }
    if (id >= directory->dsu.capacity && !dsu_resize(&directory->dsu, id * 2 + 2))
        return 0;
    return 1;
}

int directory_init(Directory *directory) {
    size_t i;
    if (!directory) return 0;
    memset(directory, 0, sizeof(*directory));
    directory->idHash = (HashTable *)calloc(1, sizeof(HashTable));
    directory->nameHash = (HashTable *)calloc(1, sizeof(HashTable));
    if (!directory->idHash || !directory->nameHash) {
        directory_free(directory);
        return 0;
    }
    if (!hash_init(directory->idHash, HASH_BY_ID, 101) ||
        !hash_init(directory->nameHash, HASH_BY_NAME, 101) ||
        !dsu_init(&directory->dsu, 101) ||
        !ensure_adjacency(directory, 100)) {
        directory_free(directory);
        return 0;
    }
    for (i = 1; i < (size_t)directory->dsu.capacity; i++)
        directory->dsu.size[i] = 0;
    return 1;
}

void directory_free(Directory *directory) {
    size_t i;
    if (!directory) return;
    hash_free(directory->idHash);
    hash_free(directory->nameHash);
    free(directory->idHash);
    free(directory->nameHash);
    dsu_free(&directory->dsu);
    avl_free(directory->avlRoot);
    directory->avlRoot = NULL;
    for (i = 0; i < directory->adjacencyCapacity; i++) {
        FriendNode *node = directory->adjacency[i];
        while (node) { FriendNode *next = node->next; free(node); node = next; }
    }
    free(directory->adjacency);
    free(directory->users);
    memset(directory, 0, sizeof(*directory));
}

User *directory_find_id(Directory *directory, int id) {
    if (!directory || !directory->idHash) return NULL;
    HashNode *node = hash_search_id(directory->idHash, id);
    return node ? (User *)node->data : NULL;
}

User *directory_find_name(Directory *directory, const char *name) {
    if (!directory || !directory->nameHash || !name) return NULL;
    HashNode *node = hash_search_name(directory->nameHash, name);
    return node ? (User *)node->data : NULL;
}

int directory_duplicate(Directory *directory, int id, const char *name) {
    return directory_find_id(directory, id) != NULL ||
           directory_find_name(directory, name) != NULL;
}

int directory_add_user(Directory *directory, int id, const char *name,
                       int age, const char *city) {
    User *user;
    size_t i;
    char nameInput[100], cityInput[100], normalizedName[50], normalizedCity[40];
    if (!directory || !name || !city || id <= 0 || age < 1 || age > 120)
        return 0;
    if (strlen(name) >= sizeof(nameInput) || strlen(city) >= sizeof(cityInput))
        return 0;
    snprintf(nameInput, sizeof(nameInput), "%s", name);
    snprintf(cityInput, sizeof(cityInput), "%s", city);
    if (!normalize_name(nameInput, normalizedName, sizeof(normalizedName)) ||
        !normalize_city(cityInput, normalizedCity, sizeof(normalizedCity)) ||
        directory_duplicate(directory, id, normalizedName)) return 0;
    if (directory->userCount == directory->userCapacity) {
        size_t newCapacity = directory->userCapacity ? directory->userCapacity * 2 : 16;
        User *newUsers = (User *)realloc(directory->users, newCapacity * sizeof(User));
        if (!newUsers) return 0;
        directory->users = newUsers;
        directory->userCapacity = newCapacity;
        for (i = 0; i < directory->userCount; i++) {
            HashNode *idNode = hash_search_id(directory->idHash, directory->users[i].id);
            HashNode *nameNode = hash_search_name(directory->nameHash,
                                                  directory->users[i].name);
            if (idNode) idNode->data = &directory->users[i];
            if (nameNode) nameNode->data = &directory->users[i];
        }
    }
    if (!ensure_adjacency(directory, id)) return 0;
    user = &directory->users[directory->userCount++];
    user->id = id;
    snprintf(user->name, sizeof(user->name), "%s", normalizedName);
    user->age = age;
    snprintf(user->city, sizeof(user->city), "%s", normalizedCity);
    user->friendCount = 0;
    if (!hash_insert(directory->idHash, id, normalizedName, user)) {
        directory->userCount--;
        return 0;
    }
    if (!hash_insert(directory->nameHash, id, normalizedName, user)) {
        hash_delete(directory->idHash, id, normalizedName);
        directory->userCount--;
        return 0;
    }
    directory->avlRoot = avl_insert(directory->avlRoot, normalizedName, id, 0);
    if (!avl_search(directory->avlRoot, normalizedName)) {
        hash_delete(directory->idHash, id, normalizedName);
        hash_delete(directory->nameHash, id, normalizedName);
        directory->userCount--;
        return 0;
    }
    rebuild_dsu(directory);
    return 1;
}

int directory_delete_user(Directory *directory, int id) {
    User *user = directory_find_id(directory, id);
    size_t i;
    if (!user) return 0;
    {
        size_t index = (size_t)(user - directory->users);
        FriendNode *node = directory->adjacency[id];
        while (node) {
            int other = node->userId;
            FriendNode **link = &directory->adjacency[other];
            while (*link && (*link)->userId != id) link = &(*link)->next;
            if (*link) { FriendNode *remove = *link; *link = remove->next; free(remove); }
            node = node->next;
        }
        while (directory->adjacency[id]) {
            FriendNode *remove = directory->adjacency[id];
            directory->adjacency[id] = remove->next;
            free(remove);
        }
        hash_delete(directory->idHash, id, "");
        hash_delete(directory->nameHash, id, user->name);
        directory->avlRoot = avl_delete(directory->avlRoot, user->name, 0);
        for (i = index + 1; i < directory->userCount; i++)
            directory->users[i - 1] = directory->users[i];
        directory->userCount--;
    }
    rebuild_dsu(directory);
    for (i = 0; i < directory->userCount; i++)
        directory->users[i].friendCount = 0;
    for (i = 0; i < directory->adjacencyCapacity; i++) {
        FriendNode *node = directory->adjacency[i];
        User *owner = directory_find_id(directory, (int)i);
        if (owner)
            owner->friendCount = 0;
        while (node) {
            if (owner) owner->friendCount++;
            node = node->next;
        }
    }
    return 1;
}

int directory_update_user(Directory *directory, int id, const char *name,
                          int age, const char *city) {
    User *user = directory_find_id(directory, id);
    char oldName[50];
    char nameInput[100], cityInput[100], normalizedName[50], normalizedCity[40];
    User *duplicate;
    if (!user || !name || !city || age < 1 || age > 120 ||
        strlen(name) >= sizeof(nameInput) || strlen(city) >= sizeof(cityInput))
        return 0;
    snprintf(nameInput, sizeof(nameInput), "%s", name);
    snprintf(cityInput, sizeof(cityInput), "%s", city);
    if (!normalize_name(nameInput, normalizedName, sizeof(normalizedName)) ||
        !normalize_city(cityInput, normalizedCity, sizeof(normalizedCity)))
        return 0;
    duplicate = directory_find_name(directory, normalizedName);
    if (duplicate && duplicate != user)
        return 0;
    snprintf(oldName, sizeof(oldName), "%s", user->name);
    if (strcmp(oldName, normalizedName) == 0) {
        user->age = age;
        snprintf(user->city, sizeof(user->city), "%s", normalizedCity);
        return 1;
    }
    if (name_compare(oldName, normalizedName) == 0) {
        HashNode *nameNode = hash_search_name(directory->nameHash, oldName);
        AVLNode *treeNode = avl_search(directory->avlRoot, oldName);
        if (!nameNode || !treeNode) return 0;
        snprintf(user->name, sizeof(user->name), "%s", normalizedName);
        snprintf(user->city, sizeof(user->city), "%s", normalizedCity);
        user->age = age;
        if (nameNode) snprintf(nameNode->name, sizeof(nameNode->name), "%s", normalizedName);
        if (treeNode) snprintf(treeNode->name, sizeof(treeNode->name), "%s", normalizedName);
        return 1;
    }
    if (!hash_insert(directory->nameHash, id, normalizedName, user)) {
        return 0;
    }
    directory->avlRoot = avl_insert(directory->avlRoot, normalizedName, id, 0);
    if (!avl_search(directory->avlRoot, normalizedName)) {
        hash_delete(directory->nameHash, id, normalizedName);
        return 0;
    }
    hash_delete(directory->nameHash, id, oldName);
    directory->avlRoot = avl_delete(directory->avlRoot, oldName, 0);
    snprintf(user->name, sizeof(user->name), "%s", normalizedName);
    user->age = age;
    snprintf(user->city, sizeof(user->city), "%s", normalizedCity);
    return 1;
}

int directory_add_friendship(Directory *directory, int first, int second) {
    FriendNode *node;
    if (first == second || !directory_find_id(directory, first) ||
        !directory_find_id(directory, second) ||
        directory_are_friends(directory, first, second)) return 0;
    if (!ensure_adjacency(directory, first) || !ensure_adjacency(directory, second))
        return 0;
    node = (FriendNode *)malloc(sizeof(FriendNode));
    if (!node) return 0;
    node->userId = second;
    node->next = directory->adjacency[first];
    directory->adjacency[first] = node;
    node = (FriendNode *)malloc(sizeof(FriendNode));
    if (!node) {
        FriendNode *remove = directory->adjacency[first];
        directory->adjacency[first] = remove->next;
        free(remove);
        return 0;
    }
    node->userId = first;
    node->next = directory->adjacency[second];
    directory->adjacency[second] = node;
    directory_find_id(directory, first)->friendCount++;
    directory_find_id(directory, second)->friendCount++;
    dsu_union(&directory->dsu, first, second);
    return 1;
}

int directory_remove_friendship(Directory *directory, int first, int second) {
    FriendNode **link;
    if (!directory_are_friends(directory, first, second)) return 0;
    link = &directory->adjacency[first];
    while (*link && (*link)->userId != second) link = &(*link)->next;
    if (*link) { FriendNode *remove = *link; *link = remove->next; free(remove); }
    link = &directory->adjacency[second];
    while (*link && (*link)->userId != first) link = &(*link)->next;
    if (*link) { FriendNode *remove = *link; *link = remove->next; free(remove); }
    if (directory_find_id(directory, first)) directory_find_id(directory, first)->friendCount--;
    if (directory_find_id(directory, second)) directory_find_id(directory, second)->friendCount--;
    rebuild_dsu(directory);
    return 1;
}

int directory_are_friends(Directory *directory, int first, int second) {
    FriendNode *node;
    if (first < 0 || (size_t)first >= directory->adjacencyCapacity) return 0;
    for (node = directory->adjacency[first]; node; node = node->next)
        if (node->userId == second) return 1;
    return 0;
}
