# Algorithm complexity

Let `V` be the number of users, `E` the number of undirected friendships,
`n` the number of entries in a structure, `k` the number of returned results,
and `g` the number of connected groups. Average hash costs assume a reasonable
distribution and load factor; worst-case collisions remain linear.

| Operation | Amortized/expected time | Single-operation/worst-case time | Notes |
|---|---:|---:|---|
| Hash lookup/insert/delete | O(1) | O(n) | Separate chaining; rehashing costs O(n + m) for new capacity `m`. |
| AVL lookup/insert/delete | O(log n) | O(log n) | AVL height remains logarithmic. |
| AVL in-order listing | O(n) | O(n) | Produces alphabetical results. |
| AVL range/prefix query | O(log n + k) | O(n) | Visits matching nodes plus traversal path. |
| DSU find/union | O(alpha(V)) amortized | O(log V) per operation | Path compression and union by rank. |
| Rebuild DSU after deletion | O(V + E) | O(V + E) | Replays active friendship edges. |
| Graph listing | O(V + E) | O(V + E) | Returns each user and undirected edge once. |
| Connected components | O((V + E) alpha(V)) amortized | O((V + E) log V) | Graph traversal/DSU work plus group collection. |
| Community ranking | O(g log g) | O(g log g) | C merge sort after components have been collected. |
| Heap insert/extract | O(log n) | O(log n) | A single priority-queue operation. |
| Bottom-up heap build | O(n) | O(n) | Linear-time heap construction. |
| Heap-based top-k | O(n + k log n) | O(n + k log n) | Candidate preparation is additional to heap operations. |
| Merge sort | O(n log n) | O(n log n) | Stable, with O(n) auxiliary memory. |

`alpha(n)` is the inverse Ackermann function and grows so slowly that it is
at most a small constant for practical input sizes.

## Memory considerations

Hash storage, user records, and DSU arrays are O(V). The adjacency lists store
O(V + E). AVL nodes and heap arrays are O(V) in the worst case. The C engine
uses DSU arrays indexed by user ID, so a large sparse ID can require more DSU
capacity than the number of actual users; API IDs are limited to 1,000,000.

## Interpreting the performance lab

The C benchmark generates synthetic data and reports lookup, sorting,
Union-Find, and AVL measurements. Lookup benchmarks run at most 1,000 matching
queries. At `N=50,000`, the quadratic sorting baseline uses only the first
10,000-record comparison dataset; the report prints the selected sort count.
The unbalanced BST height is reported theoretically rather than constructing
that worst-case tree. Timings use process CPU time and are sensitive to
compiler, optimization flags, hardware, and background load. Near-zero
measurements should not be treated as precise microbenchmarks.
