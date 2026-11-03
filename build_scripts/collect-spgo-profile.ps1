<#
.SYNOPSIS
    Collect a hardware performance sample profile and convert it into an MSVC
    Sample Profile Database (.spd) for SPGO-optimized rebuilds.

.DESCRIPTION
    This script automates the SPGO profiling workflow:
      1. Verifies that the placeholder .spd file exists (generated in Step 1 build).
      2. Starts an xperf trace with hardware branch/instruction counter sampling
         (LBR, PMC, or OS Timer).
      3. Launches the target executable and waits for it to exit or a timeout.
      4. Stops the trace and converts the ETL -> SPT -> SPD pipeline.

    Requirements:
      - Windows Performance Toolkit (xperf.exe) from the Windows ADK.
      - MSVC toolset 19.51+ (ships SPTAggregate.exe and SPDConvert.exe).
      - Administrator privileges (required for live xperf kernel tracing; optional with -ExistingEtl).

.PARAMETER ExePath
    Path to the executable to profile (e.g. release\wynpix.exe). Must have been built
    with the SPGO preset (cmake --preset wynpix-x64-release-spgo).

.PARAMETER Mode
    Profiling data collection mode:
      - 'Auto' (default): Checks hardware capability via xperf. Uses LBR if
        supported (AMD Zen 4+ or Intel Haswell+), falls back to PMC IP mode,
        or falls back to OS Timer.
      - 'LBR': Hardware Last Branch Record tracing. Highest accuracy for branch
        prediction and basic block optimization. Supported on AMD Zen 4+
        (Ryzen 7000/8000/9000) and Intel Haswell+.
      - 'PMC': Hardware Performance Monitoring Counter IP sampling (e.g.
        TotalIssues on AMD, InstructionRetired on Intel).
      - 'Timer': OS kernel timer sampling with call stacks (fallback mode for VMs
        or environments without PMU access).

.PARAMETER SampleInterval
    Sampling interval (number of events per interrupt).
    Default for LBR/PMC: 65536. Default for Timer: 10000 (1ms).

.PARAMETER DurationSeconds
    Maximum profiling duration in seconds. The trace stops early when the
    target process exits. Default: 120.

.PARAMETER OutputSpd
    Optional explicit path for the .spd file. When omitted, defaults to
    <ExePath>.spd next to the executable.

.PARAMETER Binaries
    Optional list of binary names to include in SPTAggregate filtering.
    Defaults to the target executable and any sibling project DLLs
    (e.g. wynpix.exe, upscayl-ncnn.dll, wynpixshellex.dll).

.PARAMETER ExeArgs
    Optional arguments forwarded to the target executable.

.PARAMETER SkipLaunch
    When set, the script does not launch the executable and only records an
    xperf trace for the specified duration. Use when the application is
    already running or when profiling a specific scenario by hand.

.PARAMETER ExistingEtl
    Path to an already-captured ETL trace file to convert directly, skipping
    new trace collection.

.PARAMETER CleanEtl
    When set, deletes the ETL trace file after successful SPD conversion.

.EXAMPLE
    # Profile wynpix with automatic hardware LBR detection (AMD Zen 4 / Intel Haswell+)
    .\collect-spgo-profile.ps1 -ExePath ..\release\wynpix.exe -DurationSeconds 120

    # Force explicit LBR mode with custom branch sampling interval
    .\collect-spgo-profile.ps1 -ExePath ..\release\wynpix.exe -Mode LBR -SampleInterval 65536

    # Profile with a specific image opened
    .\collect-spgo-profile.ps1 -ExePath ..\release\wynpix.exe -ExeArgs "C:\Photos\test.jpg"

    # Convert an already-recorded ETL trace without restarting trace
    .\collect-spgo-profile.ps1 -ExePath ..\release\wynpix.exe -ExistingEtl ..\release\wynpix.etl
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [string] $ExePath,

    [ValidateSet('Auto', 'LBR', 'PMC', 'Timer')]
    [string] $Mode = 'Auto',

    [int] $SampleInterval = 0,

    [int] $DurationSeconds = 120,

    [string] $OutputSpd,

    [string[]] $Binaries = @(),

    [string] $ExeArgs,

    [switch] $SkipLaunch,

    [string] $ExistingEtl,

    [switch] $CleanEtl
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# ---------------------------------------------------------------------------
# Check administrator privileges
# ---------------------------------------------------------------------------
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin -and -not $ExistingEtl) {
    Write-Host "ERROR: Administrator privileges are required for xperf kernel tracing." -ForegroundColor Red
    Write-Host "Please run this script from a PowerShell session launched with 'Run as administrator'." -ForegroundColor Yellow
    Write-Host ""
    throw "Script must be run as Administrator (or specify -ExistingEtl to convert an existing trace)."
}

