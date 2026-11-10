# QuantumVerse Simulator - CMakeLists.txt
# Production-ready 4D spacetime cognition laboratory
cmake_minimum_required(VERSION 3.20)

# Derive version from git tags (e.g. v7.4.2 → 7.4.2)
find_package(Git REQUIRED)
execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE QUANTUMVERSE_GIT_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
    ERROR_VARIABLE QUANTUMVERSE_GIT_DESCRIBE_ERROR)
if(QUANTUMVERSE_GIT_TAG MATCHES "^v(.+)$")
    set(QUANTUMVERSE_VERSION "${CMAKE_MATCH_1}")
else()
    set(QUANTUMVERSE_VERSION "0.0.0-dev")
endif()
message(STATUS "QuantumVerse version: ${QUANTUMVERSE_VERSION}")

project(QuantumVerse LANGUAGES C CXX VERSION ${QUANTUMVERSE_VERSION})

# CPack packaging configuration
set(CPACK_PROJECT_NAME "QuantumVerse")
set(CPACK_PACKAGE_VERSION ${QUANTUMVERSE_VERSION})
set(CPACK_PACKAGE_VENDOR "QuantumVerse Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "4D Spacetime Cognition Lab")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "QuantumVerse")
set(CPACK_NSIS_DISPLAY_NAME "QuantumVerse Simulator")
set(CPACK_NSIS_PACKAGE_NAME "QuantumVerse Simulator")
set(CPACK_NSIS_CONTACT "support@quantumverse.dev")
set(CPACK_NSIS_MODIFY_PATH ON)
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MENU_LINKS
    "bin/quantumverse_qml.exe" "QuantumVerse Simulator")
set(CPACK_PACKAGE_EXECUTABLES "quantumverse_qml" "QuantumVerse Simulator")

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
    add_compile_definitions(_USE_MATH_DEFINES)
else()
    add_compile_options(-Wall -Wextra)
endif()

# Suppress C4996 in third-party stb headers
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

# ONNX Runtime is optional. Declared as an option so a fresh clone configures
# cleanly on machines without onnxruntime (default OFF). Use
# -DQUANTUMVERSE_USE_ONNX=ON to enable RL agent ONNX inference.
option(QUANTUMVERSE_USE_ONNX "Enable ONNX Runtime for RL agent inference" OFF)

# Coverage instrumentation
if(QUANTUMVERSE_USE_COVERAGE)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage -O0 -g")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage -O0 -g")
endif()

# Default build type: Debug for tests.
#
# CMAKE_BUILD_TYPE is never set on the command line, so single-config
# generators (MSVC/Visual Studio via --config) defaulted to Release, which
# defines NDEBUG and compiles every assert() out of the test suite.  That
# made all "passing" test runs meaningless.  Default to Debug so assertions
# are live; this is a fallback only -- the QV_CHECK/QV_CHECK_NEAR macros in
# tests/test_assert.h are the real fix because they survive Release too.
if(QUANTUMVERSE_BUILD_TESTS AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING
        "Build type (Debug default so test assertions are active)" FORCE)
endif()

# Options
option(QUANTUMVERSE_BUILD_TESTS "Build test suite" ON)
option(QUANTUMVERSE_USE_QT "Build Qt UI" OFF)
option(QUANTUMVERSE_USE_VR "Build VR/OpenXR backend" OFF)
option(QUANTUMVERSE_USE_KAFKA "Build Kafka/GCN live ingest" OFF)
option(QUANTUMVERSE_USE_COVERAGE "Enable code coverage instrumentation" OFF)
option(QUANTUMVERSE_USE_ASAN "Build with AddressSanitizer" OFF)
option(QUANTUMVERSE_USE_UBSAN "Build with UndefinedBehaviorSanitizer" OFF)
option(QUANTUMVERSE_USE_TSAN "Build with ThreadSanitizer" OFF)
option(QUANTUMVERSE_BUILD_WASM "Build WASM Portal (Emscripten)" OFF)

# Test targets link Qt6::Core unconditionally, so BUILD_TESTS=ON implies
# USE_QT=ON. Auto-enable rather than fail-fast: tests requiring Qt is an
# implication, not a user-facing choice.
if(QUANTUMVERSE_BUILD_TESTS AND NOT QUANTUMVERSE_USE_QT)
    message(STATUS "QUANTUMVERSE_BUILD_TESTS=ON implies QUANTUMVERSE_USE_QT=ON")
    set(QUANTUMVERSE_USE_QT ON CACHE BOOL "" FORCE)
endif()

# Sanitizer configuration
if(QUANTUMVERSE_USE_ASAN)
    if(MSVC)
        add_compile_options(/fsanitize=address)
        add_link_options(/fsanitize=address)
    else()
        add_compile_options(-fsanitize=address)
        add_link_options(-fsanitize=address)
    endif()
    message(STATUS "AddressSanitizer enabled")
endif()

if(QUANTUMVERSE_USE_UBSAN)
    if(MSVC)
        add_compile_options(/fsanitize=undefined)
        add_link_options(/fsanitize=undefined)
    else()
        add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=undefined)
    endif()
    message(STATUS "UndefinedBehaviorSanitizer enabled")
endif()

if(QUANTUMVERSE_USE_TSAN)
    if(MSVC)
        add_compile_options(/fsanitize=thread)
        add_link_options(/fsanitize=thread)
    else()
        add_compile_options(-fsanitize=thread -g)
        add_link_options(-fsanitize=thread)
    endif()
    message(STATUS "ThreadSanitizer enabled")
endif()

# Performance regression gate thresholds (ms) for the QML headless baseline.
# Override on the CMake command line, e.g. -DCTEST_PERFORMANCE_THRESHOLD=50.
set(CTEST_PERFORMANCE_THRESHOLD 200 CACHE STRING "Avg frame-time threshold (ms) for the QML performance gate")
set(CTEST_PERFORMANCE_MAX_THRESHOLD 250 CACHE STRING "Max frame-time threshold (ms) for the QML performance gate")

enable_testing()

# Fetch third-party dependencies
include(FetchContent)

# nlohmann/json for JSON parsing (header-only)
include(FetchContent)
if(NOT QUANTUMVERSE_BUILD_WASM)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
    )
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(nlohmann_json)
else()
    # WASM: nlohmann_json not needed (no tests, WASM sources don't include it)
    add_library(nlohmann_json INTERFACE)
endif()

# cURL for HTTP requests
find_package(CURL QUIET)
if(CURL_FOUND)
    message(STATUS "CURL found - HTTP adapters enabled")
endif()

# librdkafka for GCN Kafka live ingest
set(LIBRDKAFKA_ROOT "" CACHE PATH "Root directory of librdkafka installation")
find_package(librdkafka QUIET)
if(LIBRDKAFKA_FOUND)
    message(STATUS "librdkafka found - Kafka/GCN live ingest enabled")
    add_compile_definitions(HAVE_LIBRDKAFKA=1)
else()
    message(STATUS "librdkafka not found - building without Kafka/GCN live ingest")
endif()

# OpenXR SDK for VR support
set(OPENXR_SDK_ROOT "" CACHE PATH "Root directory of OpenXR SDK installation")
if(QUANTUMVERSE_USE_VR)
    find_path(OPENXR_INCLUDE_DIR NAMES openxr/openxr.h PATHS "${OPENXR_SDK_ROOT}/include" NO_DEFAULT_PATH)
    if(NOT OPENXR_INCLUDE_DIR)
        find_path(OPENXR_INCLUDE_DIR NAMES openxr/openxr.h)
    endif()
    if(OPENXR_INCLUDE_DIR)
        message(STATUS "OpenXR SDK found at ${OPENXR_INCLUDE_DIR} - VR backend enabled")
        add_compile_definitions(QUANTUMVERSE_USE_OPENXR_SDK=1)
        target_include_directories(dilaton INTERFACE ${OPENXR_INCLUDE_DIR})
    else()
        message(STATUS "OpenXR SDK not found - building VR stub backend")
    endif()
endif()

# Main library interface (header-only for now)
add_library(dilaton INTERFACE)
target_include_directories(dilaton INTERFACE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)
target_link_libraries(dilaton INTERFACE nlohmann_json)
if(CURL_FOUND)
    target_compile_definitions(dilaton INTERFACE HAVE_CURL=1)
    target_link_libraries(dilaton INTERFACE CURL::libcurl)
endif()
if(NOT CURL_FOUND)
    message(STATUS "CURL not found - building adapters without HTTP support")
endif()

if(QUANTUMVERSE_USE_ONNX)
    set(ONNXRUNTIME_ROOT "${CMAKE_SOURCE_DIR}/third_party/onnxruntime")
    set(ONNXRUNTIME_PREBUILT "${ONNXRUNTIME_ROOT}/onnxruntime-win-x64-1.22.1/onnxruntime-win-x64-1.22.1")
    set(ONNXRUNTIME_INCLUDE "${CMAKE_SOURCE_DIR}/third_party/onnxruntime_src/include")
    set(ONNXRUNTIME_LIB "${ONNXRUNTIME_PREBUILT}/lib")

    set(ONNX_INCLUDE_DIR "${ONNXRUNTIME_INCLUDE}")
    set(ONNX_LIBRARY "${ONNXRUNTIME_LIB}/onnxruntime.lib")

    if(EXISTS "${ONNX_LIBRARY}" AND EXISTS "${ONNX_INCLUDE_DIR}/onnxruntime/core/session/onnxruntime_cxx_api.h")
        message(STATUS "Found ONNX Runtime: ${ONNX_LIBRARY}")
        add_library(onnxruntime::onnxruntime INTERFACE IMPORTED)
        set_target_properties(onnxruntime::onnxruntime PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${ONNX_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${ONNX_LIBRARY}"
        )
        target_link_libraries(dilaton INTERFACE onnxruntime::onnxruntime)
        add_compile_definitions(HAVE_ONNX=1)

        # Copy DLL next to executable at build time
        macro(quantumverse_copy_onnx_dll target)
            if(TARGET ${target})
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${ONNXRUNTIME_LIB}/onnxruntime.dll"
                    $<TARGET_FILE_DIR:${target}>
                )
            endif()
        endmacro()

        quantumverse_copy_onnx_dll(quantumverse_qml)
        quantumverse_copy_onnx_dll(test_neural_ode_accuracy)
        quantumverse_copy_onnx_dll(test_metric_gnn)
        quantumverse_copy_onnx_dll(test_surrogate_integration)
        quantumverse_copy_onnx_dll(test_rl_agent)
    else()
        message(FATAL_ERROR "ONNX Runtime not found. Include: ${ONNX_INCLUDE_DIR}, Lib: ${ONNX_LIBRARY}")
    endif()
endif()

