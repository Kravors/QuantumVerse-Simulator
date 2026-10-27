// QuantumVerse Sandbox: ONNX Model Load Test
// Isolates the ONNX Runtime loading path for rl_discovery_agent.onnx.
// Attempts to load the model using Ort::Session directly and prints
// the exact result (success or exception message).
//
// Three issues identified by this test:
// 1. QUANTUMVERSE_USE_ONNX is OFF by default → Ort::Session code compiled out
// 2. main_qml.cpp:703 looks for QDir::currentPath() + "/rl_discovery_agent.onnx"
//    = build_v71\rl_discovery_agent.onnx (doesn't exist), but the model
//    actually lives at F:\syyyy\rl_discovery_agent.onnx (repo root)
// 3. Model exported with ONNX opset 27; ORT 1.22.1 supports up to opset 22
//    → "The requested API version [27] is not available"

#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

// Convert UTF-8 to wide string on Windows
static std::wstring utf8_to_wide(const std::string& s) {
#ifdef _WIN32
    int needed = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (needed <= 0) return L"";
    std::wstring w(needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], needed);
    w.resize(needed - 1);
    return w;
#else
    return std::wstring(s.begin(), s.end());
#endif
}

int main(int argc, char** argv) {
    std::string modelPath = (argc > 1) ? argv[1] : "rl_discovery_agent.onnx";

    std::cout << "=== ONNX Model Load Test (Sandbox Session B) ===" << std::endl;
    std::cout << "Model path: " << modelPath << std::endl;
    std::cout.flush();

    // Check if file exists
    std::ifstream f(modelPath, std::ios::binary);
    if (!f.good()) {
        std::cerr << "FAIL: Model file not found or unreadable: " << modelPath << std::endl;
        return 1;
    }
    f.close();
    std::cout << "OK: Model file exists and is readable" << std::endl;
    std::cout.flush();

    try {
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "onnx_load_test");
        std::cout << "OK: Ort::Env created" << std::endl;
        std::cout.flush();

        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_options.SetInterOpNumThreads(1);
        session_options.DisableCpuMemArena();

        auto wpath = utf8_to_wide(modelPath);
        Ort::Session session(env, wpath.c_str(), session_options);

        std::cout << "OK: Ort::Session created successfully" << std::endl;
        std::cout.flush();

        size_t numInputs = session.GetInputCount();
        size_t numOutputs = session.GetOutputCount();
        std::cout << "Inputs: " << numInputs << ", Outputs: " << numOutputs << std::endl;
        std::cout.flush();

        std::cout << "=== ONNX Model Load Test PASSED ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: Exception loading ONNX model: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "FAIL: Unknown exception loading ONNX model" << std::endl;
        return 1;
    }
}
