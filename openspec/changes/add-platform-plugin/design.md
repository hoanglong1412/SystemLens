# Design

## Context

Motivation and scope: see `proposal.md`. Requirements: `specs/platform-plugin/spec.md` and
`specs/sample-hello/spec.md`.

Current state that shapes the approach:

- `backend/CMakeLists.txt` builds `systemlens_core` (static library, all code except `main()`) and
  `systemlens-backend`. `_WIN32_WINNT`, `WIN32_LEAN_AND_MEAN` and `NOMINMAX` are `PUBLIC` definitions of
  `systemlens_core` on every platform. The Windows build uses the multi-config generator
  `Visual Studio 16 2019`; the executable lands in `backend/build/<Config>/`, and
  `frontend/main/backendProcess.js` and `Tool/run-backend.ps1` start it from `backend/build/Debug/`.
- `Application` is the composition root. Its constructor builds services and controllers; `run()` binds,
  prints READY and serves. An exception thrown from the constructor reaches `main()`, which prints
  `Fatal: <message>` to stderr and returns exit code 1, before any READY line.
- `HttpServer` turns any exception escaping a handler into a 500 problem; controllers build responses
  only through `ok()` / `problem()` (`api/ApiResponse.h`), whose `status` namespace has no 501 yet.
- `SampleHelloService` returns a constant; `SampleHelloController` takes `ISampleHelloService` through
  its constructor and rejects null with `std::invalid_argument`.
- Only Windows can be built and run in this change; Linux and macOS code is written but not compiled.

## Goals / Non-Goals

**Goals:**
- A Provider layer that the plugin implements and services consume through interfaces, reusable by
  later providers (process, CPU…) by adding files.
- All OS-specific code confined to per-OS source files selected by CMake; no `#ifdef` per OS in shared
  code.
- Library-level failures reuse the existing fatal-startup path; provider-level failures stay local to
  the affected endpoints.

**Non-Goals:**
- Loading several plugins, discovering plugins by scanning a directory, or hot reload.
- Plugins built outside this CMake tree or with another compiler or configuration (see D3).
- Building or testing on Linux/macOS, CI, and packaging/installers.
- Frontend changes and `docs/` updates (last task, after the Windows build succeeds).

## Decisions

### D1. Source layout and targets

Legend: `[A]` added, `[M]` modified, `[D]` deleted; no marker = unchanged, shown for context. No file
is deleted in this change.

```
api/
└── openapi.yaml                               [M] message description and example, new 501 response
backend/
├── CMakeLists.txt                             [M] platform detection, targets below
├── src/
│   ├── api/
│   │   ├── ApiResponse.h                      [M] status::kNotImplemented = 501
│   │   └── sample/SampleHelloController.cpp   [M] catches FeatureUnavailableError -> 501 problem
│   ├── app/
│   │   └── Application.h/.cpp                 [M] loads PlatformPlugin, injects the hello provider
│   ├── providers/                             [A] provider contract, shared by host and plugin (headers only)
│   │   ├── PluginAbi.h                        [A] ABI version, entry point names and signatures, export macro
│   │   └── sample/IHelloProvider.h            [A] pure virtual interface: getGreeting()
│   ├── platform/                              [A] host side, part of systemlens_core
│   │   ├── DynamicLibrary.h                   [A] load a library by absolute path, look up symbols, unload
│   │   ├── ExecutablePath.h                   [A] directory of the running executable
│   │   ├── PlatformPlugin.h/.cpp              [A] load + ABI check + create providers (OS independent)
│   │   ├── windows/                           [A] DynamicLibrary.cpp (LoadLibraryExW), ExecutablePath.cpp (GetModuleFileNameW)
│   │   ├── posix/                             [A] DynamicLibrary.cpp (dlopen), shared by Linux and macOS
│   │   ├── linux/                             [A] ExecutablePath.cpp (readlink /proc/self/exe)
│   │   └── macos/                             [A] ExecutablePath.cpp (_NSGetExecutablePath)
│   └── services/
│       ├── FeatureUnavailableError.h          [A] thrown by a service whose provider is missing
│       └── sample/SampleHelloService.h/.cpp   [M] takes std::shared_ptr<const IHelloProvider>; kGreeting removed
└── plugins/                                   [A]
    ├── windows/                               [A] PluginEntry.cpp, WindowsHelloProvider.h/.cpp
    ├── linux/                                 [A] same files, "hello from Linux"
    └── macos/                                 [A] same files, "hello from MAC"
```

CMake targets:

| Target | Kind | Content | Links |
|---|---|---|---|
| `systemlens_provider_api` | `INTERFACE` | include directory of `src/providers/` | — |
| `systemlens_core` | `STATIC` | existing code + `platform/` (common + `<os>` files) + `${CMAKE_DL_LIBS}` on POSIX | `systemlens_provider_api`, httplib, json |
| `systemlens_platform` | `SHARED` | `plugins/<os>/*` | `systemlens_provider_api` only |
| `systemlens-backend` | executable | `main.cpp` | `systemlens_core`; `add_dependencies` on `systemlens_platform` |

