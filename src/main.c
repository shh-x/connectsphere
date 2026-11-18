#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/user.h"
#include "../include/graph.h"
#include "../include/heap.h"
#include "../include/storage.h"
#include "../include/perf.h"
#include "../include/mergesort.h"
#include "../include/utils.h"

static Directory directory;
static int avlVerbose = 0;

static void user_menu(void) {
    int choice;
    do {
        printf("\n--- User Management ---\n");
        printf("1 Add  2 Delete  3 Update  4 Search by ID\n");
        printf("5 Search by name  6 List directory  0 Back\n");
        choice = read_int("Choice: ", 0, 6);
        if (choice == 1) {
            int id = read_int("ID (0 for smallest free ID): ", 0, 1000000);
            char input[100], name[50], city[40];
            int age;
            if (id == 0) {
                id = 1;
                while (directory_find_id(&directory, id)) id++;
            }
            while (!read_line("Name: ", input, sizeof(input)) ||
                   !normalize_name(input, name, sizeof(name)))
                printf("Invalid name.\n");
            age = read_int("Age: ", 1, 120);
            while (!read_line("City: ", input, sizeof(input)) ||
                   !normalize_city(input, city, sizeof(city)))
                printf("Invalid city.\n");
            if (directory_add_user(&directory, id, name, age, city))
                printf("User added.\n");
            else
                printf("Could not add user: duplicate or invalid data.\n");
        } else if (choice == 2) {
            int id = read_int("User ID: ", 1, 1000000);
            printf(directory_delete_user(&directory, id)
                   ? "User deleted.\n" : "User not found.\n");
        } else if (choice == 3) {
            int id = read_int("User ID: ", 1, 1000000);
            User *user = directory_find_id(&directory, id);
            if (!user) printf("User not found.\n");
            else {
                char input[100], name[50], city[40];
                int age;
                while (!read_line("New name: ", input, sizeof(input)) ||
                       !normalize_name(input, name, sizeof(name)))
                    printf("Invalid name.\n");
                age = read_int("New age: ", 1, 120);
                while (!read_line("New city: ", input, sizeof(input)) ||
                       !normalize_city(input, city, sizeof(city)))
                    printf("Invalid city.\n");
                printf(directory_update_user(&directory, id, name, age, city)
                       ? "User updated.\n" : "Update failed.\n");
            }
        } else if (choice == 4) {
            int id = read_int("User ID: ", 1, 1000000);
            User *user = directory_find_id(&directory, id);
            printf(user ? "ID %d | %s | age %d | %s | friends %d\n"
                   : "User not found.\n",
                   user ? user->id : 0, user ? user->name : "", user ? user->age : 0,
                   user ? user->city : "", user ? user->friendCount : 0);
        } else if (choice == 5) {
            char input[100], name[50];
            while (!read_line("Name: ", input, sizeof(input)) ||
                   !normalize_name(input, name, sizeof(name)))
                printf("Invalid name.\n");
            {
                User *user = directory_find_name(&directory, name);
                printf(user ? "ID %d | %s | age %d | %s | friends %d\n"
                       : "User not found.\n",
                       user ? user->id : 0, user ? user->name : "",
                       user ? user->age : 0, user ? user->city : "",
                       user ? user->friendCount : 0);
            }
        } else if (choice == 6) {
            printf("User directory (%zu users):\n", directory.userCount);
            avl_inorder(directory.avlRoot);
        }
    } while (choice != 0);
}

static void friendship_menu(void) {
    int choice;
    do {
        printf("\n--- Friendship Management ---\n");
        printf("1 Add  2 Remove  3 List friends  4 Check connected  0 Back\n");
        choice = read_int("Choice: ", 0, 4);
        if (choice == 1 || choice == 2) {
            int first = read_int("First user ID: ", 1, 1000000);
            int second = read_int("Second user ID: ", 1, 1000000);
            if (choice == 1)
                printf(directory_add_friendship(&directory, first, second)
                       ? "Friendship added.\n" : "Invalid, duplicate, or self friendship.\n");
            else
                printf(directory_remove_friendship(&directory, first, second)
                       ? "Friendship removed; DSU rebuilt.\n" : "Friendship not found.\n");
        } else if (choice == 3) {
            int id = read_int("User ID: ", 1, 1000000);
            User *user = directory_find_id(&directory, id);
            if (!user) printf("User not found.\n");
            else if (!graph_list_friends(&directory, id)) printf("No friends.\n");
        } else if (choice == 4) {
            int first = read_int("First user ID: ", 1, 1000000);
            int second = read_int("Second user ID: ", 1, 1000000);
            if (!directory_find_id(&directory, first) ||
                !directory_find_id(&directory, second))
                printf("One or both users do not exist.\n");
            else
                printf(dsu_find(&directory.dsu, first) == dsu_find(&directory.dsu, second)
                       ? "Connected in the same friend group.\n"
                       : "Not connected.\n");
        }
    } while (choice != 0);
}

