#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/user.h"
#include "../include/graph.h"
#include "../include/heap.h"
#include "../include/storage.h"
#include "../include/perf.h"
#include "../include/mergesort.h"
#include "../include/json_protocol.h"
#include "../include/utils.h"

#define ENGINE_LINE_SIZE 65536
#define ENGINE_OUTPUT_SIZE 1048576

static Directory directory;
static BinaryHeap lastHeap;
static int lastHeapReady;

typedef struct {
    JsonBuilder *builder;
    Directory *directory;
    int first;
} UserArrayContext;

static void set_error(char *error, size_t size, int *status,
                      int code, const char *message) {
    snprintf(error, size, "%s", message);
    *status = code;
}

static int required_int(const JsonObject *request, const char *key,
                        int minimum, int maximum, int *value,
                        char *error, size_t errorSize, int *status) {
    if (!json_get_int(request, key, value, 1)) {
        set_error(error, errorSize, status, 400, "Missing or invalid integer field");
        return 0;
    }
    if (*value < minimum || *value > maximum) {
        set_error(error, errorSize, status, 400, "Integer field is out of range");
        return 0;
    }
    return 1;
}

static int optional_int(const JsonObject *request, const char *key,
                        int defaultValue, int minimum, int maximum, int *value,
                        char *error, size_t errorSize, int *status) {
    if (!json_get(request, key)) {
        *value = defaultValue;
        return 1;
    }
    return required_int(request, key, minimum, maximum, value,
                        error, errorSize, status);
}

static int required_string(const JsonObject *request, const char *key,
                           char *value, size_t capacity,
                           char *error, size_t errorSize, int *status) {
    if (!json_get_string(request, key, value, capacity, 1) || !value[0]) {
        set_error(error, errorSize, status, 400, "Missing or invalid string field");
        return 0;
    }
    return 1;
}

static void append_user(JsonBuilder *builder, const User *user) {
    json_builder_raw(builder, "{\"id\":");
    json_builder_format(builder, "%d,\"name\":", user->id);
    json_builder_string(builder, user->name);
    json_builder_format(builder, ",\"age\":%d,\"city\":", user->age);
    json_builder_string(builder, user->city);
    json_builder_format(builder, ",\"friendCount\":%d}", user->friendCount);
}

static void append_user_by_id(JsonBuilder *builder, int id) {
    User *user = directory_find_id(&directory, id);
    if (user) append_user(builder, user);
    else json_builder_raw(builder, "null");
}

static void append_users_inorder(const AVLNode *node, void *context) {
    UserArrayContext *array = (UserArrayContext *)context;
    User *user = directory_find_id(array->directory, node->userId);
    if (!user) return;
    if (!array->first) json_builder_raw(array->builder, ",");
    append_user(array->builder, user);
    array->first = 0;
}

static void append_user_ids(JsonBuilder *builder, const int *ids, size_t count) {
    size_t i;
    json_builder_raw(builder, "[");
    for (i = 0; i < count; i++) {
        if (i) json_builder_raw(builder, ",");
        append_user_by_id(builder, ids[i]);
    }
    json_builder_raw(builder, "]");
}

static void append_rotation_meta(JsonBuilder *builder, const AVLMeta *meta) {
    size_t i;
    json_builder_raw(builder, "\"rotation\":");
    if (meta && meta->rotation[0]) json_builder_string(builder, meta->rotation);
    else json_builder_raw(builder, "null");
    json_builder_raw(builder, ",\"avlPath\":[");
    if (meta) {
        for (i = 0; i < meta->pathCount; i++) {
            if (i) json_builder_raw(builder, ",");
            json_builder_format(builder, "%d", meta->path[i]);
        }
    }
    json_builder_raw(builder, "]");
}

