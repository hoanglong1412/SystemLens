# Enable the repo's git hooks (Tool/githooks) for THIS repository only (local git config, not --global).
# Run once after cloning.
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
git -C $root config --local core.hooksPath Tool/githooks
if ($LASTEXITCODE -ne 0) { throw 'git config failed' }
Write-Host "core.hooksPath = $(git -C $root config --local core.hooksPath)"
