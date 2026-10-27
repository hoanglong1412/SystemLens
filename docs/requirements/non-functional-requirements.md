# Non-functional requirements

| Item | Value |
|---|---|
| Scope | How the code is structured to reduce coupling and stay easy to extend |
| Traces to | — (top level) |
| Detailed in | [high-level-design.md](../design/high-level-design.md), [design/lld/](../design/lld/) |
| Index / matrix | [docs/README.md](../README.md#3-traceability-matrix) |

Requirements are added by the project owner. Each NFR records its current status and the evidence in the
source code. Status values: `Implemented`, `Partially implemented`, `Not implemented`, `Removed`.

## 1. Requirement list

| ID | Title | Status | Design |
|---|---|---|---|
| [NFR-01](#nfr-01) | The API contract is the single source of truth for frontend and backend | Implemented | [HLD-01](../design/high-level-design.md#hld-01), [HLD-05](../design/high-level-design.md#hld-05), [LLD-02](../design/lld/LLD-02-api-request-pipeline.md) |
| [NFR-02](#nfr-02) | FE ↔ BE communication (IPC) is isolated so the IPC method can be changed later | Implemented | [HLD-01](../design/high-level-design.md#hld-01), [HLD-03](../design/high-level-design.md#hld-03), [HLD-05](../design/high-level-design.md#hld-05), [LLD-01](../design/lld/LLD-01-backend-lifecycle.md), [LLD-02](../design/lld/LLD-02-api-request-pipeline.md) |
| [NFR-03](#nfr-03) | The backend is OS-independent through interfaces/services; OS-specific code is loaded per OS | Not implemented | — |

## 2. Requirements

<a id="nfr-01"></a>
### NFR-01 — The API contract is the single source of truth

| Item | Value |
|---|---|
| Status | Implemented |
| Requirement | Frontend and backend both rely on the API contract. The contract is the source of truth for both sides; neither side defines the API on its own. |
| Contract | [`api/openapi.yaml`](../../api/openapi.yaml) (OpenAPI 3.1.0). Readable form: [api-schema.md](../api-schema.md). Rules: [api-convention.md](../api-convention.md). |
| Detailed in | [HLD-01](../design/high-level-design.md#hld-01), [HLD-05](../design/high-level-design.md#hld-05), [LLD-02](../design/lld/LLD-02-api-request-pipeline.md) |

Evidence in the source code:

| Where | What it shows |
|---|---|
| `api/openapi.yaml` header | "SystemLens backend API contract (source of truth for backend and frontend)." |
| `backend/src/api/sample/SampleHelloController.h` | Header names the contract: "Controller for GET /api/v1/sample/hello (contract: api/openapi.yaml)". |
| `frontend/renderer/api/sample/sampleHelloApi.js` | Header names the contract: "Client for GET /api/v1/sample/hello (contract: api/openapi.yaml)". |
| `backend/src/api/ApiResponse.h` | Single place that builds responses, so every endpoint matches the contract's success / `Problem` shapes. |

<a id="nfr-02"></a>
### NFR-02 — FE ↔ BE communication is isolated

| Item | Value |
|---|---|
| Status | Implemented (current IPC method: HTTP on `127.0.0.1`; connection info passed through Electron IPC) |
| Requirement | The code handling communication between frontend and backend is kept separate from the rest, so the IPC method can be changed later without touching features. |
| Detailed in | [HLD-01](../design/high-level-design.md#hld-01), [HLD-03](../design/high-level-design.md#hld-03), [HLD-05](../design/high-level-design.md#hld-05), [LLD-01](../design/lld/LLD-01-backend-lifecycle.md), [LLD-02](../design/lld/LLD-02-api-request-pipeline.md) |

Evidence in the source code:

| Side | Where | Isolation it provides |
|---|---|---|
| FE renderer | `frontend/renderer/api/apiClient.js` (`ApiClient`) | Every api module calls the backend through this class, never through `fetch` directly. Token, timeout and error conversion live only here. |
| FE renderer | `frontend/renderer/api/sample/sampleHelloApi.js` | API modules take an `ApiClient` and return plain data; UI code (`app.js`) never sees HTTP. |
| FE main ↔ preload | `frontend/main/ipcChannels.js`, `frontend/preload/preload.js` | Electron IPC channel names are defined in one place (repeated once in the sandboxed preload, which cannot `require()` local files). The renderer only sees `window.systemlens.getBackendConfig()`. |
| FE main | `frontend/main/backendProcess.js` (`BackendProcess`) | Process start, handshake and stop of the backend are owned by one class. |
| BE | `backend/src/api/HttpTypes.h`, `backend/src/api/IRouteRegistry.h`, `backend/src/api/IController.h` | Controllers see only library-independent `Request` / `Response` / `Handler` types and register routes through an interface. |
| BE | `backend/src/api/HttpServer.h/.cpp` | cpp-httplib is used only in `HttpServer.cpp`; the header only forward-declares `httplib::Server`. Header comment: "Swapping the HTTP library later therefore only touches HttpServer.cpp" (`HttpTypes.h`). |

<a id="nfr-03"></a>
### NFR-03 — OS-independent backend

| Item | Value |
|---|---|
| Status | Not implemented |
| Requirement | The backend is written as interfaces/services so it does not depend on the OS. For each OS, the implementation (detail code) for that OS is loaded. Only Windows is supported for now, but the design must make it easy to add Linux or macOS. |
| Detailed in | — (no design yet) |

Current state of the source code (facts only):

| Item | Current state |
|---|---|
| Interface / service layer | Exists for the sample feature only: `ISampleHelloService` → `SampleHelloService` (no OS access). |
| OS-specific implementation layer | None. The backend does not read any OS data yet. |
| Windows-specific code present | `backend/CMakeLists.txt` defines `_WIN32_WINNT=0x0A00`, `WIN32_LEAN_AND_MEAN`, `NOMINMAX` for cpp-httplib. `backend/src/app/AppConfig.cpp` uses `_dupenv_s` under `#ifdef _MSC_VER` and `std::getenv` otherwise. `frontend/main/backendProcess.js` defaults to `backend/build/Debug/systemlens-backend.exe`. |

## 3. Template for a new requirement

Copy the block below, replace the placeholders, add a row to the list in section 1 and to the
[traceability matrix](../README.md#3-traceability-matrix).

```markdown
<a id="nfr-nn"></a>
### NFR-NN — <title>

| Item | Value |
|---|---|
| Status | Not implemented |
| Requirement | <the rule, in the owner's words> |
| Detailed in | <HLD-NN>, <LLD-NN> |

Evidence in the source code:

| Where | What it shows |
|---|---|
| <file> | <how the code satisfies the rule> |
```

Example: [NFR-01](#nfr-01) above follows this template.
