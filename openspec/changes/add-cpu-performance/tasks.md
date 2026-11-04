# Tasks

## 1. Provider contract

- [x] 1.1 Add `providers/cpu/ICpuProvider.h` (CpuStaticInfo, CpuTimes, ProcessCounters, CpuCounters, interface)
- [x] 1.2 Add `providers/system/ISystemInfoProvider.h` (SystemIdentity, interface)
- [x] 1.3 `PluginAbi.h`: ABI version 2, CPU and system info create/destroy entry points
- [x] 1.4 `PlatformPlugin`: generic `createProvider<T>`, `createCpuProvider()`, `createSystemInfoProvider()`

## 2. Services and controllers

- [x] 2.1 `CpuMetricsCalculator` (pure): breakdown, usage, rates with 32-bit wrap, top processes
- [x] 2.2 `CpuMonitorService`: sampling thread (1000 ms, condition variable), history of 60, 501 / 503 errors
- [x] 2.3 `SystemInfoService`
- [x] 2.4 `NotReadyError`, `status::kServiceUnavailable = 503`
- [x] 2.5 `CpuController` + `CpuJson`, `SystemInfoController`
- [x] 2.6 `Application`: create services and controllers, start the monitor before READY, stop after the server

## 3. Platform plugins

- [x] 3.1 Linux: `LinuxProcFs` parsers, `LinuxCpuProvider`, `LinuxSystemInfoProvider`, entry points
- [x] 3.2 Windows: `WindowsCpuProvider` (NtQuerySystemInformation, PDH, registry, GetLogicalProcessorInformationEx), `WindowsSystemInfoProvider`, `WideString`, entry points; link `ntdll`, `pdh`, `advapi32`
- [x] 3.3 macOS: no provider (comment in `PluginEntry.cpp`)
- [x] 3.4 CMake: plugin sources by glob of `plugins/<os>/*.cpp`, new core sources

## 4. Contract

- [x] 4.1 `api/openapi.yaml` 0.2.0: `getCpuInfo`, `getCpuMetrics`, `getSystemInfo`, schemas, `ServiceUnavailable`

## 5. Frontend

- [x] 5.1 `api/cpu/cpuApi.js`, `api/system/systemApi.js`
- [x] 5.2 `ui/format.js`, `plot.js` capacity (history grows from the right) and clamping
- [x] 5.3 `cpuPerformanceModel.js`, `perCoreHistory.js`, `shellModel.js`
- [x] 5.4 `cpuPerformanceView.js` with update() and the Per-core grid; `appShell.js` update()
- [x] 5.5 `app.js`: CPU screen by default, polling loop; `viewSwitcher.prerender()`
- [x] 5.6 `backendProcess.js`: Linux executable path

## 6. Tests

- [x] 6.1 GoogleTest target `systemlens_tests` (`SYSTEMLENS_BUILD_TESTS`): calculator, monitor, controllers, plugin loading, Linux parsers and provider
- [x] 6.2 Playwright e2e on Electron (`frontend/tests/e2e/`), video + screenshots, `npm run test:e2e`

## 7. Verification

- [x] 7.1 Linux (GCC, Ninja, `-Wall -Wextra -Wpedantic`): no warning; `ctest` 52/52 pass
- [x] 7.2 Linux e2e: `xvfb-run -a npm run test:e2e` 10/10 pass
- [x] 7.3 Windows cross-build (MinGW-w64, `-Wall -Wextra`): no warning; `systemlens_tests.exe` 36/36 pass under Wine; backend serves the three endpoints under Wine (PDH values null: not implemented by Wine)
- [ ] 7.4 Windows machine (MSVC, Visual Studio 2019): build, `ctest -C Debug`, `npm run test:e2e`

## 8. Docs

- [x] 8.1 `docs/requirements/RQ002_CpuPerformance.md`, `docs/design/RQ002_CpuPerformance/RQ002_Architecture.md`
- [x] 8.2 `docs/api/api-guide.md`, `docs/README.md`, root `README.md`
