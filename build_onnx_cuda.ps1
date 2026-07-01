$msvcCl = 'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.50.35717\bin\Hostx64\x64\cl.exe'
$nvcc = 'C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.5\bin\nvcc.exe'
$cudaRoot = 'C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.5'
$srcDir = 'F:\syyyy\third_party\onnxruntime_src\cmake'
$buildDir = 'F:\syyyy\third_party\onnxruntime_src\build_ninja'
$cmake = 'C:\Program Files\CMake\bin\cmake.exe'
$ninja = 'F:\syyyy\ninja.exe'

# Run vcvarsall.bat to set up MSVC environment, then run cmake in the same cmd session
$cmd = @"
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
"$cmake" "$srcDir" -G "Ninja" -Donnxruntime_USE_CUDA=ON -Donnxruntime_CUDA_MINIMAL=ON -DCMAKE_C_COMPILER="$msvcCl" -DCMAKE_CXX_COMPILER="$msvcCl" -DCMAKE_CUDA_COMPILER="$nvcc" -DCMAKE_CUDA_HOST_COMPILER="$msvcCl" -DCUDAToolkit_ROOT="$cudaRoot" -Donnxruntime_BUILD_SHARED_LIB=ON -Donnxruntime_SKIP_TESTS=ON -Donnxruntime_ENABLE_PYTHON=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CUDA_FLAGS_INIT="-allow-unsupported-compiler" -B "$buildDir"
"$ninja" -C "$buildDir" onnxruntime_providers_cuda
"@

cmd.exe /c $cmd 2>&1
