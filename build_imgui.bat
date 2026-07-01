@echo off
echo Building QuantumVerse ImGui 4D GUI (No Qt, No DLLs)...
echo.

REM Check if we're in the right directory
if not exist "CMakeLists.txt" (
    echo Error: CMakeLists.txt not found. Please run this script from the project root directory.
    exit /b 1
)

REM Check for GLFW and Glad (should be in third_party)
if not exist "third_party\glad\glad.cpp" (
    echo Error: Glad OpenGL loader not found in third_party/glad/
    echo Please ensure dependencies are available.
    exit /b 1
)

REM Create build directory for ImGui version
if not exist "build_imgui" (
    mkdir build_imgui
    if %errorlevel% neq 0 (
        echo Error: Failed to create build_imgui directory.
        exit /b 1
    )
)

REM Clean previous build cache
if exist "build_imgui\CMakeCache.txt" (
    echo Cleaning previous build cache...
    del /q "build_imgui\CMakeCache.txt"
)
if exist "build_imgui\CMakeFiles" (
    rmdir /s /q "build_imgui\CMakeFiles"
)

cd build_imgui
if %errorlevel% neq 0 (
    echo Error: Failed to change to build_imgui directory.
    exit /b 1
)

REM Configure with ImGui enabled, Qt disabled
echo Configuring QuantumVerse ImGui version...
cmake .. -DQUANTUMVERSE_USE_IMGUI=ON -DQUANTUMVERSE_USE_QT=OFF -G "Visual Studio 17 2022" -A x64

REM Check if configuration succeeded
if %errorlevel% neq 0 (
    echo Error: CMake configuration failed.
    echo.
    echo Possible issues:
    echo   - CMake version too old (requires CMake 3.16+).
    echo   - Missing OpenGL development libraries.
    echo   - GLFW not found (should be bundled or system-installed).
    echo.
    echo Check the output above for more details.
    exit /b 1
)

REM Build the project
echo Building QuantumVerse ImGui...
cmake --build . --config Release

REM Check if build succeeded
if %errorlevel% neq 0 (
    echo Error: Build failed.
    exit /b 1
)

echo.
echo Build completed successfully!
echo The executable is located at: %cd%\Release\quantumverse_imgui.exe
echo.
echo Key features:
echo   - Zero DLL dependencies (except graphics driver)
echo   - Self-contained executable
echo   - OpenGL 4.5 required
echo.
exit /b 0