static void append_hash_table(JsonBuilder *builder, const HashTable *table) {
    size_t i;
    HashStats stats = hash_get_stats(table);
    json_builder_format(builder, "{\"bucketCount\":%zu,\"loadFactor\":%.6f,"
                        "\"minChain\":%zu,\"maxChain\":%zu,\"averageChain\":%.6f,"
                        "\"buckets\":[",
                        table->size, stats.loadFactor, stats.minChain,
                        stats.maxChain, stats.averageChain);
    for (i = 0; i < table->size; i++) {
        HashNode *node = table->buckets[i];
        if (i) json_builder_raw(builder, ",");
        json_builder_format(builder, "{\"index\":%zu,\"chain\":[", i);
        while (node) {
            if (node != table->buckets[i]) json_builder_raw(builder, ",");
            json_builder_format(builder, "{\"id\":%d,\"name\":", node->id);
            json_builder_string(builder, node->name);
            json_builder_raw(builder, "}");
            node = node->next;
        }
        json_builder_raw(builder, "]}");
    }
    json_builder_raw(builder, "]}");
}

static void append_tree(JsonBuilder *builder, const AVLNode *node) {
    if (!node) {
        json_builder_raw(builder, "null");
        return;
    }
    json_builder_format(builder, "{\"id\":%d,\"name\":", node->userId);
    json_builder_string(builder, node->name);
    json_builder_format(builder, ",\"height\":%d,\"balance\":%d,\"left\":",
                        node->height, avl_balance_factor(node));
    append_tree(builder, node->left);
    json_builder_raw(builder, ",\"right\":");
    append_tree(builder, node->right);
    json_builder_raw(builder, "}");
}

static int command_users(const JsonObject *request, JsonBuilder *data,
                         char *error, size_t errorSize, int *status,
                         AVLMeta *avlMeta) {
    const JsonField *sortField = json_get(request, "sort");
    char sort[32] = "name";
    if (sortField) {
        if (!json_get_string(request, "sort", sort, sizeof(sort), 1)) {
            set_error(error, errorSize, status, 400, "Invalid sort field");
            return 0;
        }
    }
    json_builder_raw(data, "[");
    if (strcmp(sort, "name") == 0) {
        UserArrayContext context = {data, &directory, 1};
        avl_visit_inorder(directory.avlRoot, append_users_inorder, &context);
    } else if (strcmp(sort, "age") == 0 || strcmp(sort, "friends") == 0) {
        User *copy;
        size_t i;
        if (directory.userCount) {
            copy = (User *)malloc(directory.userCount * sizeof(User));
            if (!copy) {
                set_error(error, errorSize, status, 500, "Unable to allocate user listing");
                return 0;
            }
            memcpy(copy, directory.users, directory.userCount * sizeof(User));
            if (!merge_sort_users(copy, directory.userCount,
                                  strcmp(sort, "age") == 0
                                      ? compare_user_age : compare_user_friend_count)) {
                free(copy);
                set_error(error, errorSize, status, 500, "Unable to sort user listing");
                return 0;
            }
            for (i = 0; i < directory.userCount; i++) {
                if (i) json_builder_raw(data, ",");
                append_user(data, &copy[i]);
            }
            free(copy);
        }
    } else {
        set_error(error, errorSize, status, 400, "sort must be name, age, or friends");
        return 0;
    }
    json_builder_raw(data, "]");
    (void)avlMeta;
    return !data->failed;
}

static void append_groups(JsonBuilder *data, int includeMembers) {
    GroupInfo *groups;
    size_t groupCount, i;
    if (!graph_collect_and_rank_groups(&directory, &groups, &groupCount)) {
        data->failed = 1;
        return;
    }
    json_builder_raw(data, "[");
    for (i = 0; i < groupCount; i++) {
        int *members = NULL;
        size_t memberCount = 0;
        if (i) json_builder_raw(data, ",");
        json_builder_format(data, "{\"rootId\":%d,\"size\":%d,\"representative\":",
                            groups[i].rootId, groups[i].size);
        json_builder_string(data, groups[i].representativeName);
        if (includeMembers &&
            graph_get_group_members(&directory, groups[i].rootId, &members, &memberCount)) {
            json_builder_raw(data, ",\"members\":");
            append_user_ids(data, members, memberCount);
        }
        json_builder_raw(data, "}");
        free(members);
    }
    json_builder_raw(data, "]");
    free(groups);
}

