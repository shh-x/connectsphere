# ConnectSphere

ConnectSphere is a menu-driven C99 social-network friend-group manager. It
uses a hash table for fast lookup, an AVL tree for a sorted directory,
disjoint sets for friend-group connectivity, a binary heap for ranked
suggestions, and merge sort for community and directory rankings.

## Phase 1 architecture

### Synchronised system state

The application keeps one in-memory `UserDirectory` containing the authoritative
set of users and adjacency-list friendships. Every mutation updates the
directory and all derived structures together:

1. `ID_HASH` and `NAME_HASH` identify a user through both lookup keys.
2. `AVL_ROOT` keeps names in case-insensitive alphabetical order.
3. `DSU` represents friendship components.
4. `ADJACENCY` stores reciprocal undirected edges for friend queries and
   suggestion scoring.

No public operation may update only one structure. Adding a user inserts it
into the directory, both hash tables, the AVL tree, and DSU. Adding an edge
adds reciprocal adjacency entries and unions the corresponding DSU roots.
Removing an edge rebuilds DSU from the remaining edges. Deleting a user first
removes all incident edges, rebuilds DSU, removes the user from both hash
tables and the AVL tree, and frees its adjacency list.

### Proposed core structs

```c
typedef struct User {
    int id;
    char name[50];
    int age;
    char city[40];
    int friendCount;
} User;

typedef struct FriendNode {
    int userId;
    struct FriendNode *next;
} FriendNode;

typedef struct HashNode {
    int id;
    char *name;
    User *user;
    struct HashNode *next;
} HashNode;

typedef struct {
    HashNode **buckets;
    size_t size;
    size_t count;
} HashTable;

typedef struct AVLNode {
    char name[50];
    int userId;
    int height;
    struct AVLNode *left;
    struct AVLNode *right;
} AVLNode;

typedef struct HeapNode {
    int userId;
    int score;
} HeapNode;

typedef struct {
    HeapNode *items;
    size_t size;
    size_t capacity;
} Heap;

typedef struct {
    int rootId;
    int size;
    char representativeName[50];
} GroupInfo;

typedef struct {
    int *parent;
    int *rank;
    int *size;
    int capacity;
} DSU;

typedef struct {
    User **users;
    size_t userCount;
    size_t userCapacity;
    FriendNode **adjacency;
    size_t adjacencyCapacity;
    HashTable *idHash;
    HashTable *nameHash;
    AVLNode *avlRoot;
    DSU dsu;
} Directory;
```

`HashNode` is shown as a unified hash-chain concept. The implementation will
use separate ID and name table entries, with the name key stored as a canonical
case-insensitive string and also converted to an integer before division.

### Project structure

```text
connectsphere/
  include/  user.h dsu.h hash.h avl.h heap.h mergesort.h graph.h
            storage.h utils.h perf.h
  src/      user.c dsu.c hash.c avl.c heap.c mergesort.c graph.c
            storage.c utils.c perf.c main.c
  data/     users.txt friendships.txt sample_data.txt
  tests/    test_dsu.c test_hash.c test_avl.c test_heap.c
            test_mergesort.c test_integration.c
  Makefile
  README.md
```

### Missing rules and proposed assumptions

- User names must contain 1--49 characters and may contain letters, spaces,
  apostrophes, periods, and hyphens. The directory compares names
  case-insensitively and trims surrounding whitespace.
- IDs are positive and unique. A user can provide an ID, or the application
  assigns the smallest unused positive ID.
- Age is in the inclusive range 1--120.
- City names have 1--39 characters.
- Friendship removal and user deletion rebuild DSU in O(V + E).
- Heap suggestion ties compare smaller user ID first. A larger heap score is
  extracted first.
- Popular-user rankings rank by current `friendCount` descending, then ID
  ascending.
- Merge-sort community ties use DSU root ID ascending.
- Persistence is authoritative in the two requested text files. Each line is
  validated independently; malformed lines are reported and skipped.
- The menu defaults to save confirmation on exit. If both data files are
  missing or unreadable, loading reports a warning and starts an empty
  directory.
- Performance datasets are synthetic and do not modify persistent application
  data.

### Why these structures

Union-Find is appropriate because connectivity and component size are common
and near-constant-time under both optimisations. Hash tables provide direct
ID/name lookup. The AVL tree provides predictable sorted listings and range
queries. A max-heap extracts highest-mutual-friend candidates and top popular
users efficiently. Merge sort provides stable, predictable ranking and reusable
comparator-based sorting of user records and community groups.

Build and test instructions will be added with the implementation phases.