- The plugin links only the header-only provider contract, never `systemlens_core`, so it does not
  carry a copy of httplib, the server or the services.
- The executable does **not** link the plugin. `add_dependencies` makes building the executable also
  build the plugin, which keeps the runtime-only relationship explicit.
- Provider interfaces live under `src/providers/` because they are the Provider layer of
  HttpServer → Controller → Service → Provider: services depend on them; the plugin implements them.

Alternative considered: putting provider interfaces in `services/`. Rejected because the plugin would
then include service headers, which inverts the layering.

### D2. Platform selection in CMake

- `if(WIN32)` → `windows`, `elseif(APPLE)` → `macos`, `elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")` →
  `linux`, else `message(FATAL_ERROR ...)` naming `CMAKE_SYSTEM_NAME`. The result is one variable
  (`SYSTEMLENS_PLATFORM`) that selects both the host files and the plugin directory.
- The host loader uses `platform/windows/` on Windows and `platform/posix/` otherwise;
  `ExecutablePath.cpp` is taken from `platform/<os>/`, since Linux and macOS differ there.
- `_WIN32_WINNT`, `WIN32_LEAN_AND_MEAN` and `NOMINMAX` move under `if(WIN32)`; `/utf-8` stays under
  `if(MSVC)` and is also applied to the plugin.
- `SYSTEMLENS_ENABLE_SAMPLE_API` is also passed to the plugin: when it is OFF the plugin does not contain
  the hello provider.

Alternative considered: one source file per OS with `#ifdef _WIN32 / __APPLE__ / __linux__` blocks.
Rejected: CMake already knows the platform, and separate files keep each OS readable on its own.

### D3. Plugin ABI: C++ interfaces behind `extern "C"` entry points

`PluginAbi.h` defines:

```cpp
inline constexpr std::uint32_t kPluginAbiVersion = 1;   // bump on any change to a provider interface

extern "C" std::uint32_t   systemlens_plugin_abi_version() noexcept;
extern "C" IHelloProvider* systemlens_create_hello_provider() noexcept;   // nullptr on failure
extern "C" void            systemlens_destroy_hello_provider(IHelloProvider*) noexcept;
```

plus an export macro (`__declspec(dllexport)` on Windows, `__attribute__((visibility("default")))`
elsewhere) and the entry point names as string constants, so the host looks symbols up by the same
names the plugin exports.

- `extern "C"` gives unmangled names that `GetProcAddress` / `dlsym` can find.
- Entry points are `noexcept`; each plugin entry point catches everything and returns `nullptr`, because
  an exception must not unwind across the library boundary.
- Objects created by the plugin are destroyed by the plugin (`destroy_*`), never by `delete` in the
  host.
- Passing `std::string` and vtables across the boundary is valid only when host and plugin share the
  compiler, standard library and CRT. This is guaranteed by building both in the same CMake tree and
  configuration with the dynamic CRT (`/MD`, `/MDd`, the CMake default); it is a documented constraint,
  not something checked at runtime. `kPluginAbiVersion` catches interface changes, not compiler or CRT
  changes.

Alternative considered: a pure C API (struct of function pointers, caller-owned buffers). It is robust
to compiler and CRT differences but needs an adapter per provider. Rejected because host and plugin are
always built together; the constraint above covers the risk.

Alternative considered: Code repeats in the host (`PlatformPlugin`) and the plugin (`PluginEntry.cpp`) for each new provider, but
there is only one provider now; reconsider when there are two or more.

### D4. Locating and loading the library

- The file name comes from CMake: `systemlens_core` gets
  `SYSTEMLENS_PLATFORM_LIBRARY_FILE="$<TARGET_FILE_NAME:systemlens_platform>"`, so the host never builds
  the name from a prefix or suffix and always matches the built file.
- Both targets write to the same output directory. Executable and plugin are defined in the same
  directory scope, so with the Visual Studio generator both land in `backend/build/<Config>/` and the
  existing paths in `backendProcess.js` and `run-backend.ps1` keep working.
- Full path = `ExecutablePath` directory + file name. Loading by absolute path means no search:
  - Windows: `LoadLibraryExW(path, nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32)`,
    so dependencies of the DLL are also resolved only from its own directory and System32, never from
    the current directory or `PATH`.
  - POSIX: `dlopen(path, RTLD_NOW | RTLD_LOCAL)`; a path containing `/` is not searched, and `RTLD_NOW`
    reports missing symbols at load time instead of at first call.
- Before loading, the host checks that the file exists so the error can name the expected path.
  `DynamicLibrary` turns OS errors (`GetLastError` / `dlerror`) into `std::runtime_error` messages.

### D5. Two failure levels

