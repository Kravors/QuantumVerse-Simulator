@echo off
REM Build QuantumVerse in debug mode with sanitizers
REM Usage: build-debug.bat [sanitizer]
REM   sanitizer: ASAN, UBSAN, TSAN (default: ASAN)

set SANITIZER=%1
if "%SANITIZER%"=="" set SANITIZER=ASAN

if /I "%SANITIZER%"=="ASAN" (
    echo Building with AddressSanitizer (static)...
    cmake -B build_asan -DCMAKE_BUILD_TYPE=Debug -DQUANTUMVERSE_USE_ASAN=ON -DQUANTUMVERSE_ASAN_STATIC=ON -DQUANTUMVERSE_BUILD_TESTS=ON
) else if /I "%SANITIZER%"=="UBSAN" (
    echo Building with UndefinedBehaviorSanitizer...
    cmake -B build_ubsan -DCMAKE_BUILD_TYPE=Debug -DQUANTUMVERSE_USE_UBSAN=ON -DQUANTUMVERSE_BUILD_TESTS=ON
) else if /I "%SANITIZER%"=="TSAN" (
    echo Building with ThreadSanitizer...
    cmake -B build_tsan -DCMAKE_BUILD_TYPE=Debug -DQUANTUMVERSE_USE_TSAN=ON -DQUANTUMVERSE_BUILD_TESTS=ON
) else (
    echo Unknown sanitizer: %SANITIZER%
    exit /b 1
)

cmake --build build_asan --config Debug --parallel
echo Build complete.