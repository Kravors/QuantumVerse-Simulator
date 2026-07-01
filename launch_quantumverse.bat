@echo off
setlocal

echo.
echo =================================================
echo   QuantumVerse Simulator - Launcher
echo   A 4D Spacetime Simulation Engine
echo =================================================
echo.

set BUILD_DIR=build
set COMMAND=%1
if "%COMMAND%"=="" set COMMAND=help

if /i "%COMMAND%"=="help" goto help
if /i "%COMMAND%"=="clean" goto clean
if /i "%COMMAND%"=="build" goto build
if /i "%COMMAND%"=="run" goto run
if /i "%COMMAND%"=="test" goto test
if /i "%COMMAND%"=="install" goto install
goto help

:help
echo Usage: launch_quantumverse [COMMAND]
echo.
echo Commands:
echo   build    - Configure and build the project
echo   run      - Build and run the simulator
echo   test     - Run the test suite
echo   clean    - Remove build directory
echo   install  - Install the simulator
echo   help     - Show this help message
echo.
echo Examples:
echo   launch_quantumverse build  - Build the project
echo   launch_quantumverse run    - Build and run
echo   launch_quantumverse test   - Run tests
echo.
goto end

:clean
echo [INFO] Cleaning build directory...
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
    echo [OK] Build directory cleaned
) else (
    echo [WARN] Build directory does not exist
)
goto end

:build
call :configure
if errorlevel 1 goto end
call :build_only
if errorlevel 1 goto end
echo.
echo =================================================
echo   Build Complete!
echo   Run: launch_quantumverse run
echo =================================================
goto end

:run
call :configure
if errorlevel 1 goto end
call :build_only
if errorlevel 1 goto end
call :run_only
goto end

:test
call :configure
if errorlevel 1 goto end
call :build_only
if errorlevel 1 goto end
call :test_only
goto end

:install
call :configure
if errorlevel 1 goto end
call :build_only
if errorlevel 1 goto end
call :install_only
goto end

:configure
echo [INFO] Configuring build...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"
set CMAKE_OPTS=-DCMAKE_BUILD_TYPE=Release
set CMAKE_OPTS=%CMAKE_OPTS% -DQUANTUMVERSE_USE_IMGUI=ON
set CMAKE_OPTS=%CMAKE_OPTS% -DQUANTUMVERSE_USE_QML=OFF
cmake %CMAKE_OPTS% ..
if errorlevel 1 (
    echo [ERROR] CMake configuration failed
    cd ..
    exit /b 1
)
echo [OK] Configuration complete
cd ..
exit /b 0

:build_only
cd "%BUILD_DIR%"
echo [INFO] Building QuantumVerse Simulator...
cmake --build . --config Release --parallel
if errorlevel 1 (
    echo [ERROR] Build failed
    cd ..
    exit /b 1
)
echo [OK] Build complete
cd ..
exit /b 0

:run_only
cd "%BUILD_DIR%"
echo [INFO] Launching QuantumVerse Simulator...
echo [INFO] Forcing NVIDIA OpenGL to BLT presentation via __GL_FlipModel=0
set __GL_FlipModel=0
echo.
if exist "quantumverse.exe" (
    quantumverse.exe
) else if exist "quantumverse" (
    ./quantumverse
) else (
    echo [ERROR] Executable not found
    cd ..
    exit /b 1
)
cd ..
exit /b 0

:test_only
cd "%BUILD_DIR%"
echo [INFO] Running test suite...
echo.
ctest --output-on-failure
if errorlevel 1 (
    echo [WARN] Some tests failed
) else (
    echo [OK] All tests passed
)
cd ..
exit /b 0

:install_only
cd "%BUILD_DIR%"
echo [INFO] Installing QuantumVerse Simulator...
cmake --install . --prefix "%~dp0install"
if errorlevel 1 (
    echo [ERROR] Installation failed
    cd ..
    exit /b 1
)
echo [OK] Installation complete
echo [INFO] Installed to: %~dp0install
cd ..
exit /b 0

:end
endlocal
exit /b 0