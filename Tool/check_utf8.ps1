# Check that text files are UTF-8 without BOM.
#   check_utf8.ps1                      # all tracked + untracked (not ignored) files
#   check_utf8.ps1 -Path a.cpp b.js     # specific files
#   check_utf8.ps1 -Staged              # files in the git index (used by pre-commit)
#   check_utf8.ps1 -Fix                 # convert invalid files in place
# Exit code: 1 if invalid files remain, 0 otherwise.
[CmdletBinding()]
param(
    [string[]] $Path,
    [switch] $Staged,
    [switch] $Fix,
    [int] $FallbackCodePage = 1252
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\FileFormat.ps1')

if ($Fix -and $Staged) { throw '-Fix cannot be used with -Staged (fix the working tree file, then git add it).' }

$strictUtf8 = [System.Text.UTF8Encoding]::new($false, $true)
$invalid = 0

foreach ($rel in Get-TargetFiles -Path $Path -Staged:$Staged) {
    if (-not $Staged -and -not (Test-Path -LiteralPath (Get-FullPath $rel) -PathType Leaf)) { continue }

    $bytes = Read-FileBytes -RelPath $rel -Staged:$Staged
    if ($bytes.Length -eq 0 -or (Test-IsBinary -RelPath $rel -Bytes $bytes)) { continue }

    $hasBom = $bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF
    $isUtf8 = $true
    try { [void]$strictUtf8.GetString($bytes) } catch { $isUtf8 = $false }

    if ($isUtf8 -and -not $hasBom) { continue }

    $reason = if (-not $isUtf8) { 'not valid UTF-8' } else { 'has UTF-8 BOM' }

    if (-not $Fix) {
        Write-Output "INVALID  $rel  ($reason)"
        $invalid++
        continue
    }

    if ($isUtf8) {
        $out = $bytes[3..($bytes.Length - 1)]
        if ($bytes.Length -eq 3) { $out = [byte[]]@() }
    }
    else {
        $text = [System.Text.Encoding]::GetEncoding($FallbackCodePage).GetString($bytes)
        $out = [System.Text.UTF8Encoding]::new($false).GetBytes($text)
        Write-Warning "$rel was decoded as code page $FallbackCodePage - review the result."
    }
    [System.IO.File]::WriteAllBytes((Get-FullPath $rel), [byte[]]$out)
    Write-Output "FIXED    $rel  ($reason)"
}

exit ([int]($invalid -gt 0))