static int command_suggestions(const JsonObject *request, JsonBuilder *data,
                               int popular, char *error, size_t errorSize,
                               int *status) {
    int id = 0, k = 5;
    BinaryHeap heap;
    HeapNode current;
    size_t i;
    if (!optional_int(request, "k", 5, 1, 100000, &k, error, errorSize, status))
        return 0;
    if (!popular &&
        !required_int(request, "id", 1, 1000000, &id, error, errorSize, status))
        return 0;
    if (!popular && !directory_find_id(&directory, id)) {
        set_error(error, errorSize, status, 404, "User not found");
        return 0;
    }
    if (!heap_init(&heap, directory.userCount ? directory.userCount : 16)) {
        set_error(error, errorSize, status, 500, "Unable to allocate suggestion heap");
        return 0;
    }
    if (popular) {
        for (i = 0; i < directory.userCount; i++) {
            HeapNode item = {directory.users[i].id, directory.users[i].friendCount};
            if (!heap_insert(&heap, item)) {
                heap_free(&heap);
                set_error(error, errorSize, status, 500, "Unable to grow suggestion heap");
                return 0;
            }
        }
    } else {
        int *seen = (int *)calloc((size_t)directory.dsu.capacity, sizeof(int));
        FriendNode *friend;
        if (!seen) {
            heap_free(&heap);
            set_error(error, errorSize, status, 500, "Unable to allocate candidate set");
            return 0;
        }
        for (friend = directory.adjacency[id]; friend; friend = friend->next) {
            FriendNode *fof;
            for (fof = directory.adjacency[friend->userId]; fof; fof = fof->next) {
                int candidate = fof->userId;
                HeapNode item;
                if (candidate == id || candidate <= 0 ||
                    candidate >= directory.dsu.capacity || seen[candidate] ||
                    !directory_find_id(&directory, candidate) ||
                    directory_are_friends(&directory, id, candidate)) continue;
                seen[candidate] = 1;
                item.userId = candidate;
                item.score = graph_mutual_count(&directory, id, candidate);
                if (!heap_insert(&heap, item)) {
                    free(seen);
                    heap_free(&heap);
                    set_error(error, errorSize, status, 500, "Unable to grow suggestion heap");
                    return 0;
                }
            }
        }
        free(seen);
    }
    if (lastHeapReady) heap_free(&lastHeap);
    lastHeapReady = heap_init(&lastHeap, heap.size ? heap.size : 1);
    if (lastHeapReady) {
        for (i = 0; i < heap.size; i++) {
            if (!heap_insert(&lastHeap, heap.items[i])) {
                heap_free(&lastHeap);
                lastHeapReady = 0;
                break;
            }
        }
    }
    json_builder_raw(data, "[");
    for (i = 0; i < (size_t)k && heap_extract_max(&heap, &current); i++) {
        User *user = directory_find_id(&directory, current.userId);
        if (i) json_builder_raw(data, ",");
        json_builder_raw(data, "{\"user\":");
        append_user(data, user);
        json_builder_format(data, ",\"score\":%d", current.score);
        if (!popular) {
            FriendNode *mutual;
            int first = 1;
            json_builder_raw(data, ",\"mutualFriends\":[");
            for (mutual = directory.adjacency[id]; mutual; mutual = mutual->next) {
                if (directory_are_friends(&directory, current.userId, mutual->userId)) {
                    User *mutualUser = directory_find_id(&directory, mutual->userId);
                    if (!first) json_builder_raw(data, ",");
                    json_builder_string(data, mutualUser->name);
                    first = 0;
                }
            }
            json_builder_raw(data, "]");
        }
        json_builder_raw(data, "}");
    }
    json_builder_raw(data, "]");
    heap_free(&heap);
    return !data->failed;
}

