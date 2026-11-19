#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/hash.h"
#include "../include/utils.h"

static int is_prime(size_t n) {
    size_t i;
    if (n < 2) return 0;
    if (n % 2 == 0) return n == 2;
    for (i = 3; i * i <= n; i += 2) if (n % i == 0) return 0;
    return 1;
}

static size_t next_prime(size_t n) {
    if (n < 101) n = 101;
    if (n % 2 == 0) n++;
    while (!is_prime(n)) n += 2;
    return n;
}

static unsigned long long string_key(const char *name) {
    unsigned long long value = 0;
    while (*name) {
        value = value * 31ULL + (unsigned char)tolower((unsigned char)*name);
        name++;
    }
    return value;
}

static size_t hash_index(const HashTable *table, int id, const char *name) {
    if (table->keyType == HASH_BY_ID) return (size_t)id % table->size;
    return (size_t)(string_key(name) % table->size);
}

int hash_init(HashTable *table, HashKeyType keyType, size_t initialSize) {
    size_t i;
    table->keyType = keyType;
    table->size = next_prime(initialSize);
    table->count = 0;
    table->buckets = (HashNode **)calloc(table->size, sizeof(HashNode *));
    if (!table->buckets) return 0;
    for (i = 0; i < table->size; i++) table->buckets[i] = NULL;
    return 1;
}

void hash_free(HashTable *table) {
    size_t i;
    if (!table || !table->buckets) return;
    for (i = 0; i < table->size; i++) {
        HashNode *node = table->buckets[i];
        while (node) {
            HashNode *next = node->next;
            free(node);
            node = next;
        }
    }
    free(table->buckets);
    table->buckets = NULL;
    table->size = 0;
    table->count = 0;
}

HashNode *hash_search_id(HashTable *table, int id) {
    size_t index;
    HashNode *node;
    if (!table || !table->buckets || table->keyType != HASH_BY_ID) return NULL;
    index = hash_index(table, id, "");
    for (node = table->buckets[index]; node; node = node->next)
        if (node->id == id) return node;
    return NULL;
}

HashNode *hash_search_name(HashTable *table, const char *name) {
    size_t index;
    HashNode *node;
    if (!table || !table->buckets || table->keyType != HASH_BY_NAME) return NULL;
    index = hash_index(table, 0, name);
    for (node = table->buckets[index]; node; node = node->next)
        if (name_compare(node->name, name) == 0) return node;
    return NULL;
}

int hash_insert(HashTable *table, int id, const char *name, void *data) {
    size_t index;
    HashNode *node;
    if (!table || !table->buckets) return 0;
    if ((table->keyType == HASH_BY_ID && hash_search_id(table, id)) ||
        (table->keyType == HASH_BY_NAME && hash_search_name(table, name))) return 0;
    if (hash_load_factor(table) > 0.75 && !hash_rehash(table)) return 0;
    index = hash_index(table, id, name);
    node = (HashNode *)malloc(sizeof(HashNode));
    if (!node) return 0;
    node->id = id;
    snprintf(node->name, sizeof(node->name), "%s", name);
    node->data = data;
    node->next = table->buckets[index];
    table->buckets[index] = node;
    table->count++;
    return 1;
}

int hash_delete(HashTable *table, int id, const char *name) {
    size_t index;
    HashNode **link;
    if (!table || !table->buckets) return 0;
    index = hash_index(table, id, name);
    link = &table->buckets[index];
    while (*link) {
        HashNode *node = *link;
        int match = table->keyType == HASH_BY_ID
            ? node->id == id
            : name_compare(node->name, name) == 0;
        if (match) {
            *link = node->next;
            free(node);
            table->count--;
            return 1;
        }
        link = &node->next;
    }
    return 0;
}

double hash_load_factor(const HashTable *table) {
    if (!table || table->size == 0) return 0.0;
    return (double)table->count / (double)table->size;
}

int hash_rehash(HashTable *table) {
    HashNode **oldBuckets = table->buckets;
    size_t oldSize = table->size;
    size_t i;
    HashTable replacement;
    if (!hash_init(&replacement, table->keyType, next_prime(oldSize + 1))) return 0;
    for (i = 0; i < oldSize; i++) {
        HashNode *node = oldBuckets[i];
        while (node) {
            HashNode *next = node->next;
            HashNode *copy;
            size_t index = hash_index(&replacement, node->id, node->name);
            copy = (HashNode *)malloc(sizeof(HashNode));
            if (!copy) {
                size_t j;
                for (j = 0; j < replacement.size; j++) {
                    HashNode *n = replacement.buckets[j];
                    while (n) { HashNode *tmp = n->next; free(n); n = tmp; }
                }
                free(replacement.buckets);
                return 0;
            }
            *copy = *node;
            copy->next = replacement.buckets[index];
            replacement.buckets[index] = copy;
            replacement.count++;
            node = next;
        }
    }
    for (i = 0; i < oldSize; i++) {
        HashNode *node = oldBuckets[i];
        while (node) { HashNode *next = node->next; free(node); node = next; }
    }
    free(oldBuckets);
    table->buckets = replacement.buckets;
    table->size = replacement.size;
    table->count = replacement.count;
    return 1;
}

HashStats hash_get_stats(const HashTable *table) {
    HashStats stats = {0, 0, 0.0, 0.0};
    size_t i, total = 0;
    if (!table || table->size == 0 || !table->buckets) return stats;
    stats.minChain = (size_t)-1;
    for (i = 0; i < table->size; i++) {
        size_t length = 0;
        HashNode *node;
        for (node = table->buckets[i]; node; node = node->next) length++;
        if (length < stats.minChain) stats.minChain = length;
        if (length > stats.maxChain) stats.maxChain = length;
        total += length;
    }
    stats.averageChain = (double)total / (double)table->size;
    stats.loadFactor = hash_load_factor(table);
    return stats;
}
