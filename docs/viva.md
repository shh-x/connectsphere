# Viva questions and answers

## Data structures and algorithms

1. **Why use a hash table?** It provides expected constant-time ID and name
   lookup, while separate chaining handles collisions.
2. **Why keep an AVL tree as well?** It supports predictable logarithmic
   search and alphabetical, range, and prefix traversals.
3. **What does an AVL balance factor represent?** The height difference
   between a node's left and right subtrees.
4. **Which AVL rotations are possible?** LL, RR, LR, and RL.
5. **Why use a disjoint-set structure?** Connectivity and component queries
   are efficient with path compression and union by rank.
6. **Why rebuild DSU after deleting an edge?** DSU supports merging
   components, but does not support splitting them directly.
7. **How are friendships represented?** As reciprocal entries in adjacency
   lists; the HTTP graph endpoint emits each undirected edge once.
8. **Why use a max-heap for suggestions?** It exposes the highest scores first
   without sorting every candidate.
9. **How are suggestion scores decided?** The C engine counts mutual friends;
   the browser only renders the returned score and names.
10. **Why use merge sort for community ranking?** It provides stable
    O(n log n) ordering and supports deterministic tie-breaking.

## Web architecture

11. **What is the C engine's role?** It owns the live directory and runs
    algorithms; it communicates through one JSON object per input/output line.
12. **Why keep the engine as a persistent process?** It avoids repeatedly
    initializing the C data model and preserves in-memory state between
    requests.
13. **What does the Node server do?** Validates HTTP input, queues engine
    commands, maps responses to HTTP, and serves the static frontend.
14. **Does the browser compute graph components or rankings?** No. It receives
    graph edges, group IDs, rankings, and scores from C; browser code handles
    presentation and visual layout.
15. **How is a request matched to an engine response?** The bridge sends one
    request at a time from a queue and resolves it with the next response line.
16. **How is data saved?** The C engine writes users and edges to two
    server-side text files. Save/load operations require explicit user action.

## Limitations and design decisions

17. **Is workspace state isolated by user?** No. One server process shares one
    in-memory workspace among all clients.
18. **What happens when the process restarts?** In-memory changes are lost
    unless saved; startup does not automatically load the text files.
19. **Can the browser select arbitrary server file paths?** No. The API uses
    fixed configured data filenames and does not expose arbitrary paths.
20. **What does the performance lab measure?** Synthetic C workloads for
    lookup, sorting, Union-Find, and AVL height; results depend on the machine
    and compiler and are demonstrations, not rigorous benchmarking.

## What changed in the web conversion

- Separated terminal input helpers from the C algorithm operations.
- Added a persistent C JSON-lines engine and a validated Express HTTP bridge.
- Added browser pages for user and friendship management, communities,
  suggestions, the AVL directory, structure inspection, graph visualization,
  performance, and data management.
- Kept graph semantics, rankings, data-structure views, and benchmarks in C.
- Added protocol, API, and browser validation while preserving the original
  CLI and protecting local data files during tests.
