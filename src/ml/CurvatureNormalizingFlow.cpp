#include "CurvatureNormalizingFlow.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>

// ONNX Runtime 1.22.1 supports API versions 1 and 17, but headers define version 22.
// Use the wrapper header to ensure API version 17 compatibility.
#ifdef HAVE_ONNX
#include "onnx_wrapper.h"
#define ENABLE_ONNX 1
#else
#define ENABLE_ONNX 0
#endif

namespace quantumverse {

// ======================
// PIMPL Implementation
// ======================

struct CurvatureNormalizingFlow::Impl {
    // Normalization parameters (always available)
    std::vector<double> mean;
    std::vector<double> std;
    
    // Model metadata
    std::string model_path;
    bool model_loaded{false};
    int feature_dim{15};
    std::string model_type = "stub";

#ifdef HAVE_ONNX
    // ONNX session (only when HAVE_ONNX)
    Ort::Env env{nullptr};
    std::unique_ptr<Ort::Session, void(*)(Ort::Session*)> session{nullptr, [](Ort::Session* s) { delete s; }};
    Ort::SessionOptions session_options;
    Ort::MemoryInfo memory_info{nullptr};
    
    // Input/output info
    std::vector<int64_t> input_shape;
    std::vector<int64_t> output_shape;
    Ort::AllocatorWithDefaultOptions allocator;
#endif

    Impl() = default;
    
    ~Impl() = default;  // unique_ptr handles cleanup automatically
    
    /**
     * @brief Initialize ONNX Runtime environment
     */
    bool initONNX() {
 #ifndef HAVE_ONNX
         std::cerr << "[CurvatureFlow] ONNX Runtime not compiled in (HAVE_ONNX not defined)\n";
         return false;
 #else
         try {
             // Initialize Ort environment
             env = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "CurvatureNormalizingFlow");

             // Session options
             session_options.SetIntraOpNumThreads(1);
             session_options.SetGraphOptimizationLevel(ORT_ENABLE_ALL);
              // Let ONNX Runtime auto-select the best available execution provider

             // Memory info (CPU)
             memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

             return true;
         } catch (const std::exception& e) {
             std::cerr << "[CurvatureFlow] ONNX Runtime init failed: " << e.what() << "\n";
             return false;
         }
 #endif
     }
    
    /**
      * @brief Load ONNX model from file
      */
    bool loadModel(const std::string& path) {
#ifndef HAVE_ONNX
        (void)path;  // unused
        std::cerr << "[CurvatureFlow] ONNX not available, using stub\n";
        return false;
#else
        try {
            // Initialize if not done
            if (!env) {
                if (!initONNX()) return false;
            }
            
            // Create session (ONNX Runtime 1.22.1 requires wide string path on Windows)
             std::wstring wide_path(path.begin(), path.end());
             session.reset(new Ort::Session(env, wide_path.c_str(), session_options));
            
            // Get input/output info
            size_t num_inputs = session->GetInputCount();
            size_t num_outputs = session->GetOutputCount();
            
            if (num_inputs < 1 || num_outputs < 1) {
                std::cerr << "[CurvatureFlow] Invalid model: expected at least 1 input and 1 output\n";
                session.reset();
                return false;
            }
            
            // Input shape
            auto input_name = session->GetInputNameAllocated(0, allocator);
            auto input_info = session->GetInputTypeInfo(0);
            auto input_tensor_info = input_info.GetTensorTypeAndShapeInfo();
            input_shape = input_tensor_info.GetShape();
            
            // Output shape
            auto output_name = session->GetOutputNameAllocated(0, allocator);
            auto output_info = session->GetOutputTypeInfo(0);
            auto output_tensor_info = output_info.GetTensorTypeAndShapeInfo();
            output_shape = output_tensor_info.GetShape();
            
            std::cout << "[CurvatureFlow] Model loaded: " << path << "\n";
            std::cout << "  Input: " << input_name.get() << " shape=[";
            for (size_t i = 0; i < input_shape.size(); ++i) {
                if (i > 0) std::cout << ",";
                std::cout << input_shape[i];
            }
            std::cout << "]\n";
            std::cout << "  Output: " << output_name.get() << " shape=[";
            for (size_t i = 0; i < output_shape.size(); ++i) {
                if (i > 0) std::cout << ",";
                std::cout << output_shape[i];
            }
            std::cout << "]\n";
            
            model_loaded = true;
            model_path = path;
            feature_dim = static_cast<int>(input_shape[1]);
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "[CurvatureFlow] Model load failed: " << e.what() << "\n";
            session.reset();
            model_loaded = false;
            return false;
        }
#endif
    }
    