# Qt/QML configuration
if(QUANTUMVERSE_USE_QT AND NOT QUANTUMVERSE_BUILD_WASM)
    find_package(Qt6 QUIET COMPONENTS Multimedia)
    if(Qt6Multimedia_FOUND)
        set(QT6_MULTIMEDIA_COMPONENTS Multimedia)
        set(QT6_MULTIMEDIA_LINK_LIBS Qt6::Multimedia)
        set(QUANTUMVERSE_HAS_MULTIMEDIA 1)
    else()
        message(STATUS "Qt6 Multimedia not found - audio features disabled")
        set(QT6_MULTIMEDIA_COMPONENTS "")
        set(QT6_MULTIMEDIA_LINK_LIBS "")
        set(QUANTUMVERSE_HAS_MULTIMEDIA 0)
    endif()
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick OpenGL OpenGLWidgets QuickControls2 QuickTemplates2 QuickLayouts Network WebSockets Test ${QT6_MULTIMEDIA_COMPONENTS})

    if(Qt6Core_DIR)
        get_filename_component(QT_HOST_PATH "${Qt6Core_DIR}/../../.." ABSOLUTE)
    endif()

    set(QML_DIR ${CMAKE_SOURCE_DIR}/src)
    # ============================================================================
    # quantumverse_core STATIC library
    # All shared .cpp sources live here, defined exactly once. Every consumer
    # (quantumverse_qml, every Qt test target) links this single library, so
    # source-list drift is impossible: adding a new .cpp requires exactly one
    # edit (this list), and every target picks it up automatically.
    # ============================================================================
    file(GLOB_RECURSE QUANTUMVERSE_CORE_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
    )
    # Exclude entry points and special modules not part of the core library.
    list(FILTER QUANTUMVERSE_CORE_SOURCES EXCLUDE REGEX "/src/main_qml\\.cpp$")
    list(FILTER QUANTUMVERSE_CORE_SOURCES EXCLUDE REGEX "/src/physics/geodesic_pybind\\.cpp$")
    list(FILTER QUANTUMVERSE_CORE_SOURCES EXCLUDE REGEX "/src/wasm/.*\\.cpp$")
    list(FILTER QUANTUMVERSE_CORE_SOURCES EXCLUDE REGEX "/src/vr/.*\\.cpp$")

    add_library(quantumverse_core STATIC ${QUANTUMVERSE_CORE_SOURCES})
    target_sources(quantumverse_core PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party/glad/glad.cpp
    )
    set_target_properties(quantumverse_core PROPERTIES AUTOMOC ON)
    target_include_directories(quantumverse_core PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/tests
        ${CMAKE_SOURCE_DIR}/third_party/glad/include
        ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad
    )
    target_link_libraries(quantumverse_core PUBLIC
        dilaton
    )
    # Qt and platform deps needed to compile Qt-dependent sources.
    target_link_libraries(quantumverse_core PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        Qt6::Quick
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        Qt6::Network
        Qt6::WebSockets
        ${QT6_MULTIMEDIA_LINK_LIBS}
    )
    target_include_directories(quantumverse_core PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party
        ${CMAKE_SOURCE_DIR}/third_party/glad/include
        ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad
        ${LIBRDKAFKA_INCLUDE_DIRS}
    )
    target_compile_definitions(quantumverse_core PRIVATE QUANTUMVERSE_USE_QML)

    set(RESOURCES_QRC ${CMAKE_SOURCE_DIR}/src/resources.qrc)

    # Compile Qt resources
    qt_add_resources(RESOURCES_QRC ${RESOURCES_QRC})

    # Multi-user collaboration library
    add_library(quantumverse_multi_user STATIC
        src/vr/SignalingClient.cpp
        src/vr/SharedSession.cpp
        src/vr/MultiUserServer.cpp
    )
    target_include_directories(quantumverse_multi_user PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/third_party
    )
    target_link_libraries(quantumverse_multi_user PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::WebSockets
        dilaton
    )
    set_target_properties(quantumverse_multi_user PROPERTIES
        AUTOMOC ON
    )

    add_executable(quantumverse_qml
        src/main_qml.cpp
        ${RESOURCES_QRC}
    )
    set_target_properties(quantumverse_qml PROPERTIES AUTOMOC ON)
    target_compile_definitions(quantumverse_qml PRIVATE QUANTUMVERSE_USE_QML)
    target_link_libraries(quantumverse_qml PRIVATE quantumverse_core quantumverse_multi_user)
    if(QUANTUMVERSE_USE_VR)
        target_compile_definitions(quantumverse_qml PRIVATE QUANTUMVERSE_USE_VR)
        target_sources(quantumverse_qml PRIVATE
            src/vr/OpenXRBackend.cpp
        )
        if(OPENXR_INCLUDE_DIR)
            find_library(OPENXR_LIBRARY NAMES openxr openxr_loader PATHS "${OPENXR_SDK_ROOT}/lib" NO_DEFAULT_PATH)
            if(NOT OPENXR_LIBRARY)
                find_library(OPENXR_LIBRARY NAMES openxr openxr_loader)
            endif()
            if(OPENXR_LIBRARY)
                message(STATUS "Linking OpenXR library: ${OPENXR_LIBRARY}")
                target_link_libraries(quantumverse_qml PRIVATE ${OPENXR_LIBRARY})
            else()
                message(STATUS "OpenXR library not found - runtime loading will be used")
            endif()
        endif()
    endif()
    target_include_directories(quantumverse_qml PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/third_party
        ${CMAKE_SOURCE_DIR}/third_party/glad/include
        ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad
        ${LIBRDKAFKA_INCLUDE_DIRS}
    )
    target_link_libraries(quantumverse_qml PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickTemplates2
        Qt6::QuickLayouts
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        Qt6::Network
        Qt6::WebSockets
        ${QT6_MULTIMEDIA_LINK_LIBS}
        dilaton
        nlohmann_json
        ${LIBRDKAFKA_LIBRARIES}
    )
    if(WIN32)
        target_link_libraries(quantumverse_qml PRIVATE dbghelp)
    endif()
    set_target_properties(quantumverse_qml PROPERTIES
        AUTOMOC ON
        AUTORCC ON
        AUTOUIC ON
    )

    if(WIN32)
        # Detect the VC install directory so windeployqt --compiler-runtime can
        # locate the MSVC redistributables (msvcp140.dll, vcruntime140.dll, etc.).
        # CMAKE_VS_VC_INSTALL_DIR is only available when CMake itself runs inside
        # a VS Developer shell, so we probe vswhere / common paths at configure time.
        if(NOT DEFINED VCINSTALLDIR_FOR_WINDEPLOY)
            find_program(VSWHERE NAMES vswhere
                PATHS "C:/Program Files (x86)/Microsoft Visual Studio/Installer")
            if(VSWHERE)
                execute_process(
                    COMMAND "${VSWHERE}" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
                    OUTPUT_VARIABLE _vs_install_path
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                )
                if(EXISTS "${_vs_install_path}/VC")
                    set(VCINSTALLDIR_FOR_WINDEPLOY "${_vs_install_path}/VC" CACHE PATH
                        "VC install directory for windeployqt --compiler-runtime")
                endif()
            endif()
            if(NOT VCINSTALLDIR_FOR_WINDEPLOY)
                foreach(_vc_path
                    "$ENV{ProgramFiles}/Microsoft Visual Studio/18/Community/VC"
                    "$ENV{ProgramFiles}/Microsoft Visual Studio/18/Professional/VC"
                    "$ENV{ProgramFiles}/Microsoft Visual Studio/18/Enterprise/VC"
                    "$ENV{ProgramFiles(x86)}/Microsoft Visual Studio/18/Community/VC"
                    "$ENV{ProgramFiles(x86)}/Microsoft Visual Studio/18/Professional/VC"
                    "$ENV{ProgramFiles(x86)}/Microsoft Visual Studio/18/Enterprise/VC"
                )
                    if(EXISTS "${_vc_path}")
                        set(VCINSTALLDIR_FOR_WINDEPLOY "${_vc_path}" CACHE PATH
                            "VC install directory for windeployqt --compiler-runtime")
                        break()
                    endif()
                endforeach()
            endif()
            if(VCINSTALLDIR_FOR_WINDEPLOY)
                message(STATUS "Detected VC install directory for windeployqt: ${VCINSTALLDIR_FOR_WINDEPLOY}")
            else()
                message(WARNING "Could not detect VC install directory; windeployqt --compiler-runtime may not copy MSVC runtime DLLs")
            endif()
        endif()

        # Find the latest VC++ CRT redist directory with x64 DLLs.
        # windeployqt --compiler-runtime ships the installer (vc_redist.x64.exe),
        # not the raw DLLs, so we copy msvcp140.dll / vcruntime140.dll / concrt140.dll
        # directly to make the app portable on clean Windows installs.
        if(NOT DEFINED VC_REDIST_CRT_DIR)
            set(_vc_redist_roots)
            if(VCINSTALLDIR_FOR_WINDEPLOY)
                list(APPEND _vc_redist_roots "${VCINSTALLDIR_FOR_WINDEPLOY}/Redist/MSVC")
            endif()
            # Also check the VSINSTALLDIR environment variable (set by VS Developer shells)
            if(DEFINED ENV{VSINSTALLDIR} AND EXISTS "$ENV{VSINSTALLDIR}/VC/Redist/MSVC")
                list(APPEND _vc_redist_roots "$ENV{VSINSTALLDIR}/VC/Redist/MSVC")
            endif()
            foreach(_redist_root IN LISTS _vc_redist_roots)
                if(EXISTS "${_redist_root}")
                    file(GLOB _redist_dirs "${_redist_root}/*")
                    foreach(_dir IN LISTS _redist_dirs)
                        if(IS_DIRECTORY "${_dir}/x64")
                            file(GLOB _crt_dirs "${_dir}/x64/*")
                            foreach(_crt_dir IN LISTS _crt_dirs)
                                if(IS_DIRECTORY "${_crt_dir}" AND EXISTS "${_crt_dir}/msvcp140.dll")
                                    set(VC_REDIST_CRT_DIR "${_crt_dir}")
                                endif()
                            endforeach()
                        endif()
                    endforeach()
                endif()
            endforeach()
            if(NOT VC_REDIST_CRT_DIR)
                # Fallback: hardcode the known-good path for VS 18 / v145 toolset
                set(VC_REDIST_CRT_DIR "C:/Program Files/Microsoft Visual Studio/18/Community/VC/Redist/MSVC/14.50.35710/x64/Microsoft.VC145.CRT" CACHE PATH
                    "MSVC x64 CRT redist directory for runtime DLL deployment")
            endif()
            if(EXISTS "${VC_REDIST_CRT_DIR}/msvcp140.dll")
                message(STATUS "Detected VC++ CRT redist directory: ${VC_REDIST_CRT_DIR}")
            else()
                message(WARNING "Could not detect VC++ CRT redist directory; runtime DLLs will not be deployed")
            endif()
        endif()

        set(QT_RUNTIME_DEST "$<TARGET_FILE_DIR:quantumverse_qml>")
        if(VCINSTALLDIR_FOR_WINDEPLOY)
            file(TO_NATIVE_PATH "${VCINSTALLDIR_FOR_WINDEPLOY}" _vc_install_dir_native)
            set(_vc_install_cmd "set VCINSTALLDIR=${_vc_install_dir_native} && ")
        else()
            set(_vc_install_cmd "")
        endif()
        
        add_custom_target(deploy_qt_runtime
            COMMAND ${CMAKE_COMMAND} -E echo "Deploying Qt runtime to ${QT_RUNTIME_DEST}..."
            COMMAND ${CMAKE_COMMAND} -E make_directory "${QT_RUNTIME_DEST}"
            COMMAND ${QT_HOST_PATH}/bin/windeployqt.exe
                $<$<CONFIG:Debug>:--debug>$<$<NOT:$<CONFIG:Debug>>:--release>
                --qmldir "${CMAKE_SOURCE_DIR}/src"
                --no-compiler-runtime --no-translations
                "${QT_RUNTIME_DEST}/quantumverse_qml.exe"
            # windeployqt --compiler-runtime ships the VC++ redist INSTALLER, not the raw
            # DLLs. Copy the CRT DLLs directly so the app runs on a clean Windows install.
            COMMAND ${CMAKE_COMMAND} -E echo "Copying MSVC runtime DLLs..."
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${VC_REDIST_CRT_DIR}/msvcp140.dll"
                "${VC_REDIST_CRT_DIR}/vcruntime140.dll"
                "${VC_REDIST_CRT_DIR}/concrt140.dll"
                "${QT_RUNTIME_DEST}/"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT_HOST_PATH}/bin/Qt6Test.dll"
                "${QT_RUNTIME_DEST}/"
            COMMENT "Running windeployqt on quantumverse_qml.exe to populate ${QT_RUNTIME_DEST}"
        )
        add_dependencies(deploy_qt_runtime quantumverse_qml)
    else()
        # On non-Windows (single-config generators) test executables land in the
        # build root. Without this default the UI tests below set an empty
        # WORKING_DIRECTORY and silently become "Not Run".
        set(QT_RUNTIME_DEST "${CMAKE_BINARY_DIR}")
    endif()


    # Optional per-stage render timers (QmlGlRenderer). Off by default so the
    # release performance gate is unaffected; enable for profiling builds.
    option(QUANTUMVERSE_PERF_TRACE "Enable QmlGlRenderer per-stage timers" OFF)
    if(QUANTUMVERSE_PERF_TRACE)
        target_compile_definitions(quantumverse_qml PRIVATE PERF_TRACE=1)
    endif()

    if(QUANTUMVERSE_BUILD_TESTS)
        add_executable(qml_performance_baseline tests/benchmark/qml_performance_baseline.cpp)
        target_include_directories(qml_performance_baseline PRIVATE ${CMAKE_SOURCE_DIR}/tests/benchmark)
        target_compile_definitions(qml_performance_baseline PRIVATE
            PERF_AVG_THRESHOLD_DEFAULT=${CTEST_PERFORMANCE_THRESHOLD}
            PERF_MAX_THRESHOLD_DEFAULT=${CTEST_PERFORMANCE_MAX_THRESHOLD}
        )
        set_target_properties(qml_performance_baseline PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:quantumverse_qml>
        )
        add_test(NAME QMLPerformanceBaseline COMMAND qml_performance_baseline)
        set_tests_properties(QMLPerformanceBaseline PROPERTIES
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH};QT_QPA_PLATFORM=offscreen"
            LABELS "performance;qt"
        )
        add_dependencies(qml_performance_baseline quantumverse_qml)
        if(TARGET deploy_qt_runtime)
            add_dependencies(qml_performance_baseline deploy_qt_runtime)
        endif()

        # TDD contract test for the performance gate: validates parse/threshold
        # logic and (when built) that the baseline executable returns exit 0.
        add_executable(test_performance_gate tests/test_performance_gate.cpp)
        target_include_directories(test_performance_gate PRIVATE ${CMAKE_SOURCE_DIR}/tests/benchmark)
        target_link_libraries(test_performance_gate PRIVATE Qt6::Core)
        if(MSVC)
            target_compile_options(test_performance_gate PRIVATE /EHsc)
        endif()
        set_target_properties(test_performance_gate PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:quantumverse_qml>
        )
        set_target_properties(test_performance_gate PROPERTIES AUTOMOC ON)
        add_test(NAME PerformanceGateTest COMMAND test_performance_gate)
        set_tests_properties(PerformanceGateTest PROPERTIES
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH};QT_QPA_PLATFORM=offscreen"
            LABELS "performance;qt"
        )
        add_dependencies(test_performance_gate quantumverse_qml)
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_performance_gate deploy_qt_runtime)
        endif()

        # P3-3 contract test: verifies GL_CHECK() is compiled out in Release
        # builds (no-op) and active in Debug. Compile-time guarantee via
        # macro expansion inspection.
        add_executable(test_gl_check tests/test_gl_check.cpp)
        target_include_directories(test_gl_check PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/third_party
            ${CMAKE_SOURCE_DIR}/third_party/glad/include
            ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad
        )
        target_link_libraries(test_gl_check PRIVATE Qt6::Core)
        if(MSVC)
            target_compile_options(test_gl_check PRIVATE /EHsc)
        endif()
        set_target_properties(test_gl_check PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:quantumverse_qml>
        )
        add_test(NAME GLCheckTest COMMAND test_gl_check)
        set_tests_properties(GLCheckTest PROPERTIES
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            LABELS "performance;qt"
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_gl_check deploy_qt_runtime)
        endif()

        # FindingsModel unit test (requires Qt for QAbstractListModel)
        add_executable(test_findings_model
            tests/test_findings_model.cpp
        )
        set_target_properties(test_findings_model PROPERTIES AUTOMOC ON)
        target_include_directories(test_findings_model PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/third_party
        )
        target_link_libraries(test_findings_model PRIVATE Qt6::Core quantumverse_core)
        add_test(NAME FindingsModelTest COMMAND test_findings_model)
        set_tests_properties(FindingsModelTest PROPERTIES
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_findings_model deploy_qt_runtime)
        endif()

        # FindingsModel filtering / sorting / detail-view test
        add_executable(test_findings_model_filtering
            tests/test_findings_model_filtering.cpp
        )
        set_target_properties(test_findings_model_filtering PROPERTIES AUTOMOC ON)
        target_include_directories(test_findings_model_filtering PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/third_party
        )
        target_link_libraries(test_findings_model_filtering PRIVATE Qt6::Core quantumverse_core)
        add_test(NAME FindingsModelFilteringTest COMMAND test_findings_model_filtering)
        set_tests_properties(FindingsModelFilteringTest PROPERTIES
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_findings_model_filtering deploy_qt_runtime)
        endif()

        # Multi-messenger alert wiring test (LIGO/IceCube -> FindingsModel)
        add_executable(test_multimessenger_ui
            tests/test_multimessenger_ui.cpp
        )
        set_target_properties(test_multimessenger_ui PROPERTIES AUTOMOC ON)
        target_include_directories(test_multimessenger_ui PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/third_party
            ${CMAKE_SOURCE_DIR}/third_party/glad/include
            ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad
        )
        target_link_libraries(test_multimessenger_ui PRIVATE Qt6::Core quantumverse_core)
        add_test(NAME MultiMessengerUITest COMMAND test_multimessenger_ui)
        set_tests_properties(MultiMessengerUITest PROPERTIES
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_multimessenger_ui deploy_qt_runtime)
        endif()

        # 5.1 QmlGlViewport isolation test
        add_executable(test_viewport_isolation
            tests/ui/test_viewport_isolation.cpp
        )
        target_link_libraries(test_viewport_isolation PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Quick
            Qt6::OpenGL
            Qt6::OpenGLWidgets
            Qt6::Test
            ${QT6_MULTIMEDIA_LINK_LIBS}
            quantumverse_core
        )
        if(WIN32)
            target_link_libraries(test_viewport_isolation PRIVATE opengl32 dbghelp)
        endif()
        set_target_properties(test_viewport_isolation PROPERTIES AUTOMOC ON)
        target_compile_definitions(test_viewport_isolation PRIVATE QUANTUMVERSE_USE_QML)
        target_include_directories(test_viewport_isolation PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/third_party
            ${CMAKE_SOURCE_DIR}/third_party/glad/include
            ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad
        )
        add_test(NAME ViewportIsolationTest COMMAND test_viewport_isolation)
        set_tests_properties(ViewportIsolationTest PROPERTIES
            WORKING_DIRECTORY "${QT_RUNTIME_DEST}"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH};QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::Core>/platforms"
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_viewport_isolation deploy_qt_runtime)
        endif()

        # 5.3 Interaction stress test (rapid mouse/wheel input + property toggles)
        add_executable(test_interaction_stress
            tests/ui/test_interaction_stress.cpp
        )
        target_link_libraries(test_interaction_stress PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Quick
            Qt6::OpenGL
            Qt6::OpenGLWidgets
            Qt6::Test
            ${QT6_MULTIMEDIA_LINK_LIBS}
            quantumverse_core
        )
        if(WIN32)
            target_link_libraries(test_interaction_stress PRIVATE opengl32 dbghelp)
        endif()
        set_target_properties(test_interaction_stress PROPERTIES AUTOMOC ON)
        target_compile_definitions(test_interaction_stress PRIVATE QUANTUMVERSE_USE_QML)
        target_include_directories(test_interaction_stress PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/third_party
            ${CMAKE_SOURCE_DIR}/third_party/glad/include
            ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad
        )
        add_test(NAME InteractionStressTest COMMAND test_interaction_stress)
        set_tests_properties(InteractionStressTest PROPERTIES
            WORKING_DIRECTORY "${QT_RUNTIME_DEST}"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH};QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::Core>/platforms"
            TIMEOUT 120
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_interaction_stress deploy_qt_runtime)
        endif()

        # 5.4 Resize/toggle stress test (FBO recreation + panel toggling)
        add_executable(test_resize_stress
            tests/ui/test_resize_stress.cpp
        )
        target_link_libraries(test_resize_stress PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Quick
            Qt6::OpenGL
            Qt6::OpenGLWidgets
            Qt6::Test
            ${QT6_MULTIMEDIA_LINK_LIBS}
            quantumverse_core
        )
        if(WIN32)
            target_link_libraries(test_resize_stress PRIVATE opengl32 dbghelp)
        endif()
        set_target_properties(test_resize_stress PROPERTIES AUTOMOC ON)
        target_compile_definitions(test_resize_stress PRIVATE QUANTUMVERSE_USE_QML)
        target_include_directories(test_resize_stress PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/third_party
            ${CMAKE_SOURCE_DIR}/third_party/glad/include
            ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad
        )
        add_test(NAME ResizeStressTest COMMAND test_resize_stress)
        set_tests_properties(ResizeStressTest PROPERTIES
            WORKING_DIRECTORY "${QT_RUNTIME_DEST}"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH};QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::Core>/platforms"
            TIMEOUT 120
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_resize_stress deploy_qt_runtime)
        endif()

        # 5.5 Memory leak detection (requires ASan build)
        if(QUANTUMVERSE_USE_ASAN)
            add_executable(test_memory_leak tests/ui/test_memory_leak.cpp)
            target_link_libraries(test_memory_leak PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
            set_target_properties(test_memory_leak PROPERTIES AUTOMOC ON)
            target_compile_definitions(test_memory_leak PRIVATE QUANTUMVERSE_USE_QML)
            add_test(NAME MemoryLeakTest COMMAND test_memory_leak)
            set_tests_properties(MemoryLeakTest PROPERTIES
                WORKING_DIRECTORY "${QT_RUNTIME_DEST}"
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
                TIMEOUT 120
            )
            if(TARGET deploy_qt_runtime)
                add_dependencies(test_memory_leak deploy_qt_runtime)
            endif()
        endif()

        # 5.6 Crash handler verification (Windows-only)
        if(WIN32)
            add_executable(crash_helper
                tests/utils/crash_helper.cpp
            )
            target_link_libraries(crash_helper PRIVATE quantumverse_core)
            if(WIN32)
                target_link_libraries(crash_helper PRIVATE dbghelp)
            endif()
            if(TARGET deploy_qt_runtime)
                add_dependencies(crash_helper deploy_qt_runtime)
            endif()

            add_executable(test_crash_handler tests/utils/test_crash_handler.cpp)
            target_link_libraries(test_crash_handler PRIVATE Qt6::Core Qt6::Test)
            set_target_properties(test_crash_handler PROPERTIES AUTOMOC ON)
            add_test(NAME CrashHandlerTest COMMAND test_crash_handler)
            set_tests_properties(CrashHandlerTest PROPERTIES
                WORKING_DIRECTORY "${QT_RUNTIME_DEST}"
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
                TIMEOUT 30
            )
            add_dependencies(test_crash_handler crash_helper)

            # 9.1 Frame diagnostics JSON output
            add_executable(test_frame_diagnostics
                tests/utils/test_frame_diagnostics.cpp
            )
            target_link_libraries(test_frame_diagnostics PRIVATE Qt6::Core Qt6::Test quantumverse_core)
            set_target_properties(test_frame_diagnostics PROPERTIES AUTOMOC ON)
            add_test(NAME DiagnosticsOutputTest COMMAND test_frame_diagnostics)
            set_tests_properties(DiagnosticsOutputTest PROPERTIES
                WORKING_DIRECTORY "${QT_RUNTIME_DEST}"
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
                TIMEOUT 30
            )

            # 9.2 HUD toggle test
            add_executable(test_hud_toggle
                tests/utils/test_hud_toggle.cpp
            )
            target_link_libraries(test_hud_toggle PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Quick Qt6::Test ${QT6_MULTIMEDIA_LINK_LIBS} quantumverse_core)
            set_target_properties(test_hud_toggle PROPERTIES AUTOMOC ON)
            target_compile_definitions(test_hud_toggle PRIVATE QUANTUMVERSE_USE_QML)
            target_include_directories(test_hud_toggle PRIVATE
                ${CMAKE_SOURCE_DIR}/tests
                ${CMAKE_SOURCE_DIR}/src
                ${CMAKE_SOURCE_DIR}/third_party
                ${CMAKE_SOURCE_DIR}/third_party/glad/include
                ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad
            )
            add_test(NAME HUDToggleTest COMMAND test_hud_toggle)
            set_tests_properties(HUDToggleTest PROPERTIES
                WORKING_DIRECTORY "${QT_RUNTIME_DEST}"
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
                TIMEOUT 30
            )
            if(TARGET deploy_qt_runtime)
                add_dependencies(test_hud_toggle deploy_qt_runtime)
            endif()

            # PaperGenerator test (Phase 3.11) ??? LaTeX escaping + PDF output
            add_executable(test_paper_generator tests/test_paper_generator.cpp)
            target_link_libraries(test_paper_generator PRIVATE Qt6::Core Qt6::Test quantumverse_core)
            set_target_properties(test_paper_generator PROPERTIES AUTOMOC ON)
            target_include_directories(test_paper_generator PRIVATE
                ${CMAKE_SOURCE_DIR}/tests
                ${CMAKE_SOURCE_DIR}/src
                ${CMAKE_SOURCE_DIR}/third_party
                ${CMAKE_SOURCE_DIR}/third_party/glad/include
                ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad
            )
            add_test(NAME PaperGeneratorTest COMMAND test_paper_generator)
            set_tests_properties(PaperGeneratorTest PROPERTIES
                WORKING_DIRECTORY "${QT_RUNTIME_DEST}"
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
                TIMEOUT 30
            )
        endif()
    endif()
endif()

# Test executables
if(QUANTUMVERSE_BUILD_TESTS AND NOT QUANTUMVERSE_BUILD_WASM)
    # Core math tests
    add_executable(test_vector4d tests/test_vector4d.cpp)
    target_link_libraries(test_vector4d PRIVATE quantumverse_core)
    add_test(NAME Vector4DTest COMMAND test_vector4d)
    
    add_executable(test_matrix4x4 tests/test_matrix4x4.cpp)
    target_link_libraries(test_matrix4x4 PRIVATE quantumverse_core)
    add_test(NAME Matrix4x4Test COMMAND test_matrix4x4)

    add_executable(test_lensing_time_delay
        tests/test_lensing_time_delay.cpp
    )
    target_link_libraries(test_lensing_time_delay PRIVATE Qt6::Core quantumverse_core)
    add_test(NAME LensingTimeDelayTest COMMAND test_lensing_time_delay)

    add_executable(test_frb_generator
        tests/test_frb_generator.cpp
    )
    target_link_libraries(test_frb_generator PRIVATE Qt6::Core quantumverse_core)
    target_include_directories(test_frb_generator PRIVATE ${CMAKE_SOURCE_DIR}/tests)
    add_test(NAME FRBGeneratorTest COMMAND test_frb_generator)

    add_executable(test_pta_spectrum
        tests/test_pta_spectrum.cpp
    )
    target_link_libraries(test_pta_spectrum PRIVATE Qt6::Core quantumverse_core)
    target_include_directories(test_pta_spectrum PRIVATE ${CMAKE_SOURCE_DIR}/tests)
    add_test(NAME PTASpectrumTest COMMAND test_pta_spectrum)

    add_executable(test_gw_ringdown
        tests/test_gw_ringdown.cpp
    )
    target_link_libraries(test_gw_ringdown PRIVATE Qt6::Core quantumverse_core)
    target_include_directories(test_gw_ringdown PRIVATE ${CMAKE_SOURCE_DIR}/tests)
    add_test(NAME GWRingdownTest COMMAND test_gw_ringdown)

    add_executable(test_photon_sphere
        tests/test_photon_sphere.cpp
    )
    target_link_libraries(test_photon_sphere PRIVATE Qt6::Core quantumverse_core)
    target_include_directories(test_photon_sphere PRIVATE ${CMAKE_SOURCE_DIR}/tests)
    add_test(NAME PhotonSphereTest COMMAND test_photon_sphere)

    add_executable(test_scan_summary_builder
        tests/test_scan_summary_builder.cpp
    )
    target_link_libraries(test_scan_summary_builder PRIVATE Qt6::Core quantumverse_core)
    target_include_directories(test_scan_summary_builder PRIVATE ${CMAKE_SOURCE_DIR}/tests)
    add_test(NAME ScanSummaryBuilderTest COMMAND test_scan_summary_builder)

    add_executable(test_bayesian_evidence
        tests/test_bayesian_evidence.cpp
    )
    target_link_libraries(test_bayesian_evidence PRIVATE Qt6::Core quantumverse_core)
    target_include_directories(test_bayesian_evidence PRIVATE ${CMAKE_SOURCE_DIR}/tests)
    add_test(NAME BayesianEvidenceTest COMMAND test_bayesian_evidence)

    add_executable(test_experiment_smoke
        tests/test_experiment_smoke.cpp
    )
    target_link_libraries(test_experiment_smoke PRIVATE Qt6::Core quantumverse_core)
    target_include_directories(test_experiment_smoke PRIVATE ${CMAKE_SOURCE_DIR}/tests)
    add_test(NAME ExperimentSmokeTest COMMAND test_experiment_smoke)

    # Property-based tests
    add_executable(test_property_based
        tests/unit/test_property_based.cpp
    )
    target_include_directories(test_property_based PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(test_property_based PRIVATE quantumverse_core)
    add_test(NAME PropertyBasedTest COMMAND test_property_based)

    # 5.2 UI4D unit test (camera, slicing, probe, causal graph)
    # Guarded by QUANTUMVERSE_USE_QT: test_ui4d links Qt6 (UI4D/QML sources) and
    # would otherwise fail to configure in jobs that build without Qt enabled.
    if(QUANTUMVERSE_USE_QT)
        add_executable(test_ui4d
            tests/unit/test_ui4d.cpp
        )
        target_include_directories(test_ui4d PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/third_party
            ${CMAKE_SOURCE_DIR}/third_party/glad/include
            ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad
        )
        target_link_libraries(test_ui4d PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Test ${QT6_MULTIMEDIA_LINK_LIBS} quantumverse_core)
        if(WIN32)
            target_link_libraries(test_ui4d PRIVATE opengl32 dbghelp)
        endif()
        set_target_properties(test_ui4d PROPERTIES AUTOMOC ON)
        target_compile_definitions(test_ui4d PRIVATE QUANTUMVERSE_USE_QML)
        add_test(NAME UI4DTest COMMAND test_ui4d --mock)
        set_tests_properties(UI4DTest PROPERTIES
            WORKING_DIRECTORY "${QT_RUNTIME_DEST}"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH};QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::Core>/platforms"
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_ui4d deploy_qt_runtime)
        endif()

        # SandboxManager unit test (Qt-dependent: QObject + JSON persistence)
        add_executable(test_sandboxmanager
            tests/unit/test_sandboxmanager.cpp
        )
        target_include_directories(test_sandboxmanager PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/tests
            ${CMAKE_SOURCE_DIR}/third_party
            ${CMAKE_SOURCE_DIR}/third_party/glad/include
            ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad
        )
        target_link_libraries(test_sandboxmanager PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::Test quantumverse_core)
        if(WIN32)
            target_link_libraries(test_sandboxmanager PRIVATE opengl32 dbghelp)
        endif()
        set_target_properties(test_sandboxmanager PROPERTIES AUTOMOC ON)
        add_test(NAME SandboxManagerTest COMMAND test_sandboxmanager)
        set_tests_properties(SandboxManagerTest PROPERTIES
            WORKING_DIRECTORY "${QT_RUNTIME_DEST}"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        )
    endif()
    
    # Fuzz differential testing
    option(QUANTUMVERSE_USE_FUZZER "Build fuzzer targets" OFF)
    if(QUANTUMVERSE_USE_FUZZER)
        # -fsanitize=fuzzer is libFuzzer, which ships only with Clang. Building
        # these targets with GCC dies deep inside the compile step with the opaque
        # "unrecognized argument to '-fsanitize=' option: 'fuzzer'". Fail at
        # configure time instead, with an actionable message.
        if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            message(FATAL_ERROR
                "QUANTUMVERSE_USE_FUZZER=ON requires a Clang toolchain: libFuzzer "
                "(-fsanitize=fuzzer) is not implemented by ${CMAKE_CXX_COMPILER_ID}. "
                "Re-run CMake with -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++, "
                "or leave QUANTUMVERSE_USE_FUZZER=OFF.")
        endif()

        add_executable(fuzz_geodesic_differential tests/fuzz/fuzz_geodesic_differential.cpp)
        target_include_directories(fuzz_geodesic_differential PRIVATE
            ${CMAKE_SOURCE_DIR}/src
        )
        target_link_libraries(fuzz_geodesic_differential PRIVATE quantumverse_core)
        target_compile_options(fuzz_geodesic_differential PRIVATE -fsanitize=fuzzer)
        target_link_options(fuzz_geodesic_differential PRIVATE -fsanitize=fuzzer)

        add_executable(fuzz_discovery_instruments
            tests/fuzz/fuzz_discovery_instruments.cpp
        )
        target_include_directories(fuzz_discovery_instruments PRIVATE
                ${CMAKE_SOURCE_DIR}/src
        )
        target_link_libraries(fuzz_discovery_instruments PRIVATE quantumverse_core)
        target_compile_options(fuzz_discovery_instruments PRIVATE -fsanitize=fuzzer)
        target_link_options(fuzz_discovery_instruments PRIVATE -fsanitize=fuzzer)
    endif()

    # Standalone fuzz driver (always built when tests are enabled)
    add_executable(fuzz_instruments
        tests/fuzz/fuzz_instruments.cpp
    )
    target_link_libraries(fuzz_instruments PRIVATE quantumverse_core)
    add_test(NAME FuzzInstrumentsTest COMMAND fuzz_instruments)
    set_tests_properties(FuzzInstrumentsTest PROPERTIES
        LABELS "fuzz;discovery"
        TIMEOUT 300
    )

    # Phase 21.3: Fuzz-driven formal verification of physics invariants
    add_executable(fuzz_physics_invariants
        tests/fuzz/fuzz_physics_invariants.cpp
    )
    target_link_libraries(fuzz_physics_invariants PRIVATE quantumverse_core)
    if(MSVC)
        target_compile_definitions(fuzz_physics_invariants PRIVATE _CRT_SECURE_NO_WARNINGS=1)
    endif()
    add_test(NAME FuzzPhysicsInvariantsTest COMMAND fuzz_physics_invariants)
    set_tests_properties(FuzzPhysicsInvariantsTest PROPERTIES
        LABELS "fuzz;physics"
        TIMEOUT 120
        WORKING_DIRECTORY "${QT_RUNTIME_DEST}"
        ENVIRONMENT "PATH=${QT_RUNTIME_DEST};$ENV{PATH};FUZZ_CYCLES=100"
    )

    # Phase 22: Autonomous discovery runner (outputs JSON to stdout)
    add_executable(theory_discovery_runner
        tools/theory_discovery_runner.cpp
    )
    target_link_libraries(theory_discovery_runner PRIVATE quantumverse_core)

    # Performance regression gate: actual benchmark measurements
    add_executable(test_performance_regression
        tests/test_performance_regression.cpp
    )
    target_link_libraries(test_performance_regression PRIVATE quantumverse_core)
    add_test(NAME PerformanceRegressionTest COMMAND test_performance_regression)
    set_tests_properties(PerformanceRegressionTest PROPERTIES
        LABELS "performance"
        TIMEOUT 120
    )
    
    # Performance benchmarks
    option(QUANTUMVERSE_USE_BENCHMARK "Build benchmarks" OFF)
    if(QUANTUMVERSE_USE_BENCHMARK)
        include(FetchContent)
        FetchContent_Declare(
            google-benchmark
            GIT_REPOSITORY https://github.com/google/benchmark.git
            GIT_TAG v1.8.3
        )
        FetchContent_MakeAvailable(google-benchmark)
        
        add_executable(benchmark_physics tests/benchmark/benchmark_physics.cpp)
        target_link_libraries(benchmark_physics PRIVATE quantumverse_core google-benchmark::benchmark)
    endif()
    
    # Physics validation tests
    add_executable(test_mercury_precession
        tests/test_mercury_precession.cpp
    )
    target_link_libraries(test_mercury_precession PRIVATE quantumverse_core Qt6::Core)
    target_include_directories(test_mercury_precession PRIVATE src)
    add_test(NAME MercuryPrecessionTest COMMAND test_mercury_precession)

    # Integrator-level test: 1PN Schwarzschild correction, Mercury perihelion advance
    # (QtTest version: tests/physics/test_mercury_1pn.cpp)
    if(QUANTUMVERSE_USE_QT AND NOT QUANTUMVERSE_BUILD_WASM)
    add_executable(test_mercury_1pn_physics
        tests/physics/test_mercury_1pn.cpp
        third_party/glad/glad.cpp
    )
    target_include_directories(test_mercury_1pn_physics PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/third_party
        ${CMAKE_SOURCE_DIR}/third_party/glad/include
        ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad
    )
    target_link_libraries(test_mercury_1pn_physics PRIVATE Qt6::Core Qt6::Gui Qt6::Test quantumverse_core)
    if(WIN32)
        target_link_libraries(test_mercury_1pn_physics PRIVATE opengl32 dbghelp)
    endif()
    set_target_properties(test_mercury_1pn_physics PROPERTIES AUTOMOC ON)
    add_test(NAME Mercury1PNTest COMMAND test_mercury_1pn_physics)
    set_tests_properties(Mercury1PNTest PROPERTIES
        LABELS "physics;unit"
    )
    endif()

    # Integrator-level test: 1PN Schwarzschild correction, Mercury perihelion advance
    if(QUANTUMVERSE_USE_QT AND NOT QUANTUMVERSE_BUILD_WASM)
    add_executable(test_mercury_1pn tests/test_mercury_1pn.cpp third_party/glad/glad.cpp)
    target_include_directories(test_mercury_1pn PRIVATE src third_party/glad/include third_party/glad/include/glad)
    target_link_libraries(test_mercury_1pn PRIVATE Qt6::Core Qt6::Gui quantumverse_core)
    add_test(NAME Mercury1PNAdanceTest COMMAND test_mercury_1pn)
    endif()
    
    add_executable(test_light_deflection tests/test_light_deflection.cpp)
    target_link_libraries(test_light_deflection PRIVATE quantumverse_core)
    add_test(NAME LightDeflectionTest COMMAND test_light_deflection)
    
    add_executable(test_gravitational_redshift tests/test_gravitational_redshift.cpp)
    target_link_libraries(test_gravitational_redshift PRIVATE quantumverse_core)
    add_test(NAME GravitationalRedshiftTest COMMAND test_gravitational_redshift)

    # Runtime smoke test: launches quantumverse_qml headless and asserts on [REALITY] dumps
    if(QUANTUMVERSE_USE_QT AND NOT QUANTUMVERSE_BUILD_WASM)
    add_executable(test_runtime tests/smoke/test_runtime.cpp)
    target_link_libraries(test_runtime PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
    set_target_properties(test_runtime PROPERTIES AUTOMOC ON)
    add_test(NAME RuntimeSmokeTest COMMAND test_runtime)
    set_tests_properties(RuntimeSmokeTest PROPERTIES
        WORKING_DIRECTORY "${QT_RUNTIME_DEST}"
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH};QT_QPA_PLATFORM_PLUGIN_PATH=$<TARGET_FILE_DIR:Qt6::Core>/platforms"
        TIMEOUT 300
    )
    endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_config_loader.cpp)
    add_executable(test_config_loader tests/test_config_loader.cpp)
    target_link_libraries(test_config_loader PRIVATE quantumverse_core nlohmann_json)
    add_test(NAME ConfigLoaderTest COMMAND test_config_loader)
    set_tests_properties(ConfigLoaderTest PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_nbody.cpp AND QUANTUMVERSE_USE_QT AND Qt6Core_FOUND)
    add_executable(test_nbody tests/test_nbody.cpp)
    target_link_libraries(test_nbody PRIVATE quantumverse_core Qt6::Core Qt6::Gui)
    target_include_directories(test_nbody PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/third_party/glad/include ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad)
    add_test(NAME NBodyTest COMMAND test_nbody)
    set_tests_properties(NBodyTest PROPERTIES
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        TIMEOUT 180
    )
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_scenario_system.cpp)
    # Scenario system tests
    add_executable(test_scenario_system tests/test_scenario_system.cpp)
    target_link_libraries(test_scenario_system PRIVATE quantumverse_core nlohmann_json)
    target_include_directories(test_scenario_system PRIVATE ${CMAKE_SOURCE_DIR}/src)
    add_test(NAME ScenarioSystemTest COMMAND test_scenario_system)
