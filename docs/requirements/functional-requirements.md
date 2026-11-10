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
| [FR-SYS-01](#fr-sys-01) | Show live machine identity and quick stats in the app shell | Implemented (Windows, Linux) | [UC-SYS-01](#uc-sys-01) | [HLD-06](../design/high-level-design.md#hld-06), [LLD-04](../design/lld/LLD-04-metrics-collection.md), [LLD-05](../design/lld/LLD-05-live-performance-ui.md) |
| [FR-PERF-01](#fr-perf-01) | Show every monitored resource with a live sparkline | Implemented (Windows, Linux) | [UC-PERF-01](#uc-perf-01) | [HLD-06](../design/high-level-design.md#hld-06), [LLD-04](../design/lld/LLD-04-metrics-collection.md), [LLD-05](../design/lld/LLD-05-live-performance-ui.md) |
| [FR-PERF-02](#fr-perf-02) | Show live CPU performance details | Implemented (Windows, Linux) | [UC-PERF-01](#uc-perf-01) | [HLD-06](../design/high-level-design.md#hld-06), [LLD-04](../design/lld/LLD-04-metrics-collection.md), [LLD-05](../design/lld/LLD-05-live-performance-ui.md) |

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

<a id="fr-sys-01"></a>
### FR-SYS-01 — Show live machine identity and quick stats in the app shell

| Item | Value |
|---|---|
| Status | Implemented (Windows, Linux). macOS: provider interfaces only, the API answers `500`. |
| Area | `SYS` — machine-wide information shown by the app shell on every screen |
| Description | The header shows the computer name, operating system name + architecture, OS version, user and session. The quick stats show uptime, total CPU / memory / disk load, network send / receive rate and the process using the most memory. The tab bar shows the process count and (where countable) the service count. The status bar shows the backend connection state and the boot time. Values refresh every second. A value the operating system cannot provide is shown as `—`. |
| API | [`getSystemOverview`](../api-schema.md#getsystemoverview) |
| Detailed in | [HLD-06](../design/high-level-design.md#hld-06), [LLD-04](../design/lld/LLD-04-metrics-collection.md), [LLD-05](../design/lld/LLD-05-live-performance-ui.md) |
| Decisions | [docs/decisions/2026-11-10-live-data-windows-linux.md](../decisions/2026-11-10-live-data-windows-linux.md) |

<a id="uc-sys-01"></a>
#### UC-SYS-01 — See live system information on every screen

| Item | Value |
|---|---|
| Actor | User |
| Precondition | The backend executable has been built for the current platform. |
| Trigger | User starts the application. |

Main flow:

| Step | User | System |
|---|---|---|
| 1 | Starts the application. | Starts the backend, which begins sampling the operating system once per second. |
| 2 | — | Opens the window; every shell value shows `—` and the status bar shows `Disconnected from backend`. |
| 3 | — | Requests the overview every second and updates header, quick stats, tabs and status bar; the status bar shows `Connected to backend` with a green dot. |
| 4 | Reads the values. | Keeps refreshing while the window is open. |

Alternative flows:

| ID | Condition | System behavior |
|---|---|---|
| A1 | A refresh fails (backend down, timeout, HTTP error). | The last values stay on screen; the status bar shows `Disconnected from backend` with a red dot; the next refresh is attempted after 1 s. |
| A2 | The backend cannot sample the operating system (e.g. macOS placeholders). | The API answers `500`; behaves as A1. |

#### UI operations — FR-SYS-01

| # | Screen / element | Operation | Result | Source |
|---|---|---|---|---|
| 1 | Header | — (automatic) | Machine name, OS, version, user, session | `frontend/renderer/shell/shellModel.js` (`toShellData`), `shell/appShell.js` (`header`) |
| 2 | Quick stats | — (automatic) | Uptime, CPU / Mem / Disk chips, network rates, top memory process | same |
| 3 | Tab bar | — (automatic) | `Processes (n)`, `Services (n)` when the count is known | `shell/appShell.js` (`tabBar`) |
| 4 | Status bar | — (automatic) | Connection state (green / red dot), `System started at <local time>` | `shell/appShell.js` (`statusBar`) |

UI design: Figma "SystemLens UI" (see [ui-design/](../ui-design/)).

<a id="fr-perf-01"></a>
### FR-PERF-01 — Show every monitored resource with a live sparkline

| Item | Value |
|---|---|
| Status | Implemented (Windows, Linux). Selecting a resource other than CPU is not implemented. |
| Area | `PERF` — Performance tab |
| Description | The sidebar of the Performance screen lists CPU, memory, every physical disk, every Ethernet / Wi-Fi adapter and every GPU found on the machine, each with its current value and a 60-second sparkline, refreshed every second. |
| API | [`getPerformanceResources`](../api-schema.md#getperformanceresources) |
| Detailed in | [HLD-06](../design/high-level-design.md#hld-06), [LLD-04](../design/lld/LLD-04-metrics-collection.md), [LLD-05](../design/lld/LLD-05-live-performance-ui.md) |

<a id="fr-perf-02"></a>
### FR-PERF-02 — Show live CPU performance details

| Item | Value |
|---|---|
| Status | Implemented (Windows, Linux). The "Per-core" sub-tab is not implemented. |
| Area | `PERF` — Performance tab |
| Description | The CPU panel shows: processor model and topology; LED gauges and a 60-second chart of usage, clock and temperature; the 10 processes using the most CPU; utilization, speed, temperature, process / thread / handle counts, uptime and boot time; specifications (base speed, sockets, cores, logical processors, virtualization, cache sizes); the split of CPU time into user / kernel / DPC / interrupt / idle with a 60-second user vs kernel trend; context switches, interrupts, system calls and DPCs per second. Values refresh every second; unavailable values show `—`. |
| API | [`getCpuPerformance`](../api-schema.md#getcpuperformance) |
| Detailed in | [HLD-06](../design/high-level-design.md#hld-06), [LLD-04](../design/lld/LLD-04-metrics-collection.md), [LLD-05](../design/lld/LLD-05-live-performance-ui.md) |

<a id="uc-perf-01"></a>
#### UC-PERF-01 — Watch CPU performance

| Item | Value |
|---|---|
| Actor | User |
| Precondition | The application is running ([UC-SYS-01](#uc-sys-01)). |
| Trigger | User clicks the `⇄ CPU performance` toggle button in the tab bar. |

Main flow:

| Step | User | System |
|---|---|---|
| 1 | Clicks the toggle button. | Shows the Performance › CPU screen filled with the latest data (history collected since the backend started). |
| 2 | Watches the screen. | Updates sidebar, gauges, charts, tables and numbers every second. |
| 3 | Hovers an info icon. | Shows its tooltip; the tooltip stays open across refreshes. |

Alternative flows:

| ID | Condition | System behavior |
|---|---|---|
| A1 | A refresh fails. | The last values stay on screen; the status bar reports the disconnection ([UC-SYS-01](#uc-sys-01) A1). |
| A2 | A sensor or counter does not exist on the machine (e.g. CPU temperature in a virtual machine, system calls on Linux). | That value shows `—`; its chart series is not drawn. |

#### UI operations — FR-PERF-01, FR-PERF-02

| # | Screen / element | Operation | Result | Source |
|---|---|---|---|---|
| 1 | Tab bar, toggle button | Click | Switches between the hello screen and the CPU screen | `frontend/renderer/views/viewSwitcher.js` |
| 2 | Resource sidebar | — (automatic) | One row per resource: name, detail text, sparkline | `views/cpu/cpuPerformanceModel.js` (`toResources`), `views/cpu/cpuPerformanceView.js` (`resourceSidebar`) |
| 3 | CPU panel cards | — (automatic) | All cards show the latest sample | `views/cpu/cpuPerformanceModel.js` (`toCpuViewData`), `views/cpu/cpuPerformanceView.js` |
| 4 | Info icons | Hover / focus | Tooltip with title and description | `ui/tooltip.js`, texts in `views/cpu/cpuPerformanceTexts.js` |

UI design: Figma "SystemLens UI"; comparison screenshots in `evi/`.

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
