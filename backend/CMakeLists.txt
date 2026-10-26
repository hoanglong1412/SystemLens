# =============================================================================
# File   : backend/CMakeLists.txt
# Brief  : Build definition of the SystemLens C++ backend.
#          - systemlens_core     : static library with all backend code except main()
#          - systemlens-backend  : the executable spawned by Electron
#          Third-party libraries are downloaded at configure time with FetchContent.
# =============================================================================
cmake_minimum_required(VERSION 3.24)

project(systemlens-backend VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SYSTEMLENS_ENABLE_SAMPLE_API "Register the /api/v1/sample/* endpoints" ON)

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
# systemlens_core: everything except main(), so other targets can link the same code
# -----------------------------------------------------------------------------
add_library(systemlens_core STATIC
    src/app/AppConfig.cpp
    src/app/Application.cpp
    src/api/ApiResponse.cpp
    src/api/HttpServer.cpp
)

if(SYSTEMLENS_ENABLE_SAMPLE_API)
    target_sources(systemlens_core PRIVATE
        src/api/sample/SampleHelloController.cpp
        src/services/sample/SampleHelloService.cpp
    )
endif()

target_include_directories(systemlens_core PUBLIC src)

target_link_libraries(systemlens_core PUBLIC
    httplib::httplib
    nlohmann_json::nlohmann_json
)

target_compile_definitions(systemlens_core PUBLIC
    SYSTEMLENS_ENABLE_SAMPLE_API=$<BOOL:${SYSTEMLENS_ENABLE_SAMPLE_API}>
    # cpp-httplib requires Windows 10 APIs; WIN32_LEAN_AND_MEAN/NOMINMAX keep
    # <windows.h> (pulled in by httplib) from defining min/max macros and extra headers.
    _WIN32_WINNT=0x0A00
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

if(MSVC)
    # Source files are UTF-8.
    target_compile_options(systemlens_core PUBLIC /utf-8)
endif()

# -----------------------------------------------------------------------------
# Executable
# -----------------------------------------------------------------------------
add_executable(systemlens-backend src/main.cpp)
target_link_libraries(systemlens-backend PRIVATE systemlens_core)
