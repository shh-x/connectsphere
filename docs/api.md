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
name listings use the C AVL tree.

## Structure views and data operations

```http
GET  /structures/hash
GET  /structures/dsu
GET  /structures/heap
GET  /graph
POST /performance/run     {"n":1000}
POST /data/sample
POST /data/save
POST /data/load
POST /data/reset
```

`n` for performance is one of 1,000, 10,000, or 50,000. Saving and loading
always use the server's `data/users.txt` and `data/friendships.txt`. Arbitrary
filesystem paths and file uploads are not accepted. The engine is one
server-wide in-memory dataset; this initial version does not isolate users or
browsers.

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
engine access, applies an eight-second request timeout, and restarts a crashed
engine. CORS does not enable credentials.
