# Proposal

## Why

SystemLens has to read OS data (processes, CPU, memory) that comes from a different API on each platform
(Win32 on Windows, `/proc` on Linux, `libproc`/`sysctl` on macOS). The backend has no place for
platform-specific code yet: the planned Provider layer (HttpServer → Controller → Service → Provider)
does not exist, and the build only targets Windows. This change adds that layer as a runtime-loaded
platform plugin and proves it end to end with the sample endpoint `getSampleHello`, before any real
OS feature depends on it.

## What Changes

- New **platform plugin**: one shared library per OS, named `systemlens_platform`
  (`systemlens_platform.dll` / `libsystemlens_platform.so` / `libsystemlens_platform.dylib`), holding the
  platform-specific providers. Hello is the first provider; later providers (process, CPU…) go into the
  same library.
- CMake detects the target platform at configure time and builds only that platform's plugin, placed
  next to the backend executable.
- The backend loads the plugin **at runtime** (`LoadLibrary` / `dlopen`), only from the executable's
  directory. The plugin exposes C++ provider interfaces through `extern "C"` create/destroy functions
  and an ABI version that the backend checks before use.
- Two failure levels:
  - Library level: if the plugin cannot be loaded or its ABI version does not match, the backend stops
    before announcing READY; the frontend shows its existing startup error.
  - Provider level: if the plugin loads but one provider is missing or cannot be created, the backend
    logs a warning and keeps running; only the endpoints of that provider answer `501 Not Implemented`.
- `SampleHelloService` gets its greeting from the hello provider instead of a constant.
- **BREAKING** `GET /api/v1/sample/hello` returns a platform-dependent `message` instead of the fixed
  `"@@Hello world@@"`:
  - Windows: `"hello from Windows"`
  - Linux: `"hello from Linux"`
  - macOS: `"hello from MAC"`
  The response shape is unchanged (no new field).
- Windows-only compile definitions in `backend/CMakeLists.txt` apply only to Windows builds.
- Windows and Linux are built and verified in this change. macOS code (plugin, `ExecutablePath`, CMake
  branch) is written but not built or tested; the POSIX library loader is shared with Linux, so it is
  built there.

## Capabilities

### New Capabilities
- `platform-plugin`: how the backend selects, locates, loads, validates and releases the per-OS platform
  library, and how it behaves when loading fails.
- `sample-hello`: the `GET /api/v1/sample/hello` behavior, returning a greeting that identifies the
  platform the backend runs on.

### Modified Capabilities

None (no existing specs).

## Impact

- **Backend code**: new host-side library loader (Windows and POSIX implementations) and plugin loading
  in `Application`; new provider interface; new `backend/plugins/<os>/` sources;
  `services/sample/SampleHelloService` takes the provider through its constructor.
- **Build**: `backend/CMakeLists.txt` gains platform detection and a shared-library plugin target per
  OS; the Windows-specific definitions move behind a Windows condition. Host and plugin must be built in
  the same CMake tree with the same configuration (shared C++ ABI and CRT).
- **API contract**: `api/openapi.yaml` description and example of `SampleHelloResponse.message`, and a
  new `501` response for `getSampleHello`.
- **Packaging**: the plugin file must ship next to `systemlens-backend` (executable); a missing plugin
  prevents startup.
- **Not affected**: frontend (it displays `message` as before); `docs/` updates are done as the last
  task after the Windows and Linux builds succeed.
