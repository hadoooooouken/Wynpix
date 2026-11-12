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

function Get-VsInstallPath {
    $vswhere = Join-Path ([Environment]::GetEnvironmentVariable("ProgramFiles(x86)")) `
        "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) {
        throw "vswhere.exe not found at $vswhere"
    }

    $vsInstallPath = & $vswhere -latest -property installationPath
    if ($LASTEXITCODE -ne 0 -or -not $vsInstallPath) {
        throw "vswhere could not locate a Visual Studio installation."
    }
    return $vsInstallPath
}

function Import-VcVars {
    $cl = Get-Command cl.exe -ErrorAction SilentlyContinue
    if ($cl -and $env:INCLUDE -and $env:LIB) {
        $env:PATH = ($env:PATH -split ';' | Where-Object { $_ -ne "" } | Select-Object -Unique) -join ';'
        return
    }

    $env:PATH = ($env:PATH -split ';' | Where-Object { $_ -ne "" } | Select-Object -Unique) -join ';'

    $vcvars = Join-Path (Get-VsInstallPath) "VC\Auxiliary\Build\vcvarsall.bat"
    if (-not (Test-Path -LiteralPath $vcvars -PathType Leaf)) {
        throw "Could not locate vcvarsall.bat at $vcvars"
    }

    $environment = cmd.exe /c "call `"$vcvars`" x64 && set"
    if ($LASTEXITCODE -ne 0) {
        throw "vcvarsall.bat x64 failed with exit code $LASTEXITCODE"
    }

    foreach ($line in $environment) {
        if ($line -match "^(.*?)=(.*)$") {
            $varName = $matches[1]
            $varVal  = $matches[2]
            if ($varName -ieq "PATH") {
                $varVal = ($varVal -split ';' | Where-Object { $_ -ne "" } | Select-Object -Unique) -join ';'
            }
            [Environment]::SetEnvironmentVariable($varName, $varVal)
        }
    }

    if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
        throw "vcvarsall.bat ran but cl.exe is still not on PATH."
    }
}

# The CMake bundled with Visual Studio rather than the first one on PATH:
# Visual Studio configures the same build trees with it, and switching a tree
# to another cmake.exe reruns its custom commands and links, whose command
# lines embed that path.
function Resolve-VsCMake {
    $cmake = Join-Path (Get-VsInstallPath) "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    if (-not (Test-Path -LiteralPath $cmake -PathType Leaf)) {
        throw "CMake bundled with Visual Studio not found at $cmake. Install the 'C++ CMake tools for Windows' component."
    }
    return $cmake
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
$cmake = Resolve-VsCMake

$timer = [System.Diagnostics.Stopwatch]::StartNew()
Push-Location $projectRoot
try {
    # The build step re-runs configure by itself; an explicit configure is
    # only needed for a new build tree.
    if (-not (Test-Path -LiteralPath (Join-Path $buildDir "CMakeCache.txt") -PathType Leaf)) {
        Invoke-External -Executable $cmake -Arguments @("--preset", $preset)
    }
    Invoke-External -Executable $cmake -Arguments @("--build", "--preset", $preset)
} finally {
    Pop-Location
}
$timer.Stop()

Write-Host ("Built {0} in {1:N0} s" -f $releaseDir, $timer.Elapsed.TotalSeconds) -ForegroundColor Green
