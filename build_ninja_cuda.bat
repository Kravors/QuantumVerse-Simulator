@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set "CMAKE_CUDA_COMPILER=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.5\bin\nvcc.exe"
set "CMAKE_CUDA_HOST_COMPILER=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe"
F:\syyyy\ninja.exe -C F:\syyyy\third_party\onnxruntime_src\build_ninja onnxruntime_providers_cuda
echo BUILD_EXIT_CODE=%ERRORLEVEL%
