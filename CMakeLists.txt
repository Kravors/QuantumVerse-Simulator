# QuantumVerse Simulator - CMakeLists.txt
# Production-ready 4D spacetime cognition laboratory
cmake_minimum_required(VERSION 3.20)
project(QuantumVerse LANGUAGES C CXX VERSION 3.7.2)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror)
endif()

# Coverage instrumentation
if(QUANTUMVERSE_USE_COVERAGE)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage -O0 -g")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} --coverage -O0 -g")
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
set(CTEST_PERFORMANCE_THRESHOLD 60 CACHE STRING "Avg frame-time threshold (ms) for the QML performance gate")
set(CTEST_PERFORMANCE_MAX_THRESHOLD 250 CACHE STRING "Max frame-time threshold (ms) for the QML performance gate")

enable_testing()

# Fetch third-party dependencies
include(FetchContent)

# nlohmann/json for JSON parsing (header-only single header)
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${CMAKE_SOURCE_DIR}/third_party)

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
target_include_directories(dilaton INTERFACE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(dilaton INTERFACE nlohmann_json)
if(CURL_FOUND)
    target_compile_definitions(dilaton INTERFACE HAVE_CURL=1)
    target_link_libraries(dilaton INTERFACE CURL::libcurl)
endif()
if(NOT CURL_FOUND)
    message(STATUS "CURL not found - building adapters without HTTP support")
endif()

# Qt/QML configuration
if(QUANTUMVERSE_USE_QT)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick OpenGL OpenGLWidgets QuickControls2 QuickTemplates2 QuickLayouts Network Test)

    if(Qt6Core_DIR)
        get_filename_component(QT_HOST_PATH "${Qt6Core_DIR}/../../.." ABSOLUTE)
    endif()

    set(QML_DIR ${CMAKE_SOURCE_DIR}/src)
    set(RESOURCES_QRC ${CMAKE_SOURCE_DIR}/src/resources.qrc)

    # Compile Qt resources
    qt_add_resources(RESOURCES_QRC ${RESOURCES_QRC})

    add_executable(quantumverse_qml
        src/main_qml.cpp
        src/qmlglviewport.cpp
        src/qmlcamcontroller.cpp
        src/scenegraphmodel.cpp
        src/ui4d/UI4D.cpp
        third_party/glad/glad.cpp
        src/ui4d/Camera4DAdapter.cpp
        src/ui4d/SceneGraphManager.cpp
        src/ui4d/PlanckMicroscope.cpp
        src/ui4d/EnhancedLayout.cpp
        src/ui4d/CausalGraph.cpp
        src/ui4d/DiscoveryProbe.cpp
        src/discovery/DiscoveryPanelManager.cpp
        src/discovery/FindingsModel.cpp
        src/data/LIGOAdapter.cpp
        src/data/IceCubeAdapter.cpp
        src/data/TESSAlertAdapter.cpp
        src/data/GCNNoticeParser.cpp
        src/data/KafkaAlertListener.cpp
        src/data/AlertRouter.cpp
        src/data/AlertListener.cpp
        src/data/GCNReplayStream.cpp
        src/data/MultiMessengerCorrelator.cpp
        src/discovery/DiscoveryEngine.cpp
        src/discovery/TheoryManager.cpp
        src/rendering/CurvatureRenderer.cpp
        src/physics/CurvatureCalculator.cpp
        src/rendering/QuantumGeometryRenderer.cpp
        src/rendering/CelestialBodyRenderer.cpp
        src/rendering/ProceduralTextures.cpp
        src/rendering/Texture.cpp
        src/rendering/GLDebug.cpp
        src/quantumgravity/CDTEngine.cpp
        src/quantumgravity/CausalSet.cpp
        src/quantumgravity/SpinFoam.cpp
        src/quantumgravity/SpinNetwork.cpp
        src/quantumgravity/GFTEngine.cpp
        src/quantumgravity/SpinFoamEngine.cpp
        src/discovery/ExoplanetaryTTVFifthForceHunter.cpp
        src/discovery/GalacticRotationCurveScanner.cpp
        src/discovery/FineStructureConstantDriftObservatory.cpp
        src/discovery/BosonStarCollisionPredictor.cpp
        src/discovery/NeutronStarGlitchPhaseDetector.cpp
        src/discovery/UltralightDMWaveInterferometer.cpp
        src/discovery/BlackHoleJetAnomalyRecogniser.cpp
        src/discovery/PrimordialLithiumCrisisSolver.cpp
        src/discovery/GalacticTidalStreamCartographer.cpp
        src/discovery/RecombinationConstantVariationImager.cpp
        src/discovery/CMBLensingScanner.cpp
        src/discovery/PTAScanner.cpp
        src/discovery/FRBDispersionScanner.cpp
        src/discovery/CosmicShearScanner.cpp
        src/discovery/RLDiscoveryAgent.cpp
        src/discovery/SymbolicMath.cpp
        src/discovery/HolographicDualityLab.cpp
        src/discovery/GWMemoryDetector.cpp
        src/discovery/GWRingdownScanner.cpp
        src/discovery/EMBrightGWCounterpartDetector.cpp
        src/ml/CurvatureNormalizingFlow.cpp
        src/ml/DifferentiableSimulator.cpp
        src/spacetime/LightCone.cpp
        src/utils/ConfigManager.cpp
        src/utils/PerformanceMonitor.cpp
        src/utils/TraceLogger.cpp
        src/utils/CrashHandler.cpp
        src/utils/FrameDiagnostics.cpp
        ${RESOURCES_QRC}
    )
    target_compile_definitions(quantumverse_qml PRIVATE QUANTUMVERSE_USE_QML)
    if(QUANTUMVERSE_USE_VR)
        target_compile_definitions(quantumverse_qml PRIVATE QUANTUMVERSE_USE_VR)
        target_sources(quantumverse_qml PRIVATE
            src/vr/OpenXRBackend.cpp
            src/vr/MultiUserServer.cpp
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
        dilaton
        ${LIBRDKAFKA_LIBRARIES}
    )
    set_target_properties(quantumverse_qml PROPERTIES
        AUTOMOC ON
        AUTORCC ON
        AUTOUIC ON
    )

    if(WIN32)
        # Deploy Qt runtime files to the build output directory so test executables
        # and headless runs can find DLLs/plugins without requiring Qt on PATH.
        set(QT_RUNTIME_DEST "${CMAKE_BINARY_DIR}/Release")
        add_custom_target(deploy_qt_runtime
            COMMAND ${CMAKE_COMMAND} -E echo "Deploying Qt runtime to ${QT_RUNTIME_DEST}..."
            COMMAND ${CMAKE_COMMAND} -E make_directory "${QT_RUNTIME_DEST}"
            COMMAND "${QT_HOST_PATH}/bin/windeployqt.exe"
                --release
                --qmldir "${CMAKE_SOURCE_DIR}/src"
                --no-compiler-runtime
                --no-translations
                "${QT_RUNTIME_DEST}/quantumverse_qml.exe"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT_HOST_PATH}/bin/Qt6Test.dll"
                "${QT_RUNTIME_DEST}/"
            COMMENT "Running windeployqt on quantumverse_qml.exe to populate ${QT_RUNTIME_DEST}"
        )
        add_dependencies(deploy_qt_runtime quantumverse_qml)
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
            LABELS "performance;qt"
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(qml_performance_baseline deploy_qt_runtime)
        endif()

        # TDD contract test for the performance gate: validates parse/threshold
        # logic and (when built) that the baseline executable returns exit 0.
        add_executable(test_performance_gate tests/test_performance_gate.cpp)
        target_include_directories(test_performance_gate PRIVATE ${CMAKE_SOURCE_DIR}/tests/benchmark)
        if(MSVC)
            target_compile_options(test_performance_gate PRIVATE /EHsc)
        endif()
        set_target_properties(test_performance_gate PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:quantumverse_qml>
        )
        add_test(NAME PerformanceGateTest COMMAND test_performance_gate)
        set_tests_properties(PerformanceGateTest PROPERTIES
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
            LABELS "performance;qt"
        )
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
            LABELS "performance;qt"
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_gl_check deploy_qt_runtime)
        endif()

        # FindingsModel unit test (requires Qt for QAbstractListModel)
        add_executable(test_findings_model
            tests/test_findings_model.cpp
            src/discovery/FindingsModel.cpp
        )
        set_target_properties(test_findings_model PROPERTIES AUTOMOC ON)
        target_include_directories(test_findings_model PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/third_party
        )
        target_link_libraries(test_findings_model PRIVATE Qt6::Core dilaton)
        add_test(NAME FindingsModelTest COMMAND test_findings_model)
        set_tests_properties(FindingsModelTest PROPERTIES
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
            ENVIRONMENT "PATH=F:/qt/6.11.1/msvc2022_64/bin;$ENV{PATH}"
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_findings_model deploy_qt_runtime)
        endif()

        # FindingsModel filtering / sorting / detail-view test
        add_executable(test_findings_model_filtering
            tests/test_findings_model_filtering.cpp
            src/discovery/FindingsModel.cpp
        )
        set_target_properties(test_findings_model_filtering PROPERTIES AUTOMOC ON)
        target_include_directories(test_findings_model_filtering PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/third_party
        )
        target_link_libraries(test_findings_model_filtering PRIVATE Qt6::Core dilaton)
        add_test(NAME FindingsModelFilteringTest COMMAND test_findings_model_filtering)
        set_tests_properties(FindingsModelFilteringTest PROPERTIES
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
            ENVIRONMENT "PATH=F:/qt/6.11.1/msvc2022_64/bin;$ENV{PATH}"
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_findings_model_filtering deploy_qt_runtime)
        endif()

        # Multi-messenger alert wiring test (LIGO/IceCube -> FindingsModel)
        add_executable(test_multimessenger_ui
            tests/test_multimessenger_ui.cpp
            src/discovery/FindingsModel.cpp
            src/data/LIGOAdapter.cpp
            src/data/IceCubeAdapter.cpp
        )
        set_target_properties(test_multimessenger_ui PROPERTIES AUTOMOC ON)
        target_include_directories(test_multimessenger_ui PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/third_party
            ${CMAKE_SOURCE_DIR}/third_party/glad/include
        )
        target_link_libraries(test_multimessenger_ui PRIVATE Qt6::Core dilaton)
        add_test(NAME MultiMessengerUITest COMMAND test_multimessenger_ui)
        set_tests_properties(MultiMessengerUITest PROPERTIES
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
            ENVIRONMENT "PATH=F:/qt/6.11.1/msvc2022_64/bin;$ENV{PATH}"
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_multimessenger_ui deploy_qt_runtime)
        endif()

        # 5.1 QmlGlViewport isolation test
        add_executable(test_viewport_isolation
            tests/ui/test_viewport_isolation.cpp
            src/qmlglviewport.cpp
            third_party/glad/glad.cpp
            src/rendering/CurvatureRenderer.cpp
            src/rendering/QuantumGeometryRenderer.cpp
            src/rendering/CelestialBodyRenderer.cpp
            src/rendering/GLDebug.cpp
            src/ui4d/Camera4DAdapter.cpp
            src/physics/CurvatureCalculator.cpp
            src/ui4d/UI4D.cpp
            src/rendering/Texture.cpp
            src/rendering/ProceduralTextures.cpp
            src/utils/TraceLogger.cpp
        )
        target_link_libraries(test_viewport_isolation PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Quick
            Qt6::OpenGL
            Qt6::OpenGLWidgets
            Qt6::Test
            dilaton
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
        )
        add_test(NAME ViewportIsolationTest COMMAND test_viewport_isolation)
        set_tests_properties(ViewportIsolationTest PROPERTIES
            WORKING_DIRECTORY "${QT_RUNTIME_DEST}"
            ENVIRONMENT "PATH=F:/qt/6.11.1/msvc2022_64/bin;$ENV{PATH}"
            ENVIRONMENT "QT_QPA_PLATFORM_PLUGIN_PATH=F:/qt/6.11.1/msvc2022_64/bin/platforms"
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_viewport_isolation deploy_qt_runtime)
        endif()

        # 5.3 Interaction stress test (rapid mouse/wheel input + property toggles)
        add_executable(test_interaction_stress
            tests/ui/test_interaction_stress.cpp
            src/qmlglviewport.cpp
            third_party/glad/glad.cpp
            src/rendering/CurvatureRenderer.cpp
            src/rendering/QuantumGeometryRenderer.cpp
            src/rendering/CelestialBodyRenderer.cpp
            src/rendering/GLDebug.cpp
            src/ui4d/Camera4DAdapter.cpp
            src/physics/CurvatureCalculator.cpp
            src/ui4d/UI4D.cpp
            src/rendering/Texture.cpp
            src/rendering/ProceduralTextures.cpp
            src/utils/TraceLogger.cpp
        )
        target_link_libraries(test_interaction_stress PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Quick
            Qt6::OpenGL
            Qt6::OpenGLWidgets
            Qt6::Test
            dilaton
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
        )
        add_test(NAME InteractionStressTest COMMAND test_interaction_stress)
        set_tests_properties(InteractionStressTest PROPERTIES
            WORKING_DIRECTORY "${QT_RUNTIME_DEST}"
            ENVIRONMENT "PATH=F:/qt/6.11.1/msvc2022_64/bin;$ENV{PATH}"
            ENVIRONMENT "QT_QPA_PLATFORM_PLUGIN_PATH=F:/qt/6.11.1/msvc2022_64/bin/platforms"
            TIMEOUT 120
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_interaction_stress deploy_qt_runtime)
        endif()

        # 5.4 Resize/toggle stress test (FBO recreation + panel toggling)
        add_executable(test_resize_stress
            tests/ui/test_resize_stress.cpp
            src/qmlglviewport.cpp
            third_party/glad/glad.cpp
            src/rendering/CurvatureRenderer.cpp
            src/rendering/QuantumGeometryRenderer.cpp
            src/rendering/CelestialBodyRenderer.cpp
            src/rendering/GLDebug.cpp
            src/ui4d/Camera4DAdapter.cpp
            src/physics/CurvatureCalculator.cpp
            src/ui4d/UI4D.cpp
            src/rendering/Texture.cpp
            src/rendering/ProceduralTextures.cpp
            src/utils/TraceLogger.cpp
        )
        target_link_libraries(test_resize_stress PRIVATE
            Qt6::Core
            Qt6::Gui
            Qt6::Quick
            Qt6::OpenGL
            Qt6::OpenGLWidgets
            Qt6::Test
            dilaton
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
        )
        add_test(NAME ResizeStressTest COMMAND test_resize_stress)
        set_tests_properties(ResizeStressTest PROPERTIES
            WORKING_DIRECTORY "${QT_RUNTIME_DEST}"
            ENVIRONMENT "PATH=F:/qt/6.11.1/msvc2022_64/bin;$ENV{PATH}"
            ENVIRONMENT "QT_QPA_PLATFORM_PLUGIN_PATH=F:/qt/6.11.1/msvc2022_64/bin/platforms"
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
                ENVIRONMENT "PATH=F:/qt/6.11.1/msvc2022_64/bin;$ENV{PATH}"
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
                src/utils/CrashHandler.cpp
                src/utils/TraceLogger.cpp
            )
            target_link_libraries(crash_helper PRIVATE dilaton)
            if(TARGET deploy_qt_runtime)
                add_dependencies(crash_helper deploy_qt_runtime)
            endif()

            add_executable(test_crash_handler tests/utils/test_crash_handler.cpp)
            target_link_libraries(test_crash_handler PRIVATE Qt6::Core Qt6::Test)
            set_target_properties(test_crash_handler PROPERTIES AUTOMOC ON)
            add_test(NAME CrashHandlerTest COMMAND test_crash_handler)
            set_tests_properties(CrashHandlerTest PROPERTIES
                WORKING_DIRECTORY "${QT_RUNTIME_DEST}"
                ENVIRONMENT "PATH=F:/qt/6.11.1/msvc2022_64/bin;$ENV{PATH}"
                TIMEOUT 30
            )
            add_dependencies(test_crash_handler crash_helper)

            # 9.1 Frame diagnostics JSON output
            add_executable(test_frame_diagnostics
                tests/utils/test_frame_diagnostics.cpp
                src/utils/FrameDiagnostics.cpp
            )
            target_link_libraries(test_frame_diagnostics PRIVATE Qt6::Core Qt6::Test)
            set_target_properties(test_frame_diagnostics PROPERTIES AUTOMOC ON)
            add_test(NAME DiagnosticsOutputTest COMMAND test_frame_diagnostics)

            # 9.2 HUD toggle test
            add_executable(test_hud_toggle
                tests/utils/test_hud_toggle.cpp
                src/qmlglviewport.cpp
                third_party/glad/glad.cpp
                src/rendering/CurvatureRenderer.cpp
                src/rendering/QuantumGeometryRenderer.cpp
                src/rendering/CelestialBodyRenderer.cpp
                src/rendering/GLDebug.cpp
                src/ui4d/Camera4DAdapter.cpp
                src/physics/CurvatureCalculator.cpp
                src/ui4d/UI4D.cpp
                src/rendering/Texture.cpp
                src/rendering/ProceduralTextures.cpp
                src/utils/TraceLogger.cpp
            )
            target_link_libraries(test_hud_toggle PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets Qt6::Quick Qt6::Test)
            set_target_properties(test_hud_toggle PROPERTIES AUTOMOC ON)
            target_compile_definitions(test_hud_toggle PRIVATE QUANTUMVERSE_USE_QML)
            target_include_directories(test_hud_toggle PRIVATE
                ${CMAKE_SOURCE_DIR}/src
                ${CMAKE_SOURCE_DIR}/third_party
                ${CMAKE_SOURCE_DIR}/third_party/glad/include
            )
            add_test(NAME HUDToggleTest COMMAND test_hud_toggle)
            set_tests_properties(HUDToggleTest PROPERTIES TIMEOUT 30)
            if(TARGET deploy_qt_runtime)
                add_dependencies(test_hud_toggle deploy_qt_runtime)
            endif()
        endif()
    endif()
