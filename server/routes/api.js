"use strict";

const express = require("express");
const {
  isRecord, positiveInt, requiredName, requiredCity,
  ageValue, topK, userFields
} = require("../validators");

function createApiRouter(engine) {
  const router = express.Router();
  const asyncRoute = (handler) => (req, res, next) => {
    Promise.resolve(handler(req, res, next)).catch(next);
  };
  for (const method of ["get", "post", "put", "delete"]) {
    const register = router[method].bind(router);
    router[method] = (...args) => {
      args[args.length - 1] = asyncRoute(args[args.length - 1]);
      return register(...args);
    };
  }
  const send = async (res, command) => {
    const response = await engine.send(command);
    if (!response || typeof response.ok !== "boolean") {
      const error = new Error("C engine returned an invalid response");
      error.status = 502;
      throw error;
    }
    return res.status(response.ok ? 200 : (response.meta?.httpStatus || 500)).json(response);
  };

  router.post("/users", async (req, res) => {
    const user = userFields(req.body);
    return send(res, { cmd: "add_user", ...user });
  });
  router.get("/users/by-name/:name", async (req, res) => {
    return send(res, { cmd: "get_user_by_name", name: requiredName(req.params.name) });
  });
  router.get("/users", async (req, res) => {
    const sort = req.query.sort || "name";
    if (!["name", "age", "friends"].includes(sort)) {
      const error = new Error("sort must be name, age, or friends");
      error.status = 400;
      throw error;
    }
    return send(res, { cmd: "list_users", sort });
  });
  router.get("/users/:id", async (req, res) => {
    return send(res, { cmd: "get_user", id: positiveInt(req.params.id) });
  });
  router.put("/users/:id", async (req, res) => {
    if (!isRecord(req.body)) {
      const error = new Error("JSON object body required");
      error.status = 400;
      throw error;
    }
    const id = positiveInt(req.params.id);
    const name = requiredName(req.body.name);
    const age = ageValue(req.body.age);
    const city = requiredCity(req.body.city);
    return send(res, { cmd: "update_user", id, name, age, city });
  });
  router.delete("/users/:id", async (req, res) => {
    return send(res, { cmd: "delete_user", id: positiveInt(req.params.id) });
  });

  router.post("/friendships", async (req, res) => {
    if (!isRecord(req.body)) {
      const error = new Error("JSON object body required");
      error.status = 400;
      throw error;
    }
    return send(res, {
      cmd: "add_friend",
      id1: positiveInt(req.body.id1, "id1"),
      id2: positiveInt(req.body.id2, "id2")
    });
  });
  router.delete("/friendships", async (req, res) => {
    if (!isRecord(req.body)) {
      const error = new Error("JSON object body required");
      error.status = 400;
      throw error;
    }
    return send(res, {
      cmd: "remove_friend",
      id1: positiveInt(req.body.id1, "id1"),
      id2: positiveInt(req.body.id2, "id2")
    });
  });
  router.get("/users/:id/friends", async (req, res) => {
    return send(res, { cmd: "list_friends", id: positiveInt(req.params.id) });
  });
  router.get("/connected", async (req, res) => {
    return send(res, {
      cmd: "connected",
      u: positiveInt(req.query.u, "u"),
      v: positiveInt(req.query.v, "v")
    });
  });

  router.get("/groups", async (_req, res) =>
    send(res, { cmd: "groups" }));
  router.get("/groups/ranked", async (_req, res) =>
    send(res, { cmd: "groups" }));
  router.get("/users/:id/group", async (req, res) =>
    send(res, { cmd: "group", id: positiveInt(req.params.id) }));

  router.get("/users/:id/suggestions", async (req, res) =>
    send(res, {
      cmd: "suggestions",
      id: positiveInt(req.params.id),
      k: topK(req.query.k)
    }));
  router.get("/popular", async (req, res) =>
    send(res, { cmd: "popular", k: topK(req.query.k) }));

  router.get("/directory/range", async (req, res) => {
    const from = requiredName(req.query.from, "from");
    const to = requiredName(req.query.to, "to");
    return send(res, { cmd: "directory_range", from, to });
  });
  router.get("/directory/prefix", async (req, res) =>
    send(res, { cmd: "directory_prefix", q: requiredName(req.query.q, "q") }));
  router.get("/directory/tree", async (_req, res) =>
    send(res, { cmd: "directory_tree" }));

  router.get("/structures/hash", async (_req, res) =>
    send(res, { cmd: "hash_view" }));
  router.get("/structures/dsu", async (_req, res) =>
    send(res, { cmd: "dsu_view" }));
  router.get("/structures/heap", async (_req, res) =>
    send(res, { cmd: "heap_view" }));
  router.get("/graph", async (_req, res) =>
    send(res, { cmd: "graph" }));

  router.post("/performance/run", async (req, res) => {
    if (!isRecord(req.body) || ![1000, 10000, 50000].includes(req.body.n)) {
      const error = new Error("n must be 1000, 10000, or 50000");
      error.status = 400;
      throw error;
    }
    return send(res, { cmd: "performance", n: req.body.n });
  });
  router.post("/data/sample", async (_req, res) =>
    send(res, { cmd: "sample" }));
  router.post("/data/save", async (_req, res) =>
    send(res, { cmd: "save" }));
  router.post("/data/load", async (_req, res) =>
    send(res, { cmd: "load" }));
  router.post("/data/reset", async (_req, res) =>
    send(res, { cmd: "reset" }));

  return router;
}

module.exports = createApiRouter;
