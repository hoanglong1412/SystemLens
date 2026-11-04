# RQ002 — CPU performance screen

| Item | Value |
|---|---|
| RQ | RQ002 |
| Title | Performance › CPU screen with live data on Windows and Linux |
| Status | Implemented (Windows: built and run under Wine; run on a Windows machine pending) |
| Design | [RQ002_Architecture.md](../design/RQ002_CpuPerformance/RQ002_Architecture.md) |
| UI design | Figma "SystemLens UI" ([link](../ui-design/figma.onl)) |
| Index / matrix | [docs/README.md](../README.md#2-requirement-files) |

## 1. Requirement list

| Type | ID | Title | Status |
|---|---|---|---|
| FR | [RQ002-FR-01](#rq002-fr-01) | Show live CPU data on the Performance › CPU screen | Implemented |
| FR | [RQ002-FR-02](#rq002-fr-02) | Show per-core usage | Implemented |
| FR | [RQ002-FR-03](#rq002-fr-03) | Show the machine identity in the app shell | Implemented |
| NFR | [RQ002-NFR-01](#rq002-nfr-01) | Windows and Linux are supported; macOS answers 501 | Implemented |
| NFR | [RQ002-NFR-02](#rq002-nfr-02) | Backend and frontend have automated tests kept apart from the production code | Implemented |

## 2. Functional requirements

<a id="rq002-fr-01"></a>
### RQ002-FR-01 — Show live CPU data on the Performance › CPU screen

| Item | Value |
|---|---|
| Status | Implemented |
| Requirement | The application opens on the Performance › CPU screen. The screen shows the processor model and topology, live gauges (usage, clock, temperature), the % Utilization chart over 60 s, the 10 processes with the highest CPU usage, the summary (utilization, speed, temperature, processes, threads, handles, up time, start time), the specifications, the CPU time breakdown with the User vs Kernel trend, and the scheduler activity. Values refresh every second. A value the operating system does not report is shown as `—`. |
| API | `getCpuInfo`, `getCpuMetrics` |

<a id="rq002-uc-01"></a>
#### RQ002-UC-01 — Watch the CPU load

```mermaid
usecase-beta
direction LR
actor User
systemBoundary SystemLens["SystemLens"]
  RQ002_UC_01_1("Open the application")
  RQ002_UC_01_2("Watch live CPU values")
  RQ002_UC_01_3("Read a card's tooltip")
  RQ002_UC_01_4("Switch to the hello screen")
end
User --> RQ002_UC_01_1
User --> RQ002_UC_01_2
User --> RQ002_UC_01_3
User --> RQ002_UC_01_4
```

Main flow:

| Step | User | System |
|---|---|---|
| 1 | Starts the application. | Starts the backend; the backend starts sampling the CPU counters every second. |
| 2 | — | Opens the window on the CPU screen with `—` placeholders. |
| 3 | — | Loads the machine identity and the processor facts once, then requests the metrics every second. |
| 4 | Watches the values. | Re-renders the screen, the CPU sidebar entry and the CPU / uptime quick stats with each response. |

Alternative flows:

| ID | Condition | System behavior |
|---|---|---|
| A1 | The first interval is not measured yet (503). | Keeps the placeholders; the next request one second later succeeds. |
| A2 | A request fails (backend unreachable). | The status bar shows `Backend unreachable` with a red dot; requests continue every second and the status returns to `Connected to backend` once one succeeds. |
| A3 | The platform has no CPU provider (501). | The CPU values stay `—`; the metrics are not requested again. |

<a id="rq002-fr-02"></a>
### RQ002-FR-02 — Show per-core usage

| Item | Value |
|---|---|
| Status | Implemented |
| Requirement | The Overview / Per-core sub-tabs switch the CPU detail panel between the overview cards and a grid with one usage chart per logical processor. |
| API | `getCpuMetrics` (`current.perCoreUsagePercent`) |

<a id="rq002-fr-03"></a>
### RQ002-FR-03 — Show the machine identity in the app shell

| Item | Value |
|---|---|
| Status | Implemented |
| Requirement | The shell header shows the host name, OS name, OS version and user name; the status bar shows the backend connection state, the update speed and the boot time. |
| API | `getSystemInfo`, `getCpuMetrics` (`current.bootTime`) |

Out of scope: memory, disk, network and GPU data. Their sidebar entries and quick-stat chips show fixed values
from the UI design.

## 3. Non-functional requirements

<a id="rq002-nfr-01"></a>
### RQ002-NFR-01 — Windows and Linux are supported; macOS answers 501

| Item | Value |
|---|---|
| Status | Implemented |
| Requirement | All OS-specific code is in the platform plugin (`backend/plugins/<os>/`). Windows and Linux provide the CPU and system info providers. The macOS plugin provides neither: the CPU and system endpoints answer 501 and the backend keeps running. |
| Evidence | `plugins/windows/`, `plugins/linux/`; Windows build cross-compiled with MinGW-w64 and run under Wine. |

<a id="rq002-nfr-02"></a>
### RQ002-NFR-02 — Automated tests kept apart from the production code

| Item | Value |
|---|---|
| Status | Implemented |
| Requirement | Backend tests use GoogleTest (`backend/tests/`, target `systemlens_tests`, CMake option `SYSTEMLENS_BUILD_TESTS`). Frontend end-to-end tests use Playwright (`frontend/tests/e2e/`) on the Electron app with the real backend, and record a video and screenshots of every test. |
| Evidence | `ctest --test-dir backend/build`; `xvfb-run -a npm run test:e2e` (Linux) or `npm run test:e2e` (Windows) in `frontend/`. |
