#ifndef HASH_H
#define HASH_H

#include <stddef.h>

typedef struct HashNode {
    int id;
    char name[50];
    void *data;
    struct HashNode *next;
} HashNode;

typedef enum { HASH_BY_ID, HASH_BY_NAME } HashKeyType;

typedef struct HashTable {
    HashNode **buckets;
    size_t size;
    size_t count;
    HashKeyType keyType;
} HashTable;

/* Purpose: Initialize a hash table with a prime bucket count.
   Parameters: table, key type, initial prime.
   Return: 1 on success, 0 on failure.
   Time complexity: O(m). */
int hash_init(HashTable *table, HashKeyType keyType, size_t initialSize);

/* Purpose: Free all hash-chain nodes and buckets.
   Parameters: table.
   Return: void.
   Time complexity: O(n + m). */
void hash_free(HashTable *table);

/* Purpose: Insert an ID/name record.
   Parameters: table, ID, name, associated data.
   Return: 1 on success, 0 on duplicate/allocation failure.
   Time complexity: Average O(1), worst O(n); rehash amortized. */
int hash_insert(HashTable *table, int id, const char *name, void *data);

/* Purpose: Search by ID.
   Parameters: table, ID.
   Return: matching node or NULL.
   Time complexity: Average O(1), worst O(n). */
HashNode *hash_search_id(HashTable *table, int id);

/* Purpose: Search by name.
   Parameters: table, name.
   Return: matching node or NULL.
   Time complexity: Average O(1), worst O(n). */
HashNode *hash_search_name(HashTable *table, const char *name);

/* Purpose: Delete a record by ID or name.
   Parameters: table, ID, name.
   Return: 1 on success, 0 if absent.
   Time complexity: Average O(1), worst O(n). */
int hash_delete(HashTable *table, int id, const char *name);

/* Purpose: Resize to the next prime when load factor exceeds 0.75.
   Parameters: table.
   Return: 1 on success, 0 on allocation failure.
   Time complexity: O(n + m). */
int hash_rehash(HashTable *table);

/* Purpose: Get current load factor.
   Parameters: table.
   Return: count divided by bucket count.
   Time complexity: O(1). */
double hash_load_factor(const HashTable *table);

/* Purpose: Print buckets and chain contents.
   Parameters: table.
   Return: void.
   Time complexity: O(n + m). */
void hash_display(const HashTable *table);

/* Purpose: Print chain-length statistics.
   Parameters: table.
   Return: void.
   Time complexity: O(m). */
void hash_chain_stats(const HashTable *table);

#endif
