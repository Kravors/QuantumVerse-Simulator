@echo off
REM Run tests with sanitizer output
REM Usage: run-tests.bat [build_dir] [test_pattern]

set BUILD_DIR=%1
if "%BUILD_DIR%"=="" set BUILD_DIR=build_asan

set TEST_PATTERN=%2
if "%TEST_PATTERN%"=="" set TEST_PATTERN=.*

echo Running tests from %BUILD_DIR%...

cd %BUILD_DIR%
ctest -R "%TEST_PATTERN%" --output-on-failure -C Debug
cd ..

echo Tests complete.