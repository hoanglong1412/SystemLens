# Tasks

## 1. Provider contract (header only)

- [x] 1.1 Add `backend/src/providers/sample/IHelloProvider.h`: pure virtual `getGreeting()` returning `std::string`, with interface comments
- [x] 1.2 Add `backend/src/providers/PluginAbi.h`: `kPluginAbiVersion = 1` (with the "bump on any interface change" rule), entry point names as string constants, `extern "C"` signatures (`abi_version`, `create_hello_provider`, `destroy_hello_provider`), export macro, and the same-tree / same-config / dynamic-CRT constraint in the header comment

## 2. Host-side loader (`backend/src/platform/`)

- [x] 2.1 Add `DynamicLibrary.h`: load by absolute path, look up a symbol by name, unload in destructor; non-copyable; errors as `std::runtime_error`
- [x] 2.2 Add `platform/windows/DynamicLibrary.cpp`: `LoadLibraryExW` with `LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32`, `GetProcAddress`, `FreeLibrary`, `GetLastError` turned into messages
- [x] 2.3 Add `platform/posix/DynamicLibrary.cpp`: `dlopen(RTLD_NOW | RTLD_LOCAL)`, `dlsym`, `dlclose`, `dlerror` messages (shared by Linux and macOS, built on Linux)
- [x] 2.4 Add `ExecutablePath.h` and `platform/windows/ExecutablePath.cpp` (`GetModuleFileNameW`, growing buffer)
- [x] 2.5 Add `platform/linux/ExecutablePath.cpp` (`readlink /proc/self/exe`, growing buffer)
- [x] 2.6 Add `platform/macos/ExecutablePath.cpp` (`_NSGetExecutablePath` + `realpath`) (written, not built)
- [x] 2.7 Add `PlatformPlugin.h/.cpp`: check the file exists (error names the expected path), load, look up the ABI entry point (error names it), compare versions (error shows both), `createHelloProvider()` returning `std::shared_ptr<const IHelloProvider>` whose deleter calls `destroy_hello_provider` and holds the library `shared_ptr`; on missing symbol or `nullptr`, log `[PlatformPlugin] WARNING: hello provider unavailable: ...` and return empty

## 3. Platform plugins (`backend/plugins/<os>/`)

- [x] 3.1 Add `plugins/windows/WindowsHelloProvider.h/.cpp` returning `"hello from Windows"`
- [x] 3.2 Add `plugins/windows/PluginEntry.cpp`: exported `noexcept` entry points, each catching everything and returning `nullptr` on failure; hello entry points only when `SYSTEMLENS_ENABLE_SAMPLE_API` is ON
- [x] 3.3 Add `plugins/linux/` with the same files, `LinuxHelloProvider` returning `"hello from Linux"`
- [x] 3.4 Add `plugins/macos/` with the same files, `MacHelloProvider` returning `"hello from MAC"` (written, not built)

## 4. Service, controller and composition root

- [x] 4.1 Add `backend/src/services/FeatureUnavailableError.h`
- [x] 4.2 Change `SampleHelloService` to take `std::shared_ptr<const IHelloProvider>` through its constructor, remove `kGreeting`, throw `FeatureUnavailableError` when the provider is empty
- [x] 4.3 Add `status::kNotImplemented = 501` to `api/ApiResponse.h`
- [x] 4.4 Change `SampleHelloController` to catch `FeatureUnavailableError` and return `problem(status::kNotImplemented, "Not Implemented", <message>)`; it still rejects a null service
- [x] 4.5 Change `Application`: `PlatformPlugin` member declared before the controllers, loaded in the constructor from executable directory + `SYSTEMLENS_PLATFORM_LIBRARY_FILE`, hello provider injected into `SampleHelloService`

## 5. Build (`backend/CMakeLists.txt`)

- [x] 5.1 Detect the platform into `SYSTEMLENS_PLATFORM` (`windows` / `linux` / `macos`), `FATAL_ERROR` naming `CMAKE_SYSTEM_NAME` otherwise
- [x] 5.2 Move `_WIN32_WINNT`, `WIN32_LEAN_AND_MEAN`, `NOMINMAX` under `if(WIN32)`
- [x] 5.3 Add `systemlens_provider_api` (`INTERFACE`) and link it into `systemlens_core`
- [x] 5.4 Add the `platform/` sources to `systemlens_core` (common + `windows/` or `posix/` loader + `<os>/ExecutablePath.cpp`), `${CMAKE_DL_LIBS}` on POSIX
- [x] 5.5 Add `systemlens_platform` (`SHARED`) from `plugins/<os>/`, linked only to `systemlens_provider_api`, with `/utf-8` under MSVC and `SYSTEMLENS_ENABLE_SAMPLE_API`
- [x] 5.6 Pass `SYSTEMLENS_PLATFORM_LIBRARY_FILE="$<TARGET_FILE_NAME:systemlens_platform>"` to `systemlens_core`; same output directory for exe and plugin; `add_dependencies(systemlens-backend systemlens_platform)`

## 6. API contract

- [x] 6.1 Update `api/openapi.yaml`: `SampleHelloResponse.message` description and example (platform greeting), add the `501` problem response to `getSampleHello`

## 7. Verification on Windows (manual, see design D7)

- [ ] 7.1 Build Debug; `backend/build/Debug/` contains `systemlens-backend.exe` and `systemlens_platform.dll` as the only platform library
- [ ] 7.2 `curl` `GET /api/v1/sample/hello` returns `{"message":"hello from Windows"}`, also when started from another working directory
- [ ] 7.3 DLL renamed: `Fatal:` names the expected path, no READY, exit code 1
- [ ] 7.4 ABI mismatch (temporary local edit, not committed): `Fatal:` shows both versions
- [ ] 7.5 Provider unavailable (temporary local edit, not committed): warning on stderr, READY printed, endpoint returns 501
- [ ] 7.6 Shutdown by closing stdin: exit code 0, no crash
- [ ] 7.7 Frontend starts with the DLL present (greeting shown) and shows its startup error with the DLL missing

## 8. Verification on Linux (manual, see design D7)

- [x] 8.1 Configure with Ninja and build; `backend/build/` contains `systemlens-backend` and `libsystemlens_platform.so` as the only platform library; no Windows-only definitions in the compile commands
- [x] 8.2 `curl` `GET /api/v1/sample/hello` returns `{"message":"hello from Linux"}`, also when started from another working directory
- [x] 8.3 Library removed from `backend/build/`, a copy in the current directory and in `LD_LIBRARY_PATH`: `Fatal:` names the expected path, no READY, exit code 1
- [x] 8.4 ABI mismatch and provider unavailable (temporary local edits, not committed): same results as 7.4 and 7.5
- [x] 8.5 Shutdown by closing stdin: exit code 0, no crash

## 9. Documentation (after the Windows and Linux builds succeed)

- [ ] 9.1 Update `docs/` (architecture: Provider layer and platform plugin; `docs/api/api-guide.md`: new greeting and 501)
- [ ] 9.2 Run `Tool/check_utf8.ps1` and `Tool/check_crlf.ps1` on every changed file
