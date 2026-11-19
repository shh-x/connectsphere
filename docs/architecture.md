# ConnectSphere architecture

## Components

```text
web/                         HTML, CSS, ES modules, REST client
server/server.js             Express app, CORS, rate limiting, static UI
server/routes/api.js         HTTP validation and command mapping
server/engineBridge.js       Persistent C child process and request queue
src/engine_main.c            JSON-lines command dispatcher and Directory owner
src/*.c, include/*.h         Data structures, algorithms, persistence, CLI
data/users.txt               Saved users
data/friendships.txt         Saved undirected edges
tests/                       C, protocol, and HTTP integration tests
```

The original source tree and terminal program remain in place. `main.c` and
`cli_input.c` own interactive terminal input/output. Core modules expose
operations and results; they do not depend on the web presentation layer.

## Request flow

1. A browser page requests `/api/...` or submits a JSON body.
2. Express validates fields and maps the request to an allow-listed C command.
3. `EngineBridge` serializes access through one queue and writes one JSON
   object followed by a newline to the long-lived C child.
4. The C engine parses the line, runs the operation against its in-memory
   `Directory`, and returns exactly one JSON response line.
5. Express maps engine status to an HTTP response; the browser renders the
   returned data.

The protocol uses flat command objects with string, integer, and boolean
fields. C diagnostics are sent to standard error, never mixed with protocol
responses on standard output. JSON strings are escaped by the C builder.

## C-owned behavior

- Hash tables provide ID and name lookup.
- The AVL tree provides alphabetical order, prefix/range traversal, and tree
  metadata.
- Adjacency lists store undirected friendships.
- Disjoint sets provide connectivity and component membership.
- A binary heap ranks friend suggestions and popular users.
- Merge sort ranks connected communities.
- The C performance module measures synthetic data-structure operations.

The browser can position graph nodes, format responses, and highlight edges
that the C graph returned. It does not compute connectivity, rankings,
suggestion scores, AVL topology, or hash placement.

## State and persistence

The C engine initializes with an empty in-memory directory. The web interface
offers explicit sample loading, file loading, saving, and memory reset. The
default file paths are `data/users.txt` and `data/friendships.txt`, resolved
from the engine working directory (the repository root by default).

- `sample` replaces in-memory state with the demo network; it does not save.
- `load` replaces in-memory state with records read from the two files.
- `save` writes in-memory state to those files.
- `reset` clears in-memory state and leaves files unchanged.

The dataset is one server-wide workspace, not a per-browser session. The API
does not accept arbitrary filesystem paths or uploads.

## Runtime configuration

See [api.md](./api.md#server-configuration) for environment variables,
validation rules, status codes, and limits. The default HTTP port is 3000.
The engine bridge uses `spawn()` with `shell:false`, queues commands, applies
an eight-second timeout to ordinary requests, and retries engine restarts with
a bounded policy. Performance requests have a separate 60-second timeout to
accommodate the supported large synthetic benchmark.

## Error behavior

Invalid input returns a structured error; engine errors are not converted into
success-shaped empty results. The HTTP server applies centralized error
responses, and the UI presents failed requests through visible messages and
toasts. The engine is single-threaded and processes one command at a time.

## Deliberate scope limits

The first web version does not provide authentication, multiple workspaces,
database storage, file upload/download, or cross-process locking. These would
require separate authorization, persistence, and deployment design.
