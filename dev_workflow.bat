@echo off
REM Comprehensive development workflow script for QuantumVerse
REM This script provides a complete development workflow: clean, build, test, and run

@echo on
echo ================================================================
echo QuantumVerse Development Workflow
echo ================================================================
echo.

REM Set the build directory
set BUILD_DIR=dev_build

REM Parse command line arguments
set ACTION=%1
if "%ACTION%"=="" set ACTION=all

REM Function to show usage
:usage
echo Usage: dev_workflow.bat [action]
echo.
echo Actions:
echo   all       - Clean, build, test, and run (default)
echo   clean     - Clean build directory
echo   build     - Build only
echo   test      - Run tests only
echo   run       - Run the application only
echo   rebuild   - Clean and build
echo   ctest     - Run tests with detailed output
echo.
exit /b 1

REM Clean build directory
:clean
echo Cleaning build directory: %BUILD_DIR%
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
)
echo Build directory cleaned.
goto :eof

REM Configure and build
:build
if not exist "%BUILD_DIR%" (
    echo Creating build directory: %BUILD_DIR%
    mkdir "%BUILD_DIR%"
)
cd /d "%BUILD_DIR%"

echo Configuring QuantumVerse for development...
cmake .. ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_CXX_FLAGS_DEBUG="/Zi /Ob0 /Od /RTC1" ^
    -DQUANTUMVERSE_USE_QML=OFF ^
    -DQUANTUMVERSE_USE_IMGUI=ON ^
    -DQUANTUMVERSE_USE_ONNX=OFF

if errorlevel 1 (
    echo.
    echo ERROR: Configuration failed!
    exit /b 1
)

echo Building QuantumVerse (Debug)...
cmake --build . --config Debug -- /maxcpucount

if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    exit /b 1
)
goto :eof

REM Run tests
:test
if not exist "%BUILD_DIR%" (
    echo Build directory not found. Building first...
    call :build
)
cd /d "%BUILD_DIR%"

echo Running tests...
ctest -C Debug --output-on-failure

if errorlevel 1 (
    echo.
    echo WARNING: Some tests failed
) else (
    echo.
    echo SUCCESS: All tests passed!
)
goto :eof

REM Run tests with detailed output
:ctest
if not exist "%BUILD_DIR%" (
    echo Build directory not found. Building first...
    call :build
)
cd /d "%BUILD_DIR%"

echo Running tests with detailed output...
ctest -C Debug --verbose --output-on-failure

if errorlevel 1 (
    echo.
    echo WARNING: Some tests failed
) else (
    echo.
    echo SUCCESS: All tests passed!
)
goto :eof

REM Run the application
:run
if not exist "%BUILD_DIR%" (
    echo Build directory not found. Building first...
    call :build
)
cd /d "%BUILD_DIR%"

echo Running QuantumVerse ImGui version...
Debug\quantumverse_imgui.exe

if errorlevel 1 (
    echo.
    echo ERROR: Application failed to run!
)
goto :eof

REM Rebuild (clean then build)
:rebuild
call :clean
call :build
goto :eof

REM Default action: all
:all
call :clean
call :build
call :test
call :run
goto :eof

REM Handle unknown actions
if "%ACTION%"=="clean" goto :clean
if "%ACTION%"=="build" goto :build
if "%ACTION%"=="test" goto :test
if "%ACTION%"=="run" goto :run
if "%ACTION%"=="rebuild" goto :rebuild
if "%ACTION%"=="ctest" goto :ctest

echo Unknown action: %ACTION%
goto :usage