# ---------------------------------------------------------------------------
# Resolve paths
# ---------------------------------------------------------------------------
if (-not (Test-Path -LiteralPath $ExePath)) {
    throw "Target executable not found: $ExePath"
}
$ExePath  = (Resolve-Path $ExePath).Path
$exeDir   = Split-Path $ExePath -Parent
$exeName  = Split-Path $ExePath -Leaf
$baseName = [System.IO.Path]::GetFileNameWithoutExtension($ExePath)

if (-not $OutputSpd) {
    # MSVC creates <target_name>.spd (e.g. wynpix.spd)
    $candidate1 = Join-Path $exeDir "$baseName.spd"
    $candidate2 = "$ExePath.spd"
    if (Test-Path -LiteralPath $candidate1) {
        $OutputSpd = $candidate1
    } elseif (Test-Path -LiteralPath $candidate2) {
        $OutputSpd = $candidate2
    } else {
        $OutputSpd = $candidate1
    }
}

$etlPath = Join-Path $exeDir "$baseName.etl"
$sptPath = Join-Path $exeDir "$baseName.spt"

# ---------------------------------------------------------------------------
# Check prerequisite: SPD placeholder must exist from Step 1 build
# ---------------------------------------------------------------------------
if (-not (Test-Path -LiteralPath $OutputSpd)) {
    Write-Host "ERROR: Sample Profile Database placeholder not found:" -ForegroundColor Red
    Write-Host "  $OutputSpd" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "MSVC SPDConvert requires an existing .spd file generated by the linker." -ForegroundColor White
    Write-Host "Please compile the application with SPGO enabled first (Step 1):" -ForegroundColor White
    Write-Host "  cmake --preset wynpix-x64-release-spgo" -ForegroundColor Green
    Write-Host "  cmake --build --preset wynpix-x64-release-spgo" -ForegroundColor Green
    Write-Host ""
    throw "Missing SPD file. Build with wynpix-x64-release-spgo first."
}

# ---------------------------------------------------------------------------
# Locate tools
# ---------------------------------------------------------------------------
function Find-Tool {
    param([string] $Name)

    $tool = Get-Command $Name -ErrorAction SilentlyContinue
    if ($tool) { return $tool.Source }

    # Search MSVC toolset directories via vswhere
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -property installationPath 2>$null
        if ($vsPath) {
            $candidates = Get-ChildItem -Path $vsPath -Recurse -Filter $Name -ErrorAction SilentlyContinue
            if ($candidates) { return $candidates[0].FullName }
        }
    }

    # Search Windows Kits
    $kitsRoot = "${env:ProgramFiles(x86)}\Windows Kits\10\Windows Performance Toolkit"
    if (Test-Path $kitsRoot) {
        $candidate = Join-Path $kitsRoot $Name
        if (Test-Path $candidate) { return $candidate }
    }

    return $null
}

$xperfExe = Find-Tool "xperf.exe"
if (-not $xperfExe) {
    throw "xperf.exe not found. Install the Windows Performance Toolkit from the Windows ADK."
}

