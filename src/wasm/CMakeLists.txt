# QuantumVerse WASM Portal - Emscripten build configuration
# Compiles the C++ physics core to WebAssembly with a lightweight HTML5/JS frontend.

cmake_minimum_required(VERSION 3.20)

# Verify Emscripten is being used
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    message(FATAL_ERROR "WASM build requires Emscripten (emcc). Run with -DCMAKE_TOOLCHAIN_FILE=<emsdk>/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
endif()

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Emscripten-specific compile options
add_compile_options(
    -sWASM=1
    -sALLOW_MEMORY_GROWTH=1
    -sMODULARIZE=1
    -sEXPORT_ES6=1
    -sEXPORT_NAME="QuantumVerseModule"
    -sEXPORTED_RUNTIME_METHODS='["ccall","cwrap","FS","initialize","wrapJS"]'
    -sEXPORTED_FUNCTIONS='["_main","_malloc","_free"]'
    -sINITIAL_MEMORY=67108864
    -sMAXIMUM_MEMORY=2147483648
    -sDISABLE_EXCEPTION_CATCHING=0
    -sASSERTIONS=1
    -sUSE_ZLIB=0
    -DPLATFORM_WASM
    -DHAVE_ONNX=0
)

# Emscripten link options
add_link_options(
    -sWASM=1
    -sALLOW_MEMORY_GROWTH=1
    -sMODULARIZE=1
    -sEXPORT_ES6=1
    -sEXPORT_NAME="QuantumVerseModule"
    -sINITIAL_MEMORY=67108864
    -sMAXIMUM_MEMORY=2147483648
    -sDISABLE_EXCEPTION_CATCHING=0
    -sASSERTIONS=1
    -sFORCE_FILESYSTEM=1
    --bind
    -sFETCH=1
    --shell-file ${CMAKE_SOURCE_DIR}/src/wasm/shell.html
)

# Include paths
include_directories(${CMAKE_SOURCE_DIR}/src)

# Core source files (Qt-independent subset)
set(WASM_SOURCES
    ${CMAKE_SOURCE_DIR}/src/spacetime/KerrMetric.cpp
    ${CMAKE_SOURCE_DIR}/src/spacetime/DilatonMetric.cpp
    ${CMAKE_SOURCE_DIR}/src/physics/PhysicsValidator.cpp
    ${CMAKE_SOURCE_DIR}/src/discovery/RLDiscoveryAgent.cpp
    ${CMAKE_SOURCE_DIR}/src/wasm/WasmBridge.cpp
    ${CMAKE_SOURCE_DIR}/src/wasm/WasmLLMCoPilot.cpp
)

# Header-only dependencies are included via include_directories

# Build the QuantumVerse WASM module
add_library(quantumverse_wasm STATIC ${WASM_SOURCES})

target_compile_definitions(quantumverse_wasm PRIVATE QUANTUMVERSE_BUILD_WASM)

# The main HTML/JS entry point
add_executable(quantumverse_wasm_app
    ${CMAKE_SOURCE_DIR}/src/wasm/main.cpp
)

target_link_libraries(quantumverse_wasm_app PRIVATE quantumverse_wasm)

# Copy HTML assets to output directory
add_custom_command(TARGET quantumverse_wasm_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/src/wasm/index.html
        $<TARGET_FILE_DIR:quantumverse_wasm_app>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/src/wasm/style.css
        $<TARGET_FILE_DIR:quantumverse_wasm_app>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/src/wasm/app.js
        $<TARGET_FILE_DIR:quantumverse_wasm_app>
    COMMENT "Copying WASM frontend assets"
)
