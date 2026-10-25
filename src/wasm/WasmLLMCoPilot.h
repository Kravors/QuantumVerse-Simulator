/**
 * @file WasmLLMCoPilot.h
 * @brief WASM-compatible LLM bridge using emscripten's fetch API.
 *
 * Replaces Qt-based LLMCoPilot for the WASM Portal. Uses emscripten_fetch_async
 * to POST anomaly data to Ollama (localhost:11434) without blocking the main
 * simulation thread. Results are delivered via a std::function callback.
 */

#pragma once

#include <functional>
#include <string>
#include <memory>
#include <emscripten/fetch.h>

namespace quantumverse {

/**
 * @brief Sends anomaly/veto data to a locally running Ollama model and
 * delivers the plain-English explanation via callback.
 *
 * Uses emscripten_fetch_async for non-blocking HTTP requests from WASM.
 * No Qt dependency — safe for WebAssembly compilation.
 */
class WasmLLMCoPilot {
public:
    using AnalysisCallback = std::function<void(const std::string&)>;

    WasmLLMCoPilot();
    ~WasmLLMCoPilot();

    /// Send anomaly data to Ollama for explanation.
    void analyzeAnomaly(const std::string& anomalyData);

    /// Register a callback to receive LLM explanations.
    void setAnalysisCallback(AnalysisCallback cb);

private:
    AnalysisCallback m_callback;

    static void onFetchSucceeded(emscripten_fetch_t* fetch);
    static void onFetchFailed(emscripten_fetch_t* fetch);
};

} // namespace quantumverse