$sptAggExe = Find-Tool "sptaggregate.exe"
if (-not $sptAggExe) {
    throw "sptaggregate.exe not found. Ensure MSVC toolset 19.51+ is installed."
}

$spdConvExe = Find-Tool "spdconvert.exe"
if (-not $spdConvExe) {
    throw "spdconvert.exe not found. Ensure MSVC toolset 19.51+ is installed."
}

# ---------------------------------------------------------------------------
# Query hardware PMC counter capabilities for LBR and PMC profiling
# ---------------------------------------------------------------------------
function Get-PmcCapabilities {
    param([string] $XperfPath)

    $sources = & $XperfPath -pmcsources 2>$null

    # Branch event counters for LBR mode (AMD Zen 4 uses BranchInstructions; Intel uses BranchInstructionRetired)
    $branchCounter = if ($sources -match '\bBranchInstructions\b') {
        'BranchInstructions'
    } elseif ($sources -match '\bBranchInstructionRetired\b') {
        'BranchInstructionRetired'
    } elseif ($sources -match '\bTotalIssues\b') {
        'TotalIssues'
    } else {
        $null
    }

    # Instruction event counters for PMC IP mode
    $instCounter = if ($sources -match '\bInstructionRetired\b') {
        'InstructionRetired'
    } elseif ($sources -match '\bTotalIssues\b') {
        'TotalIssues'
    } elseif ($sources -match '\bFRRetiredx86Instructions\b') {
        'FRRetiredx86Instructions'
    } else {
        $null
    }

    return @{
        BranchCounter = $branchCounter
        InstCounter   = $instCounter
    }
}

$pmcCaps = Get-PmcCapabilities -XperfPath $xperfExe

$isWin10OrOlder = [Environment]::OSVersion.Version.Build -lt 22000
$isAmdCpu = ($pmcCaps.BranchCounter -eq 'BranchInstructions' -or $pmcCaps.InstCounter -eq 'TotalIssues')

$activeMode = $Mode
if ($activeMode -eq 'Auto') {
    # AMD Zen 4 hardware LBR requires Windows 11 kernel HAL (build 22000+).
    # On Windows 10 (e.g. build 19045), AMD LbrExtV2 events are not exposed to ETW,
    # so hardware PMC (TotalIssues with stack walking) is the verified optimal path.
    if ($isAmdCpu -and $isWin10OrOlder) {
        if ($pmcCaps.InstCounter) {
            $activeMode = 'PMC'
        } else {
            $activeMode = 'Timer'
        }
    } elseif ($pmcCaps.BranchCounter) {
        $activeMode = 'LBR'
    } elseif ($pmcCaps.InstCounter) {
        $activeMode = 'PMC'
    } else {
        $activeMode = 'Timer'
    }
}

Write-Host "SPGO Profile Collection" -ForegroundColor Cyan
Write-Host "  Target:   $ExePath"
Write-Host "  Mode:     $activeMode (Requested: $Mode)"
if ($activeMode -eq 'LBR') {
    Write-Host "  Counter:  $($pmcCaps.BranchCounter) (Hardware LBR)"
    if ($isAmdCpu -and $isWin10OrOlder) {
        Write-Host "  Notice:   AMD Zen 4 LBR requires Windows 11 (build 22000+)." -ForegroundColor DarkYellow
    }
} elseif ($activeMode -eq 'PMC') {
    Write-Host "  Counter:  $($pmcCaps.InstCounter) (Hardware PMC with call stacks)"
} else {
    Write-Host "  Counter:  OS Timer (1ms scheduler interrupt)"
}
Write-Host "  ETL:      $etlPath"
Write-Host "  SPT:      $sptPath"
Write-Host "  SPD:      $OutputSpd"
Write-Host "  Timeout:  ${DurationSeconds}s"
Write-Host ""

Write-Host "Tools:" -ForegroundColor DarkGray
Write-Host "  xperf:        $xperfExe"
Write-Host "  SPTAggregate: $sptAggExe"
Write-Host "  SPDConvert:   $spdConvExe"
Write-Host ""

