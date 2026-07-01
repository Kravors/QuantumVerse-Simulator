@echo off
REM Optimized build script for QuantumVerse with timing and progress tracking
REM This script provides workflow improvements for faster development cycles

@echo on
echo ================================================================
echo QuantumVerse Optimized Build Script
echo ================================================================
echo.

REM Record start time
set START_TIME=%TIME%
echo Build started at: %START_TIME%
echo.

REM Set the build directory
set BUILD_DIR=build_optimized

REM Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    echo Creating build directory: %BUILD_DIR%
    mkdir "%BUILD_DIR%"
) else (
    echo Using existing build directory: %BUILD_DIR%
)

REM Change to build directory
cd /d "%BUILD_DIR%"

REM Show CMake configuration options
echo.
echo Configuring QuantumVerse with optimized settings...
echo.

REM Configure with optimizations for faster builds
cmake .. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_CXX_FLAGS_RELEASE="/O2 /Ob2 /DNDEBUG" ^
    -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=TRUE ^
    -DUSE_HOST_CPU_FOR_SIMD=ON ^
    -DQUANTUMVERSE_USE_QML=OFF ^
    -DQUANTUMVERSE_USE_IMGUI=ON ^
    -DQUANTUMVERSE_USE_ONNX=OFF

if errorlevel 1 (
    echo.
    echo ERROR: Configuration failed!
    exit /b 1
)

REM Build with maximum parallelism and show progress
echo.
echo Building QuantumVerse with maximum parallelism...
echo.
cmake --build . --config Release -- /maxcpucount /verbosity:minimal

if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    exit /b 1
)

REM Record end time and calculate duration
set END_TIME=%TIME%
echo.
echo Build completed at: %END_TIME%

REM Run a subset of tests for quick validation
echo.
echo Running quick validation tests...
ctest -C Release --output-on-failure -R "^(test_spacetime|test_physics|test_rendering|test_ui4d)$"

if errorlevel 1 (
    echo.
    echo WARNING: Some validation tests failed, but build succeeded
) else (
    echo.
    echo SUCCESS: All validation tests passed!
)

echo.
echo ================================================================
echo Build Summary:
echo   Start time: %START_TIME%
echo   End time:   %END_TIME%
echo   Build directory: %BUILD_DIR%
echo ================================================================