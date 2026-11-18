#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/storage.h"
#include "../include/utils.h"

int storage_save(const Directory *directory, const char *usersFile,
                 const char *friendshipsFile) {
    FILE *users, *edges;
    size_t i;
    users = fopen(usersFile, "w");
    if (!users) {
        printf("Could not open %s for writing.\n", usersFile);
        return 0;
    }
    edges = fopen(friendshipsFile, "w");
    if (!edges) {
        fclose(users);
        printf("Could not open %s for writing.\n", friendshipsFile);
        return 0;
    }
    for (i = 0; i < directory->userCount; i++)
        fprintf(users, "%d,%s,%d,%s\n",
                directory->users[i].id, directory->users[i].name,
                directory->users[i].age, directory->users[i].city);
    fclose(users);
    for (i = 0; i < directory->adjacencyCapacity; i++) {
        FriendNode *node = directory->adjacency[i];
        while (node) {
            if ((int)i < node->userId)
                fprintf(edges, "%d,%d\n", (int)i, node->userId);
            node = node->next;
        }
    }
    fclose(edges);
    printf("Saved %zu users and friendship edges.\n", directory->userCount);
    return 1;
}

int storage_load(Directory *directory, const char *usersFile,
                 const char *friendshipsFile) {
    FILE *users = fopen(usersFile, "r");
    FILE *edges;
    char line[256];
    int loadedUsers = 0, badUsers = 0, loadedEdges = 0, badEdges = 0;
    directory_free(directory);
    if (!directory_init(directory)) return 0;
    if (!users) {
        printf("Missing or unreadable file: %s\n", usersFile);
        return 0;
    }
    while (fgets(line, sizeof(line), users)) {
        char *parts[4], *cursor = line, *comma;
        int id, age, k = 0;
        char name[50], city[40];
        for (k = 0; k < 3; k++) {
            parts[k] = cursor;
            comma = strchr(cursor, ',');
            if (!comma) break;
            *comma = '\0';
            cursor = comma + 1;
        }
        parts[3] = cursor;
        if (k != 3 || sscanf(parts[0], "%d", &id) != 1 ||
            sscanf(parts[2], "%d", &age) != 1 ||
            !normalize_name(parts[1], name, sizeof(name)) ||
            !normalize_city(parts[3], city, sizeof(city)) ||
            !directory_add_user(directory, id, name, age, city)) {
            badUsers++;
        } else {
            loadedUsers++;
        }
    }
    fclose(users);
    edges = fopen(friendshipsFile, "r");
    if (!edges) {
        printf("Missing or unreadable file: %s\n", friendshipsFile);
        return loadedUsers > 0;
    }
    while (fgets(line, sizeof(line), edges)) {
        int first, second;
        if (sscanf(line, "%d,%d", &first, &second) == 2 &&
            directory_add_friendship(directory, first, second))
            loadedEdges++;
        else
            badEdges++;
    }
    fclose(edges);
    printf("Loaded %d users and %d friendships; skipped %d invalid user lines and %d invalid edge lines.\n",
           loadedUsers, loadedEdges, badUsers, badEdges);
    return 1;
}

int storage_load_sample(Directory *directory) {
    static const char *names[18] = {
        "Aarav Shah","Maya Patel","Rohan Mehta","Nisha Rao","Kabir Singh",
        "Isha Nair","Dev Das","Leena Bose","Arjun Khan","Sara Ali",
        "Neel Joshi","Tara Menon","Vikram Roy","Anaya Iyer","Omar Sheikh",
        "Zoya Kapoor","Kiran Dutt","Meera Sen"
    };
    static const char *cities[6] = {"Mumbai","Pune","Delhi","Chennai","Kolkata","Jaipur"};
    int i;
    static const int edgePairs[][2] = {
        {1,2},{1,3},{1,4},{2,5},{2,6},{3,4},{3,7},{4,8},
        {5,6},{5,9},{6,10},{7,11},{8,12},{9,10},{9,13},
        {10,14},{11,12},{12,15},{13,14},{13,16},{14,17},
        {15,16},{16,18},{17,18},{7,14}
    };
    size_t edgeCount = sizeof(edgePairs) / sizeof(edgePairs[0]);
    directory_free(directory);
    if (!directory_init(directory)) return 0;
    for (i = 0; i < 18; i++)
        if (!directory_add_user(directory, i + 1, names[i],
                                19 + (i * 3) % 35, cities[i % 6]))
            return 0;
    for (i = 0; i < (int)edgeCount; i++)
        if (!directory_add_friendship(directory, edgePairs[i][0], edgePairs[i][1]))
            return 0;
    printf("Loaded 18 sample users and %zu friendships.\n", edgeCount);
    return 1;
}
