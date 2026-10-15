# Claude Code PostToolUse hook (Write|Edit): force UTF-8 (no BOM) + CRLF on the edited file.
# Reads the hook JSON from stdin. Only touches files inside this repo that git does not ignore.
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$payload  = [Console]::In.ReadToEnd() | ConvertFrom-Json
$file     = $payload.tool_input.file_path
if (-not $file -or -not (Test-Path -LiteralPath $file -PathType Leaf)) { exit 0 }

$full = [System.IO.Path]::GetFullPath($file)
if (-not $full.StartsWith($repoRoot + [System.IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { exit 0 }

git -C $repoRoot check-ignore -q -- $full
if ($LASTEXITCODE -eq 0) { exit 0 }

$tool = Join-Path $repoRoot 'Tool'
$out  = @(
    & (Join-Path $tool 'check_utf8.ps1') -Path $full -Fix
    & (Join-Path $tool 'check_crlf.ps1') -Path $full -Fix
) | Where-Object { $_ }

if ($out) {
    @{
        hookSpecificOutput = @{
            hookEventName     = 'PostToolUse'
            additionalContext = "fix-format hook converted the file:`n" + ($out -join "`n")
        }
    } | ConvertTo-Json -Compress -Depth 4
}
exit 0