endif()

# Copy config files to build directory (only if they exist)
if(EXISTS ${CMAKE_SOURCE_DIR}/config)
    file(COPY ${CMAKE_SOURCE_DIR}/config DESTINATION ${CMAKE_BINARY_DIR})
endif()
if(EXISTS ${CMAKE_SOURCE_DIR}/data/shaders)
    file(COPY ${CMAKE_SOURCE_DIR}/data/shaders DESTINATION ${CMAKE_BINARY_DIR}/data)
endif()
if(EXISTS ${CMAKE_SOURCE_DIR}/data/shaders/blackhole)
    file(COPY ${CMAKE_SOURCE_DIR}/data/shaders/blackhole DESTINATION ${CMAKE_BINARY_DIR}/data/shaders/blackhole)
endif()
if(EXISTS ${CMAKE_SOURCE_DIR}/data/scenarios)
    file(COPY ${CMAKE_SOURCE_DIR}/data/scenarios DESTINATION ${CMAKE_BINARY_DIR}/data)
endif()
if(EXISTS ${CMAKE_SOURCE_DIR}/data/tours)
    file(COPY ${CMAKE_SOURCE_DIR}/data/tours DESTINATION ${CMAKE_BINARY_DIR}/data)
endif()
if(EXISTS ${CMAKE_SOURCE_DIR}/data/ml)
    file(COPY ${CMAKE_SOURCE_DIR}/data/ml DESTINATION ${CMAKE_BINARY_DIR}/data)
