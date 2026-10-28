[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$projectRoot = Split-Path $PSScriptRoot -Parent
$installDir = Join-Path $projectRoot "INSTALL"
$cmakeListsPath = Join-Path $projectRoot "CMakeLists.txt"
$projectVersionMatch = [regex]::Match(
    (Get-Content -LiteralPath $cmakeListsPath -Raw),
    '(?s)\bproject\s*\(\s*wynpix\s+VERSION\s+(\d+(?:\.\d+){2,3})\b'
)
if (-not $projectVersionMatch.Success) {
    throw "Could not determine the Wynpix version from '$cmakeListsPath'."
}
$projectVersion = $projectVersionMatch.Groups[1].Value
$releaseVersion = $projectVersion
if ($projectVersion -match '^\d+\.\d+\.\d+\.0$') {
    $releaseVersion = $projectVersion -replace '\.0$', ''
}
$checkIsaScript = Join-Path $PSScriptRoot "check-isa.ps1"

$targets = @{
    AVX2 = @{
        Preset = "wynpix-x64-release"
        ReleaseDir = Join-Path $projectRoot "release"
        Archive = "wynpix-portable_v$releaseVersion.7z"
        Installer = "wynpix-setup_v$releaseVersion.exe"
    }
    SSE2 = @{
        Preset = "wynpix-x64-release-sse2"
        ReleaseDir = Join-Path $projectRoot "release-sse2"
        Archive = "wynpix-portable-sse2_v$releaseVersion.7z"
        Installer = "wynpix-setup-sse2_v$releaseVersion.exe"
    }
}

function Resolve-Executable {
    param(
        [string[]] $Names,
        [string[]] $FallbackPaths
    )

    foreach ($name in $Names) {
        $command = Get-Command $name -ErrorAction SilentlyContinue |
            Where-Object { $_.CommandType -eq "Application" } |
            Select-Object -First 1
        if ($command) {
            return $command.Source
        }
    }

    foreach ($path in $FallbackPaths) {
        if ($path -and (Test-Path -LiteralPath $path -PathType Leaf)) {
            return $path
        }
    }

    throw "Required executable not found: $($Names -join ', ')."
}

function Invoke-External {
    param(
        [string] $Executable,
        [string[]] $Arguments
    )

    $previousErrorActionPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = "Continue"
        & $Executable @Arguments 2>&1
        if ($LASTEXITCODE -ne 0) {
            throw "Command failed with exit code $LASTEXITCODE`: $Executable $($Arguments -join ' ')"
        }
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
    }
}

