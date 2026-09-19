# scripts/prepush.ps1
# Clean Release build + tests. Run before `git push`.
# Wipes the build tree so incremental caching can't hide compile errors.

$ErrorActionPreference = 'Stop'

Push-Location $PSScriptRoot\..
try {
    Write-Host "[prepush] Wiping build_ci..." -ForegroundColor Cyan
    Remove-Item -Recurse -Force build_ci -ErrorAction SilentlyContinue

    # Use MSVC toolchain: Qt6 (msvc2022_64) is MSVC-built, so MinGW's ld
    # cannot read its .lib import libraries (undefined __imp_ symbols).
    Write-Host "[prepush] Activating MSVC toolchain..." -ForegroundColor Cyan
    $VsWhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    $VcVars = ( & $VsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath ) + "\VC\Auxiliary\Build\vcvars64.bat"
    if (-not (Test-Path $VcVars)) { throw "vcvars64.bat not found at $VcVars" }
    & cmd.exe /c """$VcVars"" && set > %TEMP%\vcvars_env.txt"
    if ($LASTEXITCODE -ne 0) { throw "vcvars64 failed" }
    Get-Content "$env:TEMP\vcvars_env.txt" | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') { Set-Item "env:$($matches[1])" $matches[2] }
    }

    Write-Host "[prepush] Configuring Release..." -ForegroundColor Cyan
    cmake -B build_ci -G Ninja `
        -DCMAKE_BUILD_TYPE=Release `
        -DQUANTUMVERSE_BUILD_TESTS=ON `
        -DQUANTUMVERSE_USE_QT=ON `
        -DCMAKE_PREFIX_PATH="F:\qt\6.11.1\msvc2022_64"
    if ($LASTEXITCODE -ne 0) { throw "cmake configure failed ($LASTEXITCODE)" }

    Write-Host "[prepush] Building..." -ForegroundColor Cyan
    cmake --build build_ci --config Release --parallel 8
    if ($LASTEXITCODE -ne 0) { throw "build failed ($LASTEXITCODE)" }

    Write-Host "[prepush] Running ctest..." -ForegroundColor Cyan
    Push-Location build_ci
    try {
        ctest -C Release --output-on-failure `
            -E "RenderingDiagnosticTest|ViewportStateTest|ViewportContentTest|VisualRegressionTest|AnimationTimingTest|QMLPerformanceBaseline|PerformanceGateTest|SoftwareTourHeadlessTest"
        if ($LASTEXITCODE -ne 0) { throw "ctest failed ($LASTEXITCODE)" }
    } finally {
        Pop-Location
    }

    Write-Host "[prepush] OK - safe to push" -ForegroundColor Green
} finally {
    Pop-Location
}