endif()

# Test executables
if(QUANTUMVERSE_BUILD_TESTS)
    # Core math tests
    add_executable(test_vector4d tests/test_vector4d.cpp)
    target_link_libraries(test_vector4d PRIVATE dilaton)
    add_test(NAME Vector4DTest COMMAND test_vector4d)
    
    add_executable(test_matrix4x4 tests/test_matrix4x4.cpp)
    target_link_libraries(test_matrix4x4 PRIVATE dilaton)
    add_test(NAME Matrix4x4Test COMMAND test_matrix4x4)
    
    # Property-based tests
    add_executable(test_property_based
        tests/unit/test_property_based.cpp
        src/quantumgravity/CDTEngine.cpp
    )
    target_include_directories(test_property_based PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/build-appimage/QuantumVerse.AppDir/usr/include)
    target_link_libraries(test_property_based PRIVATE dilaton)
    add_test(NAME PropertyBasedTest COMMAND test_property_based)

    # 5.2 UI4D unit test (camera, slicing, probe, causal graph)
    add_executable(test_ui4d
        tests/unit/test_ui4d.cpp
        src/ui4d/UI4D.cpp
    )
    target_link_libraries(test_ui4d PRIVATE dilaton)
    add_test(NAME UI4DTest COMMAND test_ui4d)
    
    # Fuzz differential testing
    option(QUANTUMVERSE_USE_FUZZER "Build fuzzer targets" OFF)
    if(QUANTUMVERSE_USE_FUZZER)
        add_executable(fuzz_geodesic_differential tests/fuzz/fuzz_geodesic_differential.cpp)
        target_include_directories(fuzz_geodesic_differential PRIVATE 
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/build-appimage/QuantumVerse.AppDir/usr/include
        )
        target_link_libraries(fuzz_geodesic_differential PRIVATE dilaton)
        target_compile_options(fuzz_geodesic_differential PRIVATE -fsanitize=fuzzer,libFuzzer)
        target_link_options(fuzz_geodesic_differential PRIVATE -fsanitize=fuzzer)

        add_executable(fuzz_discovery_instruments tests/fuzz/fuzz_discovery_instruments.cpp)
        target_include_directories(fuzz_discovery_instruments PRIVATE
            ${CMAKE_SOURCE_DIR}/src
            ${CMAKE_SOURCE_DIR}/build-appimage/QuantumVerse.AppDir/usr/include
        )
        target_link_libraries(fuzz_discovery_instruments PRIVATE dilaton)
        target_compile_options(fuzz_discovery_instruments PRIVATE -fsanitize=fuzzer,libFuzzer)
        target_link_options(fuzz_discovery_instruments PRIVATE -fsanitize=fuzzer)
    endif()

    # Standalone fuzz driver (always built when tests are enabled)
    add_executable(fuzz_instruments
        tests/fuzz/fuzz_instruments.cpp
        src/discovery/ExoplanetaryTTVFifthForceHunter.cpp
        src/discovery/GalacticRotationCurveScanner.cpp
        src/discovery/FineStructureConstantDriftObservatory.cpp
        src/discovery/BosonStarCollisionPredictor.cpp
        src/discovery/NeutronStarGlitchPhaseDetector.cpp
        src/discovery/UltralightDMWaveInterferometer.cpp
        src/discovery/BlackHoleJetAnomalyRecogniser.cpp
        src/discovery/PrimordialLithiumCrisisSolver.cpp
        src/discovery/GalacticTidalStreamCartographer.cpp
        src/discovery/RecombinationConstantVariationImager.cpp
        src/discovery/CMBLensingScanner.cpp
        src/discovery/PTAScanner.cpp
        src/discovery/GWMemoryDetector.cpp
    )
    target_link_libraries(fuzz_instruments PRIVATE dilaton)
    add_test(NAME FuzzInstrumentsTest COMMAND fuzz_instruments)
    set_tests_properties(FuzzInstrumentsTest PROPERTIES
        LABELS "fuzz;discovery"
        TIMEOUT 300
    )

    # Phase 21.3: Fuzz-driven formal verification of physics invariants
    add_executable(fuzz_physics_invariants
        tests/fuzz/fuzz_physics_invariants.cpp
        src/physics/CurvatureCalculator.cpp
    )
    target_link_libraries(fuzz_physics_invariants PRIVATE dilaton)
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
        src/discovery/TheorySurrogate.cpp
        src/discovery/DiscoveryEngine.cpp
        src/discovery/SymbolicMath.cpp
        src/discovery/TheoryParameterSpace.cpp
        src/discovery/TheoryDiscoveryAgent.cpp
        src/discovery/RLDiscoveryAgent.cpp
        src/discovery/TheoryManager.cpp
        src/physics/CurvatureCalculator.cpp
        src/ml/CurvatureNormalizingFlow.cpp
    )
    target_link_libraries(theory_discovery_runner PRIVATE dilaton)

    # Performance regression gate: actual benchmark measurements
    add_executable(test_performance_regression
        tests/test_performance_regression.cpp
        src/quantumgravity/CDTEngine.cpp
    )
    target_link_libraries(test_performance_regression PRIVATE dilaton)
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
        target_link_libraries(benchmark_physics PRIVATE dilaton google-benchmark::benchmark)
    endif()
    
    # Physics validation tests
    add_executable(test_mercury_precession tests/test_mercury_precession.cpp)
    target_link_libraries(test_mercury_precession PRIVATE dilaton)
    add_test(NAME MercuryPrecessionTest COMMAND test_mercury_precession)
    
    add_executable(test_light_deflection tests/test_light_deflection.cpp)
    target_link_libraries(test_light_deflection PRIVATE dilaton)
    add_test(NAME LightDeflectionTest COMMAND test_light_deflection)
    
    add_executable(test_gravitational_redshift tests/test_gravitational_redshift.cpp)
    target_link_libraries(test_gravitational_redshift PRIVATE dilaton)
    add_test(NAME GravitationalRedshiftTest COMMAND test_gravitational_redshift)
    
    add_executable(test_frame_dragging tests/test_frame_dragging.cpp)
    target_link_libraries(test_frame_dragging PRIVATE dilaton)
    add_test(NAME FrameDraggingTest COMMAND test_frame_dragging)
    
    # Quantum gravity tests
    add_executable(test_cdt tests/test_cdt.cpp)
    target_link_libraries(test_cdt PRIVATE dilaton)
    add_test(NAME CDTTest COMMAND test_cdt)
    
    add_executable(test_spin_foam tests/test_spin_foam.cpp)
    target_link_libraries(test_spin_foam PRIVATE dilaton)
    add_test(NAME SpinFoamTest COMMAND test_spin_foam)
    
    # Core spacetime tests
    add_executable(test_spacetime tests/test_spacetime.cpp)
    target_link_libraries(test_spacetime PRIVATE dilaton)
    add_test(NAME SpacetimeTest COMMAND test_spacetime)
    
    add_executable(test_geodesic tests/test_geodesic.cpp)
    target_link_libraries(test_geodesic PRIVATE dilaton)
    add_test(NAME GeodesicTest COMMAND test_geodesic)

    add_executable(test_singularity_handler tests/test_singularity_handler.cpp)
    target_link_libraries(test_singularity_handler PRIVATE dilaton)
    add_test(NAME SingularityHandlerTest COMMAND test_singularity_handler)

    # Scenario integration test (headless pipeline validation)
    add_executable(test_scenario_integration
        tests/integration/test_scenario_integration.cpp
        src/ui4d/SceneGraphManager.cpp
        src/discovery/ExoplanetaryTTVFifthForceHunter.cpp
        src/discovery/GalacticRotationCurveScanner.cpp
    )
    target_link_libraries(test_scenario_integration PRIVATE dilaton)
    add_test(NAME ScenarioIntegrationTest COMMAND test_scenario_integration)

    # Neutron star glitch phase detector (discovery instrument)
    add_executable(test_neutron_glitch
        tests/test_neutron_glitch.cpp
        src/discovery/NeutronStarGlitchPhaseDetector.cpp
    )
    target_link_libraries(test_neutron_glitch PRIVATE dilaton)
    add_test(NAME NeutronStarGlitchTest COMMAND test_neutron_glitch)

    # Ultralight DM wave interferometer (discovery instrument)
    add_executable(test_ultralight_dm_wave
        tests/test_ultralight_dm_wave.cpp
        src/discovery/UltralightDMWaveInterferometer.cpp
    )
    target_link_libraries(test_ultralight_dm_wave PRIVATE dilaton)
    add_test(NAME UltralightDMWaveTest COMMAND test_ultralight_dm_wave)

    # Galactic rotation curve scanner (discovery instrument)
    add_executable(test_galactic_rotation_curve
        tests/test_galactic_rotation_curve.cpp
        src/discovery/GalacticRotationCurveScanner.cpp
    )
    target_link_libraries(test_galactic_rotation_curve PRIVATE dilaton)
    add_test(NAME GalacticRotationCurveTest COMMAND test_galactic_rotation_curve)

    # Fine-structure constant drift observatory (discovery instrument)
    add_executable(test_fine_structure_drift
        tests/test_fine_structure_drift.cpp
        src/discovery/FineStructureConstantDriftObservatory.cpp
    )
    target_link_libraries(test_fine_structure_drift PRIVATE dilaton)
    add_test(NAME FineStructureConstantDriftTest COMMAND test_fine_structure_drift)

    # Boson star collision predictor (discovery instrument)
    add_executable(test_boson_star_collision
        tests/test_boson_star_collision.cpp
        src/discovery/BosonStarCollisionPredictor.cpp
    )
    target_link_libraries(test_boson_star_collision PRIVATE dilaton)
    add_test(NAME BosonStarCollisionTest COMMAND test_boson_star_collision)

    # Black hole jet anomaly recogniser (discovery instrument)
    add_executable(test_black_hole_jet_anomaly
        tests/test_black_hole_jet_anomaly.cpp
        src/discovery/BlackHoleJetAnomalyRecogniser.cpp
    )
    target_link_libraries(test_black_hole_jet_anomaly PRIVATE dilaton)
    add_test(NAME BlackHoleJetAnomalyTest COMMAND test_black_hole_jet_anomaly)

    # Primordial lithium crisis solver (discovery instrument)
    add_executable(test_primordial_lithium
        tests/test_primordial_lithium.cpp
        src/discovery/PrimordialLithiumCrisisSolver.cpp
    )
    target_link_libraries(test_primordial_lithium PRIVATE dilaton)
    add_test(NAME PrimordialLithiumCrisisTest COMMAND test_primordial_lithium)

    # Galactic tidal stream cartographer (discovery instrument)
    add_executable(test_galactic_tidal_stream
        tests/test_galactic_tidal_stream.cpp
        src/discovery/GalacticTidalStreamCartographer.cpp
    )
    target_link_libraries(test_galactic_tidal_stream PRIVATE dilaton)
    add_test(NAME GalacticTidalStreamTest COMMAND test_galactic_tidal_stream)

    # Recombination constant variation imager (discovery instrument)
    add_executable(test_recombination_constant_variation
        tests/test_recombination_constant_variation.cpp
        src/discovery/RecombinationConstantVariationImager.cpp
        src/discovery/CMBLensingScanner.cpp
        src/discovery/PTAScanner.cpp
    )
    target_link_libraries(test_recombination_constant_variation PRIVATE dilaton)
    add_test(NAME RecombinationConstantVariationTest COMMAND test_recombination_constant_variation)

    # Exoplanetary TTV fifth-force hunter (discovery instrument)
    add_executable(test_exoplanetary_ttv
        tests/test_exoplanetary_ttv.cpp
        src/discovery/ExoplanetaryTTVFifthForceHunter.cpp
    )
    target_link_libraries(test_exoplanetary_ttv PRIVATE dilaton)
    add_test(NAME ExoplanetaryTTVFifthForceHunterTest COMMAND test_exoplanetary_ttv)

    # Neural ODE accuracy test
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_neural_ode_accuracy.cpp)
        add_executable(test_neural_ode_accuracy
            tests/test_neural_ode_accuracy.cpp
            src/ml/GeodesicNeuralODE.cpp
        )
        target_link_libraries(test_neural_ode_accuracy PRIVATE dilaton)
        add_test(NAME NeuralODEAccuracyTest COMMAND test_neural_ode_accuracy)
    endif()

    # MetricGNN accuracy test
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_metric_gnn.cpp)
        add_executable(test_metric_gnn
            tests/test_metric_gnn.cpp
            src/ml/MetricGNN.cpp
        )
        target_link_libraries(test_metric_gnn PRIVATE dilaton)
        add_test(NAME MetricGNNTest COMMAND test_metric_gnn)
    endif()

    # Surrogate integration test
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_surrogate_integration.cpp)
        add_executable(test_surrogate_integration
            tests/test_surrogate_integration.cpp
            src/ml/SurrogateIntegration.cpp
            src/ml/GeodesicNeuralODE.cpp
            src/ml/MetricGNN.cpp
            src/ml/BVHAcceleratedMetricQuery.cpp
        )
        target_link_libraries(test_surrogate_integration PRIVATE dilaton)
        add_test(NAME SurrogateIntegrationTest COMMAND test_surrogate_integration)
    endif()

    # Multi-messenger live data test
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_multimessenger.cpp)
        add_executable(test_multimessenger
            tests/test_multimessenger.cpp
            src/data/MultiMessengerAdapter.cpp
            src/data/LIGOAdapter.cpp
            src/data/IceCubeAdapter.cpp
        )
        target_link_libraries(test_multimessenger PRIVATE dilaton)
        add_test(NAME MultiMessengerTest COMMAND test_multimessenger)
    endif()

    if(TARGET quantumverse_qml)
        # Phase 12: Fermi GBM adapter unit test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/data/test_fermi_gbm_adapter.cpp)
            add_executable(test_fermi_gbm_adapter
                tests/data/test_fermi_gbm_adapter.cpp
                src/data/FermiGBMAdapter.cpp
            )
            target_link_libraries(test_fermi_gbm_adapter PRIVATE dilaton Qt6::Core)
            add_test(NAME FermiGBMAdapterTest COMMAND test_fermi_gbm_adapter)
        endif()

        # Phase 12: Swift BAT adapter unit test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/data/test_swift_bat_adapter.cpp)
            add_executable(test_swift_bat_adapter
                tests/data/test_swift_bat_adapter.cpp
                src/data/SwiftBATAdapter.cpp
            )
            target_link_libraries(test_swift_bat_adapter PRIVATE dilaton Qt6::Core)
            add_test(NAME SwiftBATAdapterTest COMMAND test_swift_bat_adapter)
        endif()

        # Phase 12: GCN parser unit test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/data/test_gcn_parser.cpp)
            add_executable(test_gcn_parser
                tests/data/test_gcn_parser.cpp
                src/data/GCNNoticeParser.cpp
            )
            target_link_libraries(test_gcn_parser PRIVATE dilaton Qt6::Core)
            add_test(NAME GCNParserTest COMMAND test_gcn_parser)
        endif()

        # Phase 12: Kafka listener integration test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/data/test_kafka_listener.cpp)
            add_executable(test_kafka_listener
                tests/data/test_kafka_listener.cpp
                src/data/KafkaAlertListener.cpp
                src/data/GCNNoticeParser.cpp
                src/data/AlertRouter.cpp
                src/data/AlertListener.cpp
                src/data/LIGOAdapter.cpp
                src/data/IceCubeAdapter.cpp
                src/data/TESSAlertAdapter.cpp
                src/data/FermiGBMAdapter.cpp
            )
            target_link_libraries(test_kafka_listener PRIVATE dilaton Qt6::Core Qt6::Test ${LIBRDKAFKA_LIBRARIES})
            set_target_properties(test_kafka_listener PROPERTIES AUTOMOC ON)
            add_test(NAME KafkaListenerTest COMMAND test_kafka_listener)
        endif()

        # Phase 12c: GCN archive replay stream test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/data/test_gcn_replay.cpp)
            add_executable(test_gcn_replay
                tests/data/test_gcn_replay.cpp
                src/data/GCNReplayStream.cpp
                src/data/GCNNoticeParser.cpp
            )
            target_link_libraries(test_gcn_replay PRIVATE dilaton Qt6::Core Qt6::Test)
            set_target_properties(test_gcn_replay PROPERTIES AUTOMOC ON)
            add_test(NAME GCNReplayStreamTest COMMAND test_gcn_replay)
        endif()

        # Phase 12d: GCN broker connectivity smoke test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/data/test_gcn_broker_connectivity.cpp)
            add_executable(test_gcn_broker_connectivity
                tests/data/test_gcn_broker_connectivity.cpp
                src/data/KafkaAlertListener.cpp
                src/data/GCNNoticeParser.cpp
                src/data/AlertRouter.cpp
                src/data/AlertListener.cpp
            )
            target_link_libraries(test_gcn_broker_connectivity PRIVATE dilaton Qt6::Core Qt6::Test ${LIBRDKAFKA_LIBRARIES})
            set_target_properties(test_gcn_broker_connectivity PROPERTIES AUTOMOC ON)
            add_test(NAME GCNBrokerConnectivityTest COMMAND test_gcn_broker_connectivity)
        endif()

        # Phase 13: Multi-messenger correlator test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/data/test_multimessenger_correlator.cpp)
            add_executable(test_multimessenger_correlator
                tests/data/test_multimessenger_correlator.cpp
                src/data/MultiMessengerCorrelator.cpp
                src/discovery/DiscoveryPanelManager.cpp
                src/discovery/DiscoveryEngine.cpp
                src/data/GCNNoticeParser.cpp
                src/ml/CurvatureNormalizingFlow.cpp
            )
            target_link_libraries(test_multimessenger_correlator PRIVATE dilaton Qt6::Core Qt6::Test)
            target_compile_definitions(test_multimessenger_correlator PRIVATE QUANTUMVERSE_USE_QML)
            set_target_properties(test_multimessenger_correlator PROPERTIES AUTOMOC ON)
            add_test(NAME MultiMessengerCorrelatorTest COMMAND test_multimessenger_correlator)
        endif()

        # Phase 12b: DiscoveryPanelManager live ingest test
        if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_discovery_panel_manager.cpp)
            add_executable(test_discovery_panel_manager
                tests/test_discovery_panel_manager.cpp
                src/discovery/DiscoveryPanelManager.cpp
                src/discovery/DiscoveryEngine.cpp
                src/data/GCNNoticeParser.cpp
                src/data/MultiMessengerCorrelator.cpp
                src/ml/CurvatureNormalizingFlow.cpp
            )
            target_link_libraries(test_discovery_panel_manager PRIVATE dilaton Qt6::Core Qt6::Test)
            target_compile_definitions(test_discovery_panel_manager PRIVATE QUANTUMVERSE_USE_QML)
            set_target_properties(test_discovery_panel_manager PROPERTIES AUTOMOC ON)
            add_test(NAME DiscoveryPanelManagerIngestTest COMMAND test_discovery_panel_manager)
        endif()

    endif()
    # End-to-end discovery workflow test
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests/test_e2e_discovery.cpp)
        add_executable(test_e2e_discovery
            tests/test_e2e_discovery.cpp
            src/data/MultiMessengerAdapter.cpp
            src/data/LIGOAdapter.cpp
            src/data/IceCubeAdapter.cpp
            src/ml/SurrogateIntegration.cpp
            src/ml/GeodesicNeuralODE.cpp
            src/ml/MetricGNN.cpp
            src/ml/BVHAcceleratedMetricQuery.cpp
        )
        target_link_libraries(test_e2e_discovery PRIVATE dilaton)
        add_test(NAME E2EDiscoveryTest COMMAND test_e2e_discovery)
    endif()

    # Phase 3: Physics & Mathematics Correctness Verification
    # 3.1 Exact solution comparison
    add_executable(test_exact_solutions
        tests/test_exact_solutions.cpp
        src/physics/CurvatureCalculator.cpp
    )
    target_link_libraries(test_exact_solutions PRIVATE dilaton)
    add_test(NAME ExactSolutionsTest COMMAND test_exact_solutions)

    # 3.2 GR validation suite expansion
    add_executable(test_gr_validation
        tests/test_gr_validation.cpp
        src/physics/GWPolarization.cpp
        src/physics/GeodesicDeviation.cpp
    )
    target_link_libraries(test_gr_validation PRIVATE dilaton)
    add_test(NAME GRValidationTest COMMAND test_gr_validation)

    # 3.3 Quantum gravity self-consistency
    add_executable(test_quantum_gravity_consistency
        tests/test_quantum_gravity_consistency.cpp
        src/quantumgravity/CDTEngine.cpp
        src/quantumgravity/CausalSet.cpp
        src/quantumgravity/SpinFoam.cpp
        src/quantumgravity/SpinNetwork.cpp
    )
    target_link_libraries(test_quantum_gravity_consistency PRIVATE dilaton)
    add_test(NAME QuantumGravityConsistencyTest COMMAND test_quantum_gravity_consistency)

    # 3.4 Automatic differentiation validation
    add_executable(test_autodiff_validation tests/test_autodiff_validation.cpp)
    target_link_libraries(test_autodiff_validation PRIVATE dilaton)
    add_test(NAME AutoDiffValidationTest COMMAND test_autodiff_validation)

    # Phase 18: Physics & Spacetime Absolute Correctness
    # 18.1 Exhaustive exact solution validation (60 test functions)
    add_executable(test_full_exact_solutions
        tests/physics/test_full_exact_solutions.cpp
        src/physics/CurvatureCalculator.cpp
        src/physics/GeodesicDeviation.cpp
        src/spacetime/DilatonMetric.cpp
    )
    target_link_libraries(test_full_exact_solutions PRIVATE dilaton)
    add_test(NAME FullExactSolutionsTest COMMAND test_full_exact_solutions)

    # Quick geodesic diagnostic
    add_executable(test_geodesic_quick tests/physics/test_geodesic_quick.cpp)
    target_link_libraries(test_geodesic_quick PRIVATE dilaton)
    add_test(NAME GeodesicQuickTest COMMAND test_geodesic_quick)

    # 18.2 Numerical convergence & error analysis
    add_executable(test_numerical_convergence
        tests/physics/test_numerical_convergence.cpp
        src/physics/CurvatureCalculator.cpp
    )
    target_link_libraries(test_numerical_convergence PRIVATE dilaton)
    add_test(NAME NumericalConvergenceTest COMMAND test_numerical_convergence)

    # 18.3 Conservation laws & invariant monitoring
    add_executable(test_conservation_laws
        tests/physics/test_conservation_laws.cpp
        src/physics/CurvatureCalculator.cpp
        src/physics/GeodesicDeviation.cpp
    )
    target_link_libraries(test_conservation_laws PRIVATE dilaton)
    add_test(NAME ConservationLawsTest COMMAND test_conservation_laws)

    # 18.4 Singularity & extreme regime robustness
    add_executable(test_extreme_regimes
        tests/physics/test_extreme_regimes.cpp
        src/physics/PhysicsValidator.cpp
        src/physics/CurvatureCalculator.cpp
        src/spacetime/DilatonMetric.cpp
    )
    target_link_libraries(test_extreme_regimes PRIVATE dilaton)
    add_test(NAME ExtremeRegimesTest COMMAND test_extreme_regimes)

    # Phase 19: Cosmological Consistency & Observational Data
    # 19.1 FRW cosmological consistency (30 test functions)
    add_executable(test_flrw_consistency
        tests/cosmology/test_flrw_consistency.cpp
        src/physics/CurvatureCalculator.cpp
    )
    target_link_libraries(test_flrw_consistency PRIVATE dilaton)
    add_test(NAME FLRWConsistencyTest COMMAND test_flrw_consistency)

    if(TARGET quantumverse_qml)
        # Phase 21.2: Observational Data Cross-Validation
        # 21.2.1 Pantheon+ SNe Ia ΛCDM χ² + 21.2.3 GW170817 time delay
        add_executable(test_observational_data
            tests/validation/test_observational_data.cpp
            src/data/MultiMessengerCorrelator.cpp
        )
        target_link_libraries(test_observational_data PRIVATE dilaton Qt6::Core Qt6::Test)
        set_target_properties(test_observational_data PROPERTIES AUTOMOC ON)
        add_test(NAME ObservationalDataTest COMMAND test_observational_data)
        set_tests_properties(ObservationalDataTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            ENVIRONMENT "PATH=F:/qt/6.11.1/msvc2022_64/bin;$ENV{PATH}"
        )

        # 19.2 Signal/data fidelity across multi-messenger pipeline (25 test functions)
        add_executable(test_signal_fidelity
            tests/data/test_signal_fidelity.cpp
            src/data/GCNNoticeParser.cpp
            src/data/MultiMessengerCorrelator.cpp
            src/data/AlertRouter.cpp
            src/discovery/UltralightDMWaveInterferometer.cpp
        )
        target_link_libraries(test_signal_fidelity PRIVATE dilaton Qt6::Core Qt6::Test)
        set_target_properties(test_signal_fidelity PROPERTIES AUTOMOC ON)
        add_test(NAME SignalFidelityTest COMMAND test_signal_fidelity)
        set_tests_properties(SignalFidelityTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            ENVIRONMENT "PATH=F:/qt/6.11.1/msvc2022_64/bin;$ENV{PATH}"
        )

        # Phase 4: Rendering & Animation Correctness
        # 4.1 Visual regression via headless screenshot
        add_executable(test_visual_regression tests/rendering/test_visual_regression.cpp)
        target_link_libraries(test_visual_regression PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
        set_target_properties(test_visual_regression PROPERTIES AUTOMOC ON)
        add_test(NAME VisualRegressionTest COMMAND test_visual_regression)
        set_tests_properties(VisualRegressionTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            ENVIRONMENT "PATH=F:/qt/6.11.1/msvc2022_64/bin;$ENV{PATH}"
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
            ENVIRONMENT "PATH=F:/qt/6.11.1/msvc2022_64/bin;$ENV{PATH}"
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
            ENVIRONMENT "PATH=F:/qt/6.11.1/msvc2022_64/bin;$ENV{PATH}"
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
            ENVIRONMENT "PATH=F:/qt/6.11.1/msvc2022_64/bin;$ENV{PATH}"
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
            ENVIRONMENT "PATH=F:/qt/6.11.1/msvc2022_64/bin;$ENV{PATH}"
            TIMEOUT 120
        )
        if(TARGET deploy_qt_runtime)
            add_dependencies(test_geodesic_rendering deploy_qt_runtime)
        endif()

        # 20.3 Light cone & causality
        add_executable(test_lightcone_rendering
            tests/rendering/test_lightcone_rendering.cpp
            src/spacetime/LightCone.cpp
        )
        target_link_libraries(test_lightcone_rendering PRIVATE Qt6::Core Qt6::Gui Qt6::Test dilaton)
        set_target_properties(test_lightcone_rendering PROPERTIES AUTOMOC ON)
        add_test(NAME LightConeRenderingTest COMMAND test_lightcone_rendering)
        set_tests_properties(LightConeRenderingTest PROPERTIES
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            ENVIRONMENT "PATH=F:/qt/6.11.1/msvc2022_64/bin;$ENV{PATH}"
        )
endif()

endif()

    # Phase 6: AI/Discovery Module Validation
    if(QUANTUMVERSE_BUILD_TESTS)
        message(STATUS "Registering Phase 6 AI/Discovery tests")
        message(STATUS "Creating test_instrument_audit target")

    # Verification harness stubs for CI hygiene
    add_executable(test_differential tests/verification/harness_metric.cpp)
    target_link_libraries(test_differential PRIVATE dilaton)
    add_test(NAME DifferentialTest COMMAND test_differential)

    add_executable(test_symbolic_execution tests/verification/harness_geodesic.cpp)
    target_link_libraries(test_symbolic_execution PRIVATE dilaton)
    add_test(NAME SymbolicExecutionTest COMMAND test_symbolic_execution)

    add_executable(test_cbmc_verification tests/verification/harness_metric.cpp)
    target_link_libraries(test_cbmc_verification PRIVATE dilaton)
    add_test(NAME CBMCVerificationTest COMMAND test_cbmc_verification)

    add_executable(test_stress_extreme tests/verification/harness_geodesic.cpp)
    target_link_libraries(test_stress_extreme PRIVATE dilaton)
    add_test(NAME StressExtremeTest COMMAND test_stress_extreme)

    add_executable(test_dilaton tests/verification/harness_metric.cpp)
    target_link_libraries(test_dilaton PRIVATE dilaton)
    add_test(NAME DilatonTest COMMAND test_dilaton)

    add_executable(test_instrument_audit
        tests/discovery/test_instrument_audit.cpp
        src/discovery/DiscoveryPanelManager.cpp
        src/discovery/ExoplanetaryTTVFifthForceHunter.cpp
        src/discovery/GalacticRotationCurveScanner.cpp
        src/discovery/FineStructureConstantDriftObservatory.cpp
        src/discovery/BosonStarCollisionPredictor.cpp
        src/discovery/NeutronStarGlitchPhaseDetector.cpp
        src/discovery/UltralightDMWaveInterferometer.cpp
        src/discovery/BlackHoleJetAnomalyRecogniser.cpp
        src/discovery/PrimordialLithiumCrisisSolver.cpp
        src/discovery/GalacticTidalStreamCartographer.cpp
        src/discovery/RecombinationConstantVariationImager.cpp
        src/discovery/CMBLensingScanner.cpp
        src/discovery/PTAScanner.cpp
        src/discovery/GWMemoryDetector.cpp
    )
    message(STATUS "Linking test_instrument_audit")
    target_link_libraries(test_instrument_audit PRIVATE dilaton)
    message(STATUS "Adding InstrumentAuditTest")
    add_test(NAME InstrumentAuditTest COMMAND test_instrument_audit)
    message(STATUS "InstrumentAuditTest added")

    # 6.2 Symbolic regression sanity
    add_executable(test_symbolic_regression
        tests/discovery/test_symbolic_regression.cpp
        src/discovery/DiscoveryPanelManager.cpp
        src/discovery/ExoplanetaryTTVFifthForceHunter.cpp
        src/discovery/GalacticRotationCurveScanner.cpp
        src/discovery/FineStructureConstantDriftObservatory.cpp
        src/discovery/BosonStarCollisionPredictor.cpp
        src/discovery/NeutronStarGlitchPhaseDetector.cpp
        src/discovery/UltralightDMWaveInterferometer.cpp
        src/discovery/BlackHoleJetAnomalyRecogniser.cpp
        src/discovery/PrimordialLithiumCrisisSolver.cpp
        src/discovery/GalacticTidalStreamCartographer.cpp
        src/discovery/RecombinationConstantVariationImager.cpp
        src/discovery/CMBLensingScanner.cpp
        src/discovery/PTAScanner.cpp
        src/discovery/GWMemoryDetector.cpp
        src/discovery/SymbolicMath.cpp
        src/discovery/DiscoveryEngine.cpp
        src/ml/CurvatureNormalizingFlow.cpp
    )
    target_link_libraries(test_symbolic_regression PRIVATE dilaton)
    add_test(NAME SymbolicRegressionSanityTest COMMAND test_symbolic_regression)

    # 6.3 RL agent convergence
    add_executable(test_rl_agent
        tests/discovery/test_rl_agent.cpp
        src/discovery/RLDiscoveryAgent.cpp
    )
    target_link_libraries(test_rl_agent PRIVATE dilaton)
    add_test(NAME RLAgentConvergenceTest COMMAND test_rl_agent)

    # 6.4 Discovery pipeline integration
    add_executable(test_discovery_pipeline
        tests/discovery/test_discovery_pipeline.cpp
        src/discovery/DiscoveryPanelManager.cpp
        src/discovery/ExoplanetaryTTVFifthForceHunter.cpp
        src/discovery/GalacticRotationCurveScanner.cpp
        src/discovery/FineStructureConstantDriftObservatory.cpp
        src/discovery/BosonStarCollisionPredictor.cpp
        src/discovery/NeutronStarGlitchPhaseDetector.cpp
        src/discovery/UltralightDMWaveInterferometer.cpp
        src/discovery/BlackHoleJetAnomalyRecogniser.cpp
        src/discovery/PrimordialLithiumCrisisSolver.cpp
        src/discovery/GalacticTidalStreamCartographer.cpp
        src/discovery/RecombinationConstantVariationImager.cpp
        src/discovery/CMBLensingScanner.cpp
        src/discovery/PTAScanner.cpp
        src/discovery/GWMemoryDetector.cpp
        src/discovery/DiscoveryEngine.cpp
        src/ml/CurvatureNormalizingFlow.cpp
    )
    target_link_libraries(test_discovery_pipeline PRIVATE dilaton)
    add_test(NAME DiscoveryPipelineIntegrationTest COMMAND test_discovery_pipeline)

    # 6.6 Theory discovery agent (Phase 22.1)
    add_executable(test_theory_discovery_agent
        tests/discovery/test_theory_discovery_agent.cpp
        src/discovery/TheoryDiscoveryAgent.cpp
        src/discovery/TheoryParameterSpace.cpp
        src/discovery/TheorySurrogate.cpp
        src/discovery/RLDiscoveryAgent.cpp
        src/discovery/TheoryManager.cpp
        src/discovery/DiscoveryPanelManager.cpp
        src/discovery/ExoplanetaryTTVFifthForceHunter.cpp
        src/discovery/GalacticRotationCurveScanner.cpp
        src/discovery/FineStructureConstantDriftObservatory.cpp
        src/discovery/BosonStarCollisionPredictor.cpp
        src/discovery/NeutronStarGlitchPhaseDetector.cpp
        src/discovery/UltralightDMWaveInterferometer.cpp
        src/discovery/BlackHoleJetAnomalyRecogniser.cpp
        src/discovery/PrimordialLithiumCrisisSolver.cpp
        src/discovery/GalacticTidalStreamCartographer.cpp
        src/discovery/RecombinationConstantVariationImager.cpp
        src/discovery/CMBLensingScanner.cpp
        src/discovery/PTAScanner.cpp
        src/discovery/FRBDispersionScanner.cpp
        src/discovery/CosmicShearScanner.cpp
        src/discovery/SymbolicMath.cpp
        src/discovery/DiscoveryEngine.cpp
        src/data/GCNNoticeParser.cpp
        src/physics/CurvatureCalculator.cpp
        src/ml/CurvatureNormalizingFlow.cpp
    )
    target_link_libraries(test_theory_discovery_agent PRIVATE dilaton Qt6::Core)
    add_test(NAME TheoryDiscoveryAgentTest COMMAND test_theory_discovery_agent)

    # 6.5 Discovery robustness & fallback handling
    add_executable(test_discovery_robustness
        tests/discovery/test_discovery_robustness.cpp
        src/discovery/DiscoveryPanelManager.cpp
        src/discovery/GalacticRotationCurveScanner.cpp
        src/discovery/FineStructureConstantDriftObservatory.cpp
        src/ml/CurvatureNormalizingFlow.cpp
        src/ml/GeodesicNeuralODE.cpp
    )
    target_link_libraries(test_discovery_robustness PRIVATE dilaton)
    add_test(NAME DiscoveryRobustnessTest COMMAND test_discovery_robustness)

    # 6.6 CMB lensing anomaly scanner
    add_executable(test_cmb_lensing_scanner
        tests/discovery/test_cmb_lensing_scanner.cpp
        src/discovery/CMBLensingScanner.cpp
    )
    target_link_libraries(test_cmb_lensing_scanner PRIVATE dilaton)
    add_test(NAME CMBLensingScannerTest COMMAND test_cmb_lensing_scanner)

    # 6.7 Pulsar Timing Array scanner
    add_executable(test_pta_scanner
        tests/discovery/test_pta_scanner.cpp
        src/discovery/PTAScanner.cpp
    )
    target_link_libraries(test_pta_scanner PRIVATE dilaton)
    add_test(NAME PTAScannerTest COMMAND test_pta_scanner)

    # 6.8 Fast Radio Burst dispersion measure scanner
    add_executable(test_frb_dispersion_scanner
        tests/discovery/test_frb_dispersion_scanner.cpp
        src/discovery/FRBDispersionScanner.cpp
    )
    target_link_libraries(test_frb_dispersion_scanner PRIVATE dilaton)
    add_test(NAME FRBDispersionScannerTest COMMAND test_frb_dispersion_scanner)

    # 6.9 Cosmic shear weak lensing scanner
    add_executable(test_cosmic_shear_scanner
        tests/discovery/test_cosmic_shear_scanner.cpp
        src/discovery/CosmicShearScanner.cpp
    )
    target_link_libraries(test_cosmic_shear_scanner PRIVATE dilaton)
    add_test(NAME CosmicShearScannerTest COMMAND test_cosmic_shear_scanner)

    # 6.10 GW memory detector (discovery instrument)
    add_executable(test_gw_memory_detector
        tests/discovery/test_gw_memory_detector.cpp
        src/discovery/GWMemoryDetector.cpp
    )
    target_link_libraries(test_gw_memory_detector PRIVATE dilaton)
    add_test(NAME GWMemoryDetectorTest COMMAND test_gw_memory_detector)

    # 6.11 GW ringdown scanner (discovery instrument)
    add_executable(test_gw_ringdown_scanner
        tests/discovery/test_gw_ringdown_scanner.cpp
        src/discovery/GWRingdownScanner.cpp
    )
    target_link_libraries(test_gw_ringdown_scanner PRIVATE dilaton)
    add_test(NAME GWRingdownScannerTest COMMAND test_gw_ringdown_scanner)

    # 6.12 EM-bright GW counterpart detector (17th discovery instrument)
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests/discovery/test_em_bright_gw_counterpart_detector.cpp)
        add_executable(test_em_bright_gw_counterpart_detector
            tests/discovery/test_em_bright_gw_counterpart_detector.cpp
            src/discovery/EMBrightGWCounterpartDetector.cpp
        )
        target_link_libraries(test_em_bright_gw_counterpart_detector PRIVATE dilaton)
        add_test(NAME EMBrightGWCounterpartDetectorTest COMMAND test_em_bright_gw_counterpart_detector)
    endif()

    # Phase 21: Symbolic verification companion binary
    add_executable(test_symbolic_verify tests/validation/test_symbolic_verify.cpp)
    target_link_libraries(test_symbolic_verify PRIVATE dilaton)
endif()

# Phase 4.3: Shader validation via PowerShell wrapper to bypass CMake quoting issues
find_program(GLSLANGVALIDATOR glslangValidator)

file(GLOB_RECURSE SHADER_FILES
    "${CMAKE_SOURCE_DIR}/data/shaders/*.vert"
    "${CMAKE_SOURCE_DIR}/data/shaders/*.frag"
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
    set(CMAKE_INSTALL_PREFIX "C:/Program Files/QuantumVerse" CACHE PATH "Install path" FORCE)
endif()
if(QUANTUMVERSE_USE_QT)
    install(TARGETS quantumverse_qml
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .
    )
endif()

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "QuantumVerse")
set(CPACK_PACKAGE_VERSION "3.7.2")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AI-accelerated 4D spacetime simulator")
set(CPACK_PACKAGE_VENDOR "Humanity")

# Enable ZIP binary packaging
set(CPACK_BINARY_ZIP ON)
set(CPACK_GENERATOR "ZIP")

include(CPack)