endif()

    add_executable(test_frame_dragging tests/test_frame_dragging.cpp)
    target_link_libraries(test_frame_dragging PRIVATE quantumverse_core)
    add_test(NAME FrameDraggingTest COMMAND test_frame_dragging)
    
    # Quantum gravity tests
    add_executable(test_cdt tests/test_cdt.cpp)
    target_link_libraries(test_cdt PRIVATE quantumverse_core)
    add_test(NAME CDTTest COMMAND test_cdt)
    
    add_executable(test_spin_foam tests/test_spin_foam.cpp)
    target_link_libraries(test_spin_foam PRIVATE quantumverse_core)
    add_test(NAME SpinFoamTest COMMAND test_spin_foam)
    
    # Core spacetime tests
    add_executable(test_spacetime tests/test_spacetime.cpp)
    target_link_libraries(test_spacetime PRIVATE quantumverse_core)
    add_test(NAME SpacetimeTest COMMAND test_spacetime)

    # Metric epoch test (Phase 1.1)
    add_executable(test_metric_epoch
        tests/test_metric_epoch.cpp
    )
    target_link_libraries(test_metric_epoch PRIVATE quantumverse_core Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
    target_include_directories(test_metric_epoch PRIVATE src third_party/glad/include third_party/glad/include/glad)
    target_compile_definitions(test_metric_epoch PRIVATE QUANTUMVERSE_USE_QML)
    set_target_properties(test_metric_epoch PROPERTIES AUTOMOC ON)
    add_test(NAME MetricEpochTest COMMAND test_metric_epoch)

    # Skeleton tests (Phase 0.3-now) ??? real assertions land with their modules in Phase 3
    add_executable(test_neural_ode tests/test_neural_ode.cpp)
    target_link_libraries(test_neural_ode PRIVATE quantumverse_core)
    add_test(NAME NeuralOdeTest COMMAND test_neural_ode)

    add_executable(test_neural_ode_onnx tests/test_neural_ode_onnx.cpp)
    target_link_libraries(test_neural_ode_onnx PRIVATE quantumverse_core)
    add_test(NAME NeuralOdeOnnxTest COMMAND test_neural_ode_onnx)

    add_executable(test_curvature_flow tests/test_curvature_flow.cpp)
    target_link_libraries(test_curvature_flow PRIVATE quantumverse_core)
    add_test(NAME CurvatureFlowTest COMMAND test_curvature_flow)

    # Reverse AD test (Phase 1.2)
    add_executable(test_reverse_ad tests/test_reverse_ad.cpp)
    target_link_libraries(test_reverse_ad PRIVATE quantumverse_core)
    add_test(NAME ReverseADTest COMMAND test_reverse_ad)

    # Differentiable benchmark (Phase 1.2)
    add_executable(test_differentiable_benchmark tests/test_differentiable_benchmark.cpp)
    target_link_libraries(test_differentiable_benchmark PRIVATE quantumverse_core)
    add_test(NAME DifferentiableBenchmarkTest COMMAND test_differentiable_benchmark)

    add_executable(test_rl_discovery tests/test_rl_discovery.cpp)
    target_link_libraries(test_rl_discovery PRIVATE quantumverse_core)
    add_test(NAME RlDiscoveryTest COMMAND test_rl_discovery)

    add_executable(test_anomaly_detection tests/test_anomaly_detection.cpp)
    target_link_libraries(test_anomaly_detection PRIVATE quantumverse_core)
    add_test(NAME AnomalyDetectionTest COMMAND test_anomaly_detection)

    add_executable(test_solar_system tests/test_solar_system.cpp)
    target_link_libraries(test_solar_system PRIVATE quantumverse_core)
    add_test(NAME SolarSystemTest COMMAND test_solar_system)

    # Kerr metric test
if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_kerr_metric.cpp)
    add_executable(test_kerr_metric
        tests/test_kerr_metric.cpp
    )
    target_link_libraries(test_kerr_metric PRIVATE quantumverse_core)
    add_test(NAME KerrMetricTest COMMAND test_kerr_metric)
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_lensing.cpp)
    # Gravitational lensing test
    add_executable(test_lensing
        tests/test_lensing.cpp
    )
    target_include_directories(test_lensing PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/third_party
        ${CMAKE_SOURCE_DIR}/third_party/glad/include
        ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad)
    target_link_libraries(test_lensing PRIVATE quantumverse_core)
    if(WIN32)
        target_link_libraries(test_lensing PRIVATE opengl32)
    endif()
    add_test(NAME GravitationalLensingTest COMMAND test_lensing)
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_educational.cpp)
    # TourController is a QObject subclass (moc), so this target needs Qt.
    # Guard with QUANTUMVERSE_USE_QT: the Formal Verification / CBMC
    # workflows configure CMake without Qt, and an unconditional
    # Qt6::Core link breaks their configure step.
    if(QUANTUMVERSE_USE_QT AND Qt6Core_FOUND)
        # Educational tour controller (data model + state machine)
        add_executable(test_educational
            tests/test_educational.cpp
        )
        target_include_directories(test_educational PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/third_party
        )
        target_link_libraries(test_educational PRIVATE Qt6::Core Qt6::Gui quantumverse_core)
        add_test(NAME EducationalTourTest COMMAND test_educational)
        set_tests_properties(EducationalTourTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        )
        set_target_properties(test_educational PROPERTIES AUTOMOC ON)
    endif()
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_tour_manager.cpp)
    if(QUANTUMVERSE_USE_QT AND Qt6Core_FOUND)
        # TourManager directory scanner (smoke test against data/tours/)
        add_executable(test_tour_manager
            tests/test_tour_manager.cpp
        )
        target_include_directories(test_tour_manager PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/third_party
        )
        target_link_libraries(test_tour_manager PRIVATE
            quantumverse_core
            Qt6::Core
            Qt6::Gui
        )
        add_test(NAME TourManagerTest COMMAND test_tour_manager)
        set_tests_properties(TourManagerTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        )
        set_target_properties(test_tour_manager PROPERTIES AUTOMOC ON)
    endif()
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_pbr_rendering.cpp)
    # PBR rendering test
    add_executable(test_pbr_rendering
        tests/test_pbr_rendering.cpp
    )
    target_include_directories(test_pbr_rendering PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(test_pbr_rendering PRIVATE quantumverse_core)
    add_test(NAME PBRRenderingTest COMMAND test_pbr_rendering)
endif()

    add_executable(test_geodesic tests/test_geodesic.cpp)
    target_link_libraries(test_geodesic PRIVATE quantumverse_core)
    add_test(NAME GeodesicTest COMMAND test_geodesic)

    # Sandbox: geodesic divergence benchmark at high Kerr spins
    add_executable(test_geodesic_divergence
        tests/test_geodesic_divergence.cpp
    )
    target_link_libraries(test_geodesic_divergence PRIVATE quantumverse_core)
    add_test(NAME GeodesicDivergenceTest COMMAND test_geodesic_divergence)

    # ML dead-UI fix: AnomalyMonitor modelLoaded gating
    add_executable(test_ml_dead_ui
        tests/test_ml_dead_ui.cpp
    )
    target_link_libraries(test_ml_dead_ui PRIVATE quantumverse_core Qt6::Core Qt6::Gui Qt6::Quick Qt6::Widgets Qt6::Test)
    target_include_directories(test_ml_dead_ui PRIVATE ${CMAKE_SOURCE_DIR}/tests)
    target_compile_definitions(test_ml_dead_ui PRIVATE QUANTUMVERSE_USE_QML)
    add_test(NAME MLDeadUITest COMMAND test_ml_dead_ui)

    add_executable(test_singularity_handler tests/test_singularity_handler.cpp)
    target_link_libraries(test_singularity_handler PRIVATE quantumverse_core)
    add_test(NAME SingularityHandlerTest COMMAND test_singularity_handler)

    # Scenario integration test (headless pipeline validation)
    add_executable(test_scenario_integration
        tests/integration/test_scenario_integration.cpp
    )
    target_link_libraries(test_scenario_integration PRIVATE quantumverse_core)
    add_test(NAME ScenarioIntegrationTest COMMAND test_scenario_integration)

    # Neutron star glitch phase detector (discovery instrument)
    add_executable(test_neutron_glitch
        tests/test_neutron_glitch.cpp
    )
    target_link_libraries(test_neutron_glitch PRIVATE quantumverse_core)
    add_test(NAME NeutronStarGlitchTest COMMAND test_neutron_glitch)

    # Ultralight DM wave interferometer (discovery instrument)
    add_executable(test_ultralight_dm_wave
        tests/test_ultralight_dm_wave.cpp
    )
    target_link_libraries(test_ultralight_dm_wave PRIVATE quantumverse_core)
    add_test(NAME UltralightDMWaveTest COMMAND test_ultralight_dm_wave)

    # Galactic rotation curve scanner (discovery instrument)
    add_executable(test_galactic_rotation_curve
        tests/test_galactic_rotation_curve.cpp
    )
    target_link_libraries(test_galactic_rotation_curve PRIVATE quantumverse_core)
    add_test(NAME GalacticRotationCurveTest COMMAND test_galactic_rotation_curve)

    # Fine-structure constant drift observatory (discovery instrument)
    add_executable(test_fine_structure_drift
        tests/test_fine_structure_drift.cpp
    )
    target_link_libraries(test_fine_structure_drift PRIVATE quantumverse_core)
    add_test(NAME FineStructureConstantDriftTest COMMAND test_fine_structure_drift)

    # Boson star collision predictor (discovery instrument)
    add_executable(test_boson_star_collision
        tests/test_boson_star_collision.cpp
    )
    target_link_libraries(test_boson_star_collision PRIVATE quantumverse_core)
    add_test(NAME BosonStarCollisionTest COMMAND test_boson_star_collision)

    # Hawking radiation detector (discovery instrument)
    add_executable(test_hawking_radiation_detector
        tests/discovery/test_hawking_radiation_detector.cpp
    )
    target_link_libraries(test_hawking_radiation_detector PRIVATE quantumverse_core)
    add_test(NAME HawkingRadiationDetectorTest COMMAND test_hawking_radiation_detector)

    # Kerr no-hair violation analyzer (discovery instrument)
    add_executable(test_kerr_no_hair_violation
        tests/discovery/test_kerr_no_hair_violation_analyzer.cpp
    )
    target_link_libraries(test_kerr_no_hair_violation PRIVATE quantumverse_core)
    add_test(NAME KerrNoHairViolationTest COMMAND test_kerr_no_hair_violation)

    # Merging binary inspiral analyzer (discovery instrument)
    add_executable(test_merging_binary_inspiral
        tests/discovery/test_merging_binary_inspiral_analyzer.cpp
    )
    target_link_libraries(test_merging_binary_inspiral PRIVATE quantumverse_core)
    add_test(NAME MergingBinaryInspiralTest COMMAND test_merging_binary_inspiral)

    # Black hole jet anomaly recogniser (discovery instrument)
    add_executable(test_black_hole_jet_anomaly
        tests/test_black_hole_jet_anomaly.cpp
    )
    target_link_libraries(test_black_hole_jet_anomaly PRIVATE quantumverse_core)
    add_test(NAME BlackHoleJetAnomalyTest COMMAND test_black_hole_jet_anomaly)

    # Primordial lithium crisis solver (discovery instrument)
    add_executable(test_primordial_lithium
        tests/test_primordial_lithium.cpp
    )
    target_link_libraries(test_primordial_lithium PRIVATE quantumverse_core)
    add_test(NAME PrimordialLithiumCrisisTest COMMAND test_primordial_lithium)

    # Galactic tidal stream cartographer (discovery instrument)
    add_executable(test_galactic_tidal_stream
        tests/test_galactic_tidal_stream.cpp
    )
    target_link_libraries(test_galactic_tidal_stream PRIVATE quantumverse_core)
    add_test(NAME GalacticTidalStreamTest COMMAND test_galactic_tidal_stream)

    # Recombination constant variation imager (discovery instrument)
    add_executable(test_recombination_constant_variation
        tests/test_recombination_constant_variation.cpp
    )
    target_link_libraries(test_recombination_constant_variation PRIVATE quantumverse_core)
    add_test(NAME RecombinationConstantVariationTest COMMAND test_recombination_constant_variation)

    # Exoplanetary TTV fifth-force hunter (discovery instrument)
    add_executable(test_exoplanetary_ttv
        tests/test_exoplanetary_ttv.cpp
    )
    target_link_libraries(test_exoplanetary_ttv PRIVATE quantumverse_core)
    add_test(NAME ExoplanetaryTTVFifthForceHunterTest COMMAND test_exoplanetary_ttv)

    # Neural ODE accuracy test
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_neural_ode_accuracy.cpp)
        add_executable(test_neural_ode_accuracy
            tests/test_neural_ode_accuracy.cpp
        )
        target_link_libraries(test_neural_ode_accuracy PRIVATE quantumverse_core)
        add_test(NAME NeuralODEAccuracyTest COMMAND test_neural_ode_accuracy WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    endif()

    # MetricGNN accuracy test
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_metric_gnn.cpp)
        add_executable(test_metric_gnn
            tests/test_metric_gnn.cpp
        )
        target_link_libraries(test_metric_gnn PRIVATE quantumverse_core)
        add_test(NAME MetricGNNTest COMMAND test_metric_gnn)
    endif()

    # Surrogate integration test
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_surrogate_integration.cpp)
        add_executable(test_surrogate_integration
            tests/test_surrogate_integration.cpp
        )
        target_link_libraries(test_surrogate_integration PRIVATE quantumverse_core)
        add_test(NAME SurrogateIntegrationTest COMMAND test_surrogate_integration)
    endif()

    # Multi-messenger live data test
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_multimessenger.cpp)
        add_executable(test_multimessenger
            tests/test_multimessenger.cpp
        )
        target_link_libraries(test_multimessenger PRIVATE quantumverse_core)
        add_test(NAME MultiMessengerTest COMMAND test_multimessenger)
    endif()

    # ML Anomaly Detector unit test
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_anomaly_detector.cpp)
        add_executable(test_anomaly_detector
            tests/test_anomaly_detector.cpp
        )
        target_link_libraries(test_anomaly_detector PRIVATE quantumverse_core)
        add_test(NAME AnomalyDetectorTest COMMAND test_anomaly_detector)
    endif()

    # ML TrainingDataCollector unit test
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_training_data_collector.cpp)
        add_executable(test_training_data_collector
            tests/test_training_data_collector.cpp
        )
        target_link_libraries(test_training_data_collector PRIVATE quantumverse_core)
        add_test(NAME TrainingDataCollectorTest COMMAND test_training_data_collector)
    endif()

    # ML SessionRecorder unit test ??? round-trip write/read fidelity
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_session_recorder.cpp)
        add_executable(test_session_recorder
            tests/test_session_recorder.cpp
        )
        target_link_libraries(test_session_recorder PRIVATE quantumverse_core)
        add_test(NAME SessionRecorderTest COMMAND test_session_recorder)
    endif()

    if(TARGET quantumverse_qml)
        # ML AnomalyMonitor integration test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_anomaly_monitor.cpp)
            add_executable(test_anomaly_monitor
                tests/test_anomaly_monitor.cpp
            )
            target_link_libraries(test_anomaly_monitor PRIVATE quantumverse_core Qt6::Core Qt6::Gui Qt6::Quick Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Test)
            set_target_properties(test_anomaly_monitor PROPERTIES AUTOMOC ON)
            target_compile_definitions(test_anomaly_monitor PRIVATE QUANTUMVERSE_USE_QML)
            add_test(NAME AnomalyMonitorTest COMMAND test_anomaly_monitor)
            set_tests_properties(AnomalyMonitorTest PROPERTIES
                WORKING_DIRECTORY "${QT_RUNTIME_DEST}"
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH};QT_QPA_PLATFORM_PLUGIN_PATH=${QT_RUNTIME_DEST}/platforms"
            )
            if(TARGET deploy_qt_runtime)
                add_dependencies(test_anomaly_monitor deploy_qt_runtime)
            endif()
        endif()

        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_minimal_construct.cpp)
            add_executable(test_minimal_construct
                tests/test_minimal_construct.cpp
            )
            target_link_libraries(test_minimal_construct PRIVATE quantumverse_core Qt6::Core Qt6::Gui Qt6::Quick Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Test)
            set_target_properties(test_minimal_construct PROPERTIES AUTOMOC ON)
            target_compile_definitions(test_minimal_construct PRIVATE QUANTUMVERSE_USE_QML)
            add_test(NAME MinimalConstructTest COMMAND test_minimal_construct)
            set_tests_properties(MinimalConstructTest PROPERTIES
                WORKING_DIRECTORY "${QT_RUNTIME_DEST}"
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH};QT_QPA_PLATFORM_PLUGIN_PATH=${QT_RUNTIME_DEST}/platforms"
            )
            if(TARGET deploy_qt_runtime)
                add_dependencies(test_minimal_construct deploy_qt_runtime)
            endif()
        endif()

        # Phase 12: Fermi GBM adapter unit test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/data/test_fermi_gbm_adapter.cpp)
            add_executable(test_fermi_gbm_adapter
                tests/data/test_fermi_gbm_adapter.cpp
            )
            target_link_libraries(test_fermi_gbm_adapter PRIVATE quantumverse_core Qt6::Core)
        endif()

        # Phase 12: Swift BAT adapter unit test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/data/test_swift_bat_adapter.cpp)
            add_executable(test_swift_bat_adapter
                tests/data/test_swift_bat_adapter.cpp
            )
            target_link_libraries(test_swift_bat_adapter PRIVATE quantumverse_core Qt6::Core)
        endif()

        # Phase 12: GCN parser unit test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/data/test_gcn_parser.cpp)
            add_executable(test_gcn_parser
                tests/data/test_gcn_parser.cpp
            )
            target_link_libraries(test_gcn_parser PRIVATE quantumverse_core Qt6::Core)
            add_test(NAME GCNParserTest COMMAND test_gcn_parser)
        endif()

        # Phase 12: Kafka listener integration test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/data/test_kafka_listener.cpp)
            add_executable(test_kafka_listener
                tests/data/test_kafka_listener.cpp
            )
            target_link_libraries(test_kafka_listener PRIVATE quantumverse_core Qt6::Core Qt6::Test ${LIBRDKAFKA_LIBRARIES})
            set_target_properties(test_kafka_listener PROPERTIES AUTOMOC ON)
            add_test(NAME KafkaListenerTest COMMAND test_kafka_listener)
            set_tests_properties(KafkaListenerTest PROPERTIES
                WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            )
        endif()

        # Phase 12c: GCN archive replay stream test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/data/test_gcn_replay.cpp)
            add_executable(test_gcn_replay
                tests/data/test_gcn_replay.cpp
            )
            target_link_libraries(test_gcn_replay PRIVATE quantumverse_core Qt6::Core Qt6::Test)
            set_target_properties(test_gcn_replay PROPERTIES AUTOMOC ON)
            add_test(NAME GCNReplayStreamTest COMMAND test_gcn_replay)
            set_tests_properties(GCNReplayStreamTest PROPERTIES
                WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            )
        endif()

        # Phase 12d: GCN broker connectivity smoke test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/data/test_gcn_broker_connectivity.cpp)
            add_executable(test_gcn_broker_connectivity
                tests/data/test_gcn_broker_connectivity.cpp
            )
            target_link_libraries(test_gcn_broker_connectivity PRIVATE quantumverse_core Qt6::Core Qt6::Test ${LIBRDKAFKA_LIBRARIES})
            set_target_properties(test_gcn_broker_connectivity PROPERTIES AUTOMOC ON)
            add_test(NAME GCNBrokerConnectivityTest COMMAND test_gcn_broker_connectivity)
            set_tests_properties(GCNBrokerConnectivityTest PROPERTIES
                WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            )
        endif()

        # Phase 13: Multi-messenger correlator test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/data/test_multimessenger_correlator.cpp)
            add_executable(test_multimessenger_correlator
                tests/data/test_multimessenger_correlator.cpp
            )
            target_link_libraries(test_multimessenger_correlator PRIVATE quantumverse_core Qt6::Core Qt6::Test)
            target_compile_definitions(test_multimessenger_correlator PRIVATE QUANTUMVERSE_USE_QML)
            set_target_properties(test_multimessenger_correlator PROPERTIES AUTOMOC ON)
            add_test(NAME MultiMessengerCorrelatorTest COMMAND test_multimessenger_correlator)
            set_tests_properties(MultiMessengerCorrelatorTest PROPERTIES
                WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            )
        endif()

        # Phase 12b: DiscoveryPanelManager live ingest test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_discovery_panel_manager.cpp)
            add_executable(test_discovery_panel_manager
                tests/test_discovery_panel_manager.cpp
            )
            target_link_libraries(test_discovery_panel_manager PRIVATE quantumverse_core Qt6::Core Qt6::Test)
            target_compile_definitions(test_discovery_panel_manager PRIVATE QUANTUMVERSE_USE_QML)
            set_target_properties(test_discovery_panel_manager PROPERTIES AUTOMOC ON)
            add_test(NAME DiscoveryPanelManagerIngestTest COMMAND test_discovery_panel_manager)
            set_tests_properties(DiscoveryPanelManagerIngestTest PROPERTIES
                WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            )
        endif()

        # Layer 4 + Layer 2: UI wiring stub-catcher + multi-probe validator
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_ui_wiring.cpp)
            add_executable(test_ui_wiring
                tests/test_ui_wiring.cpp
            )
            target_link_libraries(test_ui_wiring PRIVATE quantumverse_core Qt6::Core Qt6::Test)
            target_include_directories(test_ui_wiring PRIVATE src third_party/glad/include third_party/glad/include/glad)
            target_compile_definitions(test_ui_wiring PRIVATE QUANTUMVERSE_USE_QML)
            set_target_properties(test_ui_wiring PROPERTIES AUTOMOC ON)
            add_test(NAME UiWiringTest COMMAND test_ui_wiring)
            set_tests_properties(UiWiringTest PROPERTIES
                WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            )
        endif()

        # Spacetime GPS telemetry feed test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_spacetime_gps.cpp)
            add_executable(test_spacetime_gps
                tests/test_spacetime_gps.cpp
            )
            target_link_libraries(test_spacetime_gps PRIVATE quantumverse_core Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
            target_include_directories(test_spacetime_gps PRIVATE src third_party/glad/include third_party/glad/include/glad)
            target_compile_definitions(test_spacetime_gps PRIVATE QUANTUMVERSE_USE_QML)
            set_target_properties(test_spacetime_gps PROPERTIES AUTOMOC ON)
            add_test(NAME SpacetimeGpsTest COMMAND test_spacetime_gps)
            set_tests_properties(SpacetimeGpsTest PROPERTIES
                WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            )
        endif()

        # Experiment runner test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_experiment_runner.cpp)
        add_executable(test_experiment_runner
                tests/test_experiment_runner.cpp
                )
            target_link_libraries(test_experiment_runner PRIVATE quantumverse_core Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Test)
            target_include_directories(test_experiment_runner PRIVATE src third_party/glad/include third_party/glad/include/glad)
            target_compile_definitions(test_experiment_runner PRIVATE QUANTUMVERSE_USE_QML)
            set_target_properties(test_experiment_runner PROPERTIES AUTOMOC ON)
            add_test(NAME ExperimentRunnerTest COMMAND test_experiment_runner)
            set_tests_properties(ExperimentRunnerTest PROPERTIES
                WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            )
        endif()

        # QML paper pipeline headless test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_qml_paper_pipeline.cpp)
            add_executable(test_qml_paper_pipeline
                tests/test_qml_paper_pipeline.cpp
                )
            target_link_libraries(test_qml_paper_pipeline PRIVATE Qt6::Core)
            target_include_directories(test_qml_paper_pipeline PRIVATE src third_party/glad/include third_party/glad/include/glad)
            target_compile_definitions(test_qml_paper_pipeline PRIVATE QUANTUMVERSE_USE_QML)
            set_target_properties(test_qml_paper_pipeline PROPERTIES AUTOMOC ON)
            add_test(NAME QmlPaperPipelineTest COMMAND test_qml_paper_pipeline)
            set_tests_properties(QmlPaperPipelineTest PROPERTIES
                WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
            )
        endif()

        # f(R) gravity provider test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_fr_gravity_provider.cpp)
            add_executable(test_fr_gravity_provider
                tests/test_fr_gravity_provider.cpp
            )
            target_link_libraries(test_fr_gravity_provider PRIVATE quantumverse_core Qt6::Core Qt6::Test)
            target_include_directories(test_fr_gravity_provider PRIVATE src third_party/glad/include third_party/glad/include/glad)
            target_compile_definitions(test_fr_gravity_provider PRIVATE QUANTUMVERSE_USE_QML)
            add_test(NAME FRGravityProviderTest COMMAND test_fr_gravity_provider)
        endif()

        # Hu-Sawicki f(R) provider test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_hu_sawicki_provider.cpp)
            add_executable(test_hu_sawicki_provider
                tests/test_hu_sawicki_provider.cpp
            )
            target_link_libraries(test_hu_sawicki_provider PRIVATE quantumverse_core Qt6::Core Qt6::Test)
            target_include_directories(test_hu_sawicki_provider PRIVATE src third_party/glad/include third_party/glad/include/glad)
            add_test(NAME HuSawickiProviderTest COMMAND test_hu_sawicki_provider)
        endif()

        # Charge sweep experiment test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_charge_sweep.cpp)
            add_executable(test_charge_sweep
                tests/test_charge_sweep.cpp
            )
            target_link_libraries(test_charge_sweep PRIVATE quantumverse_core Qt6::Core Qt6::Test)
            target_include_directories(test_charge_sweep PRIVATE src third_party/glad/include third_party/glad/include/glad)
            target_compile_definitions(test_charge_sweep PRIVATE QUANTUMVERSE_USE_QML)
            add_test(NAME ChargeSweepTest COMMAND test_charge_sweep)
            set_tests_properties(ChargeSweepTest PROPERTIES
                WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            )
        endif()

        # Frame dragging experiment test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_frame_dragging_experiment.cpp)
            add_executable(test_frame_dragging_experiment
                tests/test_frame_dragging_experiment.cpp
            )
            target_link_libraries(test_frame_dragging_experiment PRIVATE quantumverse_core Qt6::Core Qt6::Test)
            target_include_directories(test_frame_dragging_experiment PRIVATE src third_party/glad/include third_party/glad/include/glad)
            target_compile_definitions(test_frame_dragging_experiment PRIVATE QUANTUMVERSE_USE_QML)
            add_test(NAME FrameDraggingExperimentTest COMMAND test_frame_dragging_experiment)
            set_tests_properties(FrameDraggingExperimentTest PROPERTIES
                WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            )
        endif()

        # Cosmology validation experiment tests
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_cosmology_experiments.cpp)
            add_executable(test_cosmology_experiments
                tests/test_cosmology_experiments.cpp
            )
            target_link_libraries(test_cosmology_experiments PRIVATE quantumverse_core Qt6::Core Qt6::Network)
            target_include_directories(test_cosmology_experiments PRIVATE src third_party/glad/include third_party/glad/include/glad)
            target_compile_definitions(test_cosmology_experiments PRIVATE QUANTUMVERSE_USE_QML)
            set_target_properties(test_cosmology_experiments PROPERTIES AUTOMOC ON)
            add_test(NAME CosmologyExperimentsTest COMMAND test_cosmology_experiments)
            set_tests_properties(CosmologyExperimentsTest PROPERTIES
                WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
                ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            )
        endif()

    endif()
    # End-to-end discovery workflow test
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_e2e_discovery.cpp)
        add_executable(test_e2e_discovery
            tests/test_e2e_discovery.cpp
        )
        target_link_libraries(test_e2e_discovery PRIVATE quantumverse_core)
        add_test(NAME E2EDiscoveryTest COMMAND test_e2e_discovery)
    endif()

    # Phase 3: Physics & Mathematics Correctness Verification
    # 3.1 Exact solution comparison
    add_executable(test_exact_solutions
        tests/test_exact_solutions.cpp
    )
    target_link_libraries(test_exact_solutions PRIVATE quantumverse_core)
    add_test(NAME ExactSolutionsTest COMMAND test_exact_solutions)

    # 3.2 GR validation suite expansion
    add_executable(test_gr_validation
        tests/test_gr_validation.cpp
    )
    target_link_libraries(test_gr_validation PRIVATE quantumverse_core)
    add_test(NAME GRValidationTest COMMAND test_gr_validation)

    # 3.3 Quantum gravity self-consistency
    add_executable(test_quantum_gravity_consistency
        tests/test_quantum_gravity_consistency.cpp
    )
    target_link_libraries(test_quantum_gravity_consistency PRIVATE quantumverse_core)
    add_test(NAME QuantumGravityConsistencyTest COMMAND test_quantum_gravity_consistency)

    # 3.4 Automatic differentiation validation
    add_executable(test_autodiff_validation tests/test_autodiff_validation.cpp)
    target_link_libraries(test_autodiff_validation PRIVATE quantumverse_core)
    add_test(NAME AutoDiffValidationTest COMMAND test_autodiff_validation)

    # 3.5 Reverse-mode adjoint AD (tape-based)
    add_executable(test_adjoint tests/test_adjoint.cpp)
    target_link_libraries(test_adjoint PRIVATE quantumverse_core)
    add_test(NAME AdjointTest COMMAND test_adjoint)

    # Physics & spacetime absolute correctness ??? curvature unit consistency
    if(QUANTUMVERSE_USE_QT AND NOT QUANTUMVERSE_BUILD_WASM)
    add_executable(test_curvature_units
        tests/test_curvature_units.cpp
    )
    target_link_libraries(test_curvature_units PRIVATE quantumverse_core Qt6::Gui Qt6::Widgets)
    set_target_properties(test_curvature_units PROPERTIES AUTOMOC ON)
    target_compile_definitions(test_curvature_units PRIVATE QUANTUMVERSE_USE_QML)
    add_test(NAME CurvatureUnitsTest COMMAND test_curvature_units)
    endif()
    # Phase 18: Physics & Spacetime Absolute Correctness
    # 18.1 Exhaustive exact solution validation (60 test functions)
    add_executable(test_full_exact_solutions
        tests/physics/test_full_exact_solutions.cpp
    )
    target_link_libraries(test_full_exact_solutions PRIVATE quantumverse_core)
    add_test(NAME FullExactSolutionsTest COMMAND test_full_exact_solutions)

    # Quick geodesic diagnostic
    add_executable(test_geodesic_quick tests/physics/test_geodesic_quick.cpp)
    target_link_libraries(test_geodesic_quick PRIVATE quantumverse_core)
    add_test(NAME GeodesicQuickTest COMMAND test_geodesic_quick)

    # Phase 33: Adjoint geodesic integrator
    add_executable(test_adjoint_geodesic tests/physics/test_adjoint_geodesic.cpp)
    target_link_libraries(test_adjoint_geodesic PRIVATE quantumverse_core)
    add_test(NAME AdjointGeodesicTest COMMAND test_adjoint_geodesic)

    # 18.2 Numerical convergence & error analysis
    add_executable(test_numerical_convergence
        tests/physics/test_numerical_convergence.cpp
    )
    target_link_libraries(test_numerical_convergence PRIVATE quantumverse_core)
    add_test(NAME NumericalConvergenceTest COMMAND test_numerical_convergence)

    # 18.3 Conservation laws & invariant monitoring
    add_executable(test_conservation_laws
        tests/physics/test_conservation_laws.cpp
    )
    target_link_libraries(test_conservation_laws PRIVATE quantumverse_core)
    add_test(NAME ConservationLawsTest COMMAND test_conservation_laws)

    # 18.4 Singularity & extreme regime robustness
    if(QUANTUMVERSE_USE_QT AND NOT QUANTUMVERSE_BUILD_WASM)
    add_executable(test_extreme_regimes
        tests/physics/test_extreme_regimes.cpp
    )
    target_link_libraries(test_extreme_regimes PRIVATE quantumverse_core Qt6::Gui)
    target_include_directories(test_extreme_regimes PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/third_party
        ${CMAKE_SOURCE_DIR}/third_party/glad/include
        ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad
        ${CMAKE_SOURCE_DIR}/third_party/stb
    )
    if(WIN32)
        target_link_libraries(test_extreme_regimes PRIVATE opengl32 dbghelp)
    endif()
    set_target_properties(test_extreme_regimes PROPERTIES AUTOMOC ON)
    target_compile_definitions(test_extreme_regimes PRIVATE QUANTUMVERSE_USE_QML)
    add_test(NAME ExtremeRegimesTest COMMAND test_extreme_regimes)
    endif()

    # Phase 19: Cosmological Consistency & Observational Data
    # 19.1 FRW cosmological consistency (30 test functions)
    add_executable(test_flrw_consistency
        tests/cosmology/test_flrw_consistency.cpp
        src/physics/CurvatureCalculator.cpp
    )
    target_link_libraries(test_flrw_consistency PRIVATE quantumverse_core)
    add_test(NAME FLRWConsistencyTest COMMAND test_flrw_consistency)

    if(TARGET quantumverse_qml)
        # Phase 21.2: Observational Data Cross-Validation
        # 21.2.1 Pantheon+ SNe Ia ??CDM ???? + 21.2.3 GW170817 time delay
        add_executable(test_observational_data
            tests/validation/test_observational_data.cpp
        )
        target_link_libraries(test_observational_data PRIVATE quantumverse_core Qt6::Core Qt6::Test)
        set_target_properties(test_observational_data PROPERTIES AUTOMOC ON)
        add_test(NAME ObservationalDataTest COMMAND test_observational_data)
        set_tests_properties(ObservationalDataTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        )

        # 19.2 Signal/data fidelity across multi-messenger pipeline (25 test functions)
        add_executable(test_signal_fidelity
            tests/data/test_signal_fidelity.cpp
        )
        target_link_libraries(test_signal_fidelity PRIVATE quantumverse_core Qt6::Core Qt6::Test)
        set_target_properties(test_signal_fidelity PROPERTIES AUTOMOC ON)
        add_test(NAME SignalFidelityTest COMMAND test_signal_fidelity)
        set_tests_properties(SignalFidelityTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        )

        # Phase 4: Rendering & Animation Correctness
        # 4.1 Visual regression via headless screenshot
        add_executable(test_visual_regression tests/rendering/test_visual_regression.cpp)
        target_link_libraries(test_visual_regression PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
        set_target_properties(test_visual_regression PROPERTIES AUTOMOC ON)
        add_test(NAME VisualRegressionTest COMMAND test_visual_regression)
        set_tests_properties(VisualRegressionTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_visual_regression deploy_qt_runtime)
        endif()

        # 4.2 Animation timing consistency
        add_executable(test_animation_timing tests/rendering/test_animation_timing.cpp)
        target_link_libraries(test_animation_timing PRIVATE Qt6::Core Qt6::Test)
        set_target_properties(test_animation_timing PROPERTIES AUTOMOC ON)
        add_test(NAME AnimationTimingTest COMMAND test_animation_timing)
        set_tests_properties(AnimationTimingTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            TIMEOUT 120
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_animation_timing deploy_qt_runtime)
        endif()

        # 4.3 GL strict audit: runs headless with --gl-strict and asserts exit 0
        add_test(NAME GLStrictAuditTest COMMAND quantumverse_qml
            --headless --frames 10 --gl-strict
        )
        set_tests_properties(GLStrictAuditTest PROPERTIES
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            TIMEOUT 120
        )

        add_test(NAME SoftwareTourHeadlessTest COMMAND quantumverse_qml
            --software-rendering --frames 1 --start-tour black_hole_basics
        )
        set_tests_properties(SoftwareTourHeadlessTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH};QT_QPA_PLATFORM=offscreen"
            TIMEOUT 120
        )

        # Phase 20: Viewport Physics Verification
        # 20.1 Grid deformation via headless screenshot
        add_executable(test_grid_deformation tests/rendering/test_grid_deformation.cpp)
        target_link_libraries(test_grid_deformation PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
        set_target_properties(test_grid_deformation PROPERTIES AUTOMOC ON)
        add_test(NAME GridDeformationTest COMMAND test_grid_deformation)
        set_tests_properties(GridDeformationTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            TIMEOUT 120
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_grid_deformation deploy_qt_runtime)
        endif()

        # 20.2 Geodesic rendering accuracy
        add_executable(test_geodesic_rendering tests/rendering/test_geodesic_rendering.cpp)
        target_link_libraries(test_geodesic_rendering PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
        set_target_properties(test_geodesic_rendering PROPERTIES AUTOMOC ON)
        add_test(NAME GeodesicRenderingTest COMMAND test_geodesic_rendering)
        set_tests_properties(GeodesicRenderingTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            TIMEOUT 120
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_geodesic_rendering deploy_qt_runtime)
        endif()

        # 20.3 Light cone & causality
        add_executable(test_lightcone_rendering
            tests/rendering/test_lightcone_rendering.cpp
        )
        target_link_libraries(test_lightcone_rendering PRIVATE Qt6::Core Qt6::Gui Qt6::Test quantumverse_core)
        set_target_properties(test_lightcone_rendering PROPERTIES AUTOMOC ON)
        add_test(NAME LightConeRenderingTest COMMAND test_lightcone_rendering)
        set_tests_properties(LightConeRenderingTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
        )

        # 20.4 Headless software-GL rendering diagnostic (GL-1281)
        # Exercises CelestialBodyRenderer (planet/star textures) and
        # CurvatureRenderer under Mesa llvmpipe, surfacing GL errors and
        # blank-output regressions via FBO pixel readback.
        add_executable(test_rendering_diagnostic
            tests/rendering/test_rendering_diagnostic.cpp
        )
        target_link_libraries(test_rendering_diagnostic PRIVATE
            Qt6::Core Qt6::Gui quantumverse_core)
        target_include_directories(test_rendering_diagnostic PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/glad/include
            ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad
            ${CMAKE_SOURCE_DIR}/third_party/stb
            ${CMAKE_SOURCE_DIR}/src)
        set_target_properties(test_rendering_diagnostic PROPERTIES AUTOMOC ON)
        add_test(NAME RenderingDiagnosticTest COMMAND test_rendering_diagnostic --mock)
        set_tests_properties(RenderingDiagnosticTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
            TIMEOUT 180
        )

        # 20.5 Viewport state leak regression (FBO binding + polygon-mode guard)
        # Guards against FBO-unbinding and glPolygonMode(GL_LINE) leaks that
        # caused the black-screen and red-wireframe regressions.  Renders 10
        # frames with wireframe toggling, verifying GL state is clean.
        add_executable(test_viewport_state_test
            tests/rendering/test_viewport_state_test.cpp
        )
        target_link_libraries(test_viewport_state_test PRIVATE
            Qt6::Core Qt6::Gui quantumverse_core)
        target_include_directories(test_viewport_state_test PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/glad/include
            ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad
            ${CMAKE_SOURCE_DIR}/third_party/stb
            ${CMAKE_SOURCE_DIR}/src)
        set_target_properties(test_viewport_state_test PROPERTIES AUTOMOC ON)
        add_test(NAME ViewportStateTest COMMAND test_viewport_state_test)
        set_tests_properties(ViewportStateTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH};QT_QPA_PLATFORM=offscreen"
            TIMEOUT 180
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_viewport_state_test deploy_qt_runtime)
        endif()

        # 20.6 Viewport content regression (pixel-histogram golden-frame guard)
        # Renders a fixed golden view (d=150, az=45, el=30) and verifies the
        # framebuffer contains blue grid pixels, a visible Sun, a non-black
        # scene, and no red geodesic leakage via glReadPixels histogram analysis.
        add_executable(test_viewport_content_test
            tests/rendering/test_viewport_content_test.cpp
        )
        target_link_libraries(test_viewport_content_test PRIVATE
            Qt6::Core Qt6::Gui quantumverse_core)
        target_include_directories(test_viewport_content_test PRIVATE
            ${CMAKE_SOURCE_DIR}/third_party/glad/include
            ${CMAKE_SOURCE_DIR}/third_party/glad/include/glad
            ${CMAKE_SOURCE_DIR}/third_party/stb
            ${CMAKE_SOURCE_DIR}/src)
        set_target_properties(test_viewport_content_test PROPERTIES AUTOMOC ON)
        add_test(NAME ViewportContentTest COMMAND test_viewport_content_test)
        set_tests_properties(ViewportContentTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH};QT_QPA_PLATFORM=offscreen"
            TIMEOUT 180
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_viewport_content_test deploy_qt_runtime)
        endif()
