# scripts/prepush.ps1
# Clean Release build + tests. Run before `git push`.
# Wipes the build tree so incremental caching can't hide compile errors.

$ErrorActionPreference = 'Stop'

Push-Location $PSScriptRoot\..
try {
    Write-Host "[prepush] Wiping build_ci..." -ForegroundColor Cyan
    Remove-Item -Recurse -Force build_ci -ErrorAction SilentlyContinue

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
            -E "RenderingDiagnosticTest|ViewportStateTest|ViewportContentTest|VisualRegressionTest|AnimationTimingTest|QMLPerformanceBaseline|PerformanceGateTest"
        if ($LASTEXITCODE -ne 0) { throw "ctest failed ($LASTEXITCODE)" }
    } finally {
        Pop-Location
    }

    Write-Host "[prepush] OK - safe to push" -ForegroundColor Green
} finally {
    Pop-Location
}
