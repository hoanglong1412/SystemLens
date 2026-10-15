# Configure, build (Debug) and run the backend test suites (API tests + interaction tests) with ctest.
# Works from any current directory: paths are resolved from this script's location.
# Extra arguments are passed to ctest, e.g.:  Tool\run-backend-tests.ps1 -R SampleHello
$ErrorActionPreference = 'Stop'

$root     = Split-Path -Parent $PSScriptRoot
$srcDir   = Join-Path $root 'backend'
$buildDir = Join-Path $srcDir 'build'

Write-Host '==== Configure (Visual Studio 16 2019, x64) ====' -ForegroundColor Cyan
cmake -S $srcDir -B $buildDir -G 'Visual Studio 16 2019' -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }

Write-Host '==== Build (Debug) ====' -ForegroundColor Cyan
cmake --build $buildDir --config Debug
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }

Write-Host '==== Test ====' -ForegroundColor Cyan
ctest --test-dir $buildDir -C Debug --output-on-failure @args
exit $LASTEXITCODE