static void friend_suggestion_menu(void) {
    int choice;
    do {
        printf("\n--- Friend Suggestions ---\n");
        printf("1 Top-K suggestions  2 Top-K popular users  0 Back\n");
        choice = read_int("Choice: ", 0, 2);
        if (choice == 1) {
            int id = read_int("User ID: ", 1, 1000000);
            int k = read_int("K (default 5): ", 1, 100000);
            User *user = directory_find_id(&directory, id);
            if (!user) { printf("User not found.\n"); continue; }
            {
                BinaryHeap heap;
                heap_init(&heap, 16);
                if ((size_t)id < directory.adjacencyCapacity) {
                    FriendNode *friend = directory.adjacency[id];
                    while (friend) {
                        FriendNode *fof = directory.adjacency[friend->userId];
                        while (fof) {
                            int candidate = fof->userId;
                            if (candidate != id &&
                                !directory_are_friends(&directory, id, candidate)) {
                                int mutual = graph_mutual_count(&directory, id, candidate);
                                HeapNode node;
                                size_t i;
                                int duplicate = 0;
                                for (i = 0; i < heap.size; i++)
                                    if (heap.items[i].userId == candidate) duplicate = 1;
                                if (!duplicate) {
                                    node.userId = candidate;
                                    node.score = mutual;
                                    heap_insert(&heap, node);
                                }
                            }
                            fof = fof->next;
                        }
                        friend = friend->next;
                    }
                }
                if (!heap.size) printf("No friend suggestions available.\n");
                else {
                    int printed = 0;
                    HeapNode suggestion;
                    while (printed < k && heap_extract_max(&heap, &suggestion)) {
                        User *candidate = directory_find_id(&directory, suggestion.userId);
                        FriendNode *mutual;
                        printf("%d. %s (ID %d) - %d mutual friend(s): ",
                               printed + 1, candidate->name, candidate->id,
                               suggestion.score);
                        if ((size_t)candidate->id < directory.adjacencyCapacity) {
                            for (mutual = directory.adjacency[id]; mutual; mutual = mutual->next)
                                if (directory_are_friends(&directory, candidate->id,
                                                           mutual->userId)) {
                                    User *m = directory_find_id(&directory, mutual->userId);
                                    printf("%s ", m->name);
                                }
                        }
                        printf("\n");
                        printed++;
                    }
                }
                heap_free(&heap);
            }
        } else if (choice == 2) {
            int k = read_int("K: ", 1, 100000);
            BinaryHeap heap;
            HeapNode result;
            size_t i;
            heap_init(&heap, 16);
            for (i = 0; i < directory.userCount; i++) {
                HeapNode node;
                node.userId = directory.users[i].id;
                node.score = directory.users[i].friendCount;
                heap_insert(&heap, node);
            }
            for (i = 0; i < (size_t)k && heap_extract_max(&heap, &result); i++) {
                User *user = directory_find_id(&directory, result.userId);
                printf("%zu. %s (ID %d) - %d friends\n",
                       i + 1, user->name, user->id, result.score);
            }
            heap_free(&heap);
        }
    } while (choice != 0);
}

static void group_menu(void) {
    int choice;
    do {
        printf("\n--- Friend Groups ---\n");
        printf("1 Group of user  2 All groups  3 Group count  0 Back\n");
        choice = read_int("Choice: ", 0, 3);
        if (choice == 1) {
            int id = read_int("User ID: ", 1, 1000000);
            if (!directory_find_id(&directory, id)) printf("User not found.\n");
            else {
                int root = dsu_find(&directory.dsu, id);
                int member;
                printf("Group root %d, size %d:\n", root,
                       dsu_group_size(&directory.dsu, id));
                for (member = 1; member < directory.dsu.capacity; member++)
                    if (directory_find_id(&directory, member) &&
                        dsu_find(&directory.dsu, member) == root)
                        printf("- %s (ID %d)\n",
                               directory_find_id(&directory, member)->name, member);
            }
        } else if (choice == 2) {
            graph_list_all_groups(&directory);
        } else if (choice == 3) {
            printf("Total friend groups: %d\n", dsu_count_groups(&directory.dsu));
        }
    } while (choice != 0);
}

static void ranking_menu(void) {
    int choice;
    do {
        printf("\n--- Community Ranking ---\n");
        printf("1 Ranked groups  2 Users by age  3 Users by friend count  0 Back\n");
        choice = read_int("Choice: ", 0, 3);
        if (choice == 1) {
            GroupInfo *groups;
            size_t count, i;
            if (graph_collect_and_rank_groups(&directory, &groups, &count)) {
                for (i = 0; i < count; i++)
                    printf("%zu. %s | size %d | root %d\n",
                           i + 1, groups[i].representativeName,
                           groups[i].size, groups[i].rootId);
                free(groups);
            }
        } else if (choice == 2 || choice == 3) {
            User *copy = (User *)malloc(directory.userCount * sizeof(User));
            size_t i;
            if (!copy) continue;
            for (i = 0; i < directory.userCount; i++) copy[i] = directory.users[i];
            if (choice == 2)
                merge_sort_users(copy, directory.userCount, compare_user_age);
            else
                merge_sort_users(copy, directory.userCount, compare_user_friend_count);
            for (i = 0; i < directory.userCount; i++)
                printf("%zu. %s (ID %d) | age %d | friends %d\n",
                       i + 1, copy[i].name, copy[i].id,
                       copy[i].age, copy[i].friendCount);
            free(copy);
        }
    } while (choice != 0);
}