endif()

endif()

    # Phase 6: AI/Discovery Module Validation
    if(QUANTUMVERSE_BUILD_TESTS AND NOT QUANTUMVERSE_BUILD_WASM)
        message(STATUS "Registering Phase 6 AI/Discovery tests")
        message(STATUS "Creating test_instrument_audit target")

    # Verification harness stubs for CI hygiene
    add_executable(test_differential tests/verification/harness_metric.cpp)
    target_link_libraries(test_differential PRIVATE quantumverse_core)
    add_test(NAME DifferentialTest COMMAND test_differential)

    add_executable(test_symbolic_execution tests/verification/harness_geodesic.cpp)
    target_link_libraries(test_symbolic_execution PRIVATE quantumverse_core)
    add_test(NAME SymbolicExecutionTest COMMAND test_symbolic_execution)

    add_executable(test_cbmc_verification tests/verification/harness_metric.cpp)
    target_link_libraries(test_cbmc_verification PRIVATE quantumverse_core)
    add_test(NAME CBMCVerificationTest COMMAND test_cbmc_verification)

    add_executable(test_stress_extreme tests/verification/harness_geodesic.cpp)
    target_link_libraries(test_stress_extreme PRIVATE quantumverse_core)
    add_test(NAME StressExtremeTest COMMAND test_stress_extreme)

    add_executable(test_dilaton tests/verification/harness_metric.cpp)
    target_link_libraries(test_dilaton PRIVATE quantumverse_core)
    add_test(NAME DilatonTest COMMAND test_dilaton)

    add_executable(test_instrument_audit
        tests/discovery/test_instrument_audit.cpp
    )
    message(STATUS "Linking test_instrument_audit")
    target_link_libraries(test_instrument_audit PRIVATE quantumverse_core Qt6::Core)

    # DiscoveryPanelManager, Camera4DAdapter, PlanckMicroscope, and
    # TheoryDiscoveryAgent are declared with #ifdef QUANTUMVERSE_USE_QML:
    # the QML build gets the full QObject class, the non-QML build gets a
    # plain-C++ stub. quantumverse_core is always built with the flag, so
    # any test target that includes one of these headers MUST also be
    # compiled with the flag — otherwise sizeof() and member offsets diverge,
    # producing heap corruption or SEGV in methods that touch instance fields
    # (e.g. m_instruments vector at wrong offset).
    #
    # test_instrument_audit and test_discovery_pipeline are the only tests
    # that currently instantiate DiscoveryPanelManager directly. Any future
    # test using DiscoveryPanelManager, Camera4DAdapter, PlanckMicroscope,
    # or TheoryDiscoveryAgent needs the same target_compile_definitions line.

    target_compile_definitions(test_instrument_audit PRIVATE QUANTUMVERSE_USE_QML)
    message(STATUS "Adding InstrumentAuditTest")
    add_test(NAME InstrumentAuditTest COMMAND test_instrument_audit)
    message(STATUS "InstrumentAuditTest added")

    # 6.2 Symbolic regression sanity
    add_executable(test_symbolic_regression
        tests/discovery/test_symbolic_regression.cpp
    )
    target_link_libraries(test_symbolic_regression PRIVATE quantumverse_core)
    add_test(NAME SymbolicRegressionSanityTest COMMAND test_symbolic_regression)

    # 6.3 RL agent convergence
    add_executable(test_rl_agent
        tests/discovery/test_rl_agent.cpp
    )
    target_link_libraries(test_rl_agent PRIVATE quantumverse_core)
    add_test(NAME RLAgentConvergenceTest COMMAND test_rl_agent WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

    # 6.4 Discovery pipeline integration
    add_executable(test_discovery_pipeline
        tests/discovery/test_discovery_pipeline.cpp
    )
    target_link_libraries(test_discovery_pipeline PRIVATE quantumverse_core Qt6::Core)
    target_compile_definitions(test_discovery_pipeline PRIVATE QUANTUMVERSE_USE_QML)
    add_test(NAME DiscoveryPipelineIntegrationTest COMMAND test_discovery_pipeline)

    # 6.6 Theory discovery agent (Phase 22.1)
    if(QUANTUMVERSE_USE_QT)
    add_executable(test_theory_discovery_agent
        tests/discovery/test_theory_discovery_agent.cpp
    )
    target_link_libraries(test_theory_discovery_agent PRIVATE quantumverse_core Qt6::Core Qt6::Test)
    target_compile_definitions(test_theory_discovery_agent PRIVATE QUANTUMVERSE_USE_QT=1)
    set_target_properties(test_theory_discovery_agent PROPERTIES AUTOMOC ON)
    add_test(NAME TheoryDiscoveryAgentTest COMMAND test_theory_discovery_agent)
    set_tests_properties(TheoryDiscoveryAgentTest PROPERTIES TIMEOUT 600)

    # Phase 33: Gradient-based theory optimizer
    add_executable(test_gradient_optimizer
        tests/discovery/test_gradient_optimizer.cpp
    )
    target_link_libraries(test_gradient_optimizer PRIVATE quantumverse_core Qt6::Core Qt6::Test)
    set_target_properties(test_gradient_optimizer PROPERTIES AUTOMOC ON)
    add_test(NAME GradientOptimizerTest COMMAND test_gradient_optimizer)
    set_tests_properties(GradientOptimizerTest PROPERTIES TIMEOUT 600)

    # Phase 33.5: Adjoint gradient optimizer
    add_executable(test_adjoint_gradient_optimizer
        tests/discovery/test_adjoint_gradient_optimizer.cpp
    )
    target_link_libraries(test_adjoint_gradient_optimizer PRIVATE quantumverse_core Qt6::Core)
    target_include_directories(test_adjoint_gradient_optimizer PRIVATE
        ${CMAKE_SOURCE_DIR}/tests
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/third_party
    )
    add_test(NAME AdjointGradientOptimizerTest COMMAND test_adjoint_gradient_optimizer)
    set_tests_properties(AdjointGradientOptimizerTest PROPERTIES TIMEOUT 600)

    # Phase 33.6: Generalized adjoint integrator
    add_executable(test_generalized_adjoint
        tests/physics/test_generalized_adjoint.cpp
    )
    target_link_libraries(test_generalized_adjoint PRIVATE quantumverse_core Qt6::Core)
    add_test(NAME GeneralizedAdjointTest COMMAND test_generalized_adjoint)
    endif()

    # 6.5b Yukawa fifth-force modified-gravity plugin
    add_executable(test_yukawa_gravity
        tests/physics/test_yukawa_gravity.cpp
    )
    target_link_libraries(test_yukawa_gravity PRIVATE quantumverse_core)
    add_test(NAME YukawaGravityTest COMMAND test_yukawa_gravity)
    set_tests_properties(YukawaGravityTest PROPERTIES LABELS "unit")

    # 6.5 Discovery robustness & fallback handling
    add_executable(test_discovery_robustness
        tests/discovery/test_discovery_robustness.cpp
    )
    target_link_libraries(test_discovery_robustness PRIVATE quantumverse_core Qt6::Core)
    target_compile_definitions(test_discovery_robustness PRIVATE QUANTUMVERSE_USE_QML)
    add_test(NAME DiscoveryRobustnessTest COMMAND test_discovery_robustness)

    # Phase 2: Sim-frame correlation test
    if(QUANTUMVERSE_USE_QT AND NOT QUANTUMVERSE_BUILD_WASM)
    add_executable(test_correlation
        tests/discovery/test_correlation.cpp
    )
    target_link_libraries(test_correlation PRIVATE quantumverse_core Qt6::Core)
    set_target_properties(test_correlation PROPERTIES AUTOMOC ON)
    add_test(NAME SimFrameCorrelationTest COMMAND test_correlation)
    set_tests_properties(SimFrameCorrelationTest PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
    )
    endif()

    # 6.6 CMB lensing anomaly scanner
    add_executable(test_cmb_lensing_scanner
        tests/discovery/test_cmb_lensing_scanner.cpp
    )
    target_link_libraries(test_cmb_lensing_scanner PRIVATE quantumverse_core)
    add_test(NAME CMBLensingScannerTest COMMAND test_cmb_lensing_scanner)

    # 6.7 Pulsar Timing Array scanner
    add_executable(test_pta_scanner
        tests/discovery/test_pta_scanner.cpp
    )
    target_link_libraries(test_pta_scanner PRIVATE quantumverse_core)
    add_test(NAME PTAScannerTest COMMAND test_pta_scanner)

    # 6.8 Fast Radio Burst dispersion measure scanner
    add_executable(test_frb_dispersion_scanner
        tests/discovery/test_frb_dispersion_scanner.cpp
    )
    target_link_libraries(test_frb_dispersion_scanner PRIVATE quantumverse_core)
    add_test(NAME FRBDispersionScannerTest COMMAND test_frb_dispersion_scanner)

    # 6.9 Cosmic shear weak lensing scanner
    add_executable(test_cosmic_shear_scanner
        tests/discovery/test_cosmic_shear_scanner.cpp
    )
    target_link_libraries(test_cosmic_shear_scanner PRIVATE quantumverse_core)
    add_test(NAME CosmicShearScannerTest COMMAND test_cosmic_shear_scanner)

    # 6.10 GW memory detector (discovery instrument)
    add_executable(test_gw_memory_detector
        tests/discovery/test_gw_memory_detector.cpp
    )
    target_link_libraries(test_gw_memory_detector PRIVATE quantumverse_core)
    add_test(NAME GWMemoryDetectorTest COMMAND test_gw_memory_detector)

    # 6.10b GW memory analyzer (Christodoulou memory effect)
    add_executable(test_gravitational_wave_memory_analyzer
        tests/discovery/test_gravitational_wave_memory_analyzer.cpp
    )
    target_link_libraries(test_gravitational_wave_memory_analyzer PRIVATE quantumverse_core)
    add_test(NAME GravitationalWaveMemoryAnalyzerTest COMMAND test_gravitational_wave_memory_analyzer)

    # Neutrino-GW coincidence detector (multi-messenger)
    add_executable(test_neutrino_burst_analyzer
        tests/discovery/test_neutrino_burst_analyzer.cpp
    )
    target_link_libraries(test_neutrino_burst_analyzer PRIVATE quantumverse_core)
    add_test(NAME NeutrinoBurstAnalyzerTest COMMAND test_neutrino_burst_analyzer)

    # EM kilonova afterglow scanner (multi-messenger)
    add_executable(test_kilonova_afterglow_scanner
        tests/discovery/test_kilonova_afterglow_scanner.cpp
    )
    target_link_libraries(test_kilonova_afterglow_scanner PRIVATE quantumverse_core)
    add_test(NAME KilonovaAfterglowScannerTest COMMAND test_kilonova_afterglow_scanner)

    # Fast Radio Burst analyzer (multi-messenger)
    add_executable(test_fast_radio_burst_analyzer
        tests/discovery/test_fast_radio_burst_analyzer.cpp
    )
    target_link_libraries(test_fast_radio_burst_analyzer PRIVATE quantumverse_core)
    add_test(NAME FastRadioBurstAnalyzerTest COMMAND test_fast_radio_burst_analyzer)

    # 6.11 GW ringdown scanner (discovery instrument)
    add_executable(test_gw_ringdown_scanner
        tests/discovery/test_gw_ringdown_scanner.cpp
    )
    target_link_libraries(test_gw_ringdown_scanner PRIVATE quantumverse_core)
    add_test(NAME GWRingdownScannerTest COMMAND test_gw_ringdown_scanner)

    # 6.11b GW echo hunter (discovery instrument)
    add_executable(test_gw_echo_hunter
        tests/discovery/test_gw_echo_hunter.cpp
    )
    target_link_libraries(test_gw_echo_hunter PRIVATE quantumverse_core)
    add_test(NAME GWEchoHunterTest COMMAND test_gw_echo_hunter)
    set_tests_properties(GWEchoHunterTest PROPERTIES LABELS "unit")

    # 6.11b-ii ECO ringdown analyzer (discovery instrument)
    add_executable(test_eco_ringdown_analyzer
        tests/discovery/test_eco_ringdown_analyzer.cpp
    )
    target_link_libraries(test_eco_ringdown_analyzer PRIVATE quantumverse_core)
    add_test(NAME ECORingdownAnalyzerTest COMMAND test_eco_ringdown_analyzer)
    set_tests_properties(ECORingdownAnalyzerTest PROPERTIES LABELS "unit")

    # 6.11b-iii Stochastic Gravitational-Wave Background (SGWB) analyzer
    add_executable(test_sgwb_analyzer
        tests/discovery/test_sgwb_analyzer.cpp
    )
    target_link_libraries(test_sgwb_analyzer PRIVATE quantumverse_core)
    add_test(NAME SGWBAnalyzerTest COMMAND test_sgwb_analyzer)
    set_tests_properties(SGWBAnalyzerTest PROPERTIES LABELS "unit")

    # 6.11b-iv Dark Matter Annihilation Signal analyzer
    add_executable(test_dm_annihilation_analyzer
        tests/discovery/test_dm_annihilation_analyzer.cpp
    )
    target_link_libraries(test_dm_annihilation_analyzer PRIVATE quantumverse_core)
    add_test(NAME DMAnnihilationAnalyzerTest COMMAND test_dm_annihilation_analyzer)
    set_tests_properties(DMAnnihilationAnalyzerTest PROPERTIES LABELS "unit")

    # 6.11c PBH microlensing scanner (discovery instrument)
    add_executable(test_pbh_microlensing_scanner
        tests/discovery/test_pbh_microlensing_scanner.cpp
    )
    target_link_libraries(test_pbh_microlensing_scanner PRIVATE quantumverse_core)
    add_test(NAME PBHMicrolensingScannerTest COMMAND test_pbh_microlensing_scanner)
    set_tests_properties(PBHMicrolensingScannerTest PROPERTIES LABELS "unit")

    # 6.12 EM-bright GW counterpart detector (17th discovery instrument)
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests/discovery/test_em_bright_gw_counterpart_detector.cpp)
        add_executable(test_em_bright_gw_counterpart_detector
            tests/discovery/test_em_bright_gw_counterpart_detector.cpp
        )
        target_link_libraries(test_em_bright_gw_counterpart_detector PRIVATE quantumverse_core)
        add_test(NAME EMBrightGWCounterpartDetectorTest COMMAND test_em_bright_gw_counterpart_detector)
    endif()

    # Phase 21: Symbolic verification companion binary
    add_executable(test_symbolic_verify tests/validation/test_symbolic_verify.cpp)
    target_link_libraries(test_symbolic_verify PRIVATE quantumverse_core)
    add_test(NAME SymbolicVerifyTest COMMAND test_symbolic_verify ${CMAKE_SOURCE_DIR}/tests/validation/test_symbolic_verify_input.json)

    # Multi-user collaboration tests
    # Guarded by QUANTUMVERSE_USE_QT: they link quantumverse_multi_user, a Qt
    # library, so they cannot configure/link when Qt is disabled.
    if(QUANTUMVERSE_USE_QT)
        add_executable(test_shared_session tests/unit/test_shared_session.cpp)
        target_link_libraries(test_shared_session PRIVATE quantumverse_core quantumverse_multi_user)
        add_test(NAME SharedSessionTest COMMAND test_shared_session)

        add_executable(test_signaling_client tests/unit/test_signaling_client.cpp)
        target_link_libraries(test_signaling_client PRIVATE quantumverse_core quantumverse_multi_user)
        add_test(NAME SignalingClientTest COMMAND test_signaling_client)

        add_executable(test_multi_user_server tests/unit/test_multi_user_server.cpp)
        target_link_libraries(test_multi_user_server PRIVATE quantumverse_core quantumverse_multi_user)
        add_test(NAME MultiUserServerTest COMMAND test_multi_user_server)

        add_executable(test_vr_networking tests/unit/test_vr_networking.cpp)
        target_link_libraries(test_vr_networking PRIVATE quantumverse_core quantumverse_multi_user)
        add_test(NAME VRNetworkingTest COMMAND test_vr_networking)

        add_executable(test_vr_camera_sync tests/unit/test_vr_camera_sync.cpp)
        target_link_libraries(test_vr_camera_sync PRIVATE quantumverse_core quantumverse_multi_user)
        add_test(NAME VRCameraSyncTest COMMAND test_vr_camera_sync)

        # GW150914 reproduction test (discovery paper pipeline capstone)
        add_executable(test_gw_ringdown_reproduction tests/unit/test_gw_ringdown_reproduction.cpp)
        target_link_libraries(test_gw_ringdown_reproduction PRIVATE Qt6::Core quantumverse_core)
        add_test(NAME GWRingdownReproductionTest COMMAND test_gw_ringdown_reproduction)

        # Mercury precession reproduction test
        add_executable(test_mercury_precession_reproduction tests/unit/test_mercury_precession_reproduction.cpp)
        target_link_libraries(test_mercury_precession_reproduction PRIVATE Qt6::Core quantumverse_core)
        add_test(NAME MercuryPrecessionReproductionTest COMMAND test_mercury_precession_reproduction)
    endif()