# ---------------------------------------------------------------------------
# Ensure perf_spt.dll, perf_lbr.dll, and perf_hv.dll are registered in perfcore.ini
# (Windows ADK installs them but omits them from perfcore.ini, causing parsing failures)
# ---------------------------------------------------------------------------
$wptDir = Split-Path $xperfExe -Parent
$perfcoreIni = Join-Path $wptDir "perfcore.ini"
$neededDlls = @("perf_spt.dll", "perf_lbr.dll", "perf_hv.dll")

if (Test-Path $perfcoreIni) {
    try {
        $iniContent = [System.IO.File]::ReadAllText($perfcoreIni)
        $modified = $false
        foreach ($dll in $neededDlls) {
            if (-not $iniContent.Contains($dll) -and (Test-Path (Join-Path $wptDir $dll))) {
                $iniContent = $iniContent.TrimEnd() + "`r`n$dll`r`n"
                $modified = $true
            }
        }
        if ($modified) {
            [System.IO.File]::WriteAllText($perfcoreIni, $iniContent)
            Write-Host "Registered SPGO providers in WPT perfcore.ini." -ForegroundColor Green
        }
    } catch {
        # Elevated write failed; we will fall back to a shadow WPT directory below
    }
}

# Verify if all needed DLLs are active in perfcore.ini
$currentIni = if (Test-Path $perfcoreIni) { Get-Content $perfcoreIni } else { @() }
$hasAll = $true
foreach ($dll in $neededDlls) {
    if ($currentIni -notcontains $dll) { $hasAll = $false; break }
}

if (-not $hasAll) {
    # Use shadow copy in TEMP to guarantee SPTAggregate/xperf always has all required add-ins
    $shadowWpt = Join-Path $env:TEMP "WPT_SPGO"
    if (-not (Test-Path $shadowWpt)) {
        New-Item -ItemType Directory -Path $shadowWpt -Force | Out-Null
        Copy-Item "$wptDir\*" -Destination $shadowWpt -Recurse -Force
    }
    $shadowIni = Join-Path $shadowWpt "perfcore.ini"
    $shadowList = [System.Collections.ArrayList]@(Get-Content $shadowIni)
    foreach ($dll in $neededDlls) {
        if ($shadowList -notcontains $dll) { [void]$shadowList.Add($dll) }
    }
    [System.IO.File]::WriteAllLines($shadowIni, $shadowList)
    $env:PATH = "$shadowWpt;$env:PATH"
    $xperfExe = Join-Path $shadowWpt "xperf.exe"
} else {
    $env:PATH = "$wptDir;$env:PATH"
}

