<#
.SYNOPSIS
    Quick Release-like build of wynpix into release-dev.

.DESCRIPTION
    Builds the wynpix-x64-dev preset: the Release optimization baseline
    without LTCG, hardening and the test suites, deployed to release-dev. The build is
    incremental (nothing is cleared or packaged), so after an edit only the
    changed sources are compiled and wynpix.exe is relinked.

    Needs the non-LTCG exiv2 from build_exiv2-dev.ps1. The release folders,
    presets and scripts are not involved.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path $PSScriptRoot -Parent
$preset = "wynpix-x64-dev"
$releaseDir = Join-Path $projectRoot "release-dev"
$buildDir = Join-Path $projectRoot "out\build\$preset"
$exiv2Config = Join-Path $projectRoot "formats\exiv2\install-dev\lib\cmake\exiv2\exiv2Config.cmake"

function Import-VcVars {
    $vswhere = Join-Path ([Environment]::GetEnvironmentVariable("ProgramFiles(x86)")) `
        "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) {
        throw "vswhere.exe not found at $vswhere"
    }

    $vsInstallPath = & $vswhere -latest -property installationPath
    if ($LASTEXITCODE -ne 0 -or -not $vsInstallPath) {
        throw "vswhere could not locate a Visual Studio installation."
    }

    $vcvars = Join-Path $vsInstallPath "VC\Auxiliary\Build\vcvarsall.bat"
    if (-not (Test-Path -LiteralPath $vcvars -PathType Leaf)) {
        throw "Could not locate vcvarsall.bat at $vcvars"
    }

    $environment = cmd.exe /c "call `"$vcvars`" x64 && set"
    if ($LASTEXITCODE -ne 0) {
        throw "vcvarsall.bat x64 failed with exit code $LASTEXITCODE"
    }

    foreach ($line in $environment) {
        if ($line -match "^(.*?)=(.*)$") {
            [Environment]::SetEnvironmentVariable($matches[1], $matches[2])
        }
    }

    if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
        throw "vcvarsall.bat ran but cl.exe is still not on PATH."
    }
}

function Invoke-External {
    param(
        [string] $Executable,
        [string[]] $Arguments
    )

    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE`: $Executable $($Arguments -join ' ')"
    }
}

if (-not (Test-Path -LiteralPath $exiv2Config -PathType Leaf)) {
    throw "Non-LTCG exiv2 not found at '$exiv2Config'. Run build_scripts\build_exiv2-dev.ps1 first."
}

# A running release-dev\wynpix.exe would fail the link only at the end.
$exe = Join-Path $releaseDir "wynpix.exe"
if (Test-Path -LiteralPath $exe -PathType Leaf) {
    try {
        [System.IO.File]::Open($exe, "Open", "ReadWrite", "None").Dispose()
    } catch [System.IO.IOException] {
        throw "Close $exe before building."
    }
}

if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    Write-Host "Loading the Visual Studio x64 compiler environment..." -ForegroundColor DarkGray
    Import-VcVars
}

$timer = [System.Diagnostics.Stopwatch]::StartNew()
Push-Location $projectRoot
try {
    # The build step re-runs configure by itself; an explicit configure is
    # only needed for a new build tree.
    if (-not (Test-Path -LiteralPath (Join-Path $buildDir "CMakeCache.txt") -PathType Leaf)) {
        Invoke-External -Executable "cmake" -Arguments @("--preset", $preset)
    }
    Invoke-External -Executable "cmake" -Arguments @("--build", "--preset", $preset)
} finally {
    Pop-Location
}
$timer.Stop()

Write-Host ("Built {0} in {1:N0} s" -f $releaseDir, $timer.Elapsed.TotalSeconds) -ForegroundColor Green
