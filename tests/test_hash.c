#include <assert.h>
#include <stdlib.h>
#include "../include/hash.h"

int main(void) {
    HashTable table;
    int data = 42;
    assert(hash_init(&table, HASH_BY_ID, 101));
    assert(hash_insert(&table, 1, "Alice", &data));
    assert(hash_insert(&table, 102, "Bob", &data));
    assert(hash_search_id(&table, 1) != NULL);
    assert(hash_search_id(&table, 999) == NULL);
    assert(hash_delete(&table, 1, "Alice"));
    assert(hash_search_id(&table, 1) == NULL);
    assert(hash_load_factor(&table) >= 0.0);
    hash_free(&table);
    return 0;
}
