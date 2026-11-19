"use strict";

const express = require("express");
const path = require("node:path");
const EngineBridge = require("./engineBridge");
const createApiRouter = require("./routes/api");
const { isRecord } = require("./validators");

const app = express();
const engine = new EngineBridge();
const port = Number(process.env.PORT || 3000);
const allowedOrigins = (process.env.CORS_ORIGIN ||
  "http://localhost:5173,http://127.0.0.1:5173")
  .split(",").map((origin) => origin.trim()).filter(Boolean);
const limitWindowMs = Number(process.env.RATE_WINDOW_MS || 60000);
const limitCount = Number(process.env.RATE_LIMIT || 120);
const requestsByIp = Object.create(null);

app.disable("x-powered-by");
app.use((req, res, next) => {
  const origin = req.get("origin");
  if (origin && !allowedOrigins.includes(origin)) {
    return res.status(403).json({
      ok: false, error: "Origin is not allowed",
      meta: { httpStatus: 403 }
    });
  }
  if (origin) {
    res.set("Access-Control-Allow-Origin", origin);
    res.set("Vary", "Origin");
  }
  res.set("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS");
  res.set("Access-Control-Allow-Headers", "Content-Type");
  if (req.method === "OPTIONS") return res.sendStatus(204);
  return next();
});

app.use((req, res, next) => {
  const now = Date.now();
  const ip = req.ip || req.socket.remoteAddress || "unknown";
  let state = requestsByIp[ip];
  if (!state || now - state.startedAt >= limitWindowMs) {
    state = requestsByIp[ip] = { startedAt: now, count: 0 };
  }
  state.count++;
  if (state.count > limitCount) {
    return res.status(429).json({
      ok: false, error: "Rate limit exceeded",
      meta: { httpStatus: 429 }
    });
  }
  return next();
});

app.use(express.json({ limit: "16kb", strict: true }));
app.use((req, res, next) => {
  if (req.body !== undefined && !isRecord(req.body)) {
    return res.status(400).json({
      ok: false, error: "Request body must be a JSON object",
      meta: { httpStatus: 400 }
    });
  }
  return next();
});
app.use("/api", createApiRouter(engine));
app.use(express.static(path.resolve(__dirname, "..", "web")));
app.use((_req, res) => res.status(404).json({
  ok: false, error: "Route not found", meta: { httpStatus: 404 }
}));
app.use((error, _req, res, _next) => {
  const status = Number.isInteger(error.status) ? error.status : 500;
  if (status >= 500) console.error(error);
  res.status(status).json({
    ok: false,
    error: status >= 500 ? "Internal server error" : error.message,
    meta: { httpStatus: status }
  });
});

const server = app.listen(port, () => {
  console.log(`ConnectSphere server listening on http://localhost:${port}`);
});
engine.start();

async function shutdown() {
  server.close();
  await engine.close();
}
process.on("SIGINT", () => { shutdown().finally(() => process.exit(0)); });
process.on("SIGTERM", () => { shutdown().finally(() => process.exit(0)); });

module.exports = { app, server, engine };
