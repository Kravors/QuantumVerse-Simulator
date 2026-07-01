#!/bin/bash
# Build QuantumVerse in debug mode with sanitizers
# Usage: ./build-debug.sh [sanitizer]
#   sanitizer: ASAN, UBSAN, TSAN (default: ASAN)

SANITIZER=${1:-ASAN}

case $SANITIZER in
    ASAN)
        echo "Building with AddressSanitizer (static)..."
        cmake -B build_asan -DCMAKE_BUILD_TYPE=Debug \
            -DQUANTUMVERSE_USE_ASAN=ON \
            -DQUANTUMVERSE_ASAN_STATIC=ON \
            -DQUANTUMVERSE_BUILD_TESTS=ON
        ;;
    UBSAN)
        echo "Building with UndefinedBehaviorSanitizer..."
        cmake -B build_ubsan -DCMAKE_BUILD_TYPE=Debug \
            -DQUANTUMVERSE_USE_UBSAN=ON \
            -DQUANTUMVERSE_BUILD_TESTS=ON
        ;;
    TSAN)
        echo "Building with ThreadSanitizer..."
        cmake -B build_tsan -DCMAKE_BUILD_TYPE=Debug \
            -DQUANTUMVERSE_USE_TSAN=ON \
            -DQUANTUMVERSE_BUILD_TESTS=ON
        ;;
    *)
        echo "Unknown sanitizer: $SANITIZER"
        exit 1
        ;;
esac

cmake --build ${BUILD_DIR:-build_$SANITIZER} --parallel
echo "Build complete."