    /**
     * @brief Run inference on normalized features
     */
    double predict(const std::vector<double>& normalized_features) {
#ifndef HAVE_ONNX
        (void)normalized_features;  // unused
        // Stub fallback: return 0.0 (normal)
        return 0.0;
#else
        if (!model_loaded || !session) {
            // Stub fallback: return 0.0 (normal)
            return 0.0;
        }
        
        try {
            // Prepare input tensor
            std::vector<float> input_tensor_values(normalized_features.begin(), normalized_features.end());
            auto input_tensor = Ort::Value::CreateTensor<float>(
                memory_info,
                input_tensor_values.data(),
                input_tensor_values.size(),
                input_shape.data(),
                input_shape.size()
            );
            
            // Prepare output tensor
            std::vector<float> output_tensor_values(output_shape[0] * output_shape[1]);
            auto output_tensor = Ort::Value::CreateTensor<float>(
                memory_info,
                output_tensor_values.data(),
                output_tensor_values.size(),
                output_shape.data(),
                output_shape.size()
            );
            
            // Input/output names
            const char* input_names[] = {"input"};
            const char* output_names[] = {"log_prob"};
            
            // Run inference
            session->Run(
                Ort::RunOptions{nullptr},
                input_names,
                &input_tensor,
                1,
                output_names,
                &output_tensor,
                1
            );
            
            // Extract result: log_prob (negative log-likelihood)
            // Anomaly score = -log_prob (higher = more anomalous)
            float log_prob = output_tensor_values[0];
            double anomaly_score = -static_cast<double>(log_prob);
            
            return anomaly_score;
            
        } catch (const std::exception& e) {
            std::cerr << "[CurvatureFlow] Inference failed: " << e.what() << "\n";
            return 0.0;  // Fallback to normal
        }
#endif
    }
};

// ======================
// Public Interface
// ======================

CurvatureNormalizingFlow::CurvatureNormalizingFlow()
    : pimpl_(std::make_unique<Impl>()) {
    // Initialize metadata
    metadata_.feature_dim = 15;  // Default
    metadata_.model_type = "stub";
    metadata_.n_layers = 0;
    metadata_.hidden_dim = 0;
    metadata_.is_loaded = false;
    
    // Note: ONNX initialization is deferred to loadONNXModel() to avoid potential hangs
    // on Windows during application startup. The model will be loaded on-demand.
}

CurvatureNormalizingFlow::~CurvatureNormalizingFlow() = default;

CurvatureNormalizingFlow::CurvatureNormalizingFlow(CurvatureNormalizingFlow&&) noexcept = default;
CurvatureNormalizingFlow& CurvatureNormalizingFlow::operator=(CurvatureNormalizingFlow&&) noexcept = default;

bool CurvatureNormalizingFlow::loadONNXModel(const std::string& model_path) {
    if (!pimpl_) return false;
    
    bool loaded = pimpl_->loadModel(model_path);
    if (loaded) {
        metadata_.is_loaded = true;
        metadata_.model_type = "realnvp";  // or read from separate metadata file
        // Feature dim from input shape
        if (pimpl_->feature_dim > 0) {
            metadata_.feature_dim = pimpl_->feature_dim;
        }
    } else {
        metadata_.is_loaded = false;
    }
    
    return loaded;
}

double CurvatureNormalizingFlow::predict(const std::vector<double>& features) const {
    if (!pimpl_) return 0.0;
    
    // Validate size - return 0.0 instead of throwing to avoid crashes
    if (static_cast<int>(features.size()) != metadata_.feature_dim) {
        std::cerr << "[CurvatureFlow] Warning: expected " << metadata_.feature_dim 
                  << " features, got " << features.size() << " - returning 0.0\n";
        return 0.0;
    }
    
    // Normalize
    std::vector<double> normalized = normalize(features);
    
    // Call implementation
    return pimpl_->predict(normalized);
}

double CurvatureNormalizingFlow::predictRaw(const std::vector<double>& raw_features) const {
    // Same as predict (already normalizes internally)
    return predict(raw_features);
}

std::vector<double> CurvatureNormalizingFlow::normalize(const std::vector<double>& raw) const {
    if (pimpl_->mean.empty() || pimpl_->std.empty() || 
        pimpl_->mean.size() != raw.size() || pimpl_->std.size() != raw.size()) {
        // No normalization available, return as-is
        return raw;
    }
    
    std::vector<double> normalized(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        normalized[i] = (raw[i] - pimpl_->mean[i]) / pimpl_->std[i];
    }
    return normalized;
}

std::unique_ptr<CurvatureNormalizingFlow> CurvatureNormalizingFlow::createDefault() {
    return std::make_unique<CurvatureNormalizingFlow>();
}

} // namespace quantumverse
