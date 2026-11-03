# =============================================================================
# File   : backend/CMakeLists.txt
# Brief  : Build definition of the SystemLens C++ backend.
#          - systemlens_provider_api : header-only provider contract shared by host and plugin
#          - systemlens_core         : static library with all backend code except main()
#          - systemlens_platform     : platform plugin (shared library) of the target OS,
#                                      loaded at runtime by the backend (never linked)
#          - systemlens-backend      : the executable spawned by Electron
#          Third-party libraries are downloaded at configure time with FetchContent.
# =============================================================================
cmake_minimum_required(VERSION 3.24)

project(systemlens-backend VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SYSTEMLENS_ENABLE_SAMPLE_API "Register the /api/v1/sample/* endpoints" ON)

# -----------------------------------------------------------------------------
# Target platform: selects the host-side OS files and the plugin directory
# -----------------------------------------------------------------------------
if(WIN32)
    set(SYSTEMLENS_PLATFORM windows)
elseif(APPLE)
    set(SYSTEMLENS_PLATFORM macos)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(SYSTEMLENS_PLATFORM linux)
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME} (supported: Windows, Linux, macOS)")
endif()
message(STATUS "SystemLens platform: ${SYSTEMLENS_PLATFORM}")

# The dynamic library loader is the same POSIX API on Linux and macOS.
if(WIN32)
    set(SYSTEMLENS_LOADER_DIR windows)
else()
    set(SYSTEMLENS_LOADER_DIR posix)
endif()

# -----------------------------------------------------------------------------
# Third-party dependencies (pinned versions, downloaded into the build folder)
# -----------------------------------------------------------------------------
include(FetchContent)

# cpp-httplib: header-only HTTP server/client. Optional compression/TLS backends
# are disabled so the build does not depend on whatever happens to be installed.
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE    OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE  OFF CACHE BOOL "" FORCE)
FetchContent_Declare(httplib
    URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.18.3.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON)

# nlohmann/json: JSON serialization.
FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    DOWNLOAD_EXTRACT_TIMESTAMP ON)

FetchContent_MakeAvailable(httplib nlohmann_json)

# -----------------------------------------------------------------------------
# systemlens_provider_api: provider interfaces + plugin ABI (headers only)
# -----------------------------------------------------------------------------
add_library(systemlens_provider_api INTERFACE)
target_include_directories(systemlens_provider_api INTERFACE src/providers)

# -----------------------------------------------------------------------------
# systemlens_core: everything except main(), so other targets can link the same code
# -----------------------------------------------------------------------------
add_library(systemlens_core STATIC
    src/app/AppConfig.cpp
    src/app/Application.cpp
    src/api/ApiResponse.cpp
    src/api/HttpServer.cpp
    src/platform/PlatformPlugin.cpp
    src/platform/${SYSTEMLENS_LOADER_DIR}/DynamicLibrary.cpp
    src/platform/${SYSTEMLENS_PLATFORM}/ExecutablePath.cpp
)

if(SYSTEMLENS_ENABLE_SAMPLE_API)
    target_sources(systemlens_core PRIVATE
        src/api/sample/SampleHelloController.cpp
        src/services/sample/SampleHelloService.cpp
    )
endif()

target_include_directories(systemlens_core PUBLIC src)

target_link_libraries(systemlens_core PUBLIC
    systemlens_provider_api
    httplib::httplib
    nlohmann_json::nlohmann_json
    # dlopen/dlsym live in libdl on older glibc; empty on Windows and macOS.
    ${CMAKE_DL_LIBS}
)

target_compile_definitions(systemlens_core PUBLIC
    SYSTEMLENS_ENABLE_SAMPLE_API=$<BOOL:${SYSTEMLENS_ENABLE_SAMPLE_API}>
)
# The host never builds the library name from a prefix/suffix, so it always matches the built file.
target_compile_definitions(systemlens_core PRIVATE
    SYSTEMLENS_PLATFORM_LIBRARY_FILE="$<TARGET_FILE_NAME:systemlens_platform>"
)

if(WIN32)
    target_compile_definitions(systemlens_core PUBLIC
        # cpp-httplib requires Windows 10 APIs; WIN32_LEAN_AND_MEAN/NOMINMAX keep
        # <windows.h> (pulled in by httplib) from defining min/max macros and extra headers.
        _WIN32_WINNT=0x0A00
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
endif()

if(MSVC)
    # Source files are UTF-8.
    target_compile_options(systemlens_core PUBLIC /utf-8)
endif()

# -----------------------------------------------------------------------------
# systemlens_platform: platform plugin of the target OS
# Links only the provider contract (no httplib, server or services). Host and plugin must be built
# in this same tree and configuration: they share C++ types across the boundary (PluginAbi.h).
# -----------------------------------------------------------------------------
add_library(systemlens_platform SHARED
    plugins/${SYSTEMLENS_PLATFORM}/PluginEntry.cpp
)

if(SYSTEMLENS_ENABLE_SAMPLE_API)
    file(GLOB SYSTEMLENS_PLUGIN_HELLO_SOURCES CONFIGURE_DEPENDS
        plugins/${SYSTEMLENS_PLATFORM}/*HelloProvider.cpp)
    target_sources(systemlens_platform PRIVATE ${SYSTEMLENS_PLUGIN_HELLO_SOURCES})
endif()

target_link_libraries(systemlens_platform PRIVATE systemlens_provider_api)

target_compile_definitions(systemlens_platform PRIVATE
    SYSTEMLENS_PLUGIN_BUILD
    SYSTEMLENS_ENABLE_SAMPLE_API=$<BOOL:${SYSTEMLENS_ENABLE_SAMPLE_API}>
)

# Only the entry points marked SYSTEMLENS_PLUGIN_EXPORT are visible (matches the Windows default).
set_target_properties(systemlens_platform PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)

if(MSVC)
    target_compile_options(systemlens_platform PRIVATE /utf-8)
endif()

# -----------------------------------------------------------------------------
# Executable
# Defined in the same directory scope as systemlens_platform, so both land in the same output
# directory (backend/build/<Config>/ with Visual Studio, backend/build/ with Ninja).
# -----------------------------------------------------------------------------
add_executable(systemlens-backend src/main.cpp)
target_link_libraries(systemlens-backend PRIVATE systemlens_core)
# Runtime-only dependency: building the executable also builds the plugin, without linking it.
add_dependencies(systemlens-backend systemlens_platform)
