# Proposal

## Why

The Performance › CPU screen (branch `feature/int_cpu_preformance`) renders fixed values copied from the
Figma design. The backend has the platform plugin (change `add-platform-plugin`) but no provider that
reads real OS data. This change feeds the CPU screen with live data collected by the backend on Windows
and Linux, and adds automated tests for both sides.

## What Changes

- Two new providers in the platform plugin:
  - **CPU provider** (`ICpuProvider`): static processor facts (model, topology, base/max speed, caches,
    virtualization) and cumulative counters (CPU times per core, scheduler event counts, current speed,
    temperature, process/thread/handle counts, per-process CPU time and memory, uptime, boot time).
    - Windows: `NtQuerySystemInformation`, PDH, registry, `GetLogicalProcessorInformationEx`.
    - Linux: `/proc/stat`, `/proc/cpuinfo`, `/proc/<pid>/stat`, `/proc/uptime`, `/sys/devices/system/cpu`,
      `/sys/class/thermal`, `/sys/class/hwmon`.
    - macOS: no provider (the endpoints answer 501).
  - **System info provider** (`ISystemInfoProvider`): host name, OS name and version, user name.
- `kPluginAbiVersion` 1 → 2 (new entry points).
- New service `CpuMonitorService`: samples the CPU provider every 1 s on a background thread, turns
  cumulative counters into percentages and rates, keeps the last 60 samples.
- New endpoints (contract in `api/openapi.yaml`):
  - `GET /api/v1/cpu/info` (`getCpuInfo`): static processor facts.
  - `GET /api/v1/cpu/metrics` (`getCpuMetrics`): current values + 60-sample history; 503 until the
    first interval is measured; 501 without a CPU provider.
  - `GET /api/v1/system/info` (`getSystemInfo`): machine identity for the shell header; 501 without a
    provider.
- Frontend: the CPU screen, the CPU entry of the resource sidebar, the CPU / uptime quick stats, the
  machine header and the status bar show backend data, refreshed every 1 s. The Overview / Per-core
  sub-tabs switch between the overview and a per-core usage grid. Memory, disk, network and GPU
  entries keep their mock values (out of scope).
- Frontend loads the CPU screen by default; the toggle button still switches to the hello screen.
- Tests, kept apart from production code:
  - Backend: GoogleTest target `systemlens_tests` (`backend/tests/`), built when
    `SYSTEMLENS_BUILD_TESTS=ON`.
  - Frontend: Playwright end-to-end tests (`frontend/tests/e2e/`) that launch the Electron app with the
    real backend, with screenshots and video recording.

## Capabilities

### New Capabilities
- `cpu-performance`: CPU static info and live metrics served by the backend.
- `system-info`: machine identity served by the backend.
- `cpu-performance-view`: what the Performance › CPU screen shows and how it refreshes.

### Modified Capabilities
- `platform-plugin` (change `add-platform-plugin`, not archived): ABI version 2 with CPU and system
  info entry points. Behavior of loading and failure levels is unchanged.

## Impact

- **Backend**: `src/providers/cpu/`, `src/providers/system/`, `src/services/cpu/`,
  `src/services/system/`, `src/api/cpu/`, `src/api/system/`, `plugins/windows/`, `plugins/linux/`,
  `PlatformPlugin`, `Application`, `ApiResponse.h` (`kServiceUnavailable = 503`), `CMakeLists.txt`
  (Windows plugin links `ntdll`, `pdh`; tests target).
- **Frontend**: `renderer/api/cpu/`, `renderer/api/system/`, `renderer/views/cpu/`, `renderer/shell/`,
  `renderer/app.js`; `package.json` (Playwright dev dependency, `test:e2e` script).
- **API contract**: three new paths and their schemas.
- **Docs**: `docs/requirements/RQ002_CpuPerformance.md`, `docs/design/RQ002_CpuPerformance/`,
  `docs/api/api-guide.md`, `docs/README.md`.
