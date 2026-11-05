# Tasks

## 1. Provider contract and platform detection

- [ ] 1.1 Add platform detection to `backend/CMakeLists.txt` (`SYSTEMLENS_PLATFORM`, `FATAL_ERROR` naming `CMAKE_SYSTEM_NAME` on an unsupported OS) and move `_WIN32_WINNT`, `WIN32_LEAN_AND_MEAN` and `NOMINMAX` under `if(WIN32)`; verify the existing Windows build still succeeds
- [ ] 1.2 Add `src/providers/sample/IHelloProvider.h` and `src/providers/PluginAbi.h` (ABI version, entry point declarations and names, export macro, the same-tree and CRT constraint in the header comment) and the `systemlens_provider_api` `INTERFACE` target linked by `systemlens_core`; verify the Windows build succeeds

## 2. Windows plugin

- [ ] 2.1 Add `plugins/windows/WindowsHelloProvider.h/.cpp`, returning `"hello from Windows"`, and `plugins/windows/PluginEntry.cpp` with `noexcept` entry points that catch every exception; verify the code compiles in task 2.2
- [ ] 2.2 Add the `systemlens_platform` `SHARED` target (links only `systemlens_provider_api`, `/utf-8` on MSVC, receives `SYSTEMLENS_ENABLE_SAMPLE_API`), `add_dependencies` from `systemlens-backend`, and `SYSTEMLENS_PLATFORM_LIBRARY_FILE` on `systemlens_core`; verify building the executable puts `systemlens_platform.dll` in `backend/build/Debug/` next to it

## 3. Host loader

- [ ] 3.1 Add `platform/DynamicLibrary.h` and `platform/windows/DynamicLibrary.cpp` (`LoadLibraryExW` with the D4 flags, symbol lookup, unload, OS errors as `std::runtime_error`); verify the Windows build succeeds
- [ ] 3.2 Add `platform/ExecutablePath.h` and `platform/windows/ExecutablePath.cpp` (`GetModuleFileNameW`); verify the Windows build succeeds
- [ ] 3.3 Add `platform/PlatformPlugin.h/.cpp` and the `platform/` sources to `systemlens_core`: file existence check, ABI check, `createHelloProvider()` with a warning and empty pointer when `create` or `destroy` is unavailable, and a deleter that holds the library `shared_ptr` (D6); verify the Windows build succeeds

## 4. Sample endpoint integration

- [ ] 4.1 Add `services/FeatureUnavailableError.h` and change `SampleHelloService` to take `std::shared_ptr<const IHelloProvider>`, removing `kGreeting` and throwing `FeatureUnavailableError` when the provider is empty; verify the Windows build succeeds
- [ ] 4.2 Add `status::kNotImplemented = 501` to `ApiResponse.h` and make `SampleHelloController` catch `FeatureUnavailableError` and return a 501 problem; verify the Windows build succeeds
- [ ] 4.3 Load `PlatformPlugin` in the `Application` constructor before services, declared before the controllers, and inject the hello provider; verify an authorized `GET /api/v1/sample/hello` returns `{"message":"hello from Windows"}`
- [ ] 4.4 Update `api/openapi.yaml` (description and example of `SampleHelloResponse.message`, new 501 response of `getSampleHello`); verify the file matches the response from task 4.3
- [ ] 4.5 Configure and build with `SYSTEMLENS_ENABLE_SAMPLE_API=OFF`; verify the build succeeds, the backend prints READY, and the plugin contains no hello provider

## 5. Linux and macOS code (written, not built)

- [ ] 5.1 Add `platform/posix/DynamicLibrary.cpp` (`dlopen` with `RTLD_NOW | RTLD_LOCAL`, `dlerror`), `platform/linux/ExecutablePath.cpp`, `platform/macos/ExecutablePath.cpp` and `${CMAKE_DL_LIBS}` on POSIX; not compiled in this change, verify by review against D2 and D4 and confirm the Windows build still succeeds
- [ ] 5.2 Add `plugins/linux/` (`"hello from Linux"`) and `plugins/macos/` (`"hello from MAC"`) with the same file structure as `plugins/windows/`; not compiled in this change, verify by review against the Windows plugin and the exact greeting strings in the spec

## 6. Windows verification

Temporary local edits in this group are reverted and never committed.

- [ ] 6.1 Verify the build output: `backend/build/Debug/` contains `systemlens-backend.exe` and `systemlens_platform.dll`, and no Linux or macOS library
- [ ] 6.2 Verify loading location: starting the exe from another directory returns the same greeting; with the DLL absent from the exe directory and a copy in the current directory, the backend treats the library as missing
- [ ] 6.3 Verify library-level failures: a missing DLL, a missing ABI entry point and an ABI mismatch (temporary edit) each print `Fatal:` with the expected path, entry point or both versions, print no READY and exit with code 1
- [ ] 6.4 Verify the frontend shows its existing startup error when the DLL is missing, and displays `hello from Windows` when it is present
- [ ] 6.5 Verify provider-level failures: a missing create entry point and a `nullptr` from create (temporary edits) each print a warning naming the provider, print READY, return 501 with a problem body on the hello endpoint, and keep other endpoints serving
- [ ] 6.6 Verify shutdown: closing stdin exits with code 0 and no crash

## 7. Documentation

- [ ] 7.1 Update `docs/` for the Provider layer, the platform plugin and the new hello response, after group 6 passes; verify every documented path and response matches the code
