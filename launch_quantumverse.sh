#!/bin/bash
# =============================================================================
# QuantumVerse Simulator - Launcher Script
# =============================================================================
# This script builds and launches the QuantumVerse Simulator
# A 3D real-time simulation of the Solar System in 4D spacetime
# =============================================================================

# --- Colors ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[0;97m'
RESET='\033[0m'

# --- Configuration ---
PROJECT_NAME="QuantumVerse"
BUILD_DIR="build"
SOURCE_DIR="$(pwd)"
INSTALL_DIR="${SOURCE_DIR}/install"

# --- Functions ---
print_header() {
    echo
    echo -e "${CYAN}=================================================${RESET}"
    echo -e "${CYAN}  QuantumVerse Simulator - Launcher${RESET}"
    echo -e "${CYAN}  A 4D Spacetime Simulation Engine${RESET}"
    echo -e "${CYAN}=================================================${RESET}"
    echo
}

check_tools() {
    echo -e "${YELLOW}[INFO]${RESET} Checking build tools..."

    # Check for CMake
    if ! command -v cmake &> /dev/null; then
        echo -e "${RED}[ERROR]${RESET} CMake not found. Please install CMake 3.16+${RESET}"
        echo "        Download from: https://cmake.org/download/"
        return 1
    fi
    echo -e "${GREEN}[OK]${RESET} CMake found"

    # Check for C++ compiler
    if command -v g++ &> /dev/null; then
        echo -e "${GREEN}[OK]${RESET} GCC compiler found"
        COMPILER="GCC"
    elif command -v clang++ &> /dev/null; then
        echo -e "${GREEN}[OK]${RESET} Clang compiler found"
        COMPILER="CLANG"
    elif command -v clang &> /dev/null; then
        echo -e "${GREEN}[OK]${RESET} Clang compiler found"
        COMPILER="CLANG"
    else
        echo -e "${RED}[ERROR]${RESET} No C++ compiler found.${RESET}"
        echo "        Please install one of: GCC or Clang"
        echo "        - GCC:  sudo apt-get install g++ (Ubuntu/Debian)"
        echo "                sudo yum install gcc-c++ (RHEL/CentOS)"
        echo "                brew install gcc (macOS)"
        echo "        - Clang: sudo apt-get install clang (Ubuntu/Debian)"
        echo "                brew install llvm (macOS)"
        return 1
    fi

    return 0
}

configure() {
    echo
    echo -e "${YELLOW}[INFO]${RESET} Configuring build..."

    # Create build directory
    if [ ! -d "${BUILD_DIR}" ]; then
        mkdir -p "${BUILD_DIR}"
    fi

    cd "${BUILD_DIR}" || return 1

    # Configure with CMake
    local CMAKE_OPTS="-DCMAKE_BUILD_TYPE=Release"
    CMAKE_OPTS="${CMAKE_OPTS} -DQUANTUMVERSE_BUILD_TESTS=ON"
    CMAKE_OPTS="${CMAKE_OPTS} -DQUANTUMVERSE_BUILD_EXAMPLES=ON"
    CMAKE_OPTS="${CMAKE_OPTS} -DQUANTUMVERSE_USE_OPENGL=ON"

    # Check for Qt
    if command -v qmake &> /dev/null; then
        CMAKE_OPTS="${CMAKE_OPTS} -DQUANTUMVERSE_USE_QT=ON"
        echo -e "${GREEN}[OK]${RESET} Qt found - GUI enabled"
    else
        CMAKE_OPTS="${CMAKE_OPTS} -DQUANTUMVERSE_USE_QT=OFF"
        echo -e "${YELLOW}[WARN]${RESET} Qt not found - Building without GUI"
    fi

    # Check for CUDA
    if command -v nvcc &> /dev/null; then
        CMAKE_OPTS="${CMAKE_OPTS} -DQUANTUMVERSE_USE_CUDA=ON"
        echo -e "${GREEN}[OK]${RESET} CUDA found - GPU acceleration enabled"
    else
        CMAKE_OPTS="${CMAKE_OPTS} -DQUANTUMVERSE_USE_CUDA=OFF"
        echo -e "${YELLOW}[WARN]${RESET} CUDA not found - Building without GPU acceleration"
    fi

    cmake ${CMAKE_OPTS} ..
    if [ $? -ne 0 ]; then
        echo -e "${RED}[ERROR]${RESET} CMake configuration failed${RESET}"
        cd ..
        return 1
    fi

    echo -e "${GREEN}[OK]${RESET} Configuration complete"
    cd ..
    return 0
}

