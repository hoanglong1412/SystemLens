# Functional requirements

| Item | Value |
|---|---|
| Scope | What the user can do with SystemLens and what the UI shows |
| Traces to | — (top level) |
| Detailed in | [high-level-design.md](../design/high-level-design.md), [design/lld/](../design/lld/) |
| Index / matrix | [docs/README.md](../README.md#3-traceability-matrix) |

Requirements are added by the project owner. Each FR has one or more use cases (UC) and a table of UI
operations. Status values: `Implemented`, `Partially implemented`, `Not implemented`, `Removed`.

## 1. Requirement list

| ID | Title | Status | Use case | Design |
|---|---|---|---|---|
| [FR-SMP-01](#fr-smp-01) | Show the sample greeting from the backend | Implemented | [UC-SMP-01](#uc-smp-01) | [HLD-02](../design/high-level-design.md#hld-02), [LLD-03](../design/lld/LLD-03-sample-hello.md) |

## 2. Requirements

<a id="fr-smp-01"></a>
### FR-SMP-01 — Show the sample greeting from the backend

| Item | Value |
|---|---|
| Status | Implemented |
| Area | `SMP` — sample feature; exists to prove that the frontend can call the backend |
| Description | When the application starts, the window shows the greeting returned by the backend API `GET /api/v1/sample/hello`. If the backend cannot be reached, the window shows the error instead. |
| Build condition | The backend registers the sample endpoint only when built with the CMake option `SYSTEMLENS_ENABLE_SAMPLE_API=ON` (default `ON`). |
| API | [`getSampleHello`](../api-schema.md#getsamplehello) |
| Detailed in | [HLD-02](../design/high-level-design.md#hld-02), [LLD-03](../design/lld/LLD-03-sample-hello.md) |

<a id="uc-smp-01"></a>
#### UC-SMP-01 — Open the application and see the greeting

| Item | Value |
|---|---|
| Actor | User |
| Precondition | The backend executable has been built (`backend/build/Debug/systemlens-backend.exe`, or the path in `SYSTEMLENS_BACKEND_EXE`). |
| Trigger | User starts the application (`Tool\run-frontend.ps1` or `npm start` in `frontend/`). |

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

#### UI operations — FR-SMP-01

| # | Screen / element | Operation | Result | Source |
|---|---|---|---|---|
| 1 | Main window | Open the application | Window opens with heading `#message` = `Connecting to backend...` | `frontend/main/main.js` (`createWindow`), `frontend/renderer/index.html` |
| 2 | Heading `#message` | — (automatic, no user input) | Text becomes the greeting `@@Hello world@@` | `frontend/renderer/app.js` (`bootstrap`, `render`) |
| 3 | Heading `#message` | — (automatic, on error) | Text becomes `Cannot reach backend: <message>`; CSS class `error` is added | `frontend/renderer/app.js` (`render(text, true)`), `frontend/renderer/styles.css` |

UI design: none yet (see [ui-design/](../ui-design/)).

## 3. Template for a new requirement

Copy the block below, replace the placeholders, add a row to the list in section 1 and to the
[traceability matrix](../README.md#3-traceability-matrix).

```markdown
<a id="fr-<area>-nn"></a>
### FR-<AREA>-NN — <title>

| Item | Value |
|---|---|
| Status | Not implemented |
| Area | `<AREA>` — <meaning> |
| Description | <what the user can do / what the system shows> |
| API | <operationId(s) or —> |
| Detailed in | <HLD-NN>, <LLD-NN> |

<a id="uc-<area>-nn"></a>
#### UC-<AREA>-NN — <use case title>

| Item | Value |
|---|---|
| Actor | <who> |
| Precondition | <state before> |
| Trigger | <what starts it> |

Main flow:

| Step | User | System |
|---|---|---|
| 1 | <action> | <response> |

Alternative flows:

| ID | Condition | System behavior |
|---|---|---|
| A1 | <condition> | <behavior> |

#### UI operations — FR-<AREA>-NN

| # | Screen / element | Operation | Result | Source |
|---|---|---|---|---|
| 1 | <screen / element> | <click, type, ...> | <what changes> | <file (function)> |
```

Example: [FR-SMP-01](#fr-smp-01) above follows this template.
