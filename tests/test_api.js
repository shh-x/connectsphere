"use strict";

const assert = require("node:assert/strict");
const { spawn } = require("node:child_process");
const fs = require("node:fs");
const os = require("node:os");
const path = require("node:path");
const net = require("node:net");

const root = path.resolve(__dirname, "..");
const temporaryDirectory = fs.mkdtempSync(path.join(os.tmpdir(), "connectsphere-api-"));
fs.mkdirSync(path.join(temporaryDirectory, "data"));
fs.writeFileSync(path.join(temporaryDirectory, "data", "users.txt"), "");
fs.writeFileSync(path.join(temporaryDirectory, "data", "friendships.txt"), "");

function freePort() {
  return new Promise((resolve, reject) => {
    const listener = net.createServer();
    listener.once("error", reject);
    listener.listen(0, "127.0.0.1", () => {
      const { port } = listener.address();
      listener.close((error) => error ? reject(error) : resolve(port));
    });
  });
}

async function waitForServer(baseUrl, child) {
  for (let attempt = 0; attempt < 100; attempt++) {
    if (child.exitCode !== null) throw new Error(`Server exited with ${child.exitCode}`);
    try {
      const response = await fetch(`${baseUrl}/api/users`);
      if (response.ok) return;
    } catch { }
    await new Promise((resolve) => setTimeout(resolve, 50));
  }
  throw new Error("Server did not become ready");
}

async function removeTemporaryDirectory() {
  for (let attempt = 0; attempt < 10; attempt++) {
    try {
      fs.rmSync(temporaryDirectory, { recursive: true, force: true });
      return;
    } catch (error) {
      if (error.code !== "EPERM" || attempt === 9) throw error;
      await new Promise((resolve) => setTimeout(resolve, 100));
    }
  }
}

