# Run the Electron frontend (installs npm dependencies on first run).
# Works from any current directory: paths are resolved from this script's location.
$ErrorActionPreference = 'Stop'

$root        = Split-Path -Parent $PSScriptRoot
$frontendDir = Join-Path $root 'frontend'

Push-Location $frontendDir
try {
    if (-not (Test-Path (Join-Path $frontendDir 'node_modules'))) {
        Write-Host '==== npm install ====' -ForegroundColor Cyan
        npm install
        if ($LASTEXITCODE -ne 0) { throw 'npm install failed' }
    }

    Write-Host '==== npm start ====' -ForegroundColor Cyan
    npm start
    exit $LASTEXITCODE
}
finally {
    Pop-Location
}