if (-not $ExistingEtl) {
# ---------------------------------------------------------------------------
# Step 1: Start xperf trace (NT Kernel Logger)
# ---------------------------------------------------------------------------
Write-Host "[1/4] Starting xperf kernel trace..." -ForegroundColor Yellow

# Cancel any stale kernel logger session quietly
try {
    & $xperfExe -cancel *>$null
} catch {}

$traceStarted = $false
$currentMode = $activeMode

while (-not $traceStarted) {
    if ($currentMode -eq 'LBR') {
        $counter = $pmcCaps.BranchCounter
        if (-not $counter) {
            Write-Host "  No branch counter available for LBR. Falling back to Timer mode." -ForegroundColor DarkYellow
            $currentMode = 'Timer'
            continue
        }
        $interval = if ($SampleInterval -gt 0) { $SampleInterval } else { 65536 }
        Write-Host "  Configuring hardware LBR trace (Counter: $counter, Interval: $interval)..." -ForegroundColor Yellow
        & $xperfExe -on LOADER+PROC_THREAD+PMC_PROFILE -pmcprofile $counter -LastBranch PmcInterrupt -setProfInt $counter $interval -BufferSize 4096 -MinBuffers 4096 -MaxBuffers 4096
        if ($LASTEXITCODE -eq 0) {
            $traceStarted = $true
            $activeMode = 'LBR'
            Write-Host "  Kernel trace started (Hardware LBR with counter $counter)." -ForegroundColor Green
        } else {
            Write-Host "  xperf failed to configure LBR (exit code $LASTEXITCODE). Falling back to Timer mode..." -ForegroundColor DarkYellow
            $currentMode = 'Timer'
        }
    } elseif ($currentMode -eq 'PMC') {
        $counter = $pmcCaps.InstCounter
        if (-not $counter) {
            Write-Host "  No instruction counter available for PMC. Falling back to Timer mode." -ForegroundColor DarkYellow
            $currentMode = 'Timer'
            continue
        }
        $interval = if ($SampleInterval -gt 0) { $SampleInterval } else { 65536 }
        Write-Host "  Configuring hardware PMC trace (Counter: $counter, Interval: $interval)..." -ForegroundColor Yellow
        & $xperfExe -on LOADER+PROC_THREAD+PMC_PROFILE+PROFILE -pmcprofile $counter -setProfInt $counter $interval -stackwalk Profile -BufferSize 4096 -MinBuffers 4096 -MaxBuffers 4096
        if ($LASTEXITCODE -eq 0) {
            $traceStarted = $true
            $activeMode = 'PMC'
            Write-Host "  Kernel trace started (Hardware PMC with counter $counter)." -ForegroundColor Green
        } else {
            Write-Host "  xperf failed to configure PMC (exit code $LASTEXITCODE). Falling back to Timer mode..." -ForegroundColor DarkYellow
            $currentMode = 'Timer'
        }
    } else {
        Write-Host "  Configuring OS Timer trace with call stack walking..." -ForegroundColor Yellow
        & $xperfExe -on LOADER+PROC_THREAD+PROFILE -stackwalk Profile -BufferSize 1024 -MinBuffers 128 -MaxBuffers 512
        if ($LASTEXITCODE -eq 0) {
            $traceStarted = $true
            $activeMode = 'Timer'
            Write-Host "  Kernel trace started (OS Timer with call stacks)." -ForegroundColor Green
        } else {
            throw "xperf failed to start (exit code $LASTEXITCODE). Run this script in PowerShell as Administrator."
        }
    }
}

# ---------------------------------------------------------------------------
# Step 2: Run the target (or wait)
# ---------------------------------------------------------------------------
$process = $null
try {
    if ($SkipLaunch) {
        Write-Host "[2/4] SkipLaunch: waiting ${DurationSeconds}s for manual profiling..." -ForegroundColor Yellow
        Start-Sleep -Seconds $DurationSeconds
    } else {
        Write-Host "[2/4] Launching $exeName (timeout: ${DurationSeconds}s)..." -ForegroundColor Yellow
        $startInfo = @{ FilePath = $ExePath; PassThru = $true }
        if ($ExeArgs) { $startInfo['ArgumentList'] = $ExeArgs }
        $process = Start-Process @startInfo
        $exited = $process.WaitForExit($DurationSeconds * 1000)
        if (-not $exited) {
            Write-Host "  Timeout reached; stopping trace." -ForegroundColor DarkYellow
        } else {
            Write-Host "  Process exited with code $($process.ExitCode)." -ForegroundColor Green
        }
    }
} finally {
    # ---------------------------------------------------------------------------
    # Step 3: Stop trace and save ETL
    # ---------------------------------------------------------------------------
    Write-Host "[3/4] Stopping xperf trace..." -ForegroundColor Yellow
    & $xperfExe -stop -d $etlPath
    if ($LASTEXITCODE -ne 0) {
        throw "xperf -stop failed (exit code $LASTEXITCODE)."
    }
    Write-Host "  ETL saved: $etlPath ($('{0:N1} MB' -f ((Get-Item $etlPath).Length / 1MB)))" -ForegroundColor Green
}
} else {
    if (-not (Test-Path -LiteralPath $ExistingEtl)) {
        throw "Specified ExistingEtl file '$ExistingEtl' does not exist."
    }
    $etlPath = (Resolve-Path $ExistingEtl).Path
    Write-Host "Skipping trace: using existing ETL: $etlPath ($('{0:N1} MB' -f ((Get-Item $etlPath).Length / 1MB)))" -ForegroundColor Green
}

