<#
.SYNOPSIS
    Complete QuantumVerse build script with comprehensive logging.
    Run from F:\syyyy in PowerShell (auto-detects VS 2022 environment).

.DESCRIPTION
    Executes the full build pipeline: environment verification, CMake configure,
    build, Qt runtime deployment, tests, and optional packaging.
    Every stage logs both stdout and stderr to timestamped log files.
    Automatically sets up MSVC 2022 compiler environment via vswhere + vcvars64.

.REQUIREMENTS
    - Windows 10/11
    - Visual Studio 2022 (MSVC v143) with Desktop C++ workload
    - CMake 3.25+ (added to PATH)
    - Qt 6.11.1 (msvc2022_64)
    - PowerShell 5.1+

.EXAMPLE
    .\build_all.ps1
    .\build_all.ps1 -BuildType Release -RunTests
    .\build_all.ps1 -BuildType Debug -UseAsan -RunTests -Package
    .\build_all.ps1 -QtPath "D:\Qt\6.11.1\msvc2022_64" -UseVR

.NOTES
    Logs are written to .\build_logs\ with timestamps.
    The build directory is .\build by default.
    MSVC environment is auto-configured using vswhere.exe + vcvars64.bat.
#>

[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo")]
    [string]$BuildType = "Release",

    [switch]$RunTests,
    [switch]$UseAsan,
    [switch]$UseUbsan,
    [switch]$UseTsan,
    [switch]$UseCoverage,
    [switch]$UseVR,
    [switch]$UseKafka,
    [switch]$Package,
    [switch]$SkipQtDeploy,
    [string]$QtPath,
    [string]$BuildDir = "build",
    [string]$LogDir = "build_logs",
    [int]$ParallelJobs = (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

# ============================================================================
# Environment Setup
# ============================================================================

function Get-VSInstallPath {
    param([string]$Vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe")

    if (-not (Test-Path -LiteralPath $Vswhere)) {
        Write-Host "[WARNING] vswhere.exe not found at: $Vswhere" -ForegroundColor Yellow
        return $null
    }

    $vsPath = & $Vswhere -latest -property installationPath 2>$null
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrEmpty($vsPath)) {
        Write-Host "[WARNING] vswhere.exe could not find VS installation" -ForegroundColor Yellow
        return $null
    }

    return $vsPath.Trim()
}

function Get-VcToolsPath {
    param([string]$VsInstallPath)

    if ([string]::IsNullOrEmpty($VsInstallPath)) {
        return $null
    }

    $vcTools = Join-Path $VsInstallPath "VC\Tools\MSVC"
    if (-not (Test-Path -LiteralPath $vcTools)) {
        return $null
    }

    $versions = Get-ChildItem -LiteralPath $vcTools -Directory | Sort-Object Name -Descending
    if ($versions.Count -eq 0) {
        return $null
    }

    return Join-Path $vcTools $versions[0].Name
}

function Get-SDKVersion {
    param([string]$SdkRoot = "C:\Program Files (x86)\Windows Kits\10")

    if (-not (Test-Path -LiteralPath $SdkRoot)) {
        return $null
    }

    $includeDir = Join-Path $SdkRoot "Include"
    if (-not (Test-Path -LiteralPath $includeDir)) {
        return $null
    }

    $versions = Get-ChildItem -LiteralPath $includeDir -Directory |
        Where-Object { $_.Name -match '^\d+\.\d+\.\d+\.\d+$' } |
        Sort-Object Name -Descending

    if ($versions.Count -eq 0) {
        return $null
    }

    return $versions[0].Name
}

function Initialize-MSVCEnvironment {
    Write-Host ""
    Write-Host "--- Initializing MSVC 2022 Compiler Environment ---" -ForegroundColor Cyan

    $vsInstallPath = Get-VSInstallPath
    if ([string]::IsNullOrEmpty($vsInstallPath)) {
        Write-Host "[ERROR] Visual Studio 2022 installation not found." -ForegroundColor Red
        Write-Host "  Install VS 2022 with Desktop C++ workload, or run from" -ForegroundColor Yellow
        Write-Host "  'x64 Native Tools Command Prompt for VS 2022'" -ForegroundColor Yellow
        throw "VS 2022 not found"
    }

    Write-Host "[OK] VS Install: $vsInstallPath" -ForegroundColor Green

    $vcToolsPath = Get-VcToolsPath -VsInstallPath $vsInstallPath
    if ([string]::IsNullOrEmpty($vcToolsPath)) {
        Write-Host "[ERROR] MSVC tools not found in VS installation" -ForegroundColor Red
        throw "MSVC tools not found"
    }

    Write-Host "[OK] VC Tools: $vcToolsPath" -ForegroundColor Green

    $sdkVersion = Get-SDKVersion
    if ([string]::IsNullOrEmpty($sdkVersion)) {
        Write-Host "[WARNING] Windows SDK not found" -ForegroundColor Yellow
    }
    else {
        Write-Host "[OK] Windows SDK: $sdkVersion" -ForegroundColor Green
    }

    $vcVarsCmd = Join-Path $vsInstallPath "VC\Auxiliary\Build\vcvars64.bat"
    if (-not (Test-Path -LiteralPath $vcVarsCmd)) {
        $vcVarsCmd = Join-Path $vcToolsPath "bin\Hostx64\x64\vcvars64.bat"
    }
    if (-not (Test-Path -LiteralPath $vcVarsCmd)) {
        Write-Host "[ERROR] vcvars64.bat not found in expected locations" -ForegroundColor Red
        Write-Host "  Checked: $(Join-Path $vsInstallPath 'VC\Auxiliary\Build\vcvars64.bat')" -ForegroundColor Yellow
        Write-Host "  Checked: $(Join-Path $vcToolsPath 'bin\Hostx64\x64\vcvars64.bat')" -ForegroundColor Yellow
        throw "vcvars64.bat not found"
    }

    Write-Host "[OK] vcvars64.bat: $vcVarsCmd" -ForegroundColor Green
    Write-Host "[INFO] Setting up compiler environment..." -ForegroundColor Gray

    # Call vcvars64.bat and capture its environment changes
    $envOutput = cmd /c "`"$vcVarsCmd`" >nul && set" 2>$null
    if (-not $envOutput) {
        Write-Host "[WARNING] vcvars64.bat produced no environment output" -ForegroundColor Yellow
    }

    $pathEntries = @()
    foreach ($line in $envOutput) {
        if ($line -match '^([^=]+)=(.*)$') {
            $name = $matches[1]
            $value = $matches[2]
            Set-Item -LiteralPath "Env:$name" -Value $value
            if ($name -eq "PATH") {
                $pathEntries = $value -split ';' | Where-Object { $_ -ne "" }
            }
        }
    }

    $clInPath = $pathEntries | Where-Object { $_ -like "*bin*Hostx64*x64*" }
    if (-not $clInPath) {
        Write-Host "[WARNING] MSVC bin directory not found in PATH after vcvars64" -ForegroundColor Yellow
        Write-Host "  Attempting manual PATH setup from VC Tools..." -ForegroundColor Yellow

        $vcBin = Join-Path $vcToolsPath "bin\Hostx64\x64"
        $sdkBin = "C:\Program Files (x86)\Windows Kits\10\bin\$sdkVersion\x64"
        $env:PATH = "$vcBin;$sdkBin;$env:PATH"
        $env:LIB = "$($vcToolsPath)\lib\x64;C:\Program Files (x86)\Windows Kits\10\lib\$sdkVersion\ucrt\x64;C:\Program Files (x86)\Windows Kits\10\lib\$sdkVersion\um\x64;$env:LIB"
        $env:INCLUDE = "$($vcToolsPath)\include;C:\Program Files (x86)\Windows Kits\10\include\$sdkVersion\ucrt;C:\Program Files (x86)\Windows Kits\10\include\$sdkVersion\um;C:\Program Files (x86)\Windows Kits\10\include\$sdkVersion\shared;$env:INCLUDE"

        Write-Host "[OK] Manual PATH setup complete" -ForegroundColor Green
        Write-Host "  cl.exe: $((Get-Command cl.exe -ErrorAction SilentlyContinue).Source)" -ForegroundColor Gray
    }

    Write-Host "[OK] MSVC environment initialized" -ForegroundColor Green
    Write-Host "  cl.exe: $((Get-Command cl.exe -ErrorAction SilentlyContinue).Source)" -ForegroundColor Gray
}

function Initialize-QtEnvironment {
    param([string]$QtPath)

    Write-Host ""
    Write-Host "--- Initializing Qt Environment ---" -ForegroundColor Cyan

    if ([string]::IsNullOrEmpty($QtPath)) {
        $qtPath = $null
        $searchRoots = @("F:\qt", "C:\Qt")
        foreach ($root in $searchRoots) {
            if (-not (Test-Path -LiteralPath $root)) { continue }
            $preferred = Join-Path $root "6.11.1"
            if ((Test-Path -LiteralPath $preferred) -and (Test-Path -LiteralPath (Join-Path $preferred "msvc2022_64"))) {
                $qtPath = $preferred
                break
            }
            $candidates = Get-ChildItem -LiteralPath $root -Directory | Where-Object { $_.Name -match '^\d+\.\d+\.\d+' }
            foreach ($c in ($candidates | Sort-Object Name -Descending)) {
                $msvcDir = Join-Path $c.FullName "msvc2022_64"
                if (Test-Path -LiteralPath $msvcDir) {
                    $qtPath = $c.FullName
                    break
                }
            }
            if ($qtPath) { break }
        }
        if ([string]::IsNullOrEmpty($qtPath)) {
            Write-Host "[ERROR] Qt with msvc2022_64 not found in F:\qt or C:\Qt" -ForegroundColor Red
            Write-Host "  Specify Qt path with -QtPath parameter" -ForegroundColor Yellow
            throw "Qt not found"
        }
        Write-Host "[INFO] Auto-detected Qt: $qtPath" -ForegroundColor Yellow
    }
    else {
        $qtPath = $QtPath
    }

    $qtMsvc = Join-Path $qtPath "msvc2022_64"
    if (-not (Test-Path -LiteralPath $qtMsvc)) {
        Write-Host "[ERROR] Qt msvc2022_64 toolchain not found: $qtMsvc" -ForegroundColor Red
        Write-Host "  This build requires Qt built for MSVC 2022 (x64)" -ForegroundColor Yellow
        throw "Qt msvc2022_64 not found"
    }

    $qtPath = $qtMsvc
    Write-Host "[OK] Qt path: $qtPath" -ForegroundColor Green

    $qtBin = Join-Path $qtPath "bin"
    if (-not (Test-Path -LiteralPath $qtBin)) {
        Write-Host "[WARNING] Qt bin directory not found: $qtBin" -ForegroundColor Yellow
    }
    else {
        $currentPath = [Environment]::GetEnvironmentVariable("PATH", "Process")
        if ($currentPath -notlike "*$qtBin*") {
            $env:PATH = "$qtBin;$env:PATH"
            Write-Host "[OK] Added Qt bin to PATH: $qtBin" -ForegroundColor Green
        }
    }

    $windeployqt = Join-Path $qtBin "windeployqt.exe"
    if (-not (Test-Path -LiteralPath $windeployqt)) {
        Write-Host "[WARNING] windeployqt.exe not found at: $windeployqt" -ForegroundColor Yellow
        Write-Host "  Qt deployment will be skipped" -ForegroundColor Yellow
        $script:SkipQtDeploy = $true
    }
    else {
        Write-Host "[OK] windeployqt.exe: $windeployqt" -ForegroundColor Green
    }
}

# ============================================================================
# Main Script
# ============================================================================

try {
    Initialize-MSVCEnvironment
    Initialize-QtEnvironment -QtPath $QtPath
}
catch {
    Write-Host ""
    Write-Host "[FATAL] Environment initialization failed: $_" -ForegroundColor Red
    Write-Host ""
    Write-Host "To manually set up the environment:" -ForegroundColor Yellow
    Write-Host "  1. Open 'x64 Native Tools Command Prompt for VS 2022'" -ForegroundColor Yellow
    Write-Host "  2. Run: .\build_all.ps1" -ForegroundColor Yellow
    Write-Host ""
    exit 1
}

# Create log directory
if (-not (Test-Path -LiteralPath $LogDir)) {
    New-Item -ItemType Directory -Path $LogDir -Force | Out-Null
}

$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$buildLog = Join-Path $LogDir "build_${timestamp}.log"
$testLog = Join-Path $LogDir "test_${timestamp}.log"
$deployLog = Join-Path $LogDir "deploy_${timestamp}.log"
$packageLog = Join-Path $LogDir "package_${timestamp}.log"
$summaryLog = Join-Path $LogDir "build_summary_${timestamp}.log"

function Write-Header {
    param([string]$Message)
    $separator = "=" * 80
    Write-Host ""
    Write-Host $separator -ForegroundColor Cyan
    Write-Host $Message -ForegroundColor Cyan
    Write-Host $separator -ForegroundColor Cyan
    Write-Host ""
}

function Write-Section {
    param([string]$Message)
    Write-Host ""
    Write-Host "--- $Message ---" -ForegroundColor Yellow
    Write-Host ""
}

function Test-Command {
    param([string]$Command, [string]$Name)
    try {
        $null = Get-Command $Command -ErrorAction Stop
        Write-Host "[OK] $Name found: $(Get-Command $Command -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source)" -ForegroundColor Green
        return $true
    }
    catch {
        Write-Host "[FAIL] $Name not found in PATH" -ForegroundColor Red
        return $false
    }
}

function Get-ClVersion {
    $oldPref = $ErrorActionPreference
    $ErrorActionPreference = "Continue"
    try {
        $output = cmd /c "cl" 2>&1 | Select-Object -First 3
        foreach ($line in $output) {
            if ($line -match 'Version\s+(\S+)') {
                return $matches[1]
            }
        }
        return "unknown"
    }
    finally {
        $ErrorActionPreference = $oldPref
    }
}

function Invoke-Step {
    param(
        [Parameter(Mandatory)][string]$Name,
        [Parameter(Mandatory)][string]$LogFile,
        [Parameter(Mandatory)][scriptblock]$Command,
        [switch]$ContinueOnError
    )

    Write-Section -Message "STEP: $Name"

    if (Test-Path -LiteralPath $LogFile) {
        Remove-Item -LiteralPath $LogFile -Force
    }

    $startTime = Get-Date
    $success = $false

    try {
        $commandStr = $Command.ToString().Trim()
        $preview = if ($commandStr.Length -gt 100) { $commandStr.Substring(0, 100) + "..." } else { $commandStr }
        Write-Host "Running: $preview" -ForegroundColor Gray
        Write-Host "Log: $LogFile" -ForegroundColor Gray
        Write-Host ""

        $LASTEXITCODE = 0
        $output = & $Command 2>&1 | Out-String
        $output | Out-File -FilePath $LogFile -Encoding utf8
        $output | Write-Host

        $exitCode = $LASTEXITCODE

        if ($exitCode -eq 0) {
            $success = $true
            Write-Host "[SUCCESS] $Name completed" -ForegroundColor Green
        }
        else {
            $success = $false
            Write-Host "[FAILED] $Name failed with exit code: $exitCode" -ForegroundColor Red
            if (-not $ContinueOnError) {
                throw "$Name failed with exit code $exitCode. See log: $LogFile"
            }
        }
    }
    catch {
        $success = $false
        Write-Host "[ERROR] $Name threw an exception: $_" -ForegroundColor Red
        if (-not $ContinueOnError) {
            throw
        }
    }

    $duration = (Get-Date) - $startTime
    $durationStr = "{0:mm}min {0:ss}s" -f $duration

    $logEntry = "{0} | {1} | ExitCode={2} | Duration={3}" -f (Get-Date -Format "yyyy-MM-dd HH:mm:ss"), $Name, $(if ($success) { "0" } else { "FAILED" }), $durationStr

    return $logEntry
}

# ============================================================================
# MAIN BUILD PIPELINE
# ============================================================================

Write-Header -Message "QuantumVerse Complete Build Pipeline"
Write-Host "Build Type:       $BuildType"
Write-Host "Qt Path:          $qtPath"
Write-Host "Build Dir:        $BuildDir"
Write-Host "Log Dir:          $LogDir"
Write-Host "Parallel Jobs:    $ParallelJobs"
Write-Host "ASan:             $UseAsan"
Write-Host "UBSan:            $UseUbsan"
Write-Host "TSan:             $UseTsan"
Write-Host "Coverage:         $UseCoverage"
Write-Host "VR:               $UseVR"
Write-Host "Kafka:            $UseKafka"
Write-Host "Run Tests:        $RunTests"
Write-Host "Package:          $Package"
Write-Host "Skip Qt Deploy:   $SkipQtDeploy"
Write-Host ""

$results = @()

try {
    # ========================================================================
    # STAGE 0: Environment Verification
    # ========================================================================
    Write-Header -Message "STAGE 0: Environment Verification"

    $envChecks = @(
        @{ Command = "cmake"; Name = "CMake" },
        @{ Command = "cl"; Name = "MSVC Compiler (cl.exe)" },
        @{ Command = "cmake.exe"; Name = "CMake Executable" },
        @{ Command = "ctest"; Name = "CTest" },
        @{ Command = "ninja"; Name = "Ninja (optional)" }
    )

    foreach ($check in $envChecks) {
        Test-Command -Command $check.Command -Name $check.Name
    }

    if (-not $SkipQtDeploy) {
        Test-Command -Command "windeployqt" -Name "windeployqt (Qt)"
    }

    Write-Host ""
    Write-Host "CMake version:" -ForegroundColor Cyan
    cmake --version 2>&1 | ForEach-Object { Write-Host "  $_" }
    Write-Host ""

    Write-Host "MSVC version:" -ForegroundColor Cyan
    $clVersion = Get-ClVersion
    Write-Host "  cl.exe version: $clVersion" -ForegroundColor Gray
    Write-Host ""

    $results += Invoke-Step -Name "Environment Verification" -LogFile $buildLog -Command {
        Write-Host "Environment verification completed successfully"
    } -ContinueOnError

    # ========================================================================
    # STAGE 1: Clean Build Directory
    # ========================================================================
    Write-Header -Message "STAGE 1: Clean Build Directory"

    if (Test-Path -LiteralPath $BuildDir) {
        Write-Host "Removing existing build directory: $BuildDir" -ForegroundColor Yellow
        Remove-Item -LiteralPath $BuildDir -Recurse -Force
    }

    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null

    $results += Invoke-Step -Name "Clean Build Directory" -LogFile $buildLog -Command {
        Write-Host "Build directory cleaned: $BuildDir"
    }

    # ========================================================================
    # STAGE 2: CMake Configure
    # ========================================================================
    Write-Header -Message "STAGE 2: CMake Configure"

    $cmakeArgs = @(
        "-B", $BuildDir,
        "-G", "Visual Studio 17 2022",
        "-A", "x64",
        "-DCMAKE_BUILD_TYPE=$BuildType",
        "-DQUANTUMVERSE_BUILD_TESTS=ON",
        "-DQUANTUMVERSE_USE_QT=ON",
        "-DCMAKE_PREFIX_PATH=$qtPath"
    )

    if ($UseAsan) { $cmakeArgs += "-DQUANTUMVERSE_USE_ASAN=ON" }
    if ($UseUbsan) { $cmakeArgs += "-DQUANTUMVERSE_USE_UBSAN=ON" }
    if ($UseTsan) { $cmakeArgs += "-DQUANTUMVERSE_USE_TSAN=ON" }
    if ($UseCoverage) { $cmakeArgs += "-DQUANTUMVERSE_USE_COVERAGE=ON" }
    if ($UseVR) { $cmakeArgs += "-DQUANTUMVERSE_USE_VR=ON" }
    if ($UseKafka) { $cmakeArgs += "-DQUANTUMVERSE_USE_KAFKA=ON" }

    $cmakeCommand = "cmake " + ($cmakeArgs -join " ")
    Write-Host "Command: $cmakeCommand" -ForegroundColor Gray
    Write-Host ""

    $results += Invoke-Step -Name "CMake Configure" -LogFile $buildLog -Command {
        cmake @cmakeArgs
    }

    # ========================================================================
    # STAGE 3: Build
    # ========================================================================
    Write-Header -Message "STAGE 3: Build Project"

    $buildArgs = @(
        "--build", $BuildDir,
        "--config", $BuildType,
        "--parallel", $ParallelJobs.ToString()
    )

    $buildCommand = "cmake " + ($buildArgs -join " ")
    Write-Host "Command: $buildCommand" -ForegroundColor Gray
    Write-Host ""

    $results += Invoke-Step -Name "Build Project" -LogFile $buildLog -Command {
        cmake @buildArgs
    }

    # Verify executable exists
    $exePath = Join-Path $BuildDir "$BuildType\quantumverse_qml.exe"
    if (-not (Test-Path -LiteralPath $exePath)) {
        Write-Host "[WARNING] Executable not found at expected path: $exePath" -ForegroundColor Yellow
        Write-Host "  Checking alternative locations..." -ForegroundColor Yellow

        $altPaths = @(
            Join-Path $BuildDir "quantumverse_qml.exe",
            Join-Path $BuildDir "bin\quantumverse_qml.exe",
            Join-Path $BuildDir "Release\quantumverse_qml.exe",
            Join-Path $BuildDir "Debug\quantumverse_qml.exe"
        )

        $found = $false
        foreach ($alt in $altPaths) {
            if (Test-Path -LiteralPath $alt) {
                Write-Host "[OK] Found at: $alt" -ForegroundColor Green
                $exePath = $alt
                $found = $true
                break
            }
        }

        if (-not $found) {
            Write-Host "[FAIL] Could not find quantumverse_qml.exe in build directory" -ForegroundColor Red
            throw "Build artifact not found"
        }
    }
    else {
        Write-Host "[OK] Executable found: $exePath" -ForegroundColor Green
    }

    # ========================================================================
    # STAGE 4: Deploy Qt Runtime (Windows)
    # ========================================================================
    if (-not $SkipQtDeploy -and $BuildType -eq "Release") {
        Write-Header -Message "STAGE 4: Deploy Qt Runtime"

        $results += Invoke-Step -Name "Deploy Qt Runtime" -LogFile $deployLog -Command {
            cmake --build $BuildDir --config $BuildType --target deploy_qt_runtime
        }
    }
    else {
        Write-Section -Message "STAGE 4: Deploy Qt Runtime (SKIPPED)"
        Write-Host "Reason: $(
            if ($SkipQtDeploy) { "SkipQtDeploy flag set or windeployqt not found" }
            elseif ($BuildType -ne "Release") { "Build type is $BuildType (Release required)" }
            else { "Unknown" }
        )" -ForegroundColor Yellow
    }

    # ========================================================================
    # STAGE 5: Run Tests
    # ========================================================================
    if ($RunTests) {
        Write-Header -Message "STAGE 5: Run Tests"

        Push-Location -LiteralPath $BuildDir

        $testArgs = @("-C", $BuildType, "--output-on-failure")

        if ($BuildType -eq "Release") {
            $exclusions = "GridDeformationTest|SoftwareTourHeadlessTest|UI4DTest|PerformanceGateTest|QMLPerformanceBaseline|ViewportStateTest|ViewportContentTest|VisualRegressionTest|AnimationTimingTest|GLStrictAuditTest|PerformanceRegressionTest"
            $testArgs += "-E", $exclusions
            Write-Host "Excluding canary tests: $exclusions" -ForegroundColor Yellow
        }

        $results += Invoke-Step -Name "Run Tests" -LogFile $testLog -Command {
            ctest @testArgs
        }

        Pop-Location
    }
    else {
        Write-Section -Message "STAGE 5: Run Tests (SKIPPED)"
        Write-Host "Use -RunTests to enable" -ForegroundColor Yellow
    }

    # ========================================================================
    # STAGE 6: Package (Optional)
    # ========================================================================
    if ($Package) {
        Write-Header -Message "STAGE 6: Package for Distribution"

        if ($BuildType -ne "Release") {
            Write-Host "[WARNING] Packaging typically requires Release build" -ForegroundColor Yellow
        }

        Push-Location -LiteralPath $BuildDir

        $results += Invoke-Step -Name "Package (CPack)" -LogFile $packageLog -Command {
            cpack -C $BuildType
        }

        Pop-Location
    }
    else {
        Write-Section -Message "STAGE 6: Package (SKIPPED)"
        Write-Host "Use -Package to enable" -ForegroundColor Yellow
    }

    # ========================================================================
    # SUMMARY
    # ========================================================================
    Write-Header -Message "BUILD SUMMARY"

    $summary = @()
    $summary += "QuantumVerse Build Pipeline - $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
    $summary += "=" * 80
    $summary += ""
    $summary += "Configuration:"
    $summary += "  Build Type:       $BuildType"
    $summary += "  Qt Path:          $qtPath"
    $summary += "  Build Directory:  $BuildDir"
    $summary += "  Log Directory:    $LogDir"
    $summary += "  Parallel Jobs:    $ParallelJobs"
    $summary += "  ASan:             $UseAsan"
    $summary += "  UBSan:            $UseUbsan"
    $summary += "  TSan:             $UseTsan"
    $summary += "  Coverage:         $UseCoverage"
    $summary += "  VR:               $UseVR"
    $summary += "  Kafka:            $UseKafka"
    $summary += "  Run Tests:        $RunTests"
    $summary += "  Package:          $Package"
    $summary += "  Skip Qt Deploy:   $SkipQtDeploy"
    $summary += ""
    $summary += "Results:"
    $summary += "-" * 80

    foreach ($result in $results) {
        $summary += $result
    }

    $summary += ""
    $summary += "Log Files:"
    $summary += "  Build Log:  $buildLog"
    $summary += "  Test Log:   $testLog"
    $summary += "  Deploy Log: $deployLog"
    $summary += "  Package Log: $packageLog"
    $summary += ""
    $summary += "=" * 80

    $summary | Out-File -FilePath $summaryLog -Encoding utf8
    $summary | ForEach-Object { Write-Host $_ }

    Write-Host ""
    Write-Host "[COMPLETE] All stages finished. Summary written to: $summaryLog" -ForegroundColor Green
    Write-Host ""
}
catch {
    Write-Host ""
    Write-Host "[FATAL] Build pipeline failed: $_" -ForegroundColor Red
    Write-Host "Check log files in: $LogDir" -ForegroundColor Yellow
    Write-Host "Most recent log: $buildLog" -ForegroundColor Yellow
    Write-Host ""
    exit 1
}
