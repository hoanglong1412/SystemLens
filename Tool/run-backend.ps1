# Configure, build (Debug) and run the C++ backend.
# Works from any current directory: paths are resolved from this script's location.
$ErrorActionPreference = 'Stop'

$root     = Split-Path -Parent $PSScriptRoot
$srcDir   = Join-Path $root 'backend'
$buildDir = Join-Path $srcDir 'build'
$exe      = Join-Path $buildDir 'Debug\systemlens-backend.exe'

Write-Host '==== Configure (Visual Studio 16 2019, x64) ====' -ForegroundColor Cyan
cmake -S $srcDir -B $buildDir -G 'Visual Studio 16 2019' -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }

Write-Host '==== Build (Debug) ====' -ForegroundColor Cyan
cmake --build $buildDir --config Debug
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }

Write-Host "==== Run $exe ====" -ForegroundColor Cyan
& $exe @args
exit $LASTEXITCODE
