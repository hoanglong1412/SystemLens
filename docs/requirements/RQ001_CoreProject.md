# RQ001 — Core project

| Item | Value |
|---|---|
| RQ | RQ001 |
| Title | Core project: FE ↔ BE communication core, sample feature and structural rules |
| Status | Partially implemented (RQ001-NFR-03 not implemented) |
| Index / matrix | [docs/README.md](../README.md#3-requirement-files) |

(Status values: `Implemented`, `Partially implemented`, `Not implemented`, `Removed`.)

## 1. Requirement list

| Type | ID | Title | Status |
|---|---|---|---|
| FR | [RQ001-FR-01](#rq001-fr-01) | Show the sample greeting from the backend | Implemented |
| NFR | [RQ001-NFR-01](#rq001-nfr-01) | The API contract is the single source of truth for frontend and backend | Implemented | 
| NFR | [RQ001-NFR-02](#rq001-nfr-02) | FE ↔ BE communication (IPC) is isolated so the IPC method can be changed later | Implemented |
| NFR | [RQ001-NFR-03](#rq001-nfr-03) | The backend is OS-independent through interfaces/services; OS-specific code is loaded per OS | Not implemented |

## 2. Functional requirements

<a id="rq001-fr-01"></a>
### RQ001-FR-01 — Show the sample greeting from the backend

| Item | Value |
|---|---|
| Status | Implemented |
| Requirement | When the application starts, the window shows the greeting returned by the backend API. If the backend cannot be reached, the window shows the error instead. |
| Purpose | This sample feature validates the frontend-to-backend communication path.

<a id="rq001-uc-01"></a>
#### RQ001-UC-01 — Open the application and see the greeting

```mermaid
usecase-beta
direction LR
actor User
systemBoundary SystemLens["SystemLens"]
  RQ001_UC_01_1("Open the application")
  RQ001_UC_01_2("Display the greeting if success")
  RQ001_UC_01_3("Display error if fail")
end
User --> RQ001_UC_01_1
RQ001_UC_01_1 --> RQ001_UC_01_2
RQ001_UC_01_1 --> RQ001_UC_01_3
```

Main flow:

| Step | User | System |
|---|---|---|
| 1 | Starts the application. | Starts the backend process and waits until it reports it is ready. |
| 2 | — | Opens the window (1000 × 700, title `SystemLens`) showing `Connecting to backend...`. |
| 3 | — | Requests the greeting from the backend. |
| 4 | Sees the greeting. | Replaces the text with `@@Hello world@@`. |

Alternative flows:

| ID | Condition | System behavior |
|---|---|---|
| A1 | The backend fails to start, exits before being ready, or is not ready within 10 s. | The window still opens and shows `Cannot reach backend: <error message>` in error style. |
| A2 | The API call fails (network error, timeout after 5 s, or HTTP error such as 401/404/500). | The window shows `Cannot reach backend: <error message>` in error style. |

Postcondition: the window shows either the greeting or an error message. There is no further interaction.

## 3. Non-functional requirements

<a id="rq001-nfr-01"></a>
### RQ001-NFR-01 — The API contract is the single source of truth

| Item | Value |
|---|---|
| Status | Implemented |
| Requirement | The frontend and backend must conform to a shared API contract. All API definitions and changes must be reflected in the contract, which serves as the authoritative reference for both sides. Neither side may independently define or change the API without updating the contract. |
| Contract | [`api/openapi.yaml`](../../api/openapi.yaml) (OpenAPI 3.1.0). Readable form: [api-schema.md](../api-schema.md). Rules: [api-convention.md](../api-convention.md). |


<a id="rq001-nfr-02"></a>
### RQ001-NFR-02 — FE ↔ BE communication is isolated

| Item | Value |
|---|---|
| Status | Implemented (current IPC method: HTTP on `127.0.0.1`; connection info passed through Electron IPC) |
| Requirement | The code handling communication between frontend and backend is kept separate from the rest, so the IPC method can be changed later without touching features. |


<a id="rq001-nfr-03"></a>
### RQ001-NFR-03 — OS-independent backend

| Item | Value |
|---|---|
| Status | Not implemented |
| Requirement | The backend is written as interfaces/services so it does not depend on the OS. For each OS, the implementation (detail code) for that OS is loaded. Only Windows is supported for now, but the design must make it easy to add Linux or macOS. |