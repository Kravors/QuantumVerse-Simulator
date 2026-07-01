@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set "CMAKE_C_COMPILER=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe"
set "CMAKE_CXX_COMPILER=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe"
set "CMAKE_CUDA_COMPILER=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.5\bin\nvcc.exe"
set "CMAKE_CUDA_HOST_COMPILER=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe"
set "CUDAToolkit_ROOT=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.5"
set "PATH=F:\syyyy;%PATH%"
"C:\Program Files\CMake\bin\cmake.exe" "F:\syyyy\third_party\onnxruntime_src\cmake" -G "Ninja" -Donnxruntime_USE_CUDA=ON -Donnxruntime_CUDA_MINIMAL=ON -DCMAKE_C_COMPILER=%CMAKE_C_COMPILER% -DCMAKE_CXX_COMPILER=%CMAKE_CXX_COMPILER% -DCMAKE_CUDA_COMPILER=%CMAKE_CUDA_COMPILER% -DCMAKE_CUDA_HOST_COMPILER=%CMAKE_CUDA_HOST_COMPILER% -Donnxruntime_BUILD_SHARED_LIB=ON -Donnxruntime_SKIP_TESTS=ON -Donnxruntime_ENABLE_PYTHON=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CUDA_FLAGS_INIT="-allow-unsupported-compiler" -B "F:\syyyy\third_party\onnxruntime_src\build_ninja"
