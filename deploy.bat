@echo off
REM QuantumVerse Deployment Script
REM Version: 3.7.2

echo ========================================
echo QuantumVerse v3.7.2 Deployment
echo ========================================

set QT_DIR=F:\qt\6.11.1\msvc2022_64
set BUILD_DIR=build
set DEPLOY_DIR=deploy\windows

REM Detect build type
if exist "%BUILD_DIR%\Release\quantumverse_qml.exe" (
    set BUILD_TYPE=Release
) else (
    set BUILD_TYPE=Debug
)
echo Build type: %BUILD_TYPE%

REM 1. Clean deploy folder
if exist "%DEPLOY_DIR%" rmdir /s /q "%DEPLOY_DIR%"
mkdir "%DEPLOY_DIR%"

REM 2. Copy executables
echo Copying executables...
if exist "%BUILD_DIR%\%BUILD_TYPE%\quantumverse_qml.exe" (
    copy "%BUILD_DIR%\%BUILD_TYPE%\quantumverse_qml.exe" "%DEPLOY_DIR%\" >nul
    echo   quantumverse_qml.exe copied.
)
if exist "%BUILD_DIR%\%BUILD_TYPE%\quantumverse_imgui.exe" (
    copy "%BUILD_DIR%\%BUILD_TYPE%\quantumverse_imgui.exe" "%DEPLOY_DIR%\" >nul
    echo   quantumverse_imgui.exe copied.
)

REM 3. Use windeployqt to copy Qt dependencies automatically
echo Deploying Qt dependencies...
if exist "%QT_DIR%\bin\windeployqt.exe" (
    if exist "%DEPLOY_DIR%\quantumverse_qml.exe" (
        "%QT_DIR%\bin\windeployqt.exe" --%BUILD_TYPE% --no-translations --no-system-d3d-compiler --compiler-runtime --qmldir "%~dp0src" "%DEPLOY_DIR%\quantumverse_qml.exe"
    )
    if exist "%DEPLOY_DIR%\quantumverse_imgui.exe" (
        "%QT_DIR%\bin\windeployqt.exe" --%BUILD_TYPE% --no-translations --no-system-d3d-compiler --compiler-runtime "%DEPLOY_DIR%\quantumverse_imgui.exe"
    )
    echo Qt dependencies deployed.
) else (
    echo WARNING: windeployqt not found at %QT_DIR%\bin\windeployqt.exe
    echo Please run windeployqt manually or install Qt 6.11.1.
)

REM 4. Copy ONNX models for AI features
if exist "models" (
    xcopy "models" "%DEPLOY_DIR%\models\" /E /I /Y >nul
    echo Models copied.
)

REM 4b. Remove ANGLE software renderer if present to force native OpenGL
if exist "%DEPLOY_DIR%\opengl32sw.dll" (
    del /f "%DEPLOY_DIR%\opengl32sw.dll"
    echo Removed opengl32sw.dll to force native OpenGL.
)

REM 5. Copy icon files for QML (required for qrc:/icons/ paths)
if not exist "%DEPLOY_DIR%\icons" mkdir "%DEPLOY_DIR%\icons"
if exist "src\icons\open.svg" copy "src\icons\open.svg" "%DEPLOY_DIR%\icons\" >nul
if exist "src\icons\step_back.svg" copy "src\icons\step_back.svg" "%DEPLOY_DIR%\icons\" >nul
if exist "src\icons\play.svg" copy "src\icons\play.svg" "%DEPLOY_DIR%\icons\" >nul
if exist "src\icons\pause.svg" copy "src\icons\pause.svg" "%DEPLOY_DIR%\icons\" >nul
if exist "src\icons\step_forward.svg" copy "src\icons\step_forward.svg" "%DEPLOY_DIR%\icons\" >nul
if exist "src\icons\stop.svg" copy "src\icons\stop.svg" "%DEPLOY_DIR%\icons\" >nul

REM 6. Copy QML module files
if not exist "%DEPLOY_DIR%\qml\QuantumVerse" mkdir "%DEPLOY_DIR%\qml\QuantumVerse"
if exist "src\qmldir" copy "src\qmldir" "%DEPLOY_DIR%\qml\QuantumVerse\" >nul
if exist "src\main.qml" copy "src\main.qml" "%DEPLOY_DIR%\" >nul

REM 7. Copy Qt platform plugins explicitly (windeployqt usually handles this,
REM    but we ensure it for robustness in case of plugin path issues)
if exist "%DEPLOY_DIR%\plugins\platforms" (
    echo Platform plugins present.
) else if exist "%QT_DIR%\plugins\platforms" (
    if not exist "%DEPLOY_DIR%\plugins" mkdir "%DEPLOY_DIR%\plugins"
    xcopy "%QT_DIR%\plugins\platforms" "%DEPLOY_DIR%\plugins\platforms\" /E /I /Y >nul
    echo Platform plugins copied from Qt installation.
)

REM 8. Copy MSVC runtime DLLs (windeployqt --compiler-runtime should handle this,
REM    but we keep these fallbacks for older deployments)
echo Checking MSVC runtime DLLs...
where /q dumpbin
if %errorlevel% equ 0 (
    echo Note: dumpbin found - you can use 'dumpbin /dependents quantumverse_qml.exe' to check missing DLLs.
)

REM 9. Copy resources.qrc if present
if exist "src\resources.qrc" (
    echo Copying resources.qrc...
    copy "src\resources.qrc" "%DEPLOY_DIR%\" >nul
)

REM 10. Create qt.conf to ensure Qt finds its plugins relative to the executable
echo [Paths] > "%DEPLOY_DIR%\qt.conf"
echo Prefix = . >> "%DEPLOY_DIR%\qt.conf"
echo Plugins = plugins >> "%DEPLOY_DIR%\qt.conf"
echo Qml2Imports = qml >> "%DEPLOY_DIR%\qt.conf"
echo Created qt.conf

REM List deployed files
echo.
echo Deployment complete!
echo Files in deploy\windows\:
dir "%DEPLOY_DIR%" /B

echo.
echo To run QML: deploy\windows\quantumverse_qml.exe
echo To run ImGui: deploy\windows\quantumverse_imgui.exe
echo ========================================
pause