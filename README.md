# ConnectSphere

ConnectSphere is a social-network manager with a C99 algorithm engine, an
Express REST bridge, and a responsive browser UI. The terminal client remains
available. Graph operations, ranking, lookup structures, and benchmarks run in
C; JavaScript transports and displays those results.

## Features

- Add, update, search, list, and delete users; create and remove friendships.
- Explore connected communities, mutual-friend recommendations, and popular
users.
- Search the alphabetical directory by prefix or inclusive name range and
inspect the AVL tree.
- Inspect the live hash tables, disjoint-set state, and binary heap.
- View and focus the friendship graph; run synthetic algorithm benchmarks.
- Load the built-in sample, load/save the text files, or reset the in-memory
workspace.
- Use the original terminal CLI for the same core data model and algorithms.

## Requirements

- GCC with C99 support.
- Node.js 18 or newer and npm.
- GNU Make for the convenient Linux/macOS build and test commands.

## Build and run on Windows

Open PowerShell in the repository root. Build both C programs with MinGW GCC:

```powershell
New-Item -ItemType Directory -Force bin | Out-Null
$modules = "src\utils.c src\hash.c src\dsu.c src\avl.c src\user.c src\graph.c src\heap.c src\mergesort.c src\storage.c src\perf.c"
gcc -Wall -Wextra -std=c99 -Iinclude -o bin\connectsphere_engine.exe src\engine_main.c src\json_protocol.c $modules.Split(" ")
gcc -Wall -Wextra -std=c99 -Iinclude -o bin\connectsphere.exe src\main.c src\cli_input.c $modules.Split(" ")
```

Run the terminal client:

```powershell
.\bin\connectsphere.exe
```

Install the web server dependencies and start the application:

```powershell
Set-Location server
npm.cmd ci
npm.cmd start
```

Open <http://localhost:3000>. The server starts with an empty in-memory
workspace. Use **Data management → Load sample** to explore the demo network,
or **Load from disk** to read `data/users.txt` and `data/friendships.txt`.

Run the protocol and API tests after building the engine:

```powershell
node tests\test_engine.js bin\connectsphere_engine.exe
Set-Location server
npm.cmd test
```

The API tests use an isolated temporary data directory. They do not overwrite
the repository's data files.

## Build and run on Linux/macOS

From the repository root:

```sh
make all
cd server
npm ci
npm start
```

Open <http://localhost:3000>. To use the CLI, return to the repository root
and run `./bin/connectsphere`. Run the full suite from the root with
`make tests` (after installing server dependencies with `cd server && npm ci`).

## Persistence and data safety

The server owns one in-memory dataset shared by all browser clients. User and
friendship changes are not persisted until saved. In **Data management**:

- **Load sample** replaces the live in-memory workspace with demo data; it
does not write files.
- **Load from disk** replaces the live workspace from the two text files.
- **Save to disk** overwrites those files with the current live workspace and
asks for confirmation.
- **Reset workspace** clears memory only; it leaves saved files unchanged.

The files are resolved relative to the engine working directory (the project
root by default). Keep backups of custom data before saving over it. Data
format details and routes are in [docs/api.md](./docs/api.md).

## Architecture

```text
Browser: web/index.html + CSS + ES modules
                    │ REST / JSON
Node: server/server.js + routes + engineBridge
                    │ one JSON object per line
C: src/engine_main.c + C data structures and algorithms
                    │
Persistence: data/users.txt, data/friendships.txt
```

The C process owns the directory and handles all algorithmic decisions. The
Node bridge serializes requests to the persistent process, validates API
inputs, and exposes HTTP endpoints. The browser requests data and renders
views; graph positioning and display formatting are client-side presentation
only. See [docs/architecture.md](./docs/architecture.md).

## Data structures and complexity

The implementation uses separate-chaining hash tables, an AVL name index,
adjacency lists, disjoint sets with path compression and union by rank, a
binary heap, and merge sort. See the [complexity guide](./docs/complexity.md)
for operation costs and benchmark caveats.

## Tests

- C module and integration tests: `make tests`.
- C engine JSON-lines protocol tests: `node tests/test_engine.js
  bin/connectsphere_engine` (use `.exe` on Windows).
- HTTP API integration tests: `cd server && npm test` (use `npm.cmd test` in
  PowerShell).
- Browser checks cover the live dashboard, user/friendship workflows,
  communities, suggestions, directory, inspector, graph, performance, and
  data-management screens.

On Windows, run `make tests` from a GNU Make shell such as MSYS2 or Git Bash.
The Makefile lists the exact GCC inputs for each individual C test binary.

## Limitations

- The dataset is global to one server process; there are no accounts or
  per-user workspaces.
- Persistence is two plain-text files, without transactions or concurrent
  multi-process coordination.
- The browser cannot upload arbitrary data files or choose server paths.
- Names use ASCII-oriented case-insensitive comparison.
- Performance results use synthetic inputs and are machine-dependent.

## Project and oral-exam notes

- [Architecture and JSON-lines protocol](./docs/architecture.md)
- [REST API reference](./docs/api.md)
- [Algorithm complexity and benchmark interpretation](./docs/complexity.md)
- [Viva questions and answers](./docs/viva.md)

The [viva guide](./docs/viva.md) also explains why the chosen structures fit
the supported operations and what changes were made to convert the terminal
application into a web app without moving algorithms into JavaScript.