function Assert-ArchiveMatchesDirectory {
    param(
        [string] $ArchivePath,
        [string] $SourceDirectory
    )

    $verificationDirectory = Join-Path ([System.IO.Path]::GetTempPath()) `
        ("wynpix-release-verify-" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Path $verificationDirectory | Out-Null
    try {
        Invoke-External -Executable $sevenZip -Arguments @(
            "x", "-y", "-o$verificationDirectory", $ArchivePath
        )

        $sourceFiles = @(Get-ChildItem -LiteralPath $SourceDirectory -File -Recurse -Force)
        $archivedFiles = @(Get-ChildItem -LiteralPath $verificationDirectory -File -Recurse -Force)
        if ($sourceFiles.Count -ne $archivedFiles.Count) {
            throw "Archive '$ArchivePath' has $($archivedFiles.Count) files, but '$SourceDirectory' has $($sourceFiles.Count)."
        }

        $sourceRoot = (Get-Item -LiteralPath $SourceDirectory).FullName.TrimEnd('\') + '\'
        $archiveRoot = (Get-Item -LiteralPath $verificationDirectory).FullName.TrimEnd('\') + '\'
        $sourceByRelativePath = @{}
        foreach ($file in $sourceFiles) {
            $relativePath = $file.FullName.Substring($sourceRoot.Length)
            $sourceByRelativePath[$relativePath] = $file
        }

        foreach ($archivedFile in $archivedFiles) {
            $relativePath = $archivedFile.FullName.Substring($archiveRoot.Length)
            if (-not $sourceByRelativePath.ContainsKey($relativePath)) {
                throw "Archive '$ArchivePath' contains an unexpected file: $relativePath"
            }

            $sourceFile = $sourceByRelativePath[$relativePath]
            if ($sourceFile.Length -ne $archivedFile.Length) {
                throw "Archive '$ArchivePath' has the wrong size for '$relativePath'."
            }

            $sourceHash = (Get-FileHash -LiteralPath $sourceFile.FullName -Algorithm SHA256).Hash
            $archiveHash = (Get-FileHash -LiteralPath $archivedFile.FullName -Algorithm SHA256).Hash
            if ($sourceHash -ne $archiveHash) {
                throw "Archive '$ArchivePath' does not match '$SourceDirectory' at '$relativePath'."
            }
        }

        Write-Host "Verified archive contents against $SourceDirectory (SHA-256)." -ForegroundColor Green
    } finally {
        Remove-Item -LiteralPath $verificationDirectory -Recurse -Force
    }
}

function Clear-ReleaseDirectory {
    param([string] $Path)

    if (Test-Path -LiteralPath $Path) {
        $directory = Get-Item -LiteralPath $Path -Force
        if (-not $directory.PSIsContainer -or
            ($directory.Attributes -band [System.IO.FileAttributes]::ReparsePoint)) {
            throw "Refusing to clear a release path that is not a regular directory: $Path"
        }

        try {
            Get-ChildItem -LiteralPath $Path -Force |
                Remove-Item -Recurse -Force
        } catch {
            throw "Could not clear release directory '$Path'. A file may be in use. Close Wynpix, Explorer previews, or any Visual Studio/ISA scan and retry. Details: $($_.Exception.Message)"
        }
    } else {
        New-Item -ItemType Directory -Path $Path -Force | Out-Null
    }
}

function Assert-ReleaseDirectoriesUnlocked {
    param([string[]] $Paths)

    $lockedFiles = @()
    foreach ($path in $Paths) {
        if (-not (Test-Path -LiteralPath $path -PathType Container)) {
            continue
        }

        foreach ($file in Get-ChildItem -LiteralPath $path -File -Recurse -Force) {
            $stream = $null
            try {
                $stream = [System.IO.File]::Open(
                    $file.FullName,
                    [System.IO.FileMode]::Open,
                    [System.IO.FileAccess]::Read,
                    [System.IO.FileShare]::None
                )
            } catch [System.IO.IOException] {
                $lockedFiles += $file.FullName
            } finally {
                if ($stream) {
                    $stream.Dispose()
                }
            }
        }
    }

    if ($lockedFiles.Count -gt 0) {
        $details = ($lockedFiles | Select-Object -First 5) -join "`n  "
        if ($lockedFiles.Count -gt 5) {
            $details += "`n  ... and $($lockedFiles.Count - 5) more"
        }
        throw "Cannot clean release folders because files are in use. Nothing has been cleared. Close Wynpix, Explorer previews, or Visual Studio/ISA scans, then retry:`n  $details"
    }
}

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

$cmake = Resolve-Executable -Names @("cmake.exe", "cmake")
$sevenZipPaths = @()
$programFiles = [Environment]::GetFolderPath("ProgramFiles")
$programFilesX86 = [Environment]::GetEnvironmentVariable("ProgramFiles(x86)")
if ($programFiles) {
    $sevenZipPaths += Join-Path $programFiles "7-Zip\7z.exe"
}
if ($programFilesX86) {
    $sevenZipPaths += Join-Path $programFilesX86 "7-Zip\7z.exe"
}
$sevenZip = Resolve-Executable -Names @("7z.exe", "7zz.exe") -FallbackPaths $sevenZipPaths

$innoPaths = @()
foreach ($basePath in @($programFiles, $programFilesX86)) {
    if ($basePath -and (Test-Path -LiteralPath $basePath -PathType Container)) {
        $innoPaths += Get-ChildItem -LiteralPath $basePath -Directory -Filter "Inno Setup*" |
            ForEach-Object { Join-Path $_.FullName "ISCC.exe" }
    }
}

$uninstallKeys = @(
    "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*",
    "HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*",
    "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*"
)
$registeredInno = Get-ItemProperty -Path $uninstallKeys -ErrorAction SilentlyContinue |
    Where-Object { $_.DisplayName -like "Inno Setup*" -and $_.InstallLocation }
foreach ($installation in $registeredInno) {
    $innoPaths += Join-Path $installation.InstallLocation "ISCC.exe"
}
$iscc = Resolve-Executable -Names @("ISCC.exe", "iscc.exe") -FallbackPaths $innoPaths

if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    Write-Host "Loading the Visual Studio x64 compiler environment..." -ForegroundColor DarkGray
    Import-VcVars
}

