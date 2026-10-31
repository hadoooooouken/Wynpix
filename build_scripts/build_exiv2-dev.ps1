<#
.SYNOPSIS
    Build exiv2 without LTCG for the wynpix-x64-dev preset.

.DESCRIPTION
    Same static, EXIF-only exiv2 as build_exiv2.ps1, with the ISA and Release
    baseline of the shared MSVC policy (msvc-release-policy.json) but without
    its hardening flags and without IPO: the Release objects are compiled
    without /GL. exiv2 is the only prebuilt /GL library linked into
    wynpix.exe, and with it the linker restarts every link of the dev build
    with /LTCG to generate its code.

    Only Release and AVX2 are built (the dev preset is AVX2 Release), into
    the build_msvc-dev / install-dev folders next to the release builds of
    build_exiv2.ps1, which are left untouched.

    Reuses the zlib-ng install already produced by build_qtiff_jpeg.ps1
    (formats/zlib-ng/install) -- it is not rebuilt here.

.PARAMETER FullClean
    Remove the entire exiv2 dev build folder before building (otherwise only
    CMakeCache.txt is cleared).

.EXAMPLE
    .\build_exiv2-dev.ps1
    .\build_exiv2-dev.ps1 -FullClean
#>
param(
    [switch] $FullClean
)

$ErrorActionPreference = "Stop"

# ---------------------------------------------------------------------------
# Common compiler / linker policy (single source: msvc-release-policy.json)
# ---------------------------------------------------------------------------
. (Join-Path $PSScriptRoot "MsvcPolicy.ps1")

$Isa = "AVX2"

# The ISA and Release baseline of the shared policy only: no hardening and no
# IPO (/GL), like the wynpix-x64-dev build that links it.
function Get-DevPolicyArgs {
    $policy = Get-MsvcPolicy
    $arch = (Get-MsvcIsaPolicy $Isa).arch
    return @(
        "-DCMAKE_C_FLAGS=$MSVC_CMAKE_DEFAULT_C_FLAGS $arch",
        "-DCMAKE_CXX_FLAGS=$MSVC_CMAKE_DEFAULT_CXX_FLAGS $arch",
        "-DCMAKE_C_FLAGS_RELEASE=$($policy.release_baseline)",
        "-DCMAKE_CXX_FLAGS_RELEASE=$($policy.release_baseline)",
        "-DCMAKE_STATIC_LINKER_FLAGS_RELEASE="
    )
}

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
$ROOT        = (Resolve-Path "$PSScriptRoot\..").Path
$FORMATS_DIR = Join-Path $ROOT "formats"

$ZLIB_INSTALL    = Join-Path $FORMATS_DIR "zlib-ng\$(Get-IsaFolderName -BaseName "install" -Isa $Isa)"
$EXIV2_SRC_DIR   = Join-Path $FORMATS_DIR "exiv2"
$EXIV2_BUILD_DIR = Join-Path $EXIV2_SRC_DIR "build_msvc-dev"
$EXIV2_INSTALL   = Join-Path $EXIV2_SRC_DIR "install-dev"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

function Write-Header($name) {
    Write-Host ""
    Write-Host ("=" * 60) -ForegroundColor DarkCyan
    Write-Host "  $name" -ForegroundColor Cyan
    Write-Host ("=" * 60) -ForegroundColor DarkCyan
}

function Write-OK($msg)   { Write-Host "  [OK]  $msg" -ForegroundColor Green  }
function Write-Info($msg) { Write-Host "  [ ]   $msg" -ForegroundColor Gray   }

function Clear-BuildDir($buildDir) {
    if (Test-Path $buildDir) {
        if ($FullClean) {
            Remove-Item $buildDir -Recurse -Force -ErrorAction SilentlyContinue
            Write-Info "Full clean: removed $buildDir"
        } else {
            $cache = Join-Path $buildDir "CMakeCache.txt"
            if (Test-Path $cache) {
                Remove-Item $cache -Force
                Write-Info "Cleared CMakeCache.txt"
            }
        }
    } else {
        Write-Info "No cache found -- fresh build"
    }
}

function Invoke-CMake {
    param([string[]]$CmakeArgs)
    Write-Info "cmake $($CmakeArgs -join ' ')"
    & cmake @CmakeArgs
    if ($LASTEXITCODE -ne 0) {
        throw "CMake failed (exit code $LASTEXITCODE)"
    }
}