static int command_dispatch(const JsonObject *request, JsonBuilder *data,
                            char *error, size_t errorSize, int *status,
                            AVLMeta *avlMeta) {
    char cmd[64], name[JSON_VALUE_LIMIT], city[JSON_VALUE_LIMIT];
    int id, id1, id2, age, root, path[64], pathCount;
    size_t i;
    if (!required_string(request, "cmd", cmd, sizeof(cmd), error, errorSize, status))
        return 0;

    if (strcmp(cmd, "add_user") == 0) {
        if (!required_int(request, "id", 1, 1000000, &id, error, errorSize, status) ||
            !required_string(request, "name", name, sizeof(name), error, errorSize, status) ||
            !required_int(request, "age", 1, 120, &age, error, errorSize, status) ||
            !required_string(request, "city", city, sizeof(city), error, errorSize, status))
            return 0;
        if (directory_find_id(&directory, id) || directory_find_name(&directory, name)) {
            set_error(error, errorSize, status, 409, "User ID or name already exists");
            return 0;
        }
        if (!directory_add_user_ex(&directory, id, name, age, city, avlMeta)) {
            set_error(error, errorSize, status, 400, "Invalid user data or allocation failure");
            return 0;
        }
        append_user_by_id(data, id);
        return 1;
    }
    if (strcmp(cmd, "get_user") == 0) {
        if (!required_int(request, "id", 1, 1000000, &id, error, errorSize, status))
            return 0;
        if (!directory_find_id(&directory, id)) {
            set_error(error, errorSize, status, 404, "User not found");
            return 0;
        }
        append_user_by_id(data, id);
        return 1;
    }
    if (strcmp(cmd, "get_user_by_name") == 0) {
        User *user;
        if (!required_string(request, "name", name, sizeof(name), error, errorSize, status))
            return 0;
        user = directory_find_name(&directory, name);
        if (!user) {
            set_error(error, errorSize, status, 404, "User not found");
            return 0;
        }
        append_user(data, user);
        return 1;
    }
    if (strcmp(cmd, "update_user") == 0) {
        if (!required_int(request, "id", 1, 1000000, &id, error, errorSize, status) ||
            !required_string(request, "name", name, sizeof(name), error, errorSize, status) ||
            !required_int(request, "age", 1, 120, &age, error, errorSize, status) ||
            !required_string(request, "city", city, sizeof(city), error, errorSize, status))
            return 0;
        if (!directory_find_id(&directory, id)) {
            set_error(error, errorSize, status, 404, "User not found");
            return 0;
        }
        if (directory_find_name(&directory, name) &&
            directory_find_name(&directory, name) != directory_find_id(&directory, id)) {
            set_error(error, errorSize, status, 409, "User name already exists");
            return 0;
        }
        if (!directory_update_user_ex(&directory, id, name, age, city, avlMeta)) {
            set_error(error, errorSize, status, 400, "Invalid user data");
            return 0;
        }
        append_user_by_id(data, id);
        return 1;
    }
    if (strcmp(cmd, "delete_user") == 0) {
        if (!required_int(request, "id", 1, 1000000, &id, error, errorSize, status))
            return 0;
        if (!directory_delete_user_ex(&directory, id, avlMeta)) {
            set_error(error, errorSize, status, 404, "User not found");
            return 0;
        }
        json_builder_raw(data, "{\"deleted\":true}");
        return 1;
    }
    if (strcmp(cmd, "list_users") == 0)
        return command_users(request, data, error, errorSize, status, avlMeta);

    if (strcmp(cmd, "add_friend") == 0 || strcmp(cmd, "remove_friend") == 0) {
        if (!required_int(request, "id1", 1, 1000000, &id1, error, errorSize, status) ||
            !required_int(request, "id2", 1, 1000000, &id2, error, errorSize, status))
            return 0;
        if (!directory_find_id(&directory, id1) || !directory_find_id(&directory, id2)) {
            set_error(error, errorSize, status, 404, "One or both users do not exist");
            return 0;
        }
        if (strcmp(cmd, "add_friend") == 0) {
            if (id1 == id2) {
                set_error(error, errorSize, status, 400, "Self-friendship is not allowed");
                return 0;
            }
            if (directory_are_friends(&directory, id1, id2)) {
                set_error(error, errorSize, status, 409, "Users are already friends");
                return 0;
            }
            if (!directory_add_friendship(&directory, id1, id2)) {
                set_error(error, errorSize, status, 500, "Unable to add friendship");
                return 0;
            }
        } else if (!directory_remove_friendship(&directory, id1, id2)) {
            set_error(error, errorSize, status, 404, "Friendship not found");
            return 0;
        }
        json_builder_format(data, "{\"id1\":%d,\"id2\":%d}", id1, id2);
        return 1;
    }
    if (strcmp(cmd, "list_friends") == 0) {
        int *ids;
        size_t count;
        if (!required_int(request, "id", 1, 1000000, &id, error, errorSize, status))
            return 0;
        if (!directory_find_id(&directory, id)) {
            set_error(error, errorSize, status, 404, "User not found");
            return 0;
        }
        if (!graph_get_friends(&directory, id, &ids, &count)) {
            set_error(error, errorSize, status, 500, "Unable to collect friends");
            return 0;
        }
        append_user_ids(data, ids, count);
        free(ids);
        return 1;
    }
    if (strcmp(cmd, "connected") == 0) {
        int first, second;
        if (!required_int(request, "u", 1, 1000000, &first, error, errorSize, status) ||
            !required_int(request, "v", 1, 1000000, &second, error, errorSize, status))
            return 0;
        if (!directory_find_id(&directory, first) || !directory_find_id(&directory, second)) {
            set_error(error, errorSize, status, 404, "One or both users do not exist");
            return 0;
        }
        root = dsu_find_trace(&directory.dsu, first, path, 64, &pathCount);
        json_builder_format(data, "{\"connected\":%s,\"rootU\":%d,\"pathU\":[",
                            root == dsu_find_trace(&directory.dsu, second, NULL, 0, NULL)
                                ? "true" : "false", root);
        for (i = 0; i < (size_t)pathCount; i++) {
            if (i) json_builder_raw(data, ",");
            json_builder_format(data, "%d", path[i]);
        }
        json_builder_format(data, "],\"rootV\":%d}", dsu_find(&directory.dsu, second));
        return 1;
    }
    if (strcmp(cmd, "groups") == 0) {
        append_groups(data, 1);
        return !data->failed;
    }
    if (strcmp(cmd, "group") == 0) {
        int *members;
        size_t memberCount;
        if (!required_int(request, "id", 1, 1000000, &id, error, errorSize, status))
            return 0;
        if (!graph_get_group_members(&directory, id, &members, &memberCount)) {
            set_error(error, errorSize, status, 404, "User not found");
            return 0;
        }
        root = dsu_find(&directory.dsu, id);
        json_builder_format(data, "{\"rootId\":%d,\"size\":%zu,\"members\":",
                            root, memberCount);
        append_user_ids(data, members, memberCount);
        json_builder_raw(data, "}");
        free(members);
        return 1;
    }
    if (strcmp(cmd, "suggestions") == 0)
        return command_suggestions(request, data, 0, error, errorSize, status);
    if (strcmp(cmd, "popular") == 0)
        return command_suggestions(request, data, 1, error, errorSize, status);

    if (strcmp(cmd, "directory_range") == 0 ||
        strcmp(cmd, "directory_prefix") == 0) {
        char from[JSON_VALUE_LIMIT], to[JSON_VALUE_LIMIT];
        UserArrayContext context = {data, &directory, 1};
        json_builder_raw(data, "[");
        if (strcmp(cmd, "directory_range") == 0) {
            if (!required_string(request, "from", from, sizeof(from), error, errorSize, status) ||
                !required_string(request, "to", to, sizeof(to), error, errorSize, status))
                return 0;
            if (name_compare(from, to) > 0) {
                set_error(error, errorSize, status, 400, "Range start must not exceed range end");
                return 0;
            }
            avl_visit_range(directory.avlRoot, from, to, append_users_inorder, &context);
        } else {
            if (!required_string(request, "q", from, sizeof(from), error, errorSize, status))
                return 0;
            avl_visit_prefix(directory.avlRoot, from, append_users_inorder, &context);
        }
        json_builder_raw(data, "]");
        return 1;
    }
    if (strcmp(cmd, "directory_tree") == 0) {
        append_tree(data, directory.avlRoot);
        return 1;
    }
    if (strcmp(cmd, "hash_view") == 0) {
        json_builder_raw(data, "{\"id\":");
        append_hash_table(data, directory.idHash);
        json_builder_raw(data, ",\"name\":");
        append_hash_table(data, directory.nameHash);
        json_builder_raw(data, "}");
        return !data->failed;
    }
    if (strcmp(cmd, "dsu_view") == 0) {
        json_builder_raw(data, "[");
        for (i = 0; i < directory.userCount; i++) {
            User *user = &directory.users[i];
            int userRoot = dsu_find(&directory.dsu, user->id);
            if (i) json_builder_raw(data, ",");
            json_builder_format(data, "{\"id\":%d,\"parent\":%d,\"rank\":%d,"
                                "\"size\":%d,\"root\":%s}",
                                user->id, directory.dsu.parent[user->id],
                                directory.dsu.rank[user->id],
                                directory.dsu.size[user->id],
                                userRoot == user->id ? "true" : "false");
        }
        json_builder_raw(data, "]");
        return 1;
    }
    if (strcmp(cmd, "heap_view") == 0) {
        json_builder_raw(data, "[");
        if (lastHeapReady) {
            for (i = 0; i < lastHeap.size; i++) {
                if (i) json_builder_raw(data, ",");
                json_builder_format(data, "{\"id\":%d,\"score\":%d}",
                                    lastHeap.items[i].userId, lastHeap.items[i].score);
            }
        }
        json_builder_raw(data, "]");
        return 1;
    }
    if (strcmp(cmd, "graph") == 0) {
        json_builder_raw(data, "{\"nodes\":[");
        for (i = 0; i < directory.userCount; i++) {
            User *user = &directory.users[i];
            if (i) json_builder_raw(data, ",");
            json_builder_format(data, "{\"id\":%d,\"name\":", user->id);
            json_builder_string(data, user->name);
            json_builder_format(data, ",\"friendCount\":%d,\"groupId\":%d}",
                                user->friendCount, dsu_find(&directory.dsu, user->id));
        }
        json_builder_raw(data, "],\"edges\":[");
        {
            int firstEdge = 1;
            for (i = 0; i < directory.adjacencyCapacity; i++) {
                FriendNode *edge;
                for (edge = directory.adjacency[i]; edge; edge = edge->next) {
                    if ((int)i < edge->userId) {
                        if (!firstEdge) json_builder_raw(data, ",");
                        json_builder_format(data, "{\"source\":%zu,\"target\":%d}",
                                            i, edge->userId);
                        firstEdge = 0;
                    }
                }
            }
        }
        json_builder_raw(data, "]}");
        return !data->failed;
    }
    if (strcmp(cmd, "performance") == 0) {
        char report[8192];
        int n;
        if (!required_int(request, "n", 1000, 50000, &n,
                          error, errorSize, status)) return 0;
        if (n != 1000 && n != 10000 && n != 50000) {
            set_error(error, errorSize, status, 400, "n must be 1000, 10000, or 50000");
            return 0;
        }
        if (!perf_run_for_size(n, report, sizeof(report))) {
            set_error(error, errorSize, status, 500, "Unable to run performance analysis");
            return 0;
        }
        json_builder_raw(data, "{\"report\":");
        json_builder_string(data, report);
        json_builder_raw(data, "}");
        return !data->failed;
    }
    if (strcmp(cmd, "sample") == 0) {
        if (!storage_load_sample(&directory)) {
            set_error(error, errorSize, status, 500, "Unable to load sample data");
            return 0;
        }
        json_builder_format(data, "{\"users\":%zu,\"friendships\":%d}",
                            directory.userCount, 25);
        return 1;
    }
    if (strcmp(cmd, "save") == 0) {
        if (!storage_save(&directory, "data/users.txt", "data/friendships.txt")) {
            set_error(error, errorSize, status, 500, "Unable to save data files");
            return 0;
        }
        json_builder_raw(data, "{\"saved\":true}");
        return 1;
    }
    if (strcmp(cmd, "load") == 0) {
        if (!storage_load(&directory, "data/users.txt", "data/friendships.txt")) {
            set_error(error, errorSize, status, 500, "Unable to load data files");
            return 0;
        }
        json_builder_format(data, "{\"users\":%zu}", directory.userCount);
        return 1;
    }
    if (strcmp(cmd, "reset") == 0) {
        directory_free(&directory);
        if (!directory_init(&directory)) {
            set_error(error, errorSize, status, 500, "Unable to reset directory");
            return 0;
        }
        if (lastHeapReady) {
            heap_free(&lastHeap);
            lastHeapReady = 0;
        }
        json_builder_raw(data, "{\"reset\":true}");
        return 1;
    }
    set_error(error, errorSize, status, 400, "Unknown command");
    return 0;
}