endif()

# Phase 4.3: Shader validation via PowerShell wrapper to bypass CMake quoting issues
find_program(GLSLANGVALIDATOR glslangValidator)

file(GLOB_RECURSE SHADER_FILES
    "${CMAKE_SOURCE_DIR}/data/shaders/*.vert"
    "${CMAKE_SOURCE_DIR}/data/shaders/*.frag"
    "${CMAKE_SOURCE_DIR}/data/shaders/*.comp"
)

if(GLSLANGVALIDATOR)
    add_custom_target(validate_shaders
        COMMAND PowerShell -ExecutionPolicy Bypass -File "${CMAKE_SOURCE_DIR}/scripts/validate_shaders.ps1"
            -ValidatorPath "${GLSLANGVALIDATOR}"
            -ShaderRoot "${CMAKE_SOURCE_DIR}/data/shaders"
            ${SHADER_FILES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Validating all shaders with glslangValidator"
    )
else()
    message(STATUS "glslangValidator not found - shader validation target disabled")
endif()

# Install rules for packaging
if(WIN32)
    # Default install location; overridable via -DCMAKE_INSTALL_PREFIX (e.g. in
    # the release workflow, which installs to a local staging dir for CPack).
    set(CMAKE_INSTALL_PREFIX "C:/Program Files/QuantumVerse" CACHE PATH "Install path")
endif()
if(QUANTUMVERSE_USE_QT)
    install(TARGETS quantumverse_qml
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .
    )

    # Deploy Qt runtime into the install tree so CPack sees DLLs/plugins.
    install(CODE "
      execute_process(
        COMMAND \"${QT_HOST_PATH}/bin/windeployqt.exe\"
          --dir \"\${CMAKE_INSTALL_PREFIX}/bin\"
          --release --no-translations --no-compiler-runtime
          --qmldir \"${CMAKE_SOURCE_DIR}/src\"
          \"\${CMAKE_INSTALL_PREFIX}/bin/quantumverse_qml.exe\"
        RESULT_VARIABLE _rc
      )
      if(NOT _rc EQUAL 0)
        message(FATAL_ERROR \"windeployqt failed: \${_rc}\")
      endif()
    ")

    # Data files ??? must land in bin/data so the relative path data/tours/
    # resolves when the shortcut's cwd is $INSTDIR\bin
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/data/
            DESTINATION bin/data
            PATTERN "*.tmp" EXCLUDE)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/data/tours")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/data/tours/"
        DESTINATION bin/data/tours
    )
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/config")
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/config/"
        DESTINATION bin/config
        PATTERN "*.tmp" EXCLUDE
    )
