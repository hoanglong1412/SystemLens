# Check that text files use CRLF line endings only (no lone LF or lone CR).
#   check_crlf.ps1                      # all tracked + untracked (not ignored) files
#   check_crlf.ps1 -Path a.cpp b.js     # specific files
#   check_crlf.ps1 -Staged              # files in the git index (used by pre-commit)
#   check_crlf.ps1 -Fix                 # convert invalid files in place
# Exit code: 1 if invalid files remain, 0 otherwise.
[CmdletBinding()]
param(
    [string[]] $Path,
    [switch] $Staged,
    [switch] $Fix
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'lib\FileFormat.ps1')

if ($Fix -and $Staged) { throw '-Fix cannot be used with -Staged (fix the working tree file, then git add it).' }

$invalid = 0

foreach ($rel in Get-TargetFiles -Path $Path -Staged:$Staged) {
    if (-not $Staged -and -not (Test-Path -LiteralPath (Get-FullPath $rel) -PathType Leaf)) { continue }

    $bytes = Read-FileBytes -RelPath $rel -Staged:$Staged
    if ($bytes.Length -eq 0 -or (Test-IsBinary -RelPath $rel -Bytes $bytes)) { continue }

    # Count lone LF (not preceded by CR) and lone CR (not followed by LF).
    $loneLf = 0; $loneCr = 0
    for ($i = 0; $i -lt $bytes.Length; $i++) {
        if ($bytes[$i] -eq 10 -and ($i -eq 0 -or $bytes[$i - 1] -ne 13)) { $loneLf++ }
        elseif ($bytes[$i] -eq 13 -and ($i + 1 -ge $bytes.Length -or $bytes[$i + 1] -ne 10)) { $loneCr++ }
    }
    if ($loneLf -eq 0 -and $loneCr -eq 0) { continue }

    $reason = "$loneLf lone LF, $loneCr lone CR"

    if (-not $Fix) {
        Write-Output "INVALID  $rel  ($reason)"
        $invalid++
        continue
    }

    $out = [System.Collections.Generic.List[byte]]::new($bytes.Length + $loneLf + $loneCr)
    for ($i = 0; $i -lt $bytes.Length; $i++) {
        $b = $bytes[$i]
        if ($b -eq 13) {
            $out.Add(13); $out.Add(10)
            if ($i + 1 -lt $bytes.Length -and $bytes[$i + 1] -eq 10) { $i++ }
        }
        elseif ($b -eq 10) { $out.Add(13); $out.Add(10) }
        else { $out.Add($b) }
    }
    [System.IO.File]::WriteAllBytes((Get-FullPath $rel), $out.ToArray())
    Write-Output "FIXED    $rel  ($reason)"
}

exit ([int]($invalid -gt 0))
