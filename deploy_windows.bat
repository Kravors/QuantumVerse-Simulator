@echo off
REM QuantumVerse Windows Deployment
REM Targets the CMake build directory (build/Release or build/Debug)

setlocal enabledelayedexpansion

echo === QuantumVerse v3.10.2 Windows Deployment ===

set BUILD_TYPE=Release
if not exist "build\%BUILD_TYPE%\quantumverse_qml.exe" (
    if exist "build\Debug\quantumverse_qml.exe" (
        set BUILD_TYPE=Debug
    ) else (
        echo ERROR: No build found in build/Release or build/Debug. Build first!
        pause
        exit /b 1
    )
)
echo Build type: %BUILD_TYPE%

set DEPLOY_DIR=deploy\windows

REM 1. Clean deploy folder
if exist "%DEPLOY_DIR%" rmdir /s /q "%DEPLOY_DIR%"
mkdir "%DEPLOY_DIR%"

REM 2. Copy executables
echo Copying executables...
copy "build\%BUILD_TYPE%\quantumverse_qml.exe" "%DEPLOY_DIR%\" >nul
echo   quantumverse_qml.exe copied.

REM 3. Copy Qt plugins (platforms, imageformats)
echo Deploying Qt plugins...
if exist "build\%BUILD_TYPE%\plugins" (
    xcopy "build\%BUILD_TYPE%\plugins" "%DEPLOY_DIR%\plugins\" /E /I /Y >nul
    echo   Qt plugins copied.
)

REM 4. Copy QML modules if present
if exist "build\%BUILD_TYPE%\qml" (
    xcopy "build\%BUILD_TYPE%\qml" "%DEPLOY_DIR%\qml\" /E /I /Y >nul
    echo   QML modules copied.
)

REM 5. Copy shader files
if exist "data\shaders" (
    if not exist "%DEPLOY_DIR%\shaders" mkdir "%DEPLOY_DIR%\shaders"
    xcopy "data\shaders" "%DEPLOY_DIR%\shaders\" /E /I /Y >nul
    echo   Shaders copied.
)

REM 6. Copy icons for QML
if not exist "%DEPLOY_DIR%\icons" mkdir "%DEPLOY_DIR%\icons"
if exist "src\icons\*.svg" (
    copy "src\icons\*.svg" "%DEPLOY_DIR%\icons\" >nul
    echo   Icons copied.
)

REM 7. Copy qt.conf
echo [Paths] > "%DEPLOY_DIR%\qt.conf"
echo Prefix = . >> "%DEPLOY_DIR%\qt.conf"
echo Plugins = plugins >> "%DEPLOY_DIR%\qt.conf"
echo Qml2Imports = qml >> "%DEPLOY_DIR%\qt.conf"

echo.
echo Deployment completed to %DEPLOY_DIR%
echo Run: %DEPLOY_DIR%\quantumverse_qml.exe
echo ========================================
pause