static void write_response(const char *line, int tooLong) {
    char *output = (char *)malloc(ENGINE_OUTPUT_SIZE);
    char *data = (char *)malloc(ENGINE_OUTPUT_SIZE - 256);
    char error[256] = "";
    char parseError[256] = "";
    int status = 200, ok;
    JsonObject request;
    JsonBuilder dataBuilder, responseBuilder;
    AVLMeta avlMeta;
    clock_t started = clock();
    double elapsed;
    if (!output || !data) {
        free(output);
        free(data);
        fputs("{\"ok\":false,\"error\":\"Engine response allocation failed\","
              "\"meta\":{\"httpStatus\":500}}\n", stdout);
        fflush(stdout);
        return;
    }
    memset(&avlMeta, 0, sizeof(avlMeta));
    json_builder_init(&dataBuilder, data, ENGINE_OUTPUT_SIZE - 256);
    if (tooLong) {
        set_error(error, sizeof(error), &status, 400, "Request line exceeds limit");
        ok = 0;
    } else if (!json_parse_object(line, &request, parseError, sizeof(parseError))) {
        set_error(error, sizeof(error), &status, 400,
                  parseError[0] ? parseError : "Malformed JSON request");
        ok = 0;
    } else {
        ok = command_dispatch(&request, &dataBuilder, error, sizeof(error),
                              &status, &avlMeta);
        if (dataBuilder.failed && ok) {
            set_error(error, sizeof(error), &status, 500, "Response exceeded output capacity");
            ok = 0;
        }
    }
    elapsed = (double)(clock() - started) * 1000000.0 / (double)CLOCKS_PER_SEC;
    json_builder_init(&responseBuilder, output, ENGINE_OUTPUT_SIZE);
    if (ok) {
        json_builder_raw(&responseBuilder, "{\"ok\":true,\"data\":");
        json_builder_raw(&responseBuilder, data);
        json_builder_raw(&responseBuilder, ",\"meta\":{\"elapsedUs\":");
        json_builder_format(&responseBuilder, "%.0f", elapsed);
        json_builder_raw(&responseBuilder, ",");
        append_rotation_meta(&responseBuilder, &avlMeta);
        json_builder_raw(&responseBuilder, "}}");
    } else {
        json_builder_raw(&responseBuilder, "{\"ok\":false,\"error\":");
        json_builder_string(&responseBuilder, error[0] ? error : "Request failed");
        json_builder_format(&responseBuilder,
                            ",\"meta\":{\"httpStatus\":%d,\"elapsedUs\":%.0f}}",
                            status, elapsed);
    }
    if (responseBuilder.failed) {
        fputs("{\"ok\":false,\"error\":\"Response serialization failed\","
              "\"meta\":{\"httpStatus\":500}}\n", stdout);
    } else {
        fputs(output, stdout);
        fputc('\n', stdout);
    }
    fflush(stdout);
    free(output);
    free(data);
}

int main(void) {
    char line[ENGINE_LINE_SIZE];
    if (!directory_init(&directory)) {
        fputs("Unable to initialize C engine state\n", stderr);
        return EXIT_FAILURE;
    }
    while (fgets(line, sizeof(line), stdin)) {
        size_t length = strlen(line);
        int tooLong = length == sizeof(line) - 1 && line[length - 1] != '\n';
        if (tooLong) {
            int character;
            while ((character = getchar()) != '\n' && character != EOF) { }
            write_response("", 1);
        } else {
            while (length && (line[length - 1] == '\n' || line[length - 1] == '\r'))
                line[--length] = '\0';
            write_response(line, 0);
        }
    }
    if (lastHeapReady) heap_free(&lastHeap);
    directory_free(&directory);
    return EXIT_SUCCESS;
}
