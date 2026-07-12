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

# Options
option(QUANTUMVERSE_BUILD_TESTS "Build test suite" ON)
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
        src/discovery/FindingsModel.cpp
        src/data/LIGOAdapter.cpp
        src/data/IceCubeAdapter.cpp
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

    if(QUANTUMVERSE_BUILD_TESTS)
        add_executable(qml_performance_baseline tests/benchmark/qml_performance_baseline.cpp)
        set_target_properties(qml_performance_baseline PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:quantumverse_qml>
        )
        add_test(NAME QMLPerformanceBaseline COMMAND qml_performance_baseline)
        set_tests_properties(QMLPerformanceBaseline PROPERTIES
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:quantumverse_qml>"
        )
        add_dependencies(qml_performance_baseline quantumverse_qml)

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
        )
        target_link_libraries(test_multimessenger_ui PRIVATE Qt6::Core dilaton)
        add_test(NAME MultiMessengerUITest COMMAND test_multimessenger_ui)
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
    add_executable(test_property_based tests/unit/test_property_based.cpp)
    target_include_directories(test_property_based PRIVATE ${CMAKE_SOURCE_DIR}/build-appimage/QuantumVerse.AppDir/usr/include ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(test_property_based PRIVATE dilaton)
    add_test(NAME PropertyBasedTest COMMAND test_property_based)
    
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
