@echo off
setlocal enabledelayedexpansion

echo === QuantumVerse Windows Deployment ===

set BUILD_DIR=build_test
set DEPLOY_DIR=deploy\windows

REM 1. Clean deploy folder
if exist "%DEPLOY_DIR%" rmdir /s /q "%DEPLOY_DIR%"
mkdir "%DEPLOY_DIR%"

REM 2. Copy executable
copy "%BUILD_DIR%\Release\quantumverse_imgui.exe" "%DEPLOY_DIR%\"
if errorlevel 1 (
    echo ERROR: executable not found. Build first!
    pause
    exit /b 1
)

REM 3. Copy ONNX models for AI features
if exist "models" (
    xcopy "models" "%DEPLOY_DIR%\models\" /E /I /Y
    echo Models copied.
)

REM 4. Copy any required DLLs (if not statically linked)
REM Check for GLFW DLL
if exist "third_party\glfw\lib-vc2010-64\glfw3.dll" (
    copy "third_party\glfw\lib-vc2010-64\glfw3.dll" "%DEPLOY_DIR%\"
    echo glfw3.dll copied.
)

REM Check for ONNX Runtime DLL
if exist "third_party\onnxruntime\onnxruntime-win-x64-gpu-cuda12-1.18.1\onnxruntime-win-x64-gpu-1.18.1\lib\onnxruntime.dll" (
    copy "third_party\onnxruntime\onnxruntime-win-x64-gpu-cuda12-1.18.1\onnxruntime-win-x64-gpu-1.18.1\lib\onnxruntime.dll" "%DEPLOY_DIR%\"
    echo onnxruntime.dll copied.
)

REM 5. Copy the imgui.ini if you want a pre-configured layout (optional)
if exist "imgui.ini" copy "imgui.ini" "%DEPLOY_DIR%\"

echo.
echo Deployment completed to %DEPLOY_DIR%
echo Run: %DEPLOY_DIR%\quantumverse_imgui.exe
pause