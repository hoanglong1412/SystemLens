# LLD-01 — Backend process lifecycle

| Item | Value |
|---|---|
| Status | Implemented |
| Traces to | [NFR-02](../../requirements/non-functional-requirements.md#nfr-02), [HLD-03](../high-level-design.md#hld-03), [HLD-04](../high-level-design.md#hld-04) |
| API | — (no endpoint; the handshake is on stdin/stdout) |
| Index / matrix | [docs/README.md](../../README.md#3-traceability-matrix) |

## 1. Scope

Electron main starts the C++ backend, waits for its READY handshake, hands the connection info to the
renderer over Electron IPC, and stops the backend when the app quits. The backend parses its configuration,
builds its object graph, binds a port, announces it and exits when its stdin closes.

Source files:

```
frontend/
├── main/
│   ├── main.js                 app events, IPC handler, window
│   ├── backendProcess.js       BackendProcess, resolveBackendExePath
│   └── ipcChannels.js          BACKEND_GET_CONFIG
└── preload/
    └── preload.js              window.systemlens.getBackendConfig
backend/
└── src/
    ├── main.cpp                main(), exit codes
    └── app/
        ├── AppConfig.h/.cpp    AppConfig, parseAppConfig, readEnv, parsePort
        └── Application.h/.cpp  Application, toServerOptions
```

`HttpServer::bind/listen/stop` are described in [LLD-02](LLD-02-api-request-pipeline.md).

## 2. Sequences

### 2.1 Start

```mermaid
sequenceDiagram
    autonumber
    box transparent Frontend - Electron main
        participant MJS as main.js
        participant BP as BackendProcess
    end
    box transparent Backend
        participant BM as main.cpp
        participant CFG as AppConfig.cpp
        participant APP as Application
        participant HS as HttpServer
    end

    MJS->>BP: await start()
    BP->>BP: token = randomBytes(TOKEN_BYTES).toString hex
    BP->>BM: spawn(exePath, --port 0), env SYSTEMLENS_TOKEN, stdio pipe x3
    BP->>BP: waitForReady() reads stdout lines, timer readyTimeoutMs
    BM->>CFG: parseAppConfig(argc, argv)
    CFG-->>BM: AppConfig
    BM->>APP: Application(std::move(config))
    APP->>HS: HttpServer(toServerOptions(config)) installs middleware
    APP->>APP: registerControllers()
    BM->>APP: run()
    APP->>HS: bind()
    HS-->>APP: bound port
    APP->>APP: startParentWatchdog() when not devMode
    APP-->>BP: announceReady(port) prints SYSTEMLENS_READY {"port":n}
    APP->>HS: listen() blocks
    BP->>BP: line starts with READY_PREFIX, JSON.parse, finish(null, port)
    BP-->>MJS: {baseUrl, token}
    MJS->>MJS: ipcMain.handle(BACKEND_GET_CONFIG), createWindow()
```

| # | From → To | Function | Processing |
|---|---|---|---|
| 1 | main.js → BackendProcess | `app.whenReady` handler, `start()` | Awaited inside `try`; a rejection is stored in `backendStartError` ([F9](#f9)). |
| 2 | BackendProcess | `start()` | 32 random bytes as hex. |
| 3 | BackendProcess → main.cpp | `start()` | `spawn(this.exePath, ['--port','0'], {env: {...process.env, SYSTEMLENS_TOKEN: token}, stdio: ['pipe','pipe','pipe'], windowsHide: true})`; stderr forwarded as `[backend] ...`. |
| 4 | BackendProcess | `#waitForReady()` | Listens for stdout lines, `error`, `exit` and a timeout ([F7](#f7)). |
| 5 | main.cpp → AppConfig | `parseAppConfig()` | Parses argv and `SYSTEMLENS_TOKEN` ([F2](#f2)). |
| 6 | AppConfig → main.cpp | — | `AppConfig{port=0, token, devMode=false, showHelp=false}`. |
| 7 | main.cpp → Application | `Application(AppConfig)` | Stores the config, builds `m_server`. |
| 8 | Application → HttpServer | `toServerOptions()`, `HttpServer(options)` | Host `127.0.0.1`, port from config, token = `nullopt` in dev mode. |
| 9 | Application | `registerControllers()` | Creates controllers (sample one only if `SYSTEMLENS_ENABLE_SAMPLE_API`), calls `registerRoutes(m_server)` on each ([F5](#f5)). |
| 10 | main.cpp → Application | `run()` | [F4](#f4). |
| 11 | Application → HttpServer | `bind()` | Port 0 → `bind_to_any_port`; throws `std::runtime_error` on failure. |
| 12 | HttpServer → Application | — | The real port. |
| 13 | Application | `startParentWatchdog()` | Thread blocking on stdin ([F6](#f6)); skipped in `--dev` (a warning is logged instead). |
| 14 | Application → BackendProcess | `announceReady()` | `std::cout << "SYSTEMLENS_READY " << {"port":n} << std::endl` (flush); log to stderr. |
| 15 | Application → HttpServer | `listen()` | Blocks until `stop()`. |
| 16 | BackendProcess | `#waitForReady()` | Parses the JSON after the prefix and resolves the port. |
| 17 | BackendProcess → main.js | `start()` | `this.config = {baseUrl: 'http://127.0.0.1:<port>', token}`. |
| 18 | main.js | `ipcMain.handle`, `createWindow()` | Registers `handleGetBackendConfig` ([F10](#f10)) and opens the window. |

### 2.2 Stop

```mermaid
sequenceDiagram
    autonumber
    box transparent Frontend - Electron main
        participant MJS as main.js
        participant BP as BackendProcess
    end
    box transparent Backend
        participant W as watchdog thread
        participant HS as HttpServer
        participant APP as Application run()
    end

    MJS->>BP: will-quit, stop()
    BP->>W: child.stdin.end()
    BP->>BP: child.kill() if exitCode is null, clear child and config
    W->>W: std::getline returns false on EOF
    W->>HS: stop()
    HS-->>APP: listen() returns
    APP->>W: m_parentWatchdog.join()
    APP-->>APP: run() returns 0
```

| # | From → To | Function | Processing |
|---|---|---|---|
| 1 | main.js → BackendProcess | `stop()` | Called on `will-quit` ([F8](#f8)). |
| 2 | BackendProcess → watchdog | `stop()` | Closing stdin is the backend's normal shutdown signal. |
| 3 | BackendProcess | `stop()` | Kill right away: the backend must never outlive the app. Steps 4–8 may therefore not run. |
| 4 | watchdog | `startParentWatchdog()` lambda | Loop ends; logs `stdin closed, shutting down`. |
| 5 | watchdog → HttpServer | `HttpServer::stop()` | Only stops if the server is running. |
| 6 | HttpServer → Application | `listen()` | Returns in `run()`. |
| 7 | Application → watchdog | `run()` | `join()` returns immediately: the thread has already finished. |
| 8 | Application | `run()` | Logs `[Application] stopped`; `main()` returns 0. |

## 3. Data / configuration

| Name | Type | Value / meaning | Defined in |
|---|---|---|---|
| `READY_PREFIX` / `kReadyPrefix` | string | `"SYSTEMLENS_READY "` — prefix of the handshake line | `backendProcess.js` / `Application.h` |
| `TOKEN_ENV_VAR` / `kTokenEnvVar` | string | `"SYSTEMLENS_TOKEN"` | `backendProcess.js` / `AppConfig.h` |
| `TOKEN_BYTES` | number | 32 | `backendProcess.js` |
| `DEFAULT_READY_TIMEOUT_MS` | number | 10000 | `backendProcess.js` |
| `DEFAULT_HOST` | string | `"127.0.0.1"` | `backendProcess.js` |
| `SYSTEMLENS_BACKEND_EXE` | env var | Overrides the backend path (default `backend/build/Debug/systemlens-backend.exe`) | `backendProcess.js` |
| `BACKEND_GET_CONFIG` | string | `"backend:getConfig"` (repeated in `preload.js`) | `ipcChannels.js` |
| `AppConfig` | struct | `port` (0 = OS picks), `token` (optional), `devMode`, `showHelp` | `AppConfig.h` |
| `kUsage` | string | Help text for `--help` and invalid arguments | `AppConfig.h` |
| `kMaxPort` | int | 65535 | `AppConfig.cpp` |
| `kExitOk` / `kExitRuntimeError` / `kExitUsageError` | int | 0 / 1 / 2 | `main.cpp` |

Command line of the backend: `systemlens-backend [--port <n>] [--dev] [--help]` (`-h` also accepted).

Exit codes:

| Code | When |
|---|---|
| 0 | Normal shutdown, or `--help`. |
| 1 | Exception during construction or `run()` (e.g. cannot bind). Printed as `Fatal: ...` on stderr. |
| 2 | Invalid arguments or missing token. Printed as `Error: ...` + usage on stderr. |

## 4. Functions

### 4.1 Frontend — `frontend/main/backendProcess.js`

| Function | Purpose | Parameters | Return | Errors | Flowchart |
|---|---|---|---|---|---|
| `resolveBackendExePath()` | Locate the backend executable. | — | `SYSTEMLENS_BACKEND_EXE` if set, else `<repo>/backend/build/Debug/systemlens-backend.exe` | — | — |
| `new BackendProcess({exePath, readyTimeoutMs})` | Store options; `child` and `config` start as `null`. | `exePath` (default `resolveBackendExePath()`), `readyTimeoutMs` (default 10000) | — | — | — |
| `async start()` | Spawn the backend and wait for READY. | — | `Promise<{baseUrl, token}>` | `Error` if the process cannot start, exits early or is not ready in time | [F7](#f7) (wait part) |
| `getConfig()` | Connection info of the running backend. | — | `{baseUrl, token}` | `Error('Backend is not running')` if `start()` did not complete | — |
| `stop()` | Close stdin, kill if still running, reset state. | — | — | — | [F8](#f8) |
| `#waitForReady()` | Resolve with the port from the READY line. | — | `Promise<number>` | rejects on spawn error, exit before READY, timeout | [F7](#f7) |

### 4.2 Frontend — `frontend/main/main.js`, `frontend/preload/preload.js`

| Function | Purpose | Parameters | Return | Errors | Flowchart |
|---|---|---|---|---|---|
| `createWindow()` | Create the 1000 × 700 `BrowserWindow` with `preload.js` (`contextIsolation: true`, `nodeIntegration: false`, `sandbox: true`) and load `renderer/index.html`. | — | — | — | — |
| `handleGetBackendConfig()` | IPC handler for `backend:getConfig`. | — | `{baseUrl, token}` | throws `backendStartError`, or the `getConfig()` error | [F10](#f10) |
| `app.whenReady().then(...)` | Start the backend, register the IPC handler, open the window, re-open on `activate` when no window exists. | — | — | errors are caught and kept in `backendStartError` | [F9](#f9) |
| `window-all-closed` handler | `app.quit()` unless `darwin`. | — | — | — | — |
| `will-quit` handler | `backend.stop()`. | — | — | — | — |
| `window.systemlens.getBackendConfig()` (preload) | `ipcRenderer.invoke('backend:getConfig')`. | — | `Promise<{baseUrl, token}>` | rejects with the main-process error | — |

### 4.3 Backend — `backend/src/main.cpp`, `backend/src/app/`

| Function | Purpose | Parameters | Return | Errors | Flowchart |
|---|---|---|---|---|---|
| `int main(int argc, char* argv[])` | Parse configuration and run the `Application`. | `argc`, `argv` | 0 / 1 / 2 (see exit codes) | catches all `std::exception` | [F1](#f1) |
| `AppConfig parseAppConfig(int argc, const char* const argv[])` | Build the configuration from argv and the environment. | `argc`, `argv` | `AppConfig` | `std::invalid_argument`: unknown argument, `--port` without value, invalid port, token missing outside dev / help | [F2](#f2) |
| `std::optional<std::string> readEnv(const char* name)` (anonymous namespace) | Read an environment variable (`_dupenv_s` under MSVC, `std::getenv` otherwise). | `name` | value, or `nullopt` when unset or empty | — | — |
| `int parsePort(std::string_view text)` (anonymous namespace) | Parse a port number. | `text` | port in [0, 65535] | `std::invalid_argument("Invalid port: ...")` | [F3](#f3) |
| `HttpServerOptions toServerOptions(const AppConfig&)` (anonymous namespace) | Map config to server options. | `config` | options with `token = nullopt` in dev mode | — | — |
| `Application(AppConfig config)` | Build the object graph; no socket opened yet. | `config` | — | exceptions from controller construction | — |
| `int Application::run()` | Bind, announce readiness, serve until shutdown. | — | 0 | `std::runtime_error` if the server cannot bind | [F4](#f4) |
| `void Application::registerControllers()` | Create every controller and register its routes. | — | — | — | [F5](#f5) |
| `void Application::announceReady(int port) const` | Print the READY line to stdout (flushed) and a log line to stderr. | `port` | — | — | — |
| `void Application::startParentWatchdog()` | Start the thread that stops the server when stdin closes. | — | — | — | [F6](#f6) |

## 5. Flowcharts

<a id="f1"></a>
### F1 — `main()`

```mermaid
flowchart TD
    S(["start"]) --> P["config = parseAppConfig(argc, argv)"]
    P -- "throws std::exception" --> E2["stderr: Error + kUsage"] --> R2(["return 2"])
    P --> H{"config.showHelp?"}
    H -- "yes" --> U["stdout: kUsage"] --> R0(["return 0"])
    H -- "no" --> A["Application app(config)<br/>return app.run()"]
    A -- "throws std::exception" --> E1["stderr: Fatal + message"] --> R1(["return 1"])
    A --> RR(["return run() result"])
```

<a id="f2"></a>
### F2 — `parseAppConfig()`

```mermaid
flowchart TD
    S(["start"]) --> L{"next argument?"}
    L -- "--port" --> V{"value present?"}
    V -- "no" --> X1["throw invalid_argument<br/>--port needs a value"]
    V -- "yes" --> PP["config.port = parsePort(value)"] --> L
    L -- "--dev" --> D["config.devMode = true"] --> L
    L -- "--help or -h" --> HH["config.showHelp = true"] --> L
    L -- "other" --> X2["throw invalid_argument<br/>Unknown argument"]
    L -- "none left" --> T["config.token = readEnv(SYSTEMLENS_TOKEN)"]
    T --> C{"no token AND not devMode<br/>AND not showHelp?"}
    C -- "yes" --> X3["throw invalid_argument<br/>SYSTEMLENS_TOKEN is not set"]
    C -- "no" --> R(["return config"])
```

<a id="f3"></a>
### F3 — `parsePort()`

```mermaid
flowchart TD
    S(["start"]) --> A["port = std::stoi(text, consumed)"]
    A -- "stoi throws" --> X["throw invalid_argument<br/>Invalid port: text"]
    A --> C{"all chars consumed AND<br/>0 <= port <= 65535?"}
    C -- "yes" --> R(["return port"])
    C -- "no" --> X
```

<a id="f4"></a>
### F4 — `Application::run()`

```mermaid
flowchart TD
    S(["start"]) --> B["port = m_server.bind()"]
    B -- "throws runtime_error" --> X(["exception propagates to main: exit 1"])
    B --> D{"m_config.devMode?"}
    D -- "yes" --> W["stderr: WARNING authentication is DISABLED"]
    D -- "no" --> PW["startParentWatchdog()"]
    W --> AR["announceReady(port)"]
    PW --> AR
    AR --> L["m_server.listen()<br/>blocks until stop()"]
    L --> J{"watchdog joinable?"}
    J -- "yes" --> JJ["m_parentWatchdog.join()"] --> LOG
    J -- "no" --> LOG["stderr: stopped"]
    LOG --> R(["return 0"])
```

<a id="f5"></a>
### F5 — `Application::registerControllers()`

```mermaid
flowchart TD
    S(["start"]) --> C{"SYSTEMLENS_ENABLE_SAMPLE_API?<br/>compile time"}
    C -- "1" --> A["m_controllers.push_back(<br/>SampleHelloController(SampleHelloService))"]
    C -- "0" --> L
    A --> L{"next controller?"}
    L -- "yes" --> R["controller->registerRoutes(m_server)"] --> L
    L -- "no" --> E(["end"])
```

<a id="f6"></a>
### F6 — `Application::startParentWatchdog()` (thread body)

```mermaid
flowchart TD
    S(["thread start"]) --> G{"std::getline(std::cin) succeeds?"}
    G -- "yes: line ignored" --> G
    G -- "no: EOF, parent closed stdin or died" --> LOG["stderr: stdin closed, shutting down"]
    LOG --> ST["m_server.stop()"] --> E(["thread end"])
```

<a id="f7"></a>
### F7 — `BackendProcess.#waitForReady()`

```mermaid
flowchart TD
    S(["start"]) --> I["create readline on child.stdout<br/>start timer readyTimeoutMs<br/>once error, once exit"]
    I --> EV{"first event"}
    EV -- "stdout line starts with READY_PREFIX" --> P["port = JSON.parse(rest).port"] --> FOK["finish(null, port)"]
    EV -- "other stdout line" --> EV
    EV -- "child error" --> F1["finish(Error: Cannot start backend)"]
    EV -- "child exit" --> F2["finish(Error: Backend exited before it was ready)"]
    EV -- "timer fires" --> F3["finish(Error: Backend not ready after N ms)"]
    FOK --> CL
    F1 --> CL
    F2 --> CL
    F3 --> CL
    CL["finish: clearTimeout, close readline,<br/>remove error and exit listeners"] --> R{"error?"}
    R -- "yes" --> RJ(["reject(error)"])
    R -- "no" --> RS(["resolve(port)"])
```

<a id="f8"></a>
### F8 — `BackendProcess.stop()`

```mermaid
flowchart TD
    S(["start"]) --> C{"child is null?"}
    C -- "yes" --> E(["return"])
    C -- "no" --> IN["child.stdin.end()"]
    IN --> X{"child.exitCode === null?"}
    X -- "yes" --> K["child.kill()"] --> CL
    X -- "no" --> CL["child = null, config = null"]
    CL --> E2(["return"])
```

<a id="f9"></a>
### F9 — `app.whenReady()` handler (`main.js`)

```mermaid
flowchart TD
    S(["app ready"]) --> T["await backend.start()"]
    T -- "rejects" --> ER["backendStartError = err<br/>console.error(err)"] --> H
    T -- "resolves" --> H["ipcMain.handle(BACKEND_GET_CONFIG,<br/>handleGetBackendConfig)"]
    H --> W["createWindow()"]
    W --> A["on activate: createWindow()<br/>if no window is open"]
    A --> E(["end"])
```

<a id="f10"></a>
### F10 — `handleGetBackendConfig()`

```mermaid
flowchart TD
    S(["IPC backend:getConfig"]) --> C{"backendStartError set?"}
    C -- "yes" --> X(["throw backendStartError"])
    C -- "no" --> G["backend.getConfig()"]
    G -- "config is null" --> X2(["throw Error: Backend is not running"])
    G --> R(["return baseUrl, token"])
```

## 6. Notes

- stdout of the backend carries only the READY line; all logs go to stderr (`Application.h`). The frontend
  forwards backend stderr to its own stderr with the prefix `[backend]`.
- The token is passed in the environment, not argv, "so it does not show up in process lists" (`AppConfig.h`).
- `Application` declares `m_controllers` before `m_server` so the controllers are destroyed after the server,
  whose handlers point into them (`Application.h`).
- `preload.js` repeats the channel string because a sandboxed preload can only `require('electron')`.