async function run() {
  const port = await freePort();
  const baseUrl = `http://127.0.0.1:${port}`;
  const child = spawn(process.execPath, [path.join(root, "server", "server.js")], {
    cwd: root,
    env: {
      ...process.env,
      PORT: String(port),
      ENGINE_PATH: path.join(root, "bin",
        process.platform === "win32" ? "connectsphere_engine.exe" : "connectsphere_engine"),
      ENGINE_CWD: temporaryDirectory,
      CORS_ORIGIN: "http://allowed.test"
    },
    stdio: ["ignore", "pipe", "pipe"],
    windowsHide: true
  });
  let serverOutput = "";
  child.stdout.on("data", (chunk) => { serverOutput += chunk.toString(); });
  child.stderr.on("data", (chunk) => { serverOutput += chunk.toString(); });

  const call = async (method, endpoint, body, origin) => {
    const response = await fetch(`${baseUrl}${endpoint}`, {
      method,
      headers: {
        ...(body === undefined ? {} : { "content-type": "application/json" }),
        ...(origin ? { origin } : {})
      },
      ...(body === undefined ? {} : { body: JSON.stringify(body) })
    });
    const payload = response.status === 204 ? null : await response.json();
    return { status: response.status, payload, headers: response.headers };
  };

  try {
    await waitForServer(baseUrl, child);
    let result = await call("OPTIONS", "/api/users", undefined, "http://allowed.test");
    assert.equal(result.status, 204);
    result = await call("GET", "/api/users", undefined, "http://blocked.test");
    assert.equal(result.status, 403);
    result = await call("POST", "/api/users", {
      id: 1, name: "Alice", age: 28, city: "London \"West\""
    });
    assert.equal(result.status, 200);
    result = await call("POST", "/api/users", {
      id: 2, name: "Bob", age: 24, city: "Pune"
    });
    assert.equal(result.status, 200);
    result = await call("POST", "/api/users", {
      id: 3, name: "Carol", age: 31, city: "Delhi"
    });
    assert.equal(result.status, 200);

    assert.equal((await call("POST", "/api/users", {
      id: 1, name: "Duplicate", age: 30, city: "Pune"
    })).status, 409);
    assert.equal((await call("POST", "/api/users", {
      id: 4, name: "Invalid", age: 121, city: "Pune"
    })).status, 400);
    assert.equal((await call("GET", "/api/users/999")).status, 404);
    assert.equal((await call("GET", "/api/users/not-an-id")).status, 400);

    assert.equal((await call("GET", "/api/users")).payload.data.length, 3);
    assert.equal((await call("GET", "/api/users?sort=age")).payload.data[0].name, "Bob");
    assert.equal((await call("GET", "/api/users?sort=friends")).status, 200);
    assert.equal((await call("GET", "/api/users?sort=invalid")).status, 400);
    assert.equal((await call("GET", "/api/users/1")).payload.data.name, "Alice");
    assert.equal((await call("GET", "/api/users/by-name/alice")).payload.data.id, 1);
    assert.equal((await call("PUT", "/api/users/2", {
      name: "Bobby", age: 25, city: "Pune"
    })).payload.data.name, "Bobby");
    assert.equal((await call("GET", "/api/users/by-name/Bobby")).payload.data.id, 2);

    assert.equal((await call("POST", "/api/friendships", { id1: 1, id2: 2 })).status, 200);
    assert.equal((await call("POST", "/api/friendships", { id1: 2, id2: 3 })).status, 200);
    assert.equal((await call("POST", "/api/friendships", { id1: 1, id2: 1 })).status, 400);
    assert.equal((await call("POST", "/api/friendships", { id1: 1, id2: 2 })).status, 409);
    assert.equal((await call("GET", "/api/users/1/friends")).payload.data[0].id, 2);
    assert.equal((await call("GET", "/api/connected?u=1&v=3")).payload.data.connected, true);
    assert.equal((await call("GET", "/api/groups")).status, 200);
    assert.equal((await call("GET", "/api/groups/ranked")).status, 200);
    assert.equal((await call("GET", "/api/users/1/group")).payload.data.size, 3);
    assert.equal((await call("GET", "/api/users/1/suggestions?k=5")).payload.data[0].user.id, 3);
    assert.equal((await call("GET", "/api/popular?k=2")).payload.data.length, 2);
    assert.equal((await call("GET", "/api/popular?k=0")).status, 400);
    assert.equal((await call("GET", "/api/directory/range?from=Alice&to=Carol")).status, 200);
    assert.equal((await call("GET", "/api/directory/prefix?q=Ca")).payload.data[0].name, "Carol");
    assert.equal((await call("GET", "/api/directory/tree")).status, 200);
    assert.equal((await call("GET", "/api/structures/hash")).status, 200);
    assert.equal((await call("GET", "/api/structures/dsu")).status, 200);
    assert.equal((await call("GET", "/api/structures/heap")).status, 200);
    assert.equal((await call("GET", "/api/graph")).status, 200);
    assert.equal((await call("POST", "/api/performance/run", { n: 1000 })).status, 200);
    assert.equal((await call("POST", "/api/performance/run", { n: 12 })).status, 400);

    assert.equal((await call("POST", "/api/data/save")).status, 200);
    assert.equal((await call("POST", "/api/data/reset")).status, 200);
    assert.equal((await call("GET", "/api/users/1")).status, 404);
    assert.equal((await call("POST", "/api/data/load")).status, 200);
    assert.equal((await call("GET", "/api/users/1")).status, 200);
    assert.equal((await call("DELETE", "/api/friendships", { id1: 2, id2: 3 })).status, 200);
    assert.equal((await call("DELETE", "/api/users/3")).status, 200);
    assert.equal((await call("POST", "/api/data/sample")).payload.data.users, 18);
    assert.equal((await call("POST", "/api/data/reset")).status, 200);

    console.log("REST API tests passed for all endpoints, validation, CORS, and persistence.");
  } finally {
    child.kill("SIGTERM");
    await new Promise((resolve) => {
      if (child.exitCode !== null) resolve();
      else child.once("exit", resolve);
      setTimeout(resolve, 3000).unref();
    });
    await removeTemporaryDirectory();
    if (child.exitCode !== 0 && child.exitCode !== null)
      throw new Error(`Server failed (${child.exitCode}): ${serverOutput}`);
  }
}

run().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
