# =============================================================================
# File   : backend/tests/CMakeLists.txt
# Brief  : GoogleTest target systemlens_tests (enabled with -DSYSTEMLENS_BUILD_TESTS=ON).
#          - OS-independent tests link systemlens_core and use fakes (tests/fakes/).
#          - Platform tests compile the plugin sources of the build OS directly into the test
#            executable (the plugin hides its symbols, so they cannot be linked from the .so/.dll).
#          - PlatformPluginTest loads the real built plugin by absolute path.
# Run    : ctest --test-dir backend/build --output-on-failure   (or run systemlens_tests directly)
# =============================================================================
include(FetchContent)

# Same CRT as the code under test on MSVC (/MD); without this gtest defaults to the static CRT.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON)
FetchContent_MakeAvailable(googletest)

add_executable(systemlens_tests
    CpuMetricsCalculatorTest.cpp
    CpuMonitorServiceTest.cpp
    CpuControllerTest.cpp
    SystemInfoControllerTest.cpp
    PlatformPluginTest.cpp
)

target_include_directories(systemlens_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(systemlens_tests PRIVATE systemlens_core GTest::gtest_main)
target_compile_definitions(systemlens_tests PRIVATE
    SYSTEMLENS_TEST_PLUGIN_PATH="$<TARGET_FILE:systemlens_platform>"
)
add_dependencies(systemlens_tests systemlens_platform)

if(SYSTEMLENS_PLATFORM STREQUAL "linux")
    target_sources(systemlens_tests PRIVATE
        linux/LinuxProcFsTest.cpp
        linux/LinuxCpuProviderTest.cpp
        ${PROJECT_SOURCE_DIR}/plugins/linux/LinuxProcFs.cpp
        ${PROJECT_SOURCE_DIR}/plugins/linux/LinuxCpuProvider.cpp
        ${PROJECT_SOURCE_DIR}/plugins/linux/LinuxSystemInfoProvider.cpp
    )
    # Plugin sources include provider headers relative to src/providers, like in the plugin build.
    target_include_directories(systemlens_tests PRIVATE
        ${PROJECT_SOURCE_DIR}/plugins/linux
        ${PROJECT_SOURCE_DIR}/src/providers
    )
endif()

if(MSVC)
    target_compile_options(systemlens_tests PRIVATE /utf-8)
endif()

include(GoogleTest)
# PRE_TEST: discover tests when ctest runs, so a cross-compiled build does not need to run the binary.
gtest_discover_tests(systemlens_tests DISCOVERY_MODE PRE_TEST)