$cmakeVersionOutput = & $cmake --version
if ($LASTEXITCODE -ne 0 -or
    ($cmakeVersionOutput -join "`n") -notmatch '(?m)^cmake version (\d+(?:\.\d+){1,2})') {
    throw "Could not determine the installed CMake version."
}
if ([version]$matches[1] -lt [version]"3.25.0") {
    throw "CMake 3.25 or newer is required; found $($matches[1])."
}

$isas = @("AVX2", "SSE2")
New-Item -ItemType Directory -Path $installDir -Force | Out-Null

Write-Host "This release run will build and package both AVX2 and SSE2." -ForegroundColor Cyan
Assert-ReleaseDirectoriesUnlocked -Paths @(
    $targets.AVX2.ReleaseDir,
    $targets.SSE2.ReleaseDir
)
foreach ($isaName in $isas) {
    $target = $targets[$isaName]
    $archivePath = Join-Path $installDir $target.Archive
    $installerPath = Join-Path $installDir $target.Installer

    Clear-ReleaseDirectory -Path $target.ReleaseDir
    foreach ($outputPath in @($archivePath, $installerPath)) {
        if (Test-Path -LiteralPath $outputPath) {
            Remove-Item -LiteralPath $outputPath -Force
        }
    }
}

Push-Location $projectRoot
try {
    foreach ($isaName in $isas) {
        $target = $targets[$isaName]
        $archivePath = Join-Path $installDir $target.Archive
        $installerPath = Join-Path $installDir $target.Installer
        Write-Host ""
        Write-Host "=== Building $isaName ($($target.Preset)) ===" -ForegroundColor Cyan

        Invoke-External -Executable $cmake -Arguments @("--fresh", "--preset", $target.Preset)
        Invoke-External -Executable $cmake -Arguments @("--build", "--preset", $target.Preset)

        $appPath = Join-Path $target.ReleaseDir "wynpix.exe"
        if (-not (Test-Path -LiteralPath $appPath -PathType Leaf)) {
            throw "Build did not produce the expected executable: $appPath"
        }

        & $checkIsaScript -Path $target.ReleaseDir -Isa $isaName
        if ($LASTEXITCODE -and $LASTEXITCODE -ne 0) {
            throw "ISA verification failed for $isaName."
        }

        Push-Location $target.ReleaseDir
        try {
            Invoke-External -Executable $sevenZip -Arguments @(
                "a", "-t7z", "-mx=9", "-y", $archivePath, ".\*", "-r"
            )
        } finally {
            Pop-Location
        }
        if (-not (Test-Path -LiteralPath $archivePath -PathType Leaf)) {
            throw "7-Zip did not create the expected archive: $archivePath"
        }
        Invoke-External -Executable $sevenZip -Arguments @("t", $archivePath)
        Assert-ArchiveMatchesDirectory -ArchivePath $archivePath -SourceDirectory $target.ReleaseDir

        Invoke-External -Executable $iscc -Arguments @(
            "/DIsa=$isaName",
            "/DMyAppVersion=$projectVersion",
            "/DReleaseVersion=$releaseVersion",
            "src\wynpix.iss"
        )

        $installerPath = Join-Path $installDir $target.Installer
        if (-not (Test-Path -LiteralPath $installerPath -PathType Leaf)) {
            throw "Inno Setup did not produce the expected installer: $installerPath"
        }
        Write-Host "Created $($target.Archive) and $($target.Installer)." -ForegroundColor Green
    }
} finally {
    Pop-Location
}

Write-Host ""
foreach ($isaName in $isas) {
    $target = $targets[$isaName]
    foreach ($outputName in @($target.Archive, $target.Installer)) {
        $outputPath = Join-Path $installDir $outputName
        if (-not (Test-Path -LiteralPath $outputPath -PathType Leaf)) {
            throw "Release is incomplete; expected artifact is missing: $outputPath"
        }
        $outputFile = Get-Item -LiteralPath $outputPath
        if ($outputFile.Length -le 0) {
            throw "Release artifact is empty: $outputPath"
        }
        $outputHash = (Get-FileHash -LiteralPath $outputPath -Algorithm SHA256).Hash
        Write-Host ("  {0} ({1:N0} bytes, SHA-256 {2})" -f $outputName, $outputFile.Length, $outputHash) -ForegroundColor Green
    }
}
Write-Host "Release packaging completed successfully." -ForegroundColor Green
