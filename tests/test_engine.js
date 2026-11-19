const assert = require("node:assert/strict");
const { spawn } = require("node:child_process");
const readline = require("node:readline");

const enginePath = process.argv[2] || "bin/connectsphere_engine.exe";
const engine = spawn(enginePath, [], { stdio: ["pipe", "pipe", "inherit"] });
const lines = readline.createInterface({ input: engine.stdout });
const responses = [];
let wake;

lines.on("line", (line) => {
  responses.push(JSON.parse(line));
  if (wake) {
    wake();
    wake = undefined;
  }
});

function command(input) {
  return new Promise((resolve, reject) => {
    const previousCount = responses.length;
    wake = () => resolve(responses[previousCount]);
    engine.stdin.write(`${input}\n`);
    const timeout = setTimeout(() => {
      wake = undefined;
      reject(new Error("Timed out waiting for engine response"));
    }, 5000);
    const originalWake = wake;
    wake = () => {
      clearTimeout(timeout);
      originalWake();
    };
  });
}

async function request(payload) {
  return command(JSON.stringify(payload));
}

async function run() {
  let response = await command("{");
  assert.equal(response.ok, false);
  assert.equal(response.meta.httpStatus, 400);

  response = await request({ cmd: "not_a_command" });
  assert.equal(response.ok, false);

  response = await request({
    cmd: "add_user", id: 1, name: "Alice", age: 28,
    city: "London \"West\""
  });
  assert.equal(response.ok, true);
  assert.equal(response.data.city, "London \"West\"");

  response = await request({ cmd: "get_user", id: 1 });
  assert.equal(response.data.name, "Alice");

  response = await request({
    cmd: "add_user", id: 1, name: "Other", age: 20, city: "Pune"
  });
  assert.equal(response.ok, false);
  assert.equal(response.meta.httpStatus, 409);

  response = await request({
    cmd: "add_user", id: 2, name: "Alice \"Quoted\"", age: 24, city: "Pune"
  });
  assert.equal(response.ok, false);
  assert.equal(response.meta.httpStatus, 400);

  response = await request({
    cmd: "add_user", id: 2, name: "Bob", age: 24, city: "Pune"
  });
  assert.equal(response.ok, true);
  response = await request({
    cmd: "add_user", id: 3, name: "Carol", age: 31, city: "Delhi"
  });
  assert.equal(response.ok, true);

  response = await request({ cmd: "add_friend", id1: 1, id2: 2 });
  assert.equal(response.ok, true);
  response = await request({ cmd: "add_friend", id1: 2, id2: 3 });
  assert.equal(response.ok, true);
  response = await request({ cmd: "connected", u: 1, v: 3 });
  assert.equal(response.data.connected, true);
  assert.ok(response.data.pathU.length >= 1);
  response = await request({ cmd: "suggestions", id: 1, k: 5 });
  assert.equal(response.data[0].user.name, "Carol");
  assert.deepEqual(response.data[0].mutualFriends, ["Bob"]);
  response = await request({ cmd: "heap_view" });
  assert.equal(response.data[0].id, 3);

  for (const payload of [
    { cmd: "list_users", sort: "name" },
    { cmd: "list_users", sort: "age" },
    { cmd: "groups" },
    { cmd: "group", id: 1 },
    { cmd: "popular", k: 2 },
    { cmd: "directory_range", from: "Alice", to: "Carol" },
    { cmd: "directory_prefix", q: "Ca" },
    { cmd: "directory_tree" },
    { cmd: "hash_view" },
    { cmd: "dsu_view" },
    { cmd: "graph" },
  ]) {
    response = await request(payload);
    assert.equal(response.ok, true, payload.cmd);
  }

  response = await request({ cmd: "popular", k: 0 });
  assert.equal(response.ok, false);
  response = await request({ cmd: "connected", u: 1 });
  assert.equal(response.ok, false);
  response = await request({ cmd: "delete_user", id: 3 });
  assert.equal(response.ok, true);
  response = await request({ cmd: "reset" });
  assert.equal(response.ok, true);
  response = await request({ cmd: "list_users" });
  assert.deepEqual(response.data, []);

  response = await command(`${"x".repeat(70000)}`);
  assert.equal(response.ok, false);
  assert.equal(response.meta.httpStatus, 400);

  engine.stdin.end();
  await new Promise((resolve, reject) => {
    engine.on("exit", (code) => code === 0 ? resolve() : reject(new Error(`Engine exited ${code}`)));
  });
  console.log("Engine protocol tests passed (malformed, commands, validation, escaping, and state).");
}

run().catch((error) => {
  engine.kill();
  console.error(error);
  process.exitCode = 1;
});
