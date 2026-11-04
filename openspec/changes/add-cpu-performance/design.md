# Design

## Context

Motivation and scope: see `proposal.md`. Requirements: `specs/cpu-performance/spec.md`,
`specs/system-info/spec.md`, `specs/cpu-performance-view/spec.md`.

Current state that shapes the approach:

- The platform plugin (`add-platform-plugin`) is loaded at startup; `PlatformPlugin` creates providers
  through `extern "C"` create/destroy entry points and returns them as `shared_ptr` whose deleter keeps
  the library loaded. A missing provider is a warning, and its endpoints answer 501.
- The CPU screen (`renderer/views/cpu/`) renders one data object shaped like `CPU_PERFORMANCE_MOCK`.
  The shell (`renderer/shell/`) renders one object shaped like `SHELL_MOCK`.
- Routing is an exact method + path match; responses are built only through `ok()` / `problem()`.

## Goals / Non-Goals

**Goals:**
- Real CPU data on Windows and Linux, with OS code confined to the plugin.
- Rate and percentage computations in OS-independent code, unit tested with fake providers.
- The existing view code keeps its structure: a mapper turns API responses into the view-model shape
  the view already renders.

**Non-Goals:**
- macOS providers (the endpoints answer 501 there).
- Memory, disk, network and GPU data (their sidebar entries and quick stats stay mock).
- More than 64 logical processors on Windows (only processor group 0 is read).
- WebSocket push; the frontend polls.

## Decisions

### D1. Source layout

Legend: `[A]` added, `[M]` modified.

```
api/openapi.yaml                                  [M] getCpuInfo, getCpuMetrics, getSystemInfo + schemas
backend/
├── CMakeLists.txt                                [M] new sources, Windows libs, SYSTEMLENS_BUILD_TESTS
├── src/
│   ├── api/
│   │   ├── ApiResponse.h                         [M] status::kServiceUnavailable = 503
│   │   ├── cpu/CpuController.h/.cpp              [A] GET /api/v1/cpu/info, /api/v1/cpu/metrics
│   │   ├── cpu/CpuJson.h/.cpp                    [A] CpuInfo / CpuMetrics → JSON
│   │   └── system/SystemInfoController.h/.cpp    [A] GET /api/v1/system/info
│   ├── app/Application.h/.cpp                    [M] creates the providers, services and controllers
│   ├── platform/PlatformPlugin.h/.cpp            [M] createCpuProvider(), createSystemInfoProvider()
│   ├── providers/
│   │   ├── PluginAbi.h                           [M] ABI 2, new entry points
│   │   ├── cpu/ICpuProvider.h                    [A] interface + CpuStaticInfo, CpuCounters
│   │   └── system/ISystemInfoProvider.h          [A] interface + SystemIdentity
│   └── services/
│       ├── cpu/CpuMetricsCalculator.h/.cpp       [A] two CpuCounters + elapsed time → CpuSnapshot (pure)
│       ├── cpu/CpuMetrics.h                      [A] CpuSnapshot, CpuHistory, CpuMetrics
│       ├── cpu/ICpuMonitorService.h              [A] getInfo(), getMetrics()
│       ├── cpu/CpuMonitorService.h/.cpp          [A] sampling thread, history ring
│       └── system/SystemInfoService.h/.cpp       [A] wraps the provider, FeatureUnavailableError
├── plugins/
│   ├── windows/WindowsCpuProvider.h/.cpp         [A] NtQuerySystemInformation, PDH, registry
│   ├── windows/WindowsSystemInfoProvider.h/.cpp  [A] GetComputerNameW, RtlGetVersion, GetUserNameW
│   ├── windows/PluginEntry.cpp                   [M] new entry points
│   ├── linux/LinuxCpuProvider.h/.cpp             [A] reads /proc and /sys through LinuxProcFs
│   ├── linux/LinuxProcFs.h/.cpp                  [A] pure parsers of /proc and /sys text (unit tested)
│   ├── linux/LinuxSystemInfoProvider.h/.cpp      [A] uname, /etc/os-release, getpwuid
│   ├── linux/PluginEntry.cpp                     [M] new entry points
│   └── macos/PluginEntry.cpp                     [M] ABI 2 only (no new provider)
└── tests/                                        [A] GoogleTest, target systemlens_tests
frontend/
├── package.json                                  [M] @playwright/test, scripts test:e2e
├── playwright.config.js                          [A] video + screenshot on, output tests/e2e/results
├── tests/e2e/                                    [A] Electron end-to-end specs and helpers
└── renderer/
    ├── app.js                                    [M] CPU screen by default, polling
    ├── api/cpu/cpuApi.js, api/system/systemApi.js [A]
    ├── shell/appShell.js                         [M] update(data) for live values
    ├── shell/shellModel.js                       [A] SystemInfo + CpuMetrics → shell view-model
    ├── views/cpu/cpuPerformanceView.js           [M] update(data), per-core grid, sub-tabs
    └── views/cpu/cpuPerformanceModel.js          [A] CpuInfo + CpuMetrics → view-model
```

