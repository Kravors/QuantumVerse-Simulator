/**
 * @file main.cpp
 * @brief WASM Portal entry point.
 *
 * The EMSCRIPTEN_BINDINGS in WasmBridge.h expose the core API to JavaScript.
 * This file is required by the emcc linker but the actual application logic
 * runs in the HTML5 frontend (index.html + app.js).
 */

#include <emscripten/emscripten.h>

extern "C" {

int main() {
    // Entry point for the WASM module. The HTML5 frontend drives the app
    // through EMSCRIPTEN_BINDINGS on WasmBridge.
    return 0;
}

} // extern "C"