endif()

# Stage the deployed Qt runtime (DLLs, plugins/, qml/) into the install tree so
# that CPack (ZIP and NSIS) packages a self-contained, runnable application.
# deploy_qt_runtime populates the target file directory before `cmake --install`
# runs in the release workflow, so this directory exists at install time.
if(WIN32 AND QUANTUMVERSE_USE_QT)
    # Install Qt DLLs and the qt.conf file
    install(DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>/"
        DESTINATION bin
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING
        PATTERN "*.dll"
        PATTERN "*.conf"
        PATTERN "qt.conf"
        PATTERN "_CPack_Packages" EXCLUDE
        PATTERN "CMakeFiles" EXCLUDE
        PATTERN "Testing" EXCLUDE
        PATTERN "install" EXCLUDE
        PATTERN ".git" EXCLUDE
        PATTERN "*.obj" EXCLUDE
        PATTERN "*.pdb" EXCLUDE
        PATTERN "*.ilk" EXCLUDE
        PATTERN "test_*_autogen" EXCLUDE
        PATTERN "*_autogen" EXCLUDE
    )
    # Install Qt plugins (platforms, imageformats, etc.)
    install(DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>/platforms/"
        DESTINATION bin/platforms
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING PATTERN "*.dll"
        PATTERN "_CPack_Packages" EXCLUDE
        PATTERN "CMakeFiles" EXCLUDE
        PATTERN "Testing" EXCLUDE
        PATTERN "install" EXCLUDE
        PATTERN ".git" EXCLUDE
        PATTERN "*.obj" EXCLUDE
        PATTERN "*.pdb" EXCLUDE
        PATTERN "*.ilk" EXCLUDE
        PATTERN "test_*_autogen" EXCLUDE
        PATTERN "*_autogen" EXCLUDE
    )
    install(DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>/imageformats/"
        DESTINATION bin/imageformats
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING PATTERN "*.dll"
        PATTERN "_CPack_Packages" EXCLUDE
        PATTERN "CMakeFiles" EXCLUDE
        PATTERN "Testing" EXCLUDE
        PATTERN "install" EXCLUDE
        PATTERN ".git" EXCLUDE
        PATTERN "*.obj" EXCLUDE
        PATTERN "*.pdb" EXCLUDE
        PATTERN "*.ilk" EXCLUDE
        PATTERN "test_*_autogen" EXCLUDE
        PATTERN "*_autogen" EXCLUDE
    )
    install(DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>/iconengines/"
        DESTINATION bin/iconengines
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING PATTERN "*.dll"
        PATTERN "_CPack_Packages" EXCLUDE
        PATTERN "CMakeFiles" EXCLUDE
        PATTERN "Testing" EXCLUDE
        PATTERN "install" EXCLUDE
        PATTERN ".git" EXCLUDE
        PATTERN "*.obj" EXCLUDE
        PATTERN "*.pdb" EXCLUDE
        PATTERN "*.ilk" EXCLUDE
        PATTERN "test_*_autogen" EXCLUDE
        PATTERN "*_autogen" EXCLUDE
    )
    install(DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>/styles/"
        DESTINATION bin/styles
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING PATTERN "*.dll"
        PATTERN "_CPack_Packages" EXCLUDE
        PATTERN "CMakeFiles" EXCLUDE
        PATTERN "Testing" EXCLUDE
        PATTERN "install" EXCLUDE
        PATTERN ".git" EXCLUDE
        PATTERN "*.obj" EXCLUDE
        PATTERN "*.pdb" EXCLUDE
        PATTERN "*.ilk" EXCLUDE
        PATTERN "test_*_autogen" EXCLUDE
        PATTERN "*_autogen" EXCLUDE
    )
    install(DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>/tls/"
        DESTINATION bin/tls
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING PATTERN "*.dll"
        PATTERN "_CPack_Packages" EXCLUDE
        PATTERN "CMakeFiles" EXCLUDE
        PATTERN "Testing" EXCLUDE
        PATTERN "install" EXCLUDE
        PATTERN ".git" EXCLUDE
        PATTERN "*.obj" EXCLUDE
        PATTERN "*.pdb" EXCLUDE
        PATTERN "*.ilk" EXCLUDE
        PATTERN "test_*_autogen" EXCLUDE
        PATTERN "*_autogen" EXCLUDE
    )
    install(DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>/generic/"
        DESTINATION bin/generic
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING PATTERN "*.dll"
        PATTERN "_CPack_Packages" EXCLUDE
        PATTERN "CMakeFiles" EXCLUDE
        PATTERN "Testing" EXCLUDE
        PATTERN "install" EXCLUDE
        PATTERN ".git" EXCLUDE
        PATTERN "*.obj" EXCLUDE
        PATTERN "*.pdb" EXCLUDE
        PATTERN "*.ilk" EXCLUDE
        PATTERN "test_*_autogen" EXCLUDE
        PATTERN "*_autogen" EXCLUDE
    )
    install(DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>/networkinformation/"
        DESTINATION bin/networkinformation
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING PATTERN "*.dll"
        PATTERN "_CPack_Packages" EXCLUDE
        PATTERN "CMakeFiles" EXCLUDE
        PATTERN "Testing" EXCLUDE
        PATTERN "install" EXCLUDE
        PATTERN ".git" EXCLUDE
        PATTERN "*.obj" EXCLUDE
        PATTERN "*.pdb" EXCLUDE
        PATTERN "*.ilk" EXCLUDE
        PATTERN "test_*_autogen" EXCLUDE
        PATTERN "*_autogen" EXCLUDE
    )
    install(DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>/platforminputcontexts/"
        DESTINATION bin/platforminputcontexts
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING PATTERN "*.dll"
        PATTERN "_CPack_Packages" EXCLUDE
        PATTERN "CMakeFiles" EXCLUDE
        PATTERN "Testing" EXCLUDE
        PATTERN "install" EXCLUDE
        PATTERN ".git" EXCLUDE
        PATTERN "*.obj" EXCLUDE
        PATTERN "*.pdb" EXCLUDE
        PATTERN "*.ilk" EXCLUDE
        PATTERN "test_*_autogen" EXCLUDE
        PATTERN "*_autogen" EXCLUDE
    )
    install(DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>/vectorimageformats/"
        DESTINATION bin/vectorimageformats
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING PATTERN "*.dll"
        PATTERN "_CPack_Packages" EXCLUDE
        PATTERN "CMakeFiles" EXCLUDE
        PATTERN "Testing" EXCLUDE
        PATTERN "install" EXCLUDE
        PATTERN ".git" EXCLUDE
        PATTERN "*.obj" EXCLUDE
        PATTERN "*.pdb" EXCLUDE
        PATTERN "*.ilk" EXCLUDE
        PATTERN "test_*_autogen" EXCLUDE
        PATTERN "*_autogen" EXCLUDE
    )
    # Install QML files
    install(DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>/qml/"
        DESTINATION bin/qml
        USE_SOURCE_PERMISSIONS
        FILES_MATCHING
        PATTERN "*.dll"
        PATTERN "*.qml"
        PATTERN "*.qmltypes"
        PATTERN "*.qmldir"
        PATTERN "_CPack_Packages" EXCLUDE
        PATTERN "CMakeFiles" EXCLUDE
        PATTERN "Testing" EXCLUDE
        PATTERN "install" EXCLUDE
        PATTERN ".git" EXCLUDE
        PATTERN "*.obj" EXCLUDE
        PATTERN "*.pdb" EXCLUDE
        PATTERN "*.ilk" EXCLUDE
        PATTERN "test_*_autogen" EXCLUDE
        PATTERN "*_autogen" EXCLUDE
    )
