# Shared helpers for check_utf8.ps1 / check_crlf.ps1 (dot-source this file).

$script:RepoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)

$script:BinaryExtensions = @(
    '.png', '.jpg', '.jpeg', '.gif', '.bmp', '.ico', '.icns', '.webp',
    '.exe', '.dll', '.pdb', '.obj', '.lib', '.exp', '.ilk', '.so', '.dylib', '.node',
    '.woff', '.woff2', '.ttf', '.otf', '.eot',
    '.zip', '.7z', '.gz', '.tar', '.rar',
    '.pdf', '.mp3', '.mp4', '.wav'
)

# Returns repo-relative paths (forward slashes) of the files to check.
function Get-TargetFiles {
    param([string[]] $Path, [switch] $Staged)

    if ($Staged) {
        return @(git -C $script:RepoRoot -c core.quotepath=off diff --cached --name-only --diff-filter=ACMR)
    }
    if ($Path) {
        return @($Path | ForEach-Object {
            $full = [System.IO.Path]::GetFullPath($_, (Get-Location).Path)
            [System.IO.Path]::GetRelativePath($script:RepoRoot, $full).Replace('\', '/')
        })
    }
    return @(git -C $script:RepoRoot -c core.quotepath=off ls-files -co --exclude-standard)
}

function Get-FullPath([string] $RelPath) {
    # Files on another drive stay absolute after GetRelativePath.
    if ([System.IO.Path]::IsPathRooted($RelPath)) { return $RelPath }
    return Join-Path $script:RepoRoot $RelPath
}

# Reads raw bytes from the working tree, or from the index when -Staged.
function Read-FileBytes {
    param([string] $RelPath, [switch] $Staged)

    if (-not $Staged) {
        return [System.IO.File]::ReadAllBytes((Get-FullPath $RelPath))
    }

    $psi = [System.Diagnostics.ProcessStartInfo]::new('git')
    $psi.ArgumentList.Add('-C'); $psi.ArgumentList.Add($script:RepoRoot)
    $psi.ArgumentList.Add('cat-file'); $psi.ArgumentList.Add('blob'); $psi.ArgumentList.Add(":$RelPath")
    $psi.RedirectStandardOutput = $true
    $psi.UseShellExecute = $false
    $proc = [System.Diagnostics.Process]::Start($psi)
    $ms = [System.IO.MemoryStream]::new()
    $proc.StandardOutput.BaseStream.CopyTo($ms)
    $proc.WaitForExit()
    return $ms.ToArray()
}

function Test-IsBinary {
    param([string] $RelPath, [byte[]] $Bytes)

    if ($script:BinaryExtensions -contains [System.IO.Path]::GetExtension($RelPath).ToLowerInvariant()) {
        return $true
    }
    $n = [Math]::Min($Bytes.Length, 8192)
    for ($i = 0; $i -lt $n; $i++) {
        if ($Bytes[$i] -eq 0) { return $true }
    }
    return $false
}