# ---------------------------------------------------------------------------
# Step 4: Convert ETL -> SPT -> SPD
# ---------------------------------------------------------------------------
Write-Host "[4/4] Converting ETL -> SPT -> SPD..." -ForegroundColor Yellow

# Build binary filter list: target executable plus known project DLLs in the same directory
$targetBinaryList = [System.Collections.Generic.List[string]]::new()
$targetBinaryList.Add($exeName)
if ($Binaries.Count -gt 0) {
    foreach ($bin in $Binaries) {
        if (-not $targetBinaryList.Contains($bin)) { $targetBinaryList.Add($bin) }
    }
} else {
    # Auto-detect project DLLs (e.g. upscayl-ncnn.dll, wynpixshellex.dll)
    $siblingDlls = Get-ChildItem -Path $exeDir -Filter "*.dll" -File | Where-Object {
        $_.Name -like "wynpix*" -or $_.Name -like "upscayl*"
    }
    foreach ($dll in $siblingDlls) {
        if (-not $targetBinaryList.Contains($dll.Name)) { $targetBinaryList.Add($dll.Name) }
    }
}
$binaryFilterStr = ($targetBinaryList -join ',')

Write-Host "  SPTAggregate: /binary $binaryFilterStr /nokernel -etl $etlPath $sptPath"
& $sptAggExe /binary $binaryFilterStr /nokernel -etl $etlPath $sptPath
if ($LASTEXITCODE -ne 0) {
    Write-Host "  Retrying SPTAggregate with process filter /process $exeName..." -ForegroundColor DarkYellow
    & $sptAggExe /process $exeName /nokernel -etl $etlPath $sptPath
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  Retrying SPTAggregate without binary filter..." -ForegroundColor DarkYellow
        & $sptAggExe /nokernel -etl $etlPath $sptPath
        if ($LASTEXITCODE -ne 0) {
            throw "SPTAggregate failed (exit code $LASTEXITCODE)."
        }
    }
}

# Use /mode:LBR for hardware branch traces, or /mode:IP for timer/PMC traces
$spdMode = if ($activeMode -eq 'LBR') { '/mode:LBR' } else { '/mode:IP' }
Write-Host "  SPDConvert: $spdMode $OutputSpd $sptPath"
& $spdConvExe $spdMode $OutputSpd $sptPath
if ($LASTEXITCODE -ne 0) {
    if ($activeMode -eq 'LBR') {
        Write-Host "  SPDConvert with $spdMode returned code $LASTEXITCODE; retrying with /mode:IP..." -ForegroundColor DarkYellow
        & $spdConvExe /mode:IP $OutputSpd $sptPath
        if ($LASTEXITCODE -ne 0) {
            throw "SPDConvert failed (exit code $LASTEXITCODE)."
        }
    } else {
        throw "SPDConvert failed (exit code $LASTEXITCODE)."
    }
}

# Clean up intermediate SPT trace file
Remove-Item $sptPath -ErrorAction SilentlyContinue

if ($CleanEtl -and -not $ExistingEtl) {
    Remove-Item $etlPath -ErrorAction SilentlyContinue
}

Write-Host ""
Write-Host "Done! Profile imported into: $OutputSpd" -ForegroundColor Green
Write-Host "  Size: $('{0:N1} KB' -f ((Get-Item $OutputSpd).Length / 1KB))"
Write-Host ""
Write-Host "Next step: rebuild with the profile to apply optimizations:" -ForegroundColor Cyan
Write-Host "  cmake --build --preset wynpix-x64-release-spgo"
Write-Host ""