```
Application()                                    (constructor, before bind and READY)
 |
 +-- PlatformPlugin::load(dir + file name)
 |     file missing / load error / no abi symbol / ABI mismatch
 |       --> throw std::runtime_error --> main(): "Fatal: ..." on stderr, exit code 1
 |
 +-- plugin.createHelloProvider()
 |     symbol missing or nullptr --> stderr "[PlatformPlugin] WARNING: hello provider unavailable: ..."
 |                                   --> returns empty pointer, startup continues
 |
 +-- SampleHelloService(provider or empty) --> SampleHelloController --> routes
```

- **Library level** throws from the `Application` constructor, reusing the existing fatal path. No new
  exit code is introduced; the frontend already reports a backend that exits before READY.
- **Provider level**: `PlatformPlugin` returns an empty `shared_ptr` and logs the warning once at
  startup. The route is still registered.
- `SampleHelloService::getGreeting()` throws `services::FeatureUnavailableError` when it has no
  provider. `SampleHelloController` catches it and returns `problem(status::kNotImplemented, "Not
  Implemented", <message>)`; `status::kNotImplemented = 501` is added to `ApiResponse.h`.
- `SampleHelloController` still rejects a null *service*; only the *provider* inside the service may be
  empty.

Alternative considered: `HttpServer` maps `FeatureUnavailableError` to 501 for every route. It avoids a
`try/catch` per controller, but makes the HTTP infrastructure depend on a service-layer type. Rejected
for this change; it can be revisited when several controllers repeat the same catch.

Alternative considered: not registering the route (404). Rejected by the specs: 501 tells "not on this
platform" apart from a wrong URL.

### D6. Lifetime of the library

`PlatformPlugin` holds the library as `std::shared_ptr<DynamicLibrary>`. Each provider is returned as
`std::shared_ptr<const IHelloProvider>` whose deleter calls `systemlens_destroy_hello_provider` and keeps
a copy of the library `shared_ptr`. The library is therefore unloaded only after the last provider is
destroyed, whatever the member destruction order in `Application`. `Application` also keeps the
`PlatformPlugin` as a member declared before the controllers, matching the existing rule that what
handlers point into is destroyed last.

Alternative considered: relying only on member declaration order. Rejected as the only safeguard
because a provider copied into another object would silently outlive the library and crash on
destruction (its vtable lives in the unloaded code).

### D7. Verification on Windows

There is no automated test target. Verification is manual, with `Tool/run-backend.ps1` and `curl`:

| Spec scenario | How |
|---|---|
| Windows build output | Build; `backend/build/Debug/` contains `systemlens-backend.exe` and `systemlens_platform.dll` only as platform library |
| Greeting | `curl http://127.0.0.1:8765/api/v1/sample/hello` returns `{"message":"hello from Windows"}` |
| Other working directory | Start the exe from another directory; same result |
| Library missing | Rename the DLL; the exe prints `Fatal:` with the expected path, no READY, exit code 1 |
| ABI mismatch | Temporary local edit of the plugin's reported version, not committed; `Fatal:` with both versions |
| Provider unavailable | Temporary local edit making `create_hello_provider` return `nullptr`, not committed; warning on stderr, READY printed, endpoint returns 501 |
| Shutdown | Close stdin (or quit the app); process exits with code 0 |

Alternative considered: automated tests. Deferred: they will be developed later.

## Risks / Trade-offs

- [Host and plugin built with different configurations or CRTs (for example a Release plugin copied next
  to a Debug exe)] → heap corruption when a `std::string` is freed. Mitigation: same CMake tree and
  output directory (D3, D4); constraint recorded in the plugin header comment.
- [A provider interface changes and `kPluginAbiVersion` is not bumped] → undefined behavior instead of a
  clean error. Mitigation: the constant sits next to the interfaces in `src/providers/` with a comment
  stating the rule; host and plugin are rebuilt together anyway.
- [Linux and macOS code is never compiled in this change] → it may not build. Mitigation: those tasks
  are marked "written, not built"; the OS-specific surface is kept to `DynamicLibrary`,
  `ExecutablePath` and the plugin files.
- [DLL planting in the executable directory] → a malicious `systemlens_platform.dll` placed there is
  loaded. Accepted: whoever can write to the executable directory can already replace the executable.
- [The executable and its DLL must ship together] → copying only the exe gives a fatal startup.
  Mitigation: the error names the expected path; packaging is out of scope.
- [501 handling is per controller] → each future controller repeats a small `try/catch`. Accepted for
  now (D5).
- [An entry point name differs between host and plugin (typo, or renamed on one side only)] → not caught
  at build time; the symbol lookup fails at runtime. Not handled for now; consider a fix later.

## Migration Plan

No data or deployment migration. `SampleHelloResponse.message` changes value
(`"@@Hello world@@"` → `"hello from Windows"` on Windows); the frontend only displays it.
`api/openapi.yaml` is updated with the new description, example and the 501 response. Rollback is
reverting the commit.
