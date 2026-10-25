#include "WasmLLMCoPilot.h"
#include <emscripten/val.h>
#include <iostream>
#include <cstring>
#include <sstream>
#include <memory>
#include <vector>
#include <mutex>

namespace quantumverse {

// Context kept alive for the duration of an async fetch.
struct FetchContext {
    WasmLLMCoPilot* self = nullptr;
    std::shared_ptr<std::string> body;
};

// Active fetch contexts, kept alive until their callbacks fire.
static std::vector<std::shared_ptr<FetchContext>> g_activeContexts;
static std::mutex g_contextsMutex;

WasmLLMCoPilot::WasmLLMCoPilot() = default;

WasmLLMCoPilot::~WasmLLMCoPilot() = default;

void WasmLLMCoPilot::setAnalysisCallback(AnalysisCallback cb)
{
    m_callback = std::move(cb);
}

void WasmLLMCoPilot::analyzeAnomaly(const std::string& anomalyData)
{
    // Build JSON payload for Ollama /api/generate
    std::ostringstream json;
    json << R"({"model":"llama3","prompt":")";
    std::string prompt = "You are a brilliant astrophysicist analyzing a 4D spacetime simulation. "
        "The following data was logged by the QuantumVerse Discovery Engine: "
        + anomalyData + ". "
        "Explain what this means for General Relativity in 2-3 sentences. "
        "Be precise, use scientific terms (metric signature, Kretschmann scalar, "
        "cosmic censorship, event horizon, naked singularity), and focus on "
        "spacetime curvature and physical validity.";
    for (char c : prompt) {
        switch (c) {
            case '"':  json << "\\\""; break;
            case '\\': json << "\\\\"; break;
            case '\n': json << "\\n"; break;
            case '\r': json << "\\r"; break;
            case '\t': json << "\\t"; break;
            default:   json << c; break;
        }
    }
    json << R"(","stream":false})";
    auto body = std::make_shared<std::string>(json.str());

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "POST");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = WasmLLMCoPilot::onFetchSucceeded;
    attr.onerror = WasmLLMCoPilot::onFetchFailed;
    attr.userData = nullptr;  // set below

    const char* headers[] = {
        "Content-Type", "application/json",
        "Accept", "application/json",
        nullptr
    };
    attr.requestHeaders = headers;
    attr.requestData = body->c_str();
    attr.requestDataSize = body->size();

    auto ctx = std::make_shared<FetchContext>();
    ctx->self = this;
    ctx->body = body;

    {
        std::lock_guard<std::mutex> lock(g_contextsMutex);
        g_activeContexts.push_back(ctx);
    }
    attr.userData = ctx.get();

    std::string url = "http://localhost:11434/api/generate";
    emscripten_fetch_t* fetch = emscripten_fetch(&attr, url.c_str());
    // Fetch managed by emscripten; callbacks receive ctx via userData
}

void WasmLLMCoPilot::onFetchSucceeded(emscripten_fetch_t* fetch)
{
    auto ctx = static_cast<FetchContext*>(fetch->userData);
    if (!ctx || !ctx->self || !ctx->self->m_callback) {
        emscripten_fetch_close(fetch);
        return;
    }

    std::string response;
    if (fetch->data && fetch->numBytes > 0) {
        response.assign(static_cast<const char*>(fetch->data), fetch->numBytes);
    }

    // Parse JSON response: extract "response" field
    std::string explanation;
    size_t respPos = response.find("\"response\"");
    if (respPos != std::string::npos) {
        size_t colonPos = response.find(':', respPos);
        if (colonPos != std::string::npos) {
            size_t quotePos = response.find('"', colonPos + 1);
            if (quotePos != std::string::npos) {
                size_t endQuote = response.find('"', quotePos + 1);
                if (endQuote != std::string::npos) {
                    explanation = response.substr(quotePos + 1, endQuote - quotePos - 1);
                    // Unescape JSON string
                    std::string unescaped;
                    for (size_t i = 0; i < explanation.size(); ++i) {
                        if (explanation[i] == '\\' && i + 1 < explanation.size()) {
                            char next = explanation[i + 1];
                            switch (next) {
                                case '"': unescaped += '"'; break;
                                case '\\': unescaped += '\\'; break;
                                case 'n': unescaped += '\n'; break;
                                case 'r': unescaped += '\r'; break;
                                case 't': unescaped += '\t'; break;
                                default: unescaped += next; break;
                            }
                            ++i;
                        } else {
                            unescaped += explanation[i];
                        }
                    }
                    explanation = unescaped;
                }
            }
        }
    }

    if (explanation.empty()) {
        explanation = "LLM returned an empty response.";
    }

    ctx->self->m_callback(explanation);
    emscripten_fetch_close(fetch);

    // Remove context from active list
    {
        std::lock_guard<std::mutex> lock(g_contextsMutex);
        for (auto it = g_activeContexts.begin(); it != g_activeContexts.end(); ++it) {
            if (it->get() == ctx) {
                g_activeContexts.erase(it);
                break;
            }
        }
    }
}

void WasmLLMCoPilot::onFetchFailed(emscripten_fetch_t* fetch)
{
    auto ctx = static_cast<FetchContext*>(fetch->userData);
    if (ctx && ctx->self && ctx->self->m_callback) {
        std::string msg = "Error connecting to LLM: HTTP status ";
        msg += std::to_string(fetch->status);
        ctx->self->m_callback(msg);
    }
    emscripten_fetch_close(fetch);

    {
        std::lock_guard<std::mutex> lock(g_contextsMutex);
        for (auto it = g_activeContexts.begin(); it != g_activeContexts.end(); ++it) {
            if (it->get() == ctx) {
                g_activeContexts.erase(it);
                break;
            }
        }
    }
}

} // namespace quantumverse
