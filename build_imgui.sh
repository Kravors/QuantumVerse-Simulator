#!/bin/bash
# Build script for QuantumVerse ImGui version (No Qt, No DLLs)

set -e

echo "Building QuantumVerse ImGui 4D GUI (No Qt, No DLLs)..."
echo

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found. Please run this script from the project root directory."
    exit 1
fi

# Check for Glad OpenGL loader
if [ ! -f "third_party/glad/glad.c" ]; then
    echo "Error: Glad OpenGL loader not found in third_party/glad/"
    echo "Please ensure dependencies are available."
    exit 1
fi

# Create build directory for ImGui version
mkdir -p build_imgui
cd build_imgui

# Configure with ImGui enabled, Qt disabled
echo "Configuring QuantumVerse ImGui version..."
cmake .. -DQUANTUMVERSE_USE_IMGUI=ON -DQUANTUMVERSE_USE_QT=OFF

if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed."
    echo
    echo "Possible issues:"
    echo "  - CMake version too old (requires CMake 3.16+)."
    echo "  - Missing OpenGL development libraries."
    echo "  - GLFW not found (should be bundled or system-installed)."
    echo
    echo "Check the output above for more details."
    exit 1
fi

# Build the project
echo "Building QuantumVerse ImGui..."
cmake --build . --config Release

if [ $? -ne 0 ]; then
    echo "Error: Build failed."
    exit 1
fi

echo
echo "Build completed successfully!"
echo "The executable is located at: $(pwd)/quantumverse_imgui"
echo
echo "Key features:"
echo "  - Zero DLL dependencies (except graphics driver)"
echo "  - Self-contained executable"
echo "  - OpenGL 4.5 required"
echo
exit 0
