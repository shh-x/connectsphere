# ConnectSphere Web Architecture and Phase 1 Analysis

## Existing C project assessment

The current project already has a reusable data model in `include/user.h` and
mutation/search functions in `src/user.c`. It is not yet a print-free core:

- `src/main.c` owns the interactive menus and is the natural source for
  `main_cli.c`.
- `src/utils.c` combines input prompting/parsing with basic string helpers.
- `src/avl.c` mixes AVL operations with traversal output and rotation
  diagnostics.
- `src/hash.c` mixes hash operations with table/statistics display.
- `src/heap.c` prints its backing array.
- `src/graph.c` prints friends and groups; mutual-friend count is already
  returned as a value.
- `src/storage.c` mixes file persistence with user-facing status messages and
  has embedded sample records.
- `src/perf.c` both executes benchmarks and prints the results.
- `src/dsu.c`, `src/mergesort.c`, and most mutations in `src/user.c` are
  already mostly free of terminal output.

The application currently owns state in a file-static `Directory` in the
terminal `main`. Search and mutation methods operate directly on this state.
The Makefile builds one CLI executable and the existing module tests.

## Refactor plan

Preserve the algorithms and data model, moving existing modules under `core/`
without replacing their implementations. Split terminal presentation into
`core/src/main_cli.c` and keep existing menus calling the same core operations.
Expose structured, caller-owned results for traversals, graph queries,
suggestions, rankings, and structure-inspector data. Report operation metadata
using result structs rather than printing from the core. Keep persistence and
benchmark computation in C, returning status/results to the caller.

Add a C `engine_main.c` that owns one `Directory` for the process lifetime.
It reads one request per line, dispatches against the existing C functions,
and writes exactly one JSON response per line. A small parser will handle JSON
objects with known scalar fields and escaped strings; a bounded string-builder
will serialize JSON with correct escaping. Diagnostics go to stderr, never
stdout. The engine is transport-neutral apart from stdin/stdout.

Add a Node.js/Express server that validates HTTP inputs, queues one request at
a time to the persistent engine, matches each line response to the pending
request, enforces a response timeout, restarts a failed engine, applies a
configurable CORS origin policy and rate limit, and centralizes errors. It
will not implement any data-structure logic and will not construct shell
commands from input.

Add a plain HTML/CSS/ES-module frontend that consumes only the REST API.
Visualizations render data returned by C; the browser will not calculate
components, rankings, sorting, heap scores, or hash behavior.

## JSON protocol proposal

One UTF-8 JSON object per input line, one response object per output line:

```json
{"cmd":"add_user","id":42,"name":"Ada \"A\" Lovelace","age":36,"city":"London"}
{"ok":true,"data":{"user":{"id":42,"name":"Ada \"A\" Lovelace","age":36,"city":"London","friendCount":0}},"meta":{"elapsedUs":12}}
```

```json
{"cmd":"add_friend","id1":3,"id2":7}
{"ok":false,"error":"Users are already friends","meta":{"elapsedUs":4}}
```

Every response uses `ok`, optional `data`, optional `error`, and `meta`.
Commands mirror the REST resources: `add_user`, `get_user`, `get_user_by_name`,
`update_user`, `delete_user`, `list_users`, `add_friend`, `remove_friend`,
`list_friends`, `connected`, `groups`, `group`, `suggestions`, `popular`,
`directory_range`, `directory_prefix`, `directory_tree`, `hash_view`,
`dsu_view`, `heap_view`, `graph`, `performance`, `sample`, `save`, `load`,
and `reset`. Command fields are allow-listed and strictly typed; missing or
out-of-range fields return a structured error. Engine errors must never be
formatted as successful empty data.

Operation-specific metadata includes the selected AVL rotation, DSU path
length/compression information, hash bucket/comparison count, heap contents,
and elapsed microseconds when available. Timings use `clock()` in C and are
reported as non-negative integer microseconds.

## Proposed directory structure

```text
connectsphere/
  core/
    include/ src/ tests/
    engine_main.c
    Makefile
  server/
    server.js engineBridge.js routes/ validators/ package.json
  web/
    index.html css/ js/ assets/
  data/
    users.txt friendships.txt sample_data.txt
  docs/
    architecture.md api.md complexity.md viva.md
  README.md
```

The current repository root may host these directories incrementally; preserve
the terminal executable and current C tests throughout the migration.

## Rules needing an explicit implementation default

- Keep IDs positive and unique; ages are 1--120; names are 1--49 characters,
  unique case-insensitively; cities are 1--39 characters.
- JSON names/cities accept valid UTF-8 and escaped JSON control/quote
  characters; the existing C comparator/hash are ASCII case-insensitive, so
  non-ASCII case folding is not promised.
- Use a configurable `PORT` (default 3000), configurable `CORS_ORIGIN`
  (development defaults to localhost only), and a conservative IP request
  rate limit. The persistent engine is one global in-memory dataset, not
  per-browser/per-user state.
- `reset` clears memory only; `load` replaces memory with file contents;
  `save` writes the existing `data/users.txt` and `data/friendships.txt`.
  The server must start with an empty state and require explicit load/sample,
  unless the user chooses automatic startup loading.
- The engine reports one response for every valid or invalid input line,
  preserving protocol synchronization.
- Export/import file uploads and browser-side file paths require a separate
  decision; do not expose arbitrary server filesystem paths through the API.
- Rate limit, CORS and engine restart policy will be documented and configurable
  through environment variables, not user-provided shell arguments.

## Current worktree note

`data/users.txt` and `data/friendships.txt` have local modifications from before
this phase. They are user data and must not be overwritten or included in
unrelated commits. Migration tests should use temporary test paths.
