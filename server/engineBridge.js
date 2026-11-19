"use strict";

const { spawn } = require("node:child_process");
const readline = require("node:readline");
const path = require("node:path");

class EngineBridge {
  constructor(options = {}) {
    this.executable = options.executable || process.env.ENGINE_PATH ||
      path.resolve(__dirname, "..", "bin",
        process.platform === "win32" ? "connectsphere_engine.exe" : "connectsphere_engine");
    this.cwd = options.cwd || process.env.ENGINE_CWD || path.resolve(__dirname, "..");
    this.timeoutMs = options.timeoutMs || 8000;
    this.child = null;
    this.pending = null;
    this.queue = Promise.resolve();
    this.closed = false;
    this.restartAttempts = 0;
    this.restartTimer = null;
  }

  start() {
    if (this.closed) throw new Error("Engine bridge has been closed");
    if (this.child && this.child.exitCode === null && !this.child.killed) return;

    const child = spawn(this.executable, [], {
      cwd: this.cwd,
      stdio: ["pipe", "pipe", "pipe"],
      windowsHide: true,
      shell: false
    });
    this.child = child;
    child.once("spawn", () => { this.restartAttempts = 0; });
    const output = readline.createInterface({ input: child.stdout });
    output.on("line", (line) => this.onLine(line, child));
    child.stderr.on("data", (chunk) => {
      process.stderr.write(`[connectsphere-engine] ${chunk}`);
    });
    child.on("error", (error) => this.onExit(child, error));
    child.on("exit", (code, signal) => {
      this.onExit(child, new Error(`C engine exited (code=${code}, signal=${signal || "none"})`));
    });
  }

  onLine(line, child) {
    if (child !== this.child || !this.pending) {
      process.stderr.write("Ignoring an unsolicited C engine response\n");
      return;
    }
    let response;
    try {
      response = JSON.parse(line);
    } catch {
      this.failChild(child, new Error("C engine returned malformed JSON"));
      return;
    }
    const pending = this.pending;
    this.pending = null;
    clearTimeout(pending.timer);
    pending.resolve(response);
  }

  onExit(child, error) {
    if (child !== this.child) return;
    this.child = null;
    if (this.pending) {
      const pending = this.pending;
      this.pending = null;
      clearTimeout(pending.timer);
      pending.reject(error);
    }
    if (!this.closed && this.restartAttempts < 3) {
      this.restartAttempts++;
      this.restartTimer = setTimeout(() => {
        this.restartTimer = null;
        if (this.closed || this.child) return;
        try {
          this.start();
        } catch (restartError) {
          process.stderr.write(`[connectsphere-engine] restart failed: ${restartError.message}\n`);
        }
      }, Math.min(250 * this.restartAttempts, 1000));
    }
  }

  failChild(child, error) {
    if (child === this.child) {
      this.onExit(child, error);
      child.kill();
    }
  }

  send(command) {
    const operation = this.queue.then(() => this.sendNow(command));
    this.queue = operation.catch(() => undefined);
    return operation;
  }

  sendNow(command) {
    if (this.closed) return Promise.reject(new Error("Engine bridge has been closed"));
    try {
      this.start();
    } catch (error) {
      return Promise.reject(error);
    }

    return new Promise((resolve, reject) => {
      const child = this.child;
      const timer = setTimeout(() => {
        if (this.pending && this.pending.child === child) {
          const pending = this.pending;
          this.pending = null;
          pending.reject(new Error("C engine request timed out"));
          this.failChild(child, new Error("C engine request timed out"));
        }
      }, this.timeoutMs);
      this.pending = { resolve, reject, timer, child };
      child.stdin.write(`${JSON.stringify(command)}\n`, (error) => {
        if (error && this.pending && this.pending.child === child) {
          const pending = this.pending;
          this.pending = null;
          clearTimeout(pending.timer);
          pending.reject(error);
          this.failChild(child, error);
        }
      });
    });
  }

  async close() {
    this.closed = true;
    if (this.restartTimer) {
      clearTimeout(this.restartTimer);
      this.restartTimer = null;
    }
    if (this.pending) {
      clearTimeout(this.pending.timer);
      this.pending.reject(new Error("Engine bridge closed"));
      this.pending = null;
    }
    const child = this.child;
    if (child) {
      this.child = null;
      if (child.exitCode === null && child.signalCode === null) {
        const exited = new Promise((resolve) => child.once("exit", resolve));
        child.kill();
        await exited;
      }
    }
    await this.queue.catch(() => undefined);
  }
}

module.exports = EngineBridge;
