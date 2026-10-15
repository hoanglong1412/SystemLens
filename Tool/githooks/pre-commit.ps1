# Called by Tool/githooks/pre-commit. If staged files are not UTF-8 (no BOM) + CRLF,
# show a Yes/No dialog listing them: Yes = continue the commit, No = abort it.
$ErrorActionPreference = 'Stop'

$tool   = Split-Path -Parent $PSScriptRoot
$issues = @(
    & (Join-Path $tool 'check_utf8.ps1') -Staged
    & (Join-Path $tool 'check_crlf.ps1') -Staged
) | Where-Object { $_ }

if (-not $issues) { exit 0 }

Write-Host 'Files with wrong format (expected UTF-8 without BOM + CRLF):' -ForegroundColor Yellow
$issues | ForEach-Object { Write-Host "  $_" }

Add-Type -AssemblyName System.Windows.Forms
$message = "Các file dưới đây đang sai định dạng (yêu cầu UTF-8 không BOM + CRLF):`n`n" +
           ($issues -join "`n") +
           "`n`nBạn có muốn tiếp tục commit không?"
$answer = [System.Windows.Forms.MessageBox]::Show(
    $message, 'SystemLens - pre-commit',
    [System.Windows.Forms.MessageBoxButtons]::YesNo,
    [System.Windows.Forms.MessageBoxIcon]::Warning,
    [System.Windows.Forms.MessageBoxDefaultButton]::Button2)

if ($answer -eq [System.Windows.Forms.DialogResult]::Yes) {
    Write-Host 'Commit continued by user despite format issues.' -ForegroundColor Yellow
    exit 0
}

Write-Host 'Commit aborted. Fix with:' -ForegroundColor Red
Write-Host '  Tool\check_utf8.ps1 -Fix; Tool\check_crlf.ps1 -Fix; git add -A'
exit 1
