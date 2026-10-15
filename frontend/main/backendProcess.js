/**
 * @file   backendProcess.js
 * @brief  Starts, tracks and stops the C++ backend (systemlens-backend.exe) for the Electron app.
 *
 * Handshake with the backend (see backend/src/app/Application.h):
 *  1. We generate a random token and pass it in the SYSTEMLENS_TOKEN environment variable.
 *  2. We start the backend with "--port 0" so the OS picks a free port.
 *  3. The backend prints one line on stdout: SYSTEMLENS_READY {"port":<n>}.
 *  4. We keep its stdin pipe open; closing it (or our process dying) makes the backend exit.
 */
const { spawn } = require('node:child_process');
const crypto = require('node:crypto');
const path = require('node:path');
const readline = require('node:readline');

const READY_PREFIX = 'SYSTEMLENS_READY ';
const TOKEN_ENV_VAR = 'SYSTEMLENS_TOKEN';
const TOKEN_BYTES = 32;
const DEFAULT_READY_TIMEOUT_MS = 10000;
const DEFAULT_HOST = '127.0.0.1';

/**
 * Locate the backend executable.
 * SYSTEMLENS_BACKEND_EXE overrides the default, which is the Debug build of the dev tree.
 * @returns {string} Absolute path to systemlens-backend.exe.
 */
function resolveBackendExePath() {
  if (process.env.SYSTEMLENS_BACKEND_EXE) {
    return process.env.SYSTEMLENS_BACKEND_EXE;
  }
  return path.join(__dirname, '..', '..', 'backend', 'build', 'Debug', 'systemlens-backend.exe');
}

/** Owns one backend child process and the connection info it reported. */
class BackendProcess {
  /**
   * @param {object} [options]
   * @param {string} [options.exePath] Backend executable; defaults to resolveBackendExePath().
   * @param {number} [options.readyTimeoutMs] How long to wait for the READY line.
   */
  constructor({ exePath = resolveBackendExePath(), readyTimeoutMs = DEFAULT_READY_TIMEOUT_MS } = {}) {
    this.exePath = exePath;
    this.readyTimeoutMs = readyTimeoutMs;
    this.child = null;
    this.config = null;
  }

  /**
   * Spawn the backend and wait for its READY line.
   * @returns {Promise<{baseUrl: string, token: string}>} Connection info for the renderer.
   * @throws {Error} If the process cannot start, exits early or does not become ready in time.
   */
  async start() {
    const token = crypto.randomBytes(TOKEN_BYTES).toString('hex');

    this.child = spawn(this.exePath, ['--port', '0'], {
      env: { ...process.env, [TOKEN_ENV_VAR]: token },
      // stdin must be a pipe we keep open: its closure is the backend's signal to exit.
      stdio: ['pipe', 'pipe', 'pipe'],
      windowsHide: true,
    });

    this.child.stderr.on('data', (chunk) => process.stderr.write(`[backend] ${chunk}`));

    const port = await this.#waitForReady();
    this.config = { baseUrl: `http://${DEFAULT_HOST}:${port}`, token };
    return this.config;
  }

  /**
   * Connection info reported by the running backend.
   * @returns {{baseUrl: string, token: string}}
   * @throws {Error} If start() has not completed successfully.
   */
  getConfig() {
    if (!this.config) {
      throw new Error('Backend is not running');
    }
    return this.config;
  }

  /**
   * Stop the backend. Closing stdin is its normal shutdown signal; kill() follows right away
   * because the app is quitting and the backend must never outlive it.
   */
  stop() {
    if (!this.child) {
      return;
    }
    this.child.stdin.end();
    if (this.child.exitCode === null) {
      this.child.kill();
    }
    this.child = null;
    this.config = null;
  }

  /**
   * Resolve with the port from the READY line; reject on error, early exit or timeout.
   * @returns {Promise<number>}
   */
  #waitForReady() {
    return new Promise((resolve, reject) => {
      const lines = readline.createInterface({ input: this.child.stdout });

      // Every exit path below goes through finish() so listeners and the timer are always cleaned up.
      const finish = (error, port) => {
        clearTimeout(timer);
        lines.close();
        this.child.off('error', onError);
        this.child.off('exit', onExit);
        error ? reject(error) : resolve(port);
      };
      const onError = (err) => finish(new Error(`Cannot start backend (${this.exePath}): ${err.message}`));
      const onExit = (code) => finish(new Error(`Backend exited before it was ready (exit code ${code})`));
      const timer = setTimeout(
        () => finish(new Error(`Backend not ready after ${this.readyTimeoutMs} ms`)),
        this.readyTimeoutMs,
      );

      lines.on('line', (line) => {
        if (line.startsWith(READY_PREFIX)) {
          const { port } = JSON.parse(line.slice(READY_PREFIX.length));
          finish(null, port);
        }
      });
      this.child.once('error', onError);
      this.child.once('exit', onExit);
    });
  }
}

module.exports = { BackendProcess, resolveBackendExePath };
