# Spec Delta

## Purpose

Gives the backend one place for operating-system-specific behavior: a platform library built for the
target OS and loaded by the backend at runtime, so features that read OS data share one loading and
failure model.

## ADDED Requirements

### Requirement: Build produces the platform library of the target OS
The build SHALL detect the target operating system at configure time and SHALL build exactly one
platform library, the one for that OS (Windows, Linux or macOS). The library file SHALL follow the OS
shared-library naming convention and SHALL be placed in the same directory as the backend executable.

#### Scenario: Windows build
- **WHEN** the backend is configured and built on Windows
- **THEN** the output directory of the backend executable contains `systemlens_platform.dll`
- **AND** it contains no Linux or macOS platform library

#### Scenario: Linux build
- **WHEN** the backend is configured and built on Linux
- **THEN** the output directory of the backend executable contains `libsystemlens_platform.so`

#### Scenario: macOS build
- **WHEN** the backend is configured and built on macOS
- **THEN** the output directory of the backend executable contains `libsystemlens_platform.dylib`

#### Scenario: Unsupported operating system
- **WHEN** the backend is configured on an operating system other than Windows, Linux or macOS
- **THEN** configuration fails with an error naming the unsupported platform

### Requirement: Platform library is loaded at runtime from the executable directory
The backend SHALL load the platform library at runtime during startup, before it announces READY. It
SHALL look for the library only in the directory that contains the backend executable, regardless of
the current working directory and of the system library search path.

#### Scenario: Library next to the executable
- **WHEN** the backend starts and the platform library is in the executable's directory
- **THEN** the backend loads it and continues startup

#### Scenario: Started from another working directory
- **WHEN** the backend is started with a current working directory different from the executable's directory
- **THEN** the backend still loads the platform library from the executable's directory

#### Scenario: Library with the same name elsewhere on the search path
- **WHEN** the platform library is missing from the executable's directory but a file with the same name exists in the current working directory or on the system library search path
- **THEN** the backend does not load that file and treats the library as missing

### Requirement: Platform library must match the backend ABI version
The platform library SHALL report an ABI version. The backend SHALL compare it with the ABI version it
was built with and SHALL use the library only when both are equal.

#### Scenario: Matching ABI version
- **WHEN** the loaded platform library reports the same ABI version as the backend
- **THEN** the backend uses the providers of the library

#### Scenario: Different ABI version
- **WHEN** the loaded platform library reports a different ABI version from the backend
- **THEN** the backend does not use any provider of the library and treats the load as failed

### Requirement: Backend stops when the platform library is unusable
If the platform library is missing, cannot be loaded, does not export its ABI version entry point, or
reports a different ABI version, the backend SHALL write an error describing the cause to stderr, SHALL
NOT print the READY line, and SHALL exit with a non-zero exit code.

#### Scenario: Library missing
- **WHEN** the backend starts and the platform library is not in the executable's directory
- **THEN** stderr contains an error naming the expected library file
- **AND** stdout contains no READY line
- **AND** the process exits with a non-zero exit code

#### Scenario: ABI version entry point missing
- **WHEN** the platform library loads but does not export its ABI version entry point
- **THEN** stderr contains an error naming the missing entry point
- **AND** stdout contains no READY line
- **AND** the process exits with a non-zero exit code

#### Scenario: ABI version mismatch
- **WHEN** the platform library reports a different ABI version from the backend
- **THEN** stderr contains an error with both ABI versions
- **AND** stdout contains no READY line
- **AND** the process exits with a non-zero exit code

#### Scenario: Frontend sees a failed start
- **WHEN** the backend exits because the platform library cannot be used
- **THEN** the frontend window opens and shows its existing backend startup error

### Requirement: Unavailable provider disables only its own feature
When the platform library is usable but a provider cannot be obtained from it (its create entry point is
not exported, or creation fails), the backend SHALL write a warning naming the provider to stderr, SHALL
continue startup and print the READY line, and every endpoint that depends on that provider SHALL respond
with status 501 and a problem body.

#### Scenario: Provider entry point missing
- **WHEN** the platform library passes the ABI check but does not export the create entry point of a provider
- **THEN** stderr contains a warning naming that provider
- **AND** stdout contains the READY line
- **AND** endpoints that depend on that provider respond with status 501 and a problem body

#### Scenario: Provider creation fails
- **WHEN** the create entry point of a provider exists but returns no provider
- **THEN** stderr contains a warning naming that provider
- **AND** stdout contains the READY line
- **AND** endpoints that depend on that provider respond with status 501 and a problem body

#### Scenario: Other providers keep working
- **WHEN** one provider is unavailable and another provider of the same platform library is available
- **THEN** endpoints that depend on the available provider respond normally

### Requirement: Platform library stays loaded while the backend runs
The backend SHALL keep the platform library loaded until every object obtained from it has been
released, and SHALL unload it only during shutdown.

#### Scenario: Normal shutdown
- **WHEN** the backend stops because its stdin is closed
- **THEN** it releases every provider obtained from the platform library before unloading the library
- **AND** the process exits without a crash
