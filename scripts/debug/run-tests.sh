#!/bin/bash
# Run tests with sanitizer output
# Usage: ./run-tests.sh [build_dir] [test_pattern]

BUILD_DIR=${1:-build_asan}
TEST_PATTERN=${2:-".*"}

echo "Running tests from $BUILD_DIR..."

cd $BUILD_DIR
ctest -R "$TEST_PATTERN" --output-on-failure -C Debug
cd ..

echo "Tests complete."