The mock files stay: the memory / disk / network / GPU entries and the tooltips are read from them.

### D2. Provider contract: cumulative counters, not rates

`ICpuProvider::readCounters()` returns cumulative values; the host computes deltas.

| Field | Meaning |
|---|---|
| `total`, `perCore[]` | `CpuTimes {user, kernel, dpc, interrupt, idle}`: disjoint parts, same unit within one provider |
| `contextSwitches`, `interrupts`, `systemCalls`, `dpcs` | `optional<uint64_t>` cumulative event counts |
| `speedMhz`, `temperatureCelsius` | `optional<double>` instantaneous gauges |
| `processCount`, `threadCount`, `handleCount` | instantaneous counts (`handleCount` optional) |
| `uptimeSeconds`, `bootTimeUnixSeconds` | instantaneous |
| `processes[]` | `{pid, name, cpuTime, memoryBytes}`; `cpuTime` in `processTimeUnitsPerSecond` units |

Reasons: deltas, wrap handling and history are then written and tested once (`CpuMetricsCalculator`),
and providers stay thin readers. `readCounters()` is non-const because the Windows provider keeps a PDH
query open between calls.

Platform sources:

| Data | Windows | Linux |
|---|---|---|
| CPU times | `NtQuerySystemInformation(SystemProcessorPerformanceInformation)`; `kernel` = Kernel − Idle − Dpc − Interrupt | `/proc/stat` `cpu`/`cpuN`: user+nice, system+steal, softirq, irq, idle+iowait |
| Context switches, interrupts, system calls, DPCs | PDH raw values of `\System\Context Switches/sec`, `\Processor(_Total)\Interrupts/sec`, `\System\System Calls/sec`, `\Processor(_Total)\DPCs Queued/sec` | `/proc/stat` `ctxt`, `intr`, — (`null`), `softirq` |
| Speed | PDH `\Processor Information(_Total)\% Processor Performance` × base MHz | mean of `cpufreq/scaling_cur_freq`, else `/proc/cpuinfo` `cpu MHz` |
| Temperature | PDH `\Thermal Zone Information(*)\Temperature` (K), max zone | hwmon `coretemp`/`k10temp`/`zenpower`, else thermal zone `x86_pkg_temp`/`cpu*`, else `null` |
| Processes, threads, handles | `SystemProcessInformation`: entries, Σ threads, Σ handles | `/proc/<pid>` count, Σ `num_threads`, `/proc/sys/fs/file-nr` |
| Process CPU time / memory | `UserTime + KernelTime` (100 ns), private working set | `utime + stime` (`_SC_CLK_TCK`), RSS (`statm` × page size) |
| Uptime / boot time | `GetTickCount64`, now − uptime | `/proc/uptime`, `btime` of `/proc/stat` |
| Model, base speed | registry `CentralProcessor\0` `ProcessorNameString`, `~MHz` | `/proc/cpuinfo` `model name`; `cpufreq/base_frequency`, else `@ x.xxGHz` in the model |
| Max speed | — (`null`) | `cpufreq/cpuinfo_max_freq` |
| Topology, caches | `GetLogicalProcessorInformationEx(RelationAll)` | `/sys/devices/system/cpu/cpu*/topology`, `cache/index*` (each shared cache counted once) |
| Virtualization | `IsProcessorFeaturePresent(PF_VIRT_FIRMWARE_ENABLED)` | `vmx`/`svm` flag in `/proc/cpuinfo` |

The Windows plugin links `ntdll` and `pdh`. Process 0 (System Idle Process) is excluded from the top
processes.

### D3. CpuMetricsCalculator

Pure function: `compute(previous, current, elapsedSeconds, logicalProcessors) → CpuSnapshot`.

- Time shares: `Δpart / Δ(user + kernel + dpc + interrupt + idle) × 100`, usage = 100 − idle share,
  clamped to [0, 100]. A zero total delta gives 0 % usage.
