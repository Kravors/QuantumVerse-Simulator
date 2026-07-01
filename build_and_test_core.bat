@echo off
REM Build and test core libraries (excluding GUI, VR, and ONNX tests for faster iteration)
REM Assumes you are in the root of the QuantumVerse project

REM Set the build directory
set BUILD_DIR=build

REM Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

REM Change to build directory
cd /d "%BUILD_DIR%"

REM Configure the project if not already configured (check for CMakeCache.txt)
if not exist "CMakeCache.txt" (
    echo Configuring project...
    cmake .. -DCMAKE_BUILD_TYPE=Release
    if errorlevel 1 (
        echo Configuration failed!
        exit /b 1
    )
)

REM Build the project
echo Building project...
cmake --build . --config Release -- /maxcpucount
if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

REM Run tests, excluding GUI, VR, and ONNX tests (to speed up iteration)
echo Running core tests (excluding GUI, VR, ONNX)...
ctest -C Release --output-on-failure -E "(qml|imgui|vr|onnx)"
if errorlevel 1 (
    echo Some tests failed!
    exit /b 1
)

echo Build and test completed successfully!