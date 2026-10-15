# =============================================================================
# File   : backend/tests/CMakeLists.txt
# Brief  : GoogleTest suites of the backend.
#          - api/         : API tests, black-box HTTP calls against a real HttpServer
#          - interaction/ : interaction tests, gmock-based checks that layers call each other correctly
#          Run with: ctest --test-dir backend/build -C Debug --output-on-failure
# =============================================================================
include(FetchContent)

# Use the same (dynamic) C runtime as the rest of the build; MSVC fails to link otherwise.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP ON)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_executable(systemlens-tests)

if(SYSTEMLENS_ENABLE_SAMPLE_API)
    target_sources(systemlens-tests PRIVATE
        api/SampleHelloApiTest.cpp
        interaction/SampleHelloControllerTest.cpp
    )
endif()

target_include_directories(systemlens-tests PRIVATE .)
target_link_libraries(systemlens-tests PRIVATE systemlens_core GTest::gmock_main)

gtest_discover_tests(systemlens-tests)
