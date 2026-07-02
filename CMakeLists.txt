# QuantumVerse Simulator - CMakeLists.txt
# Production-ready 4D spacetime cognition laboratory
cmake_minimum_required(VERSION 3.16)
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
option(QUANTUMVERSE_USE_IMGUI "Build ImGui UI" ON)
option(QUANTUMVERSE_USE_QT "Build Qt UI" OFF)
option(QUANTUMVERSE_USE_COVERAGE "Enable code coverage instrumentation" OFF)

enable_testing()

# Main library interface (header-only for now)
add_library(dilaton INTERFACE)
target_include_directories(dilaton INTERFACE ${CMAKE_SOURCE_DIR}/src)

# ImGui/ImPlot/GLFW configuration
if(QUANTUMVERSE_USE_IMGUI)
    # Third-party libraries
    set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)
    set(IMPLOT_DIR ${CMAKE_SOURCE_DIR}/third_party/implot)
    set(GLFW_DIR ${CMAKE_SOURCE_DIR}/third_party/glfw)
    set(GLAD_DIR ${CMAKE_SOURCE_DIR}/third_party/glad)

    # GLFW library - use prebuilt from build directory
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

    # Headless test
    add_executable(test_headless_ui 
        tests/integration/test_headless_ui.cpp 
        src/ui4d/ImGuiBackend.cpp src/ui4d/PlotPanel.cpp
    )
    target_include_directories(test_headless_ui PRIVATE 
        ${CMAKE_SOURCE_DIR}/src 
        ${GLFW_DIR}/include 
        ${GLAD_DIR}
    )
    target_link_libraries(test_headless_ui PRIVATE imgui implot imgui_impl_glfw imgui_impl_opengl3 glad ${GLFW_TARGET} opengl32 user32 gdi32)
    add_test(NAME HeadlessUITest COMMAND test_headless_ui --headless --frames 10)
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
endif()