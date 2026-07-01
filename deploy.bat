@echo off
REM QuantumVerse Deployment Script
REM Version: 2.2.0

echo ========================================
echo QuantumVerse v2.2.0 Deployment
echo ========================================

REM Create deployment directory
if not exist deploy\windows mkdir deploy\windows

REM Clean up old imgui.ini to prevent UI freeze issues
if exist deploy\windows\imgui.ini del deploy\windows\imgui.ini >nul 2>&1

REM Remove empty layouts directory if it exists (causes QML import issues)
if exist deploy\windows\layouts (
    rd /s /q deploy\windows\layouts >nul 2>&1
)
REM Also clean up any existing empty directories after deployment
if exist deploy\windows\layouts rd /s /q deploy\windows\layouts >nul 2>&1

REM Copy QML executable (from build_qml)
echo Copying quantumverse_qml.exe...
if exist build_qml\Release\quantumverse_qml.exe (
    copy build_qml\Release\quantumverse_qml.exe deploy\windows\ >nul
)

REM Copy ImGui executable (from build_qml)
echo Copying quantumverse_imgui.exe...
if exist build_qml\Release\quantumverse_imgui.exe (
    copy build_qml\Release\quantumverse_imgui.exe deploy\windows\ >nul
)

REM Copy library (use the larger static library from build_imgui)
echo Copying dilaton.lib...
if exist build_imgui\Release\dilaton.lib (
    copy build_imgui\Release\dilaton.lib deploy\windows\ >nul
) else if exist build_qml\Release\dilaton.lib (
    copy build_qml\Release\dilaton.lib deploy\windows\ >nul
) else if exist build\Release\dilaton.lib (
    copy build\Release\dilaton.lib deploy\windows\ >nul
)

REM Copy Qt6 dependencies if available
if exist Qt6\6.5.3\msvc2019_64\bin\Qt6Core.dll (
    echo Copying Qt6 DLLs...
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6Core.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6Gui.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6Widgets.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6Qml.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6Quick.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6QmlModels.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6QuickControls2.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6QuickControls2Impl.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6QuickTemplates2.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6QuickLayouts.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6Network.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6OpenGL.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6OpenGLWidgets.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6Svg.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6QmlWorkerScript.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6QmlCore.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6QmlCompiler.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6QuickEffects.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6QuickParticles.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6QuickShapes.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6QuickDialogs2.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6QuickDialogs2QuickImpl.dll deploy\windows\ >nul
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6QuickDialogs2Utils.dll deploy\windows\ >nul
    REM Qt Labs modules for Qt Quick Controls 2
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6LabsAnimation.dll deploy\windows\ >nul 2>&1
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6LabsFolderListModel.dll deploy\windows\ >nul 2>&1
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6LabsQmlModels.dll deploy\windows\ >nul 2>&1
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6LabsSettings.dll deploy\windows\ >nul 2>&1
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6LabsSharedImage.dll deploy\windows\ >nul 2>&1
    copy Qt6\6.5.3\msvc2019_64\bin\Qt6LabsWavefrontMesh.dll deploy\windows\ >nul 2>&1
)

REM Copy GLFW DLL if available
if exist glfw-3.4.bin.WIN64\lib-vc2022\glfw3.dll (
    echo Copying glfw3.dll...
    copy glfw-3.4.bin.WIN64\lib-vc2022\glfw3.dll deploy\windows\ >nul
)

REM Copy QML plugins (required for Qt Quick controls)
if exist Qt6\6.5.3\msvc2019_64\qml (
    echo Copying QML plugins...
    xcopy Qt6\6.5.3\msvc2019_64\qml deploy\windows\qml\ /E /I /Y >nul
)
REM Copy QML directory from build (contains compiled resources)
if exist build_qml\Release\qml (
    echo Copying build QML resources...
    xcopy build_qml\Release\qml deploy\windows\qml\ /E /I /Y >nul
)

REM Copy QML source files (for reference)
if exist src\main.qml (
    echo Copying QML source...
    copy src\main.qml deploy\windows\ >nul
    copy src\qmldir deploy\windows\ >nul
)

REM Copy QML module directory (required for import QuantumVerse 1.0)
if not exist deploy\windows\qml\QuantumVerse mkdir deploy\windows\qml\QuantumVerse
if exist src\qmldir (
    copy src\qmldir deploy\windows\qml\QuantumVerse\ >nul
)

REM Copy resources
if exist src\resources.qrc (
    echo Copying resources...
    xcopy src\resources.qrc deploy\windows\ /Y >nul
)

REM Copy MSVC runtime DLLs (required for deployment)
echo Copying MSVC runtime DLLs...
REM Check for Visual Studio 2022 x64 redistributable first, then System32
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140.dll" (
    copy "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140.dll" deploy\windows\ >nul
) else if exist "C:\Windows\System32\vcruntime140.dll" (
    copy "C:\Windows\System32\vcruntime140.dll" deploy\windows\ >nul
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\msvcp140.dll" (
    copy "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\msvcp140.dll" deploy\windows\ >nul
) else if exist "C:\Windows\System32\msvcp140.dll" (
    copy "C:\Windows\System32\msvcp140.dll" deploy\windows\ >nul
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140_1.dll" (
    copy "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140_1.dll" deploy\windows\ >nul
) else if exist "C:\Windows\System32\vcruntime140_1.dll" (
    copy "C:\Windows\System32\vcruntime140_1.dll" deploy\windows\ >nul
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\msvcp140_1.dll" (
    copy "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\msvcp140_1.dll" deploy\windows\ >nul
) else if exist "C:\Windows\System32\msvcp140_1.dll" (
    copy "C:\Windows\System32\msvcp140_1.dll" deploy\windows\ >nul
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\msvcp140_2.dll" (
    copy "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\msvcp140_2.dll" deploy\windows\ >nul
) else if exist "C:\Windows\System32\msvcp140_2.dll" (
    copy "C:\Windows\System32\msvcp140_2.dll" deploy\windows\ >nul
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140_atomic_wait.dll" (
    copy "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140_atomic_wait.dll" deploy\windows\ >nul
) else if exist "C:\Windows\System32\vcruntime140_atomic_wait.dll" (
    copy "C:\Windows\System32\vcruntime140_atomic_wait.dll" deploy\windows\ >nul
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140_atomic_wait.dll" (
    copy "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140_atomic_wait.dll" deploy\windows\ >nul
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140_atomic_wait.dll" (
    copy "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140_atomic_wait.dll" deploy\windows\ >nul
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140_atomic_wait.dll" (
    copy "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140_atomic_wait.dll" deploy\windows\ >nul
) else (
    echo Note: vcruntime140_atomic_wait.dll not found - may be available in Windows 11 24H2 or later
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140_threads.dll" (
    copy "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140_threads.dll" deploy\windows\ >nul
) else if exist "C:\Windows\System32\vcruntime140_threads.dll" (
    copy "C:\Windows\System32\vcruntime140_threads.dll" deploy\windows\ >nul
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140_threads.dll" (
    copy "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140_threads.dll" deploy\windows\ >nul
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140_threads.dll" (
    copy "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140_threads.dll" deploy\windows\ >nul
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140_threads.dll" (
    copy "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\vcruntime140_threads.dll" deploy\windows\ >nul
) else (
    echo Note: vcruntime140_threads.dll not found - may be available in Windows 11 24H2 or later
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\msvcp140_atomic_wait.dll" (
    copy "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\msvcp140_atomic_wait.dll" deploy\windows\ >nul
) else if exist "C:\Windows\System32\msvcp140_atomic_wait.dll" (
    copy "C:\Windows\System32\msvcp140_atomic_wait.dll" deploy\windows\ >nul
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\msvcp140_atomic_wait.dll" (
    copy "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\msvcp140_atomic_wait.dll" deploy\windows\ >nul
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\msvcp140_atomic_wait.dll" (
    copy "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\msvcp140_atomic_wait.dll" deploy\windows\ >nul
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\msvcp140_atomic_wait.dll" (
    copy "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\msvcp140_atomic_wait.dll" deploy\windows\ >nul
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\msvcp140_codecvt_ids.dll" (
    copy "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\msvcp140_codecvt_ids.dll" deploy\windows\ >nul
) else if exist "C:\Windows\System32\msvcp140_codecvt_ids.dll" (
    copy "C:\Windows\System32\msvcp140_codecvt_ids.dll" deploy\windows\ >nul
)

REM Copy concrt140.dll (MSVC concurrency runtime - required for std::thread)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\concrt140.dll" (
    copy "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\concrt140.dll" deploy\windows\ >nul
) else if exist "C:\Windows\System32\concrt140.dll" (
    copy "C:\Windows\System32\concrt140.dll" deploy\windows\ >nul
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\concrt140.dll" (
    copy "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC141.VC142.CRT\concrt140.dll" deploy\windows\ >nul
)

REM Copy additional runtime DLLs from build directory (D3D, OpenGL software renderer)
if exist build_qml\Release\D3Dcompiler_47.dll (
    echo Copying D3Dcompiler_47.dll...
    copy build_qml\Release\D3Dcompiler_47.dll deploy\windows\ >nul
)
if exist build_qml\Release\opengl32sw.dll (
    echo Copying opengl32sw.dll...
    copy build_qml\Release\opengl32sw.dll deploy\windows\ >nul
)
REM Copy Qt6QmlModels.dll from build if it exists there
if exist build_qml\Release\Qt6QmlModels.dll (
    echo Copying Qt6QmlModels.dll from build...
    copy build_qml\Release\Qt6QmlModels.dll deploy\windows\ >nul
)

REM Copy icon files for QML (required for qrc:/icons/ paths)
if not exist deploy\windows\icons mkdir deploy\windows\icons
if exist src\icons\open.svg (
    copy src\icons\open.svg deploy\windows\icons\ >nul
)
if exist src\icons\step_back.svg (
    copy src\icons\step_back.svg deploy\windows\icons\ >nul
)
if exist src\icons\play.svg (
    copy src\icons\play.svg deploy\windows\icons\ >nul
)
if exist src\icons\pause.svg (
    copy src\icons\pause.svg deploy\windows\icons\ >nul
)
if exist src\icons\step_forward.svg (
    copy src\icons\step_forward.svg deploy\windows\icons\ >nul
)
if exist src\icons\stop.svg (
    copy src\icons\stop.svg deploy\windows\icons\ >nul
)

REM Copy additional Qt6 plugins (platforms, styles, etc.)
if exist Qt6\6.5.3\msvc2019_64\plugins (
    echo Copying Qt6 plugins...
    xcopy Qt6\6.5.3\msvc2019_64\plugins deploy\windows\plugins\ /E /I /Y >nul
)

REM Copy iconengines and imageformats from build (for SVG icon support)
if exist build_qml\Release\iconengines (
    echo Copying iconengines...
    xcopy build_qml\Release\iconengines deploy\windows\plugins\iconengines\ /E /I /Y >nul
)
if exist build_qml\Release\imageformats (
    echo Copying imageformats...
    xcopy build_qml\Release\imageformats deploy\windows\plugins\imageformats\ /E /I /Y >nul
)

REM List deployed files
echo.
echo Deployment complete!
echo Files in deploy\windows\:
dir deploy\windows\ /B

echo.
echo To run QML: deploy\windows\quantumverse_qml.exe
echo To run ImGui: deploy\windows\quantumverse_imgui.exe
echo See deploy\windows\README.md for requirements and troubleshooting
echo ========================================