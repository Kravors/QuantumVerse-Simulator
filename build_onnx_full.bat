@echo off
echo ============================================
echo ONNX Runtime CUDA Build - Full Reconfigure + Build
echo ============================================
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
if %ERRORLEVEL% neq 0 (
    echo ERROR: vcvarsall.bat failed
    exit /b 1
)
echo [1/2] Reconfiguring CMake with explicit CUDA compiler...
"C:\Program Files\CMake\bin\cmake.exe" "F:\syyyy\third_party\onnxruntime_src" ^
    -G "Ninja" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_CUDA_COMPILER="C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.5\bin\nvcc.exe" ^
    -DCMAKE_CUDA_HOST_COMPILER="C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe" ^
    -Donnxruntime_USE_CUDA=ON ^
    -Donnxruntime_CUDA_MINIMAL=ON ^
    -Donnxruntime_BUILD_UNIT_TESTS=OFF ^
    -Donnxruntime_BUILD_PYTHON=OFF ^
    -Donnxruntime_BUILD_BENCHMARKS=OFF ^
    -Donnxruntime_BUILD_EXAMPLES=OFF ^
    -B "F:\syyyy\third_party\onnxruntime_src\build_ninja"
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configure failed
    exit /b 1
)
echo [2/2] Building onnxruntime_providers_cuda...
F:\syyyy\ninja.exe -C "F:\syyyy\third_party\onnxruntime_src\build_ninja" onnxruntime_providers_cuda
echo BUILD_EXIT_CODE=%ERRORLEVEL%
if %ERRORLEVEL% equ 0 (
    echo.
    echo SUCCESS: Build complete. DLLs at:
    echo   F:\syyyy\third_party\onnxruntime_src\build_ninja\bin\
) else (
    echo ERROR: Build failed
)
exit /b %ERRORLEVEL%