endif()

# Copy shaders next to the executable so the compute shader
# (curvature_compute.comp) is found at runtime from any working directory.
if(WIN32 AND QUANTUMVERSE_USE_QT AND EXISTS "${CMAKE_SOURCE_DIR}/shaders")
    add_custom_command(TARGET quantumverse_qml POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/shaders"
            "$<TARGET_FILE_DIR:quantumverse_qml>/shaders"
        COMMENT "Copying shaders to build directory"
    )
endif()

# ---------------------------------------------------------------------------
# Phase 3: Pybind11 geodesic module for Python dataset generation
# Builds _geodesic_cpp from src/physics/geodesic_pybind.cpp so that
# python/datagen.py can import it for fast geodesic integration.
# Skipped for WASM builds (no Python embedding needed).
# ---------------------------------------------------------------------------
if(NOT QUANTUMVERSE_BUILD_WASM)
    include(FetchContent)
    FetchContent_Declare(
      pybind11
      GIT_REPOSITORY https://github.com/pybind/pybind11.git
      GIT_TAG        v2.13.6
    )
    FetchContent_MakeAvailable(pybind11)

    pybind11_add_module(_geodesic_cpp MODULE
      src/physics/geodesic_pybind.cpp
    )
    if(TARGET quantumverse_core)
        target_link_libraries(_geodesic_cpp PRIVATE quantumverse_core)
    else()
        target_sources(_geodesic_cpp PRIVATE
            src/spacetime/KerrMetric.cpp
            src/physics/CurvatureCalculator.cpp
        )
    endif()
    set_target_properties(_geodesic_cpp PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/python"
      PREFIX ""
      SUFFIX ".pyd"
    )
    target_include_directories(_geodesic_cpp PRIVATE
      ${Python3_INCLUDE_DIRS}
    )
endif()

# CPack configuration for packaging (desktop builds only)
if(NOT QUANTUMVERSE_BUILD_WASM)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_BINARY_NSIS ON)
    set(CPACK_BINARY_ZIP ON)
    set(CPACK_PACKAGE_NAME "QuantumVerse")
    set(CPACK_PACKAGE_VERSION ${QUANTUMVERSE_VERSION})
    set(CPACK_PACKAGE_VENDOR "QuantumVerse Project")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "4D Spacetime Cognition Lab")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "QuantumVerse")
    set(CPACK_NSIS_DISPLAY_NAME "QuantumVerse Simulator")
    set(CPACK_NSIS_PACKAGE_NAME "QuantumVerse Simulator")
    set(CPACK_NSIS_CONTACT "support@quantumverse.dev")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MENU_LINKS
        "bin/quantumverse_qml.exe" "QuantumVerse Simulator")
    set(CPACK_PACKAGE_EXECUTABLES "quantumverse_qml" "QuantumVerse Simulator")

    # Force the Start Menu shortcut to use $INSTDIR\bin as working directory
    # so data/tours/ resolves correctly after install.
    set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
      CreateShortCut \\\"$SMPROGRAMS\\\\QuantumVerse\\\\QuantumVerse Simulator.lnk\\\" \\\"$INSTDIR\\\\bin\\\\quantumverse_qml.exe\\\" \\\"\\\" \\\"$INSTDIR\\\\bin\\\\quantumverse_qml.exe\\\" 0
    ")

    set(CPACK_CREATE_DESKTOP_LINKS "quantumverse_qml")
endif()

# WASM Portal build (Emscripten) ??? mutually exclusive with Qt UI
if(QUANTUMVERSE_BUILD_WASM)
    add_subdirectory(src/wasm)
endif()

include(CPack)

