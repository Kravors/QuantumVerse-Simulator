#!/bin/bash
# Build script for QuantumVerse Phase 1 (Quantum Foundation)
# This script configures and builds the project with quantum gravity support

set -e

echo "=== QuantumVerse Phase 1 Build Script ==="
echo ""

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DQUANTUMVERSE_ENABLE_QUANTUM_GRAVITY=ON -DCMAKE_BUILD_TYPE=Release

# Build
echo ""
echo "Building project..."
cmake --build . --config Release --parallel $(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Run tests
echo ""
echo "Running tests..."
ctest -R "quantum_gravity_tests|theory_plugins_tests" -V

echo ""
echo "=== Build complete ==="