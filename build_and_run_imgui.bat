@echo off
echo ================================================
echo QuantumVerse ImGui - Build and Run
 echo ================================================
echo.

REM Check if we're in the right directory
if not exist "CMakeLists.txt" (
    echo Error: CMakeLists.txt not found.
    echo Please run this script from the project root directory.
    pause
    exit /b 1
)

REM Check for Glad OpenGL loader
if not exist "third_party/glad/glad.c" (
    echo Error: Glad OpenGL loader not found in third_party/glad/
    echo Please ensure dependencies are available.
    pause
    exit /b 1
)

REM Create build directory for ImGui version
if not exist "build_imgui" (
    echo Creating build_imgui directory...
    mkdir build_imgui
)

cd build_imgui
if %errorlevel% neq 0 (
    echo Error: Failed to change to build_imgui directory.
    pause
    exit /b 1
)

REM Configure with ImGui enabled, Qt disabled
echo.
echo [1/3] Configuring QuantumVerse ImGui version...
echo       (CMake -DQUANTUMVERSE_USE_IMGUI=ON -DQUANTUMVERSE_USE_QT=OFF)
echo.
cmake .. -DQUANTUMVERSE_USE_IMGUI=ON -DQUANTUMVERSE_USE_QT=OFF -G "Visual Studio 18 2026" -A x64

if %errorlevel% neq 0 (
    echo.
    echo Error: CMake configuration failed.
    echo.
    echo Possible issues:
    echo   - CMake version too old (requires CMake 3.16+)
    echo   - Missing Visual Studio 2022 with C++ support
    echo   - Missing OpenGL development libraries
    echo.
    echo Check the output above for more details.
    pause
    exit /b 1
)

REM Build the project
echo.
echo [2/3] Building QuantumVerse ImGui...
echo       (This may take a few minutes...)
echo.
cmake --build . --config Release

if %errorlevel% neq 0 (
    echo.
    echo Error: Build failed.
    echo.
    echo Check the output above for compilation errors.
    pause
    exit /b 1
)

echo.
echo [3/3] Build completed successfully!
echo.
echo Executable: %cd%\Release\quantumverse_imgui.exe
echo.
echo ================================================
echo Starting QuantumVerse ImGui...
echo ================================================
echo.

cd Release
if exist "quantumverse_imgui.exe" (
    quantumverse_imgui.exe
) else (
    echo Error: quantumverse_imgui.exe not found!
    pause
    exit /b 1
)

echo.
echo Application has exited.
echo.
pause
exit /b 0