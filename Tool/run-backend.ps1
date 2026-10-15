# Configure, build (Debug) and run the C++ backend.
# Works from any current directory: paths are resolved from this script's location.
# Without arguments the backend starts in dev mode (no auth) on port 8765, so it can be called with curl:
#   curl http://127.0.0.1:8765/api/v1/sample/hello
# Any arguments replace these defaults and are passed to the exe as-is.
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

$exeArgs = if ($args.Count -gt 0) { $args } else { @('--dev', '--port', '8765') }

Write-Host "==== Run $exe $exeArgs ====" -ForegroundColor Cyan
& $exe @exeArgs
exit $LASTEXITCODE
