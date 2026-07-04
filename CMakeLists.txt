# QuantumVerse Simulator - CMakeLists.txt
# Production-ready 4D spacetime cognition laboratory
cmake_minimum_required(VERSION 3.20)
project(QuantumVerse LANGUAGES C CXX VERSION 3.5.0)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Coverage instrumentation
if(QUANTUMVERSE_USE_COVERAGE)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage -O0 -g")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} --coverage -O0 -g")
endif()

# MSVC static runtime linking (reduces DLL dependencies)
# Note: Pre-built glfw3.lib uses dynamic CRT, so static linking requires recompiling glfw

# Options
option(QUANTUMVERSE_BUILD_TESTS "Build test suite" ON)
option(QUANTUMVERSE_USE_IMGUI "Build ImGui UI" ON)
option(QUANTUMVERSE_USE_QT "Build Qt UI" OFF)
option(QUANTUMVERSE_USE_COVERAGE "Enable code coverage instrumentation" OFF)

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

# ImGui/ImPlot/GLFW configuration
if(QUANTUMVERSE_USE_IMGUI)
    # Third-party libraries
    set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)
    set(IMPLOT_DIR ${CMAKE_SOURCE_DIR}/third_party/implot)
    set(GLFW_DIR ${CMAKE_SOURCE_DIR}/third_party/glfw)
    set(GLAD_DIR ${CMAKE_SOURCE_DIR}/third_party/glad)

    # GLFW library - use Release lib for both Debug/Release (compatible)
    set(GLFW_LIB_PATH "${CMAKE_SOURCE_DIR}/build/third_party/glfw/src/Release/glfw3.lib")
    add_library(glfw_lib STATIC IMPORTED)
    set_target_properties(glfw_lib PROPERTIES 
        IMPORTED_LOCATION "${GLFW_LIB_PATH}"
        LINKER_LANGUAGE C
    )
    set_target_properties(glfw_lib PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${GLFW_DIR}/include")
    set(GLFW_TARGET glfw_lib)

    # ImGui library
    add_library(imgui STATIC
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
    )
    target_include_directories(imgui PUBLIC ${IMGUI_DIR})

    # ImPlot library
    add_library(implot STATIC
        ${IMPLOT_DIR}/implot.cpp
        ${IMPLOT_DIR}/implot_demo.cpp
        ${IMPLOT_DIR}/implot_items.cpp
    )
    target_include_directories(implot PUBLIC ${IMPLOT_DIR})
    target_link_libraries(implot PUBLIC imgui)

    # Glad OpenGL loader
    add_library(glad STATIC ${GLAD_DIR}/glad.cpp)
    target_include_directories(glad PUBLIC ${GLAD_DIR})

    # ImGui backends
    add_library(imgui_impl_glfw STATIC
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    )
    target_include_directories(imgui_impl_glfw PUBLIC ${IMGUI_DIR}/backends ${IMGUI_DIR} ${GLFW_DIR}/include)
    target_link_libraries(imgui_impl_glfw PUBLIC imgui)

    add_library(imgui_impl_opengl3 STATIC
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )
    target_include_directories(imgui_impl_opengl3 PUBLIC ${IMGUI_DIR}/backends ${IMGUI_DIR} ${GLAD_DIR})
    target_link_libraries(imgui_impl_opengl3 PUBLIC imgui)

# Main UI executable
    add_executable(quantumverse_imgui 
        main_glfw.cpp 
        src/ui4d/ImGuiBackend.cpp src/ui4d/PlotPanel.cpp
        src/ui4d/UI4D_ImGui.cpp
    )
    target_include_directories(quantumverse_imgui PRIVATE
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${IMPLOT_DIR}
        ${GLFW_DIR}/include
        ${GLAD_DIR}
        ${CMAKE_SOURCE_DIR}/src
    )
    target_link_libraries(quantumverse_imgui PRIVATE imgui implot imgui_impl_glfw imgui_impl_opengl3 glad ${GLFW_TARGET} opengl32 user32 gdi32)

    # ImNodes for causal graph visualization (after executable defined)
    set(IMNODES_DIR ${CMAKE_SOURCE_DIR}/third_party)
    if(EXISTS "${IMNODES_DIR}/imnodes.h")
        target_include_directories(quantumverse_imgui PRIVATE ${IMNODES_DIR})
        target_sources(quantumverse_imgui PRIVATE ${IMNODES_DIR}/imnodes.cpp)
        target_compile_definitions(quantumverse_imgui PRIVATE HAVE_IMNODES)
        message(STATUS "ImNodes integrated for causal graph UI")
    else()
        message(WARNING "ImNodes not found; causal graph UI disabled")
    endif()

    # Headless test
    add_executable(test_headless_ui 
        tests/integration/test_headless_ui.cpp 
        src/ui4d/ImGuiBackend.cpp src/ui4d/PlotPanel.cpp
        src/ui4d/UI4D_ImGui.cpp
    )
    target_include_directories(test_headless_ui PRIVATE 
        ${CMAKE_SOURCE_DIR}/src 
        ${GLFW_DIR}/include 
        ${GLAD_DIR}
    )
    target_link_libraries(test_headless_ui PRIVATE imgui implot imgui_impl_glfw imgui_impl_opengl3 glad ${GLFW_TARGET} opengl32 user32 gdi32)
    add_test(NAME HeadlessUITest COMMAND test_headless_ui --headless --frames 10)
endif()

# Qt/QML configuration
if(QUANTUMVERSE_USE_QT)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Qml Quick OpenGL OpenGLWidgets QuickControls2 QuickTemplates2 QuickLayouts Network)

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
        src/discovery/RLDiscoveryAgent.cpp
        src/discovery/SymbolicMath.cpp
        src/discovery/HolographicDualityLab.cpp
        src/ml/CurvatureNormalizingFlow.cpp
        src/ml/DifferentiableSimulator.cpp
        src/spacetime/LightCone.cpp
        src/utils/ConfigManager.cpp
        src/utils/PerformanceMonitor.cpp
        src/utils/TraceLogger.cpp
        src/utils/CrashHandler.cpp
        ${RESOURCES_QRC}
    )
    target_compile_definitions(quantumverse_qml PRIVATE QUANTUMVERSE_USE_QML)
    target_include_directories(quantumverse_qml PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/third_party
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
        opengl32
        user32
        gdi32
    )
    set_target_properties(quantumverse_qml PROPERTIES
        AUTOMOC ON
        AUTORCC ON
        AUTOUIC ON
    )
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
    add_executable(test_property_based tests/unit/test_property_based.cpp)
    target_include_directories(test_property_based PRIVATE ${CMAKE_SOURCE_DIR}/build-appimage/QuantumVerse.AppDir/usr/include ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(test_property_based PRIVATE dilaton)
    add_test(NAME PropertyBasedTest COMMAND test_property_based)
    
    # UI automation test
    add_executable(test_ui_automation 
        tests/ui/test_ui_automation.cpp 
        src/ui4d/ImGuiBackend.cpp src/ui4d/PlotPanel.cpp
    )
    target_include_directories(test_ui_automation PRIVATE ${CMAKE_SOURCE_DIR}/src ${GLFW_DIR}/include ${GLAD_DIR} ${CMAKE_SOURCE_DIR}/third_party/stb)
    target_link_libraries(test_ui_automation PRIVATE imgui implot imgui_impl_glfw imgui_impl_opengl3 glad ${GLFW_TARGET} opengl32 user32 gdi32)
    add_test(NAME UIAutomationTest COMMAND test_ui_automation --screenshot)
    
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
    endif()
    
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

    # Neural ODE accuracy test
    add_executable(test_neural_ode_accuracy
        tests/test_neural_ode_accuracy.cpp
        src/ml/GeodesicNeuralODE.cpp
    )
    target_link_libraries(test_neural_ode_accuracy PRIVATE dilaton)
    add_test(NAME NeuralODEAccuracyTest COMMAND test_neural_ode_accuracy)

    # MetricGNN accuracy test
    add_executable(test_metric_gnn
        tests/test_metric_gnn.cpp
        src/ml/MetricGNN.cpp
    )
    target_link_libraries(test_metric_gnn PRIVATE dilaton)
    add_test(NAME MetricGNNTest COMMAND test_metric_gnn)

    # Surrogate integration test
    add_executable(test_surrogate_integration
        tests/test_surrogate_integration.cpp
        src/ml/SurrogateIntegration.cpp
        src/ml/GeodesicNeuralODE.cpp
        src/ml/MetricGNN.cpp
        src/ml/BVHAcceleratedMetricQuery.cpp
    )
    target_link_libraries(test_surrogate_integration PRIVATE dilaton)
    add_test(NAME SurrogateIntegrationTest COMMAND test_surrogate_integration)

    # Multi-messenger live data test
    add_executable(test_multimessenger
        tests/test_multimessenger.cpp
        src/data/MultiMessengerAdapter.cpp
        src/data/LIGOAdapter.cpp
        src/data/IceCubeAdapter.cpp
    )
    target_link_libraries(test_multimessenger PRIVATE dilaton)
    add_test(NAME MultiMessengerTest COMMAND test_multimessenger)

    # End-to-end discovery workflow test
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
option(QUANTUMVERSE_USE_ONNX "Build with ONNX Runtime support" OFF)
if(QUANTUMVERSE_USE_ONNX)
    find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h
        HINTS ${CMAKE_SOURCE_DIR}/third_party/onnxruntime)
    find_library(ONNXRUNTIME_LIB onnxruntime
        HINTS ${CMAKE_SOURCE_DIR}/third_party/onnxruntime)
    if(ONNXRUNTIME_INCLUDE_DIR AND ONNXRUNTIME_LIB)
        target_compile_definitions(dilaton PRIVATE HAVE_ONNX)
        target_include_directories(dilaton INTERFACE ${ONNXRUNTIME_INCLUDE_DIR})
        target_link_libraries(dilaton INTERFACE ${ONNXRUNTIME_LIB})
    endif()
endif()

# Install rules for packaging
if(WIN32)
    set(CMAKE_INSTALL_PREFIX "C:/Program Files/QuantumVerse" CACHE PATH "Install path" FORCE)
endif()
if(QUANTUMVERSE_USE_IMGUI)
    install(TARGETS quantumverse_imgui
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .
    )
endif()
if(QUANTUMVERSE_USE_QT)
    install(TARGETS quantumverse_qml
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION .
    )
endif()

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "QuantumVerse")
set(CPACK_PACKAGE_VERSION "3.5.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AI-accelerated 4D spacetime simulator")
set(CPACK_PACKAGE_VENDOR "Humanity")

# Enable ZIP binary packaging
set(CPACK_BINARY_ZIP ON)
set(CPACK_GENERATOR "ZIP")

include(CPack)
