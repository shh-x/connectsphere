# ConnectSphere REST API

The Express server is a thin transport bridge. It validates HTTP inputs and
sends one JSON command at a time to the persistent C engine. Engine responses
use `{"ok":true,"data":...,"meta":...}` or
`{"ok":false,"error":"...","meta":{"httpStatus":...}}`.

Default address: `http://localhost:3000/api`.

## Users

```http
POST /users
Content-Type: application/json

{"id":7,"name":"Ada Lovelace","age":36,"city":"London"}
```

```http
GET    /users/7
GET    /users/by-name/ada%20lovelace
GET    /users?sort=name
GET    /users?sort=age
GET    /users?sort=friends
PUT    /users/7
DELETE /users/7
```

For `PUT`, send `{"name":"Ada Byron","age":37,"city":"London"}`. User IDs must
be positive integers no greater than 1,000,000; names are 1--49 ASCII letters,
spaces, apostrophes, periods, or hyphens; ages are 1--120; cities are
1--39 characters and cannot contain commas/control characters. Names are
unique without case sensitivity.

## Friendships and groups

```http
POST   /friendships       {"id1":7,"id2":8}
DELETE /friendships       {"id1":7,"id2":8}
GET    /users/7/friends
GET    /connected?u=7&v=8
GET    /groups
GET    /groups/ranked
GET    /users/7/group
```

Edges are undirected. Self-edges are rejected; duplicate edges return 409.
Edge deletion rebuilds the C disjoint-set structure from remaining edges.

## Suggestions and directory

```http
GET /users/7/suggestions?k=5
GET /popular?k=5
GET /directory/range?from=Ada&to=Grace
GET /directory/prefix?q=Al
GET /directory/tree
```

Suggestions and popularity use the C binary heap. Range, prefix, and sorted
name listings use the C AVL tree. Range endpoints are inclusive and use the
directory's name comparison. `/directory/tree` returns the C AVL topology,
including each node's `id`, `name`, `height`, `balance`, `left`, and `right`.

## Graph and structure views

`GET /graph` returns:

```json
{
  "nodes": [{"id": 7, "name": "Ada Lovelace", "friendCount": 1, "groupId": 7}],
  "edges": [{"source": 7, "target": 8}]
}
```

Each undirected friendship is represented once in `edges`. A node's `groupId`
is the C disjoint-set representative. The frontend may compute display
coordinates, but graph membership and edges are supplied by the C engine.

`GET /structures/hash` returns the ID and name hash bucket chains and load
statistics. `/structures/dsu` returns parent, rank, set size, and root status
for each active user. `/structures/heap` returns the current C heap array;
it is empty until a heap-based query has been run.

## Performance and data operations

```http
POST /performance/run     {"n":1000}
POST /data/sample
POST /data/save
POST /data/load
POST /data/reset
```

`n` for performance is one of 1,000, 10,000, or 50,000. The response data for
that endpoint contains a human-readable C-generated `report` string.

Data operations affect the single in-memory workspace as follows:

- `POST /data/sample` replaces it with the built-in demo network and does not
  write files.
- `POST /data/load` replaces it from `data/users.txt` and
  `data/friendships.txt` under `ENGINE_CWD`.
- `POST /data/save` overwrites those two files with the current workspace.
- `POST /data/reset` clears memory only and does not change saved files.

Arbitrary filesystem paths and file uploads are not accepted. The engine is
one server-wide in-memory dataset; users and browsers are not isolated.

## Status codes

- `200`: operation completed.
- `400`: malformed request or invalid field.
- `403`: disallowed browser origin.
- `404`: requested user, friendship, or route does not exist.
- `409`: duplicate user or friendship.
- `429`: IP rate limit exceeded.
- `500`: engine or server failure.

## Server configuration

Environment variables:

- `PORT` (default `3000`)
- `ENGINE_PATH` (defaults to the platform's `bin/connectsphere_engine`)
- `ENGINE_CWD` (default project root; useful for isolated deployments/tests)
- `CORS_ORIGIN` (comma-separated exact origins; defaults to localhost ports
  5173)
- `RATE_WINDOW_MS` (default `60000`)
- `RATE_LIMIT` (default `120` requests per IP per window)

The bridge uses `spawn()` with an argument array and `shell:false`, serializes
engine access, applies an eight-second timeout to ordinary requests and a
60-second timeout to performance requests, and restarts a crashed engine.
CORS does not enable credentials.