# ---------------------------------------------------------------------------
# Preconditions
# ---------------------------------------------------------------------------

$zlibLib = Join-Path $ZLIB_INSTALL "lib\zlib.lib"
if (-not (Test-Path $zlibLib)) {
    throw "zlib-ng import library not found at: $zlibLib -- run build_qtiff_jpeg.ps1 first"
}

if (-not (Test-Path (Join-Path $EXIV2_SRC_DIR "CMakeLists.txt"))) {
    throw "exiv2 source not found at: $EXIV2_SRC_DIR -- run setup-deps.ps1"
}

# ---------------------------------------------------------------------------
# Step 1: exiv2 (static, no LTCG)
# ---------------------------------------------------------------------------
function Build-Exiv2 {
    Clear-BuildDir $EXIV2_BUILD_DIR

    Write-Info "Configuring exiv2 (static, EXIF-only, zlib-ng, BMFF, no LTCG, no hardening)..."
    # Feature set kept identical to build_exiv2.ps1.
    $cmakeArgs = @(
        "-S", $EXIV2_SRC_DIR,
        "-B", $EXIV2_BUILD_DIR,
        "-A", "x64",
        "-DCMAKE_INSTALL_PREFIX=$EXIV2_INSTALL",
        "-DCMAKE_PREFIX_PATH=$ZLIB_INSTALL",
        "-DCMAKE_POLICY_DEFAULT_CMP0091=NEW",
        # exiv2 links Iconv when any is found; a stray one on PATH must not leak in.
        "-DCMAKE_DISABLE_FIND_PACKAGE_Iconv=ON",
        "-DBUILD_SHARED_LIBS=OFF",
        "-DEXIV2_ENABLE_DYNAMIC_RUNTIME=ON",
        "-DEXIV2_ENABLE_XMP=OFF",
        "-DEXIV2_ENABLE_EXTERNAL_XMP=OFF",
        "-DEXIV2_ENABLE_PNG=ON",
        "-DEXIV2_ENABLE_BMFF=ON",
        "-DEXIV2_ENABLE_BROTLI=OFF",
        "-DEXIV2_ENABLE_VIDEO=OFF",
        "-DEXIV2_ENABLE_WEBREADY=OFF",
        "-DEXIV2_ENABLE_CURL=OFF",
        "-DEXIV2_ENABLE_NLS=OFF",
        "-DEXIV2_ENABLE_INIH=OFF",
        "-DEXIV2_ENABLE_LENSDATA=OFF",
        "-DEXIV2_BUILD_EXIV2_COMMAND=OFF",
        "-DEXIV2_BUILD_SAMPLES=OFF",
        "-DEXIV2_BUILD_UNIT_TESTS=OFF",
        "-DEXIV2_BUILD_FUZZ_TESTS=OFF",
        "-DEXIV2_BUILD_DOC=OFF"
    ) + (Get-DevPolicyArgs)
    Invoke-CMake $cmakeArgs

    Write-Info "Building exiv2 (Release)..."
    Invoke-CMake @("--build", $EXIV2_BUILD_DIR, "--config", "Release", "--parallel")

    Write-Info "Installing exiv2 (Release)..."
    Invoke-CMake @("--install", $EXIV2_BUILD_DIR, "--config", "Release")

    Write-OK "exiv2 (Release, no LTCG) installed to $EXIV2_INSTALL"
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

$sw = [System.Diagnostics.Stopwatch]::StartNew()

Write-Host ""
Write-Host "  exiv2 (static, no LTCG) dev build" -ForegroundColor White
Write-Host "  Root    : $ROOT"                   -ForegroundColor DarkGray
Write-Host "  zlib    : $ZLIB_INSTALL"           -ForegroundColor DarkGray
Write-Host "  Install : $EXIV2_INSTALL"          -ForegroundColor DarkGray
Write-Host ""

Write-Header "exiv2 (dev)"
try {
    Build-Exiv2
} catch {
    $sw.Stop()
    Write-Host "  [FAIL]  $_" -ForegroundColor Red
    exit 1
}

$sw.Stop()
$elapsed = "{0:mm\:ss}" -f $sw.Elapsed

Write-Host ""
Write-Host ("=" * 60) -ForegroundColor DarkCyan
Write-Host "  Done  ($elapsed)" -ForegroundColor Cyan
Write-Host ("=" * 60) -ForegroundColor DarkCyan
Write-Host "  Next: build_scripts\build-dev.ps1"
Write-Host ""