static void sorted_directory_menu(void) {
    int choice;
    do {
        printf("\n--- Sorted Directory (AVL) ---\n");
        printf("1 Inorder  2 Preorder  3 Level order  4 Height\n");
        printf("5 Range  6 Prefix  7 Toggle verbose  8 Balance factors  0 Back\n");
        choice = read_int("Choice: ", 0, 8);
        if (choice == 1) avl_inorder(directory.avlRoot);
        else if (choice == 2) avl_preorder(directory.avlRoot);
        else if (choice == 3) avl_level_order(directory.avlRoot);
        else if (choice == 4) printf("AVL height: %d\n", avl_height(directory.avlRoot));
        else if (choice == 5) {
            char lower[50], upper[50];
            read_line("Lower name: ", lower, sizeof(lower));
            read_line("Upper name: ", upper, sizeof(upper));
            avl_range(directory.avlRoot, lower, upper);
        } else if (choice == 6) {
            char prefix[50];
            read_line("Prefix: ", prefix, sizeof(prefix));
            avl_prefix(directory.avlRoot, prefix);
        } else if (choice == 7) {
            avlVerbose = !avlVerbose;
            printf("AVL verbose mode %s.\n", avlVerbose ? "ON" : "OFF");
        } else if (choice == 8) avl_display(directory.avlRoot);
    } while (choice != 0);
}

static void data_views_menu(void) {
    int choice;
    do {
        printf("\n--- Data Structure Views ---\n");
        printf("1 ID hash  2 Name hash  3 AVL tree  4 DSU arrays  5 Heap demo  0 Back\n");
        choice = read_int("Choice: ", 0, 5);
        if (choice == 1) { hash_display(directory.idHash); hash_chain_stats(directory.idHash); }
        else if (choice == 2) { hash_display(directory.nameHash); hash_chain_stats(directory.nameHash); }
        else if (choice == 3) avl_display(directory.avlRoot);
        else if (choice == 4) {
            int i;
            printf("Index parent rank size\n");
            for (i = 1; i < directory.dsu.capacity; i++)
                if (directory_find_id(&directory, i))
                    printf("%d %d %d %d\n", i, directory.dsu.parent[i],
                           directory.dsu.rank[i], directory.dsu.size[i]);
        } else if (choice == 5) {
            BinaryHeap heap;
            heap_init(&heap, 16);
            {
                size_t i;
                for (i = 0; i < directory.userCount; i++) {
                    HeapNode node = {directory.users[i].id, directory.users[i].friendCount};
                    heap_insert(&heap, node);
                }
            }
            heap_display(&heap);
            heap_free(&heap);
        }
    } while (choice != 0);
}

static void persistence_menu(void) {
    int choice;
    do {
        printf("\n--- Save / Load Data ---\n");
        printf("1 Save  2 Load  3 Load sample data  0 Back\n");
        choice = read_int("Choice: ", 0, 3);
        if (choice == 1)
            storage_save(&directory, "data/users.txt", "data/friendships.txt");
        else if (choice == 2)
            storage_load(&directory, "data/users.txt", "data/friendships.txt");
        else if (choice == 3)
            storage_load_sample(&directory);
    } while (choice != 0);
}

int main(void) {
    int choice;
    if (!directory_init(&directory)) {
        fprintf(stderr, "Fatal: unable to initialize ConnectSphere.\n");
        return EXIT_FAILURE;
    }
    printf("ConnectSphere - Social Network Friend Group Manager\n");
    do {
        printf("\n========== Main Menu ==========\n");
        printf("1 User management\n");
        printf("2 Friendship management\n");
        printf("3 Friend groups\n");
        printf("4 Friend suggestions\n");
        printf("5 Community ranking\n");
        printf("6 Sorted directory\n");
        printf("7 Data structure views\n");
        printf("8 Performance analysis\n");
        printf("9 Save / Load data\n");
        printf("0 Exit\n");
        choice = read_int("Choice: ", 0, 9);
        if (choice == 1) user_menu();
        else if (choice == 2) friendship_menu();
        else if (choice == 3) group_menu();
        else if (choice == 4) friend_suggestion_menu();
        else if (choice == 5) ranking_menu();
        else if (choice == 6) sorted_directory_menu();
        else if (choice == 7) data_views_menu();
        else if (choice == 8) perf_run_all();
        else if (choice == 9) persistence_menu();
    } while (choice != 0);
    {
        char answer[16];
        read_line("Save before exit? (y/n): ", answer, sizeof(answer));
        if (answer[0] == 'y' || answer[0] == 'Y')
            storage_save(&directory, "data/users.txt", "data/friendships.txt");
    }
    directory_free(&directory);
    return EXIT_SUCCESS;
}
