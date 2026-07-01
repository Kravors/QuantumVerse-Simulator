#!/bin/bash
# Task 2.1: Geodesic Neural ODE Surrogate - Complete Build & Run Pipeline
# This script builds the C++ pybind11 module, generates the dataset, and trains the model.
# Run in WSL (Windows Subsystem for Linux) from the project root: f:/syyyy

set -e  # Exit on error

echo "=========================================="
echo "Task 2.1: Geodesic Neural ODE Surrogate"
echo "=========================================="
echo ""

# Step 1: Check prerequisites
echo "Step 1: Checking prerequisites..."
if ! command -v g++ &> /dev/null; then
    echo "ERROR: g++ not found. Please run: sudo apt-get install -y build-essential"
    exit 1
fi
if ! command -v cmake &> /dev/null; then
    echo "ERROR: cmake not found. Please run: sudo apt-get install -y cmake"
    exit 1
fi
if ! command -v python3 &> /dev/null; then
    echo "ERROR: python3 not found. Please install Python 3."
    exit 1
fi
echo "✓ g++ $(g++ --version | head -1)"
echo "✓ cmake $(cmake --version | head -1)"
echo "✓ python3 $(python3 --version)"
echo ""

# Step 2: Install Python dependencies
echo "Step 2: Installing Python dependencies..."
sudo apt-get install -y python3-dev python3-pip > /dev/null 2>&1
pip3 install --upgrade pip > /dev/null 2>&1
pip3 install pybind11 numpy h5py torch onnx onnxruntime > /dev/null 2>&1
echo "✓ Python dependencies installed"
echo ""

# Step 3: Build C++ Python module
echo "Step 3: Building C++ geodesic module (_geodesic_cpp)..."
cd /mnt/f/syyyy

# Clean previous build
rm -f python/_geodesic_cpp*.so python/_geodesic_cpp*.pyd 2>/dev/null || true
rm -rf build_task2_1 2>/dev/null || true

# Build using pybind11's CMake (simplest)
mkdir -p build_task2_1
cd build_task2_1

# Find pybind11 config
PYBIND11_CMAKE_DIR=$(python3 -c "import pybind11; import os; print(os.path.join(os.path.dirname(pybind11.__file__), 'share', 'cmake', 'pybind11'))" 2>/dev/null)

if [ -d "$PYBIND11_CMAKE_DIR" ]; then
    echo "  Using pybind11 CMake config from: $PYBIND11_CMAKE_DIR"
    cmake .. -Dpybind11_DIR="$PYBIND11_CMAKE_DIR" -DPYTHON_EXECUTABLE=$(which python3)
else
    echo "  pybind11 CMake config not found, using direct g++ compilation..."
    cd ..
    # Direct compile without CMake
    PYTHON_INCLUDE=$(python3 -c "from sysconfig import get_paths; print(get_paths()['include'])" 2>/dev/null)
    PYBIND11_INCLUDE=$(python3 -c "import pybind11; print(pybind11.get_include())" 2>/dev/null)
    
    g++ -shared -std=c++17 -fPIC \
        -I"$PYTHON_INCLUDE" \
        -I"$PYBIND11_INCLUDE" \
        -I"/mnt/f/syyyy/src" \
        -I"/mnt/f/syyyy/src/physics" \
        -I"/mnt/f/syyyy/src/math" \
        -I"/mnt/f/syyyy/src/spacetime" \
        src/physics/geodesic_pybind.cpp \
        -o python/_geodesic_cpp.cpython-$(python3 -c "import sys; print(f'{sys.version_info.major}{sys.version_info.minor}')")-x86_64-linux-gnu.so 2>/dev/null \
        || g++ -shared -std=c++17 -fPIC \
            -I"$PYTHON_INCLUDE" \
            -I"$PYBIND11_INCLUDE" \
            -I"/mnt/f/syyyy/src" \
            src/physics/geodesic_pybind.cpp \
            -o python/_geodesic_cpp.so
    echo "✓ Module built: python/_geodesic_cpp.so"
    exit 0
fi

# If using CMake
cmake .. -Dpybind11_DIR="$PYBIND11_CMAKE_DIR" -DPYTHON_EXECUTABLE=$(which python3)
cmake --build . --target _geodesic_cpp --config Release

# Copy the built module to python/ directory
if [ -f "_geodesic_cpp*.so" ]; then
    cp _geodesic_cpp*.so ../python/ 2>/dev/null || true
fi
if [ -f "Release/_geodesic_cpp.pyd" ]; then
    cp Release/_geodesic_cpp.pyd ../python/ 2>/dev/null || true
fi

cd ..
echo "✓ Module built successfully"
echo ""

# Step 4: Test the module
echo "Step 4: Testing the module..."
python3 -c "import sys; sys.path.insert(0, 'python'); import _geodesic_cpp; print('  Module imported successfully'); print('  Available functions:', dir(_geodesic_cpp))"
echo ""

# Step 5: Generate dataset
echo "Step 5: Generating dataset (1M geodesics)..."
# Check if dataset already exists
if [ -f "data/geodesics_1M.h5" ]; then
    echo "  Dataset already exists at data/geodesics_1M.h5 - skipping generation"
else
    echo "  This will take 1-2 minutes with the C++ backend..."
    python3 python/datagen.py --output data/geodesics_1M.h5 --samples 1000000
    echo "✓ Dataset generated: data/geodesics_1M.h5"
fi
echo ""

# Step 6: Train the model
echo "Step 6: Training the neural network..."
if [ -d "models/geodesic_ode" ]; then
    echo "  Model directory already exists - skipping training (use --retrain to force)"
    python3 python/train.py --dataset data/geodesics_1M.h5 --output-dir models/geodesic_ode --epochs 50 --no-early-stop
else
    echo "  Training for 100 epochs (GPU recommended, else CPU ~6-12h)..."
    python3 python/train.py --dataset data/geodesics_1M.h5 --output-dir models/geodesic_ode --epochs 100
fi
echo "✓ Model trained: models/geodesic_ode/geodesic_ode.onnx"
echo ""

# Step 7: Summary
echo "=========================================="
echo "Task 2.1 Pipeline Complete!"
echo "=========================================="
echo ""
echo "Artifacts:"
echo "  - C++ Module: python/_geodesic_cpp.*.so"
echo "  - Dataset: data/geodesics_1M.h5"
echo "  - Model: models/geodesic_ode/geodesic_ode.onnx"
echo "  - Checkpoints: models/geodesic_ode/checkpoints/"
echo ""
echo "Next steps:"
echo "  1. Validate model accuracy (see TASK2_1_VALIDATION.md)"
echo "  2. Integrate ONNX Runtime into src/ml/GeodesicNeuralODE.cpp"
echo "  3. Build C++ tests: ctest -R neural_ode"
echo ""