- Rates: `counterDelta(prev, cur) / elapsedSeconds`. `counterDelta` handles wrap-around: when
  `cur < prev` and `prev ≤ UINT32_MAX` the counter is treated as a wrapped 32-bit counter
  (`cur + 2^32 − prev`); otherwise the counter was reset and the rate is `null`.
- Process CPU %: `ΔcpuTime / (elapsedSeconds × unitsPerSecond × logicalProcessors) × 100`, matched by
  pid **and** name (a reused pid is a new process: 0 % in its first interval). Top 10, ties by pid.

### D4. CpuMonitorService

- Constructed with `shared_ptr<ICpuProvider>` (may be empty), an interval (1000 ms) and a history
  length (60). `start()` reads the static info and a baseline sample, then starts the sampling thread;
  `stop()` (also in the destructor) wakes it through a condition variable so shutdown is immediate.
- Each tick: `readCounters()` + `steady_clock` elapsed time → `CpuMetricsCalculator` → latest snapshot
  and history ring (deque, oldest first), under a mutex. An exception from the provider is logged once
  per distinct message and the tick is skipped.
- `getInfo()` / `getMetrics()` throw `FeatureUnavailableError` without provider and `NotReadyError`
  before the first interval; they return copies.
- `Application` starts the service after the plugin is loaded and before READY.

### D5. Controllers and status codes

- `CpuController` maps `FeatureUnavailableError` → 501, `services::NotReadyError` → 503
  (`status::kServiceUnavailable`), success → 200 with `CpuJson`.
- `SystemInfoController` maps `FeatureUnavailableError` → 501.
- `null` values are written as JSON `null`, never omitted, so the schema keeps a fixed shape.

### D6. Frontend

- `app.js` creates the `ApiClient`, loads `getSystemInfo` and `getCpuInfo` once, then polls
  `getCpuMetrics` every 1000 ms (`setTimeout` after each response, so slow responses never overlap).
- `cpuPerformanceModel.js` turns `CpuInfo` + `CpuMetrics` into the `CPU_PERFORMANCE_MOCK` shape
  (formatting: `%`, GHz, °C, sizes, `—` for `null`; LED percentages; breakdown segment widths; CPU cell
  shade interpolated between the design's lightest and darkest shade).
- Clock series and gauge use `maxSpeedMhz`, else 2 × `baseSpeedMhz`, as full scale; temperature uses
  100 °C.
- The CPU view keeps one DOM tree and re-renders its parts on `update()`, which preserves tooltip
  state and the active sub-tab.
- Before the first metrics response, the view renders with `—` placeholders and empty charts.

### D7. Tests

Backend (`backend/tests/`, GoogleTest 1.15.2 via FetchContent, `SYSTEMLENS_BUILD_TESTS`, default OFF):

| Test file | Covers |
|---|---|
| `CpuMetricsCalculatorTest.cpp` | shares, usage clamp, rates, 32-bit wrap, reset, top processes, pid reuse |
| `CpuMonitorServiceTest.cpp` | 501 without provider, 503 before first sample, history length cap, stop |
| `CpuControllerTest.cpp` | routes, JSON shape, `null` fields, 200 / 501 / 503 |
| `SystemInfoControllerTest.cpp` | 200 / 501 |
| `LinuxProcFsTest.cpp` (Linux only) | parsers of `/proc/stat`, `cpuinfo`, `<pid>/stat`, cache size strings, cpu lists |
| `LinuxCpuProviderTest.cpp` (Linux only) | reads the live `/proc`: counts ≥ 1, times increase |

Frontend (`frontend/tests/e2e/`, `@playwright/test` with `_electron`): the app is launched with the
real Debug backend; every test records a video and saves named screenshots under
`frontend/tests/e2e/results/` (ignored by git). On Linux the tests run under `xvfb-run`.

## Risks / Trade-offs

- [Windows code cannot be run in the Linux build environment] → compiled with MinGW-w64 for syntax and
  type checking only; runtime verification on Windows is a remaining task.
- [PDH counter names are English] → `PdhAddEnglishCounterW` is used, independent of the UI language.
- [PDH thermal zones are missing on many machines] → temperature is `null` and shown as `—`.
- [`/proc` scan cost on Linux] → one pass per second over `/proc/<pid>/stat` and `statm`; acceptable for
  a desktop (a few hundred processes).
- [Polling every second from the renderer] → one small request per second on loopback.