build() {
    echo
    echo -e "${YELLOW}[INFO]${RESET} Building QuantumVerse Simulator..."

    cd "${BUILD_DIR}" || return 1

    # Build with CMake
    cmake --build . --config Release --parallel
    if [ $? -ne 0 ]; then
        echo -e "${RED}[ERROR]${RESET} Build failed${RESET}"
        cd ..
        return 1
    fi

    echo -e "${GREEN}[OK]${RESET} Build complete"
    cd ..
    return 0
}

run() {
    echo
    echo -e "${YELLOW}[INFO]${RESET} Launching QuantumVerse Simulator..."
    echo

    cd "${BUILD_DIR}" || return 1

    if [ -f "quantumverse" ]; then
        echo -e "${GREEN}[INFO]${RESET} Starting simulation..."
        echo
        ./quantumverse
    elif [ -f "quantumverse.exe" ]; then
        echo -e "${GREEN}[INFO]${RESET} Starting simulation..."
        echo
        ./quantumverse.exe
    else
        echo -e "${RED}[ERROR]${RESET} Executable not found${RESET}"
        cd ..
        return 1
    fi

    cd ..
    return 0
}

test_suite() {
    echo
    echo -e "${YELLOW}[INFO]${RESET} Running test suite..."
    echo

    cd "${BUILD_DIR}" || return 1

    # Run CTest
    ctest --output-on-failure
    if [ $? -ne 0 ]; then
        echo -e "${YELLOW}[WARN]${RESET} Some tests failed${RESET}"
    else
        echo -e "${GREEN}[OK]${RESET} All tests passed${RESET}"
    fi

    cd ..
    return 0
}

clean() {
    echo
    echo -e "${YELLOW}[INFO]${RESET} Cleaning build directory..."

    if [ -d "${BUILD_DIR}" ]; then
        rm -rf "${BUILD_DIR}"
        echo -e "${GREEN}[OK]${RESET} Build directory cleaned${RESET}"
    else
        echo -e "${YELLOW}[WARN]${RESET} Build directory does not exist${RESET}"
    fi

    return 0
}

help() {
    echo
    echo "Usage: ./launch_quantumverse.sh [COMMAND]"
    echo
    echo "Commands:"
    echo "  build    - Configure and build the project"
    echo "  run      - Build and run the simulator"
    echo "  test     - Run the test suite"
    echo "  clean    - Remove build directory"
    echo "  install  - Install the simulator"
    echo "  help     - Show this help message"
    echo
    echo "Examples:"
    echo "  ./launch_quantumverse.sh build  - Build the project"
    echo "  ./launch_quantumverse.sh run    - Build and run"
    echo "  ./launch_quantumverse.sh test   - Run tests"
    echo
}

install_sim() {
    echo
    echo -e "${YELLOW}[INFO]${RESET} Installing QuantumVerse Simulator..."

    cd "${BUILD_DIR}" || return 1

    cmake --install . --prefix "${INSTALL_DIR}"
    if [ $? -ne 0 ]; then
        echo -e "${RED}[ERROR]${RESET} Installation failed${RESET}"
        cd ..
        return 1
    fi

    echo -e "${GREEN}[OK]${RESET} Installation complete"
    echo -e "${GREEN}[INFO]${RESET} Installed to: ${INSTALL_DIR}"

    cd ..
    return 0
}

# --- Main Script ---
print_header

# Parse command line argument
COMMAND=${1:-help}

case "${COMMAND}" in
    build)
        check_tools || exit 1
        configure || exit 1
        build || exit 1
        echo
        echo -e "${GREEN}=================================================${RESET}"
        echo -e "${GREEN}  Build Complete!${RESET}"
        echo -e "${GREEN}  Run: ./launch_quantumverse.sh run${RESET}"
        echo -e "${GREEN}=================================================${RESET}"
        ;;
    run)
        check_tools || exit 1
        configure || exit 1
        build || exit 1
        run || exit 1
        ;;
    test)
        check_tools || exit 1
        configure || exit 1
        build || exit 1
        test_suite || exit 1
        ;;
    clean)
        clean || exit 1
        ;;
    install)
        check_tools || exit 1
        configure || exit 1
        build || exit 1
        install_sim || exit 1
        ;;
    help|--help|-h)
        help
        ;;
    *)
        echo -e "${RED}[ERROR]${RESET} Unknown command: ${COMMAND}"
        echo
        help
        exit 1
        ;;
esac

exit 0