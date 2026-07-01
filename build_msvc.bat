@echo off
setlocal enabledelayedexpansion

echo [INFO] Setting up VS 2022 build environment (Qt-free build)...

set PYTHON_ROOT=C:\Users\raffa\AppData\Local\Programs\Python\Python310
set VSWHERE="C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -property installationPath`) do set VSINSTALL=%%i
echo VSINSTALL=%VSINSTALL%

set VCTOOLS=%VSINSTALL%\VC\Tools\MSVC
for /f "delims=" %%d in ('dir /b /ad "%VCTOOLS%" 2^>nul') do set LAST_VC=%%d
set VCTOOLS=%VCTOOLS%\%LAST_VC%
echo VCTOOLS=%VCTOOLS%

set SDKVER=10.0.26100.0
set SDKPATH=C:\Program Files ^(x86^)\Windows Kits\10

set PATH=%VCTOOLS%\bin\Hostx64\x64;%SDKPATH%\bin\%SDKVER%\x64;%PYTHON_ROOT%;%PYTHON_ROOT%\Scripts;%PATH%
set LIB=%VCTOOLS%\lib\x64;%SDKPATH%\lib\%SDKVER%\ucrt\x64;%SDKPATH%\lib\%SDKVER%\um\x64;%PYTHON_ROOT%\libs;%LIB%
set INCLUDE=%VCTOOLS%\include;%SDKPATH%\include\%SDKVER%\ucrt;%SDKPATH%\include\%SDKVER%\um;%SDKPATH%\include\%SDKVER%\shared;%PYTHON_ROOT%\include;%INCLUDE%

echo [INFO] Cleaning old build...
cd /d f:\syyyy
if exist build rmdir /s /q build
mkdir build
cd build

echo [INFO] Running CMake configuration (Qt-free, with ONNX Runtime from third_party)...
cmake -G "Visual Studio 18 2026" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DQUANTUMVERSE_USE_QML=OFF ^
    -DQUANTUMVERSE_USE_IMGUI=ON ^
    -DQUANTUMVERSE_USE_ONNX=ON ^
    -DPython3_EXECUTABLE="C:/Users/raffa/AppData/Local/Programs/Python/Python310/python.exe" ^
    -DPython3_ROOT_DIR="C:/Users/raffa/AppData/Local/Programs/Python/Python310" ^
    -DPYBIND11_FINDPYTHON=ON ^
    .. 2>&1
if errorlevel 1 (
    echo [ERROR] CMake configuration failed
    exit /b 1
)

set ONNX_RUNTIME_ROOT=F:\syyyy\third_party\onnxruntime\onnxruntime-win-x64-1.22.1
echo [INFO] Building...
cmake --build . --config Release 2>&1
if errorlevel 1 (
    echo [ERROR] Build failed
    exit /b 1
)

echo [INFO] Deploying ONNX Runtime DLLs to build output...
copy /Y "%ONNX_RUNTIME_ROOT%\lib\onnxruntime.dll" "%CD%\" 2>&1
copy /Y "%ONNX_RUNTIME_ROOT%\lib\onnxruntime_providers_shared.dll" "%CD%\" 2>&1
echo [OK] Build complete!
exit /b 0