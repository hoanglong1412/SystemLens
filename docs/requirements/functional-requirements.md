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
| [FR-NAV-01](#fr-nav-01) | Switch between the screens with the tab bar | Implemented | [UC-NAV-01](#uc-nav-01) | [HLD-07](../design/high-level-design.md#hld-07), [LLD-07](../design/lld/LLD-07-screens.md) |
| [FR-PERF-03](#fr-perf-03) | Memory panel | Implemented (Windows, Linux) | [UC-PERF-02](#uc-perf-02) | [LLD-06](../design/lld/LLD-06-detail-and-inventory.md), [LLD-07](../design/lld/LLD-07-screens.md) |
| [FR-PERF-04](#fr-perf-04) | Disk panel | Implemented (Windows, Linux) | [UC-PERF-02](#uc-perf-02) | same |
| [FR-PERF-05](#fr-perf-05) | Ethernet / Wi-Fi panel | Implemented (Windows, Linux) | [UC-PERF-02](#uc-perf-02) | same |
| [FR-PERF-06](#fr-perf-06) | GPU panel | Implemented (Windows, Linux; fewer values on Linux) | [UC-PERF-02](#uc-perf-02) | same |
| [FR-PERF-07](#fr-perf-07) | CPU "Per-core" sub-tab | Implemented (Windows, Linux) | [UC-PERF-02](#uc-perf-02) | same |
| [FR-PROC-01](#fr-proc-01) | Processes tab (grouped by name) | Implemented (Windows, Linux) | [UC-PROC-01](#uc-proc-01) | same |
| [FR-PROC-02](#fr-proc-02) | Details tab (flat process table) | Implemented (Windows, Linux) | [UC-PROC-01](#uc-proc-01) | same |
| [FR-HIST-01](#fr-hist-01) | App history tab | Implemented (Windows, Linux) | [UC-PROC-01](#uc-proc-01) | same |
| [FR-SVC-01](#fr-svc-01) | Services tab | Implemented (Windows; Linux with systemd) | [UC-INV-01](#uc-inv-01) | same |
| [FR-START-01](#fr-start-01) | Startup tab | Implemented (Windows, Linux) | [UC-INV-01](#uc-inv-01) | same |
| [FR-USER-01](#fr-user-01) | Users tab | Implemented (Windows, Linux) | [UC-INV-01](#uc-inv-01) | same |

## 2. Requirements

<a id="fr-smp-01"></a>
### FR-SMP-01 — Show the sample greeting from the backend

| Item | Value |
|---|---|
| Status | Implemented |
| Area | `SMP` — sample feature; exists to prove that the frontend can call the backend |
| Description | The sample greeting screen shows the greeting returned by the backend API `GET /api/v1/sample/hello`, or the error if the backend cannot be reached. Since [FR-NAV-01](#fr-nav-01) the application opens on the Performance tab; the greeting screen is reached with the temporary `⇄ Hello` button of the tab bar. |
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
| Status | Implemented (Windows, Linux). Rows are selectable ([FR-PERF-03](#fr-perf-03)…[FR-PERF-06](#fr-perf-06)). |
| Area | `PERF` — Performance tab |
| Description | The sidebar of the Performance screen lists CPU, memory, every physical disk, every Ethernet / Wi-Fi adapter and every GPU found on the machine, each with its current value and a 60-second sparkline, refreshed every second. |
| API | [`getPerformanceResources`](../api-schema.md#getperformanceresources) |
| Detailed in | [HLD-06](../design/high-level-design.md#hld-06), [LLD-04](../design/lld/LLD-04-metrics-collection.md), [LLD-05](../design/lld/LLD-05-live-performance-ui.md) |

<a id="fr-perf-02"></a>
### FR-PERF-02 — Show live CPU performance details

| Item | Value |
|---|---|
| Status | Implemented (Windows, Linux). The "Per-core" sub-tab: [FR-PERF-07](#fr-perf-07). |
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

<a id="fr-nav-01"></a>
### FR-NAV-01 — Switch between the screens with the tab bar

| Item | Value |
|---|---|
| Status | Implemented |
| Area | `NAV` — navigation |
| Description | The tab bar lists Processes, Performance, App history, Startup, Users, Details and Services. Clicking a tab (or pressing Enter / Space on it) shows that screen and updates the window title. The application opens on Performance. A screen refreshes from the backend only while it is visible and keeps its sort order, filter text and scroll position while hidden. The temporary `⇄ Hello` button shows the sample greeting screen ([FR-SMP-01](#fr-smp-01)). |
| API | — (each screen uses its own endpoints) |
| Detailed in | [HLD-07](../design/high-level-design.md#hld-07), [LLD-07](../design/lld/LLD-07-screens.md) |

<a id="uc-nav-01"></a>
#### UC-NAV-01 — Open another screen

Main flow: the user clicks a tab → the system hides the current screen (it stops polling), shows the chosen screen (rendered on first use, it starts polling) and marks the tab active. Alternative: the backend is unreachable → the screen keeps its last values and the status bar reports the disconnection ([UC-SYS-01](#uc-sys-01) A1).

<a id="fr-perf-03"></a>
### FR-PERF-03 — Memory panel

| Item | Value |
|---|---|
| Status | Implemented (Windows, Linux) |
| Area | `PERF` |
| Description | Selecting Memory in the Performance sidebar shows: 60-second chart of memory in use; in use, available, cached, committed and usage figures; total memory and commit limit; the 10 processes using the most memory. |
| API | [`getMemoryPerformance`](../api-schema.md#getmemoryperformance) |
| Detailed in | [LLD-06](../design/lld/LLD-06-detail-and-inventory.md), [LLD-07](../design/lld/LLD-07-screens.md) |

<a id="fr-perf-04"></a>
### FR-PERF-04 — Disk panel

| Item | Value |
|---|---|
| Status | Implemented (Windows, Linux) |
| Area | `PERF` |
| Description | Selecting a disk shows: 60-second charts of active time and of read / write transfer rate (scaled to the peak of the window); active time, read speed, write speed; model, media type (SSD / HDD), capacity and device name. |
| API | [`getDiskPerformance`](../api-schema.md#getdiskperformance) |
| Detailed in | [LLD-06](../design/lld/LLD-06-detail-and-inventory.md), [LLD-07](../design/lld/LLD-07-screens.md) |

<a id="fr-perf-05"></a>
### FR-PERF-05 — Ethernet / Wi-Fi panel

| Item | Value |
|---|---|
| Status | Implemented (Windows, Linux) |
| Area | `PERF` |
| Description | Selecting an adapter shows: 60-second chart of send and receive throughput (scaled to the peak of the window); send rate, receive rate and link speed; adapter name, description, type, MAC address. Values the platform does not provide (e.g. link speed of virtual adapters) show `—`. |
| API | [`getNetworkPerformance`](../api-schema.md#getnetworkperformance) |
| Detailed in | [LLD-06](../design/lld/LLD-06-detail-and-inventory.md), [LLD-07](../design/lld/LLD-07-screens.md) |

<a id="fr-perf-06"></a>
### FR-PERF-06 — GPU panel

| Item | Value |
|---|---|
| Status | Implemented (Windows, Linux). Linux reports utilization and memory only for drivers that export them (amdgpu); Windows has no GPU temperature or memory in use. |
| Area | `PERF` |
| Description | Selecting a GPU shows: 60-second utilization chart; utilization, temperature and video memory in use; model and dedicated memory. |
| API | [`getGpuPerformance`](../api-schema.md#getgpuperformance) |
| Detailed in | [LLD-06](../design/lld/LLD-06-detail-and-inventory.md), [LLD-07](../design/lld/LLD-07-screens.md) |

<a id="fr-perf-07"></a>
### FR-PERF-07 — CPU "Per-core" sub-tab

| Item | Value |
|---|---|
| Status | Implemented (Windows, Linux) |
| Area | `PERF` |
| Description | The "Per-core" sub-tab of the CPU panel shows one card per logical processor with its current usage and 60-second plot; "Overview" returns to the summary cards. |
| API | [`getCpuCores`](../api-schema.md#getcpucores) |
| Detailed in | [LLD-06](../design/lld/LLD-06-detail-and-inventory.md), [LLD-07](../design/lld/LLD-07-screens.md) |

<a id="uc-perf-02"></a>
#### UC-PERF-02 — Inspect a resource

Main flow: on the Performance tab the user clicks a sidebar row (Memory, a disk, an adapter, a GPU) → the panel of that resource opens and fills with its data within a second; the sidebar keeps updating. Clicking CPU and then "Per-core" shows the per-processor cards. Alternative: the selected resource disappears (e.g. a removed disk) → the CPU panel is shown.

<a id="fr-proc-01"></a>
### FR-PROC-01 — Processes tab (grouped by name)

| Item | Value |
|---|---|
| Status | Implemented (Windows, Linux) |
| Area | `PROC` |
| Description | Lists the processes grouped by name. A name with several instances is a group row (name and count, summed CPU %, memory and threads, user or `Various`) that expands / collapses on click; a single instance is a plain row. Columns: Name, PID, User, CPU %, Memory, Threads. Sortable by every column (numbers descending first); a filter box matches name, PID and user (groups open while filtering). Own processes (the backend and the UI) are highlighted. Refreshes every 2 s. Read-only: processes cannot be ended. |
| API | [`getProcesses`](../api-schema.md#getprocesses) |
| Detailed in | [LLD-06](../design/lld/LLD-06-detail-and-inventory.md), [LLD-07](../design/lld/LLD-07-screens.md) |

<a id="fr-proc-02"></a>
### FR-PROC-02 — Details tab (flat process table)

| Item | Value |
|---|---|
| Status | Implemented (Windows, Linux) |
| Area | `PROC` |
| Description | One row per process with PID, Name, Status, User, CPU %, CPU time, Memory, Threads, Parent PID and Command line. Status and command line exist on Linux only (`—` on Windows). Sortable by every column; filter box matches name, PID, user and command line; shows "n of m processes". Refreshes every 2 s. |
| API | [`getProcesses`](../api-schema.md#getprocesses) |
| Detailed in | [LLD-06](../design/lld/LLD-06-detail-and-inventory.md), [LLD-07](../design/lld/LLD-07-screens.md) |

<a id="fr-hist-01"></a>
### FR-HIST-01 — App history tab

| Item | Value |
|---|---|
| Status | Implemented (Windows, Linux) |
| Area | `HIST` |
| Description | Per application name: accumulated CPU time (including instances that already exited), peak memory, memory now and running instances, since the backend started (shown as "Since <time>"; kept in memory only, not persisted). Sortable, filterable by name, refreshes every 2 s. |
| API | [`getAppHistory`](../api-schema.md#getapphistory) |
| Detailed in | [LLD-06](../design/lld/LLD-06-detail-and-inventory.md), [LLD-07](../design/lld/LLD-07-screens.md) |

<a id="uc-proc-01"></a>
#### UC-PROC-01 — Find and inspect processes

Main flow: the user opens Processes, Details or App history → the table shows the latest data and refreshes → the user clicks a header to sort, types in the filter box to narrow the rows, clicks a row to select it (Processes: a group row expands). Alternative: no row matches → the table shows "No … matches the filter".

<a id="fr-svc-01"></a>
### FR-SVC-01 — Services tab

| Item | Value |
|---|---|
| Status | Implemented (Windows; Linux with systemd) |
| Area | `SVC` |
| Description | Lists installed services: Name, Description (Windows: display name), Status (running services in green), Startup type, PID (Windows only). Sortable; filter box plus a status filter (all / running / stopped); refreshes every 5 s. When the platform cannot list services (no systemd, macOS) a banner "Not available on this system: <reason>" is shown. Read-only: services cannot be started or stopped. |
| API | [`getServices`](../api-schema.md#getservices) |
| Detailed in | [LLD-06](../design/lld/LLD-06-detail-and-inventory.md), [LLD-07](../design/lld/LLD-07-screens.md) |

<a id="fr-start-01"></a>
### FR-START-01 — Startup tab

| Item | Value |
|---|---|
| Status | Implemented (Windows, Linux) |
| Area | `START` |
| Description | Lists the programs started at logon: Name, Command, Applies to (current user / all users), Status (enabled / disabled, disabled rows dimmed), Location (registry key or folder). Windows: `Run` keys and Startup folders; Linux: XDG autostart `.desktop` files. Sortable, filterable, refreshes every 5 s; banner when unavailable. Read-only: items cannot be enabled or disabled. |
| API | [`getStartupItems`](../api-schema.md#getstartupitems) |
| Detailed in | [LLD-06](../design/lld/LLD-06-detail-and-inventory.md), [LLD-07](../design/lld/LLD-07-screens.md) |

<a id="fr-user-01"></a>
### FR-USER-01 — Users tab

| Item | Value |
|---|---|
| Status | Implemented (Windows, Linux) |
| Area | `USER` |
| Description | Two tables: logon sessions (User, Session, ID, Status, Logon time, Client) and resource usage by user (processes, CPU %, memory, summed over the processes each user owns; `(unknown)` for processes whose owner cannot be read). Linux shows a single entry for the user running the backend when there is no login record (containers, servers without a login manager). Refreshes every 5 s; banner when sessions are unavailable. |
| API | [`getUsers`](../api-schema.md#getusers) |
| Detailed in | [LLD-06](../design/lld/LLD-06-detail-and-inventory.md), [LLD-07](../design/lld/LLD-07-screens.md) |

<a id="uc-inv-01"></a>
#### UC-INV-01 — Review services, startup items and users

Main flow: the user opens Services, Startup or Users → the screen shows the list and refreshes every 5 s → the user sorts, filters or selects rows. Alternative: the platform cannot provide the list → a banner explains why and the table is empty.

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
