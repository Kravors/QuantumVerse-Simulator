#include "ml/GeodesicNeuralODE.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <stdexcept>
#include <cstring>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef HAVE_ONNX
#ifndef ORT_API_VERSION
#define ORT_API_VERSION 22
#endif
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>
#endif

namespace quantumverse {
namespace ml {

namespace {
struct NormStats {
    std::vector<float> input_mean;
    std::vector<float> input_std;
    std::vector<float> output_mean;
    std::vector<float> output_std;
};

inline std::string read_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.good()) return {};
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

inline std::vector<float> parse_float_array(const std::string& json,
                                            const std::string& key) {
    size_t pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return {};
    pos = json.find('[', pos);
    if (pos == std::string::npos) return {};
    size_t end = json.find(']', pos);
    if (end == std::string::npos) return {};
    std::stringstream ss(json.substr(pos + 1, end - pos - 1));
    std::vector<float> vals;
    std::string token;
    while (std::getline(ss, token, ',')) {
        auto b = token.find_first_not_of(" \t\n\r");
        auto e = token.find_last_not_of(" \t\n\r");
        if (b == std::string::npos) continue;
        vals.push_back(std::stof(token.substr(b, e - b + 1)));
    }
    return vals;
}

inline NormStats LoadNormalization(const std::string& path) {
    NormStats s;
    std::string json = read_file(path);
    if (json.empty()) return s;
    s.input_mean  = parse_float_array(json, "mean");
    s.input_std   = parse_float_array(json, "std");
    s.output_mean = parse_float_array(json, "output_mean");
    s.output_std  = parse_float_array(json, "output_std");
    return s;
}

inline std::vector<float> NormalizeInput(const std::vector<float>& raw,
                                         const NormStats& ns) {
    std::vector<float> out(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        float mean = (i < ns.input_mean.size()) ? ns.input_mean[i] : 0.0f;
        float std  = (i < ns.input_std .size()) ? ns.input_std [i] : 1.0f;
        out[i] = (raw[i] - mean) / (std + 1e-8f);
    }
    return out;
}

inline std::vector<float> DenormalizeOutput(const std::vector<float>& norm,
                                            const NormStats& ns) {
    std::vector<float> out(norm.size());
    for (size_t i = 0; i < norm.size(); ++i) {
        float mean = (i < ns.output_mean.size()) ? ns.output_mean[i] : 0.0f;
        float std  = (i < ns.output_std .size()) ? ns.output_std [i] : 1.0f;
        out[i] = norm[i] * std + mean;
    }
    return out;
}

#ifdef _WIN32
inline std::wstring widen_path(const std::string& path) {
    int needed = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (needed <= 0) return L"";
    std::wstring wpath(needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wpath[0], needed);
    wpath.resize(needed - 1);
    return wpath;
}
#endif
}

struct GeodesicNeuralODE::Impl {
    bool loaded_ = false;
    bool use_onnx_ = false;
    NormStats norm_;
    std::string model_path_;

#ifdef HAVE_ONNX
    std::unique_ptr<Ort::Env> env_;
    std::unique_ptr<Ort::Session> session_;
    std::vector<std::string> input_names_;
    std::vector<std::string> output_names_;
    std::vector<ONNXTensorElementDataType> input_types_;
    std::vector<ONNXTensorElementDataType> output_types_;
    std::vector<int64_t> input_dims_;
    std::vector<int64_t> output_dims_;
#endif

    std::array<double,4> (*cpp_predict_fn_)(const double*) = nullptr;

    [[nodiscard]] bool tryLoadONNX(const std::string& path) {
#ifdef HAVE_ONNX
        try {
            env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "GeodesicNeuralODE");
            Ort::SessionOptions opts;
            opts.SetIntraOpNumThreads(1);
            opts.SetInterOpNumThreads(1);
            opts.DisableCpuMemArena();

            session_ = std::unique_ptr<Ort::Session>(
                new Ort::Session(*env_, widen_path(path).c_str(), opts)
            );

            size_t num_inputs = session_->GetInputCount();
            size_t num_outputs = session_->GetOutputCount();

            input_names_ = session_->GetInputNames();
            output_names_ = session_->GetOutputNames();
            input_types_.reserve(num_inputs);
            output_types_.reserve(num_outputs);
            input_dims_.reserve(num_inputs);
            output_dims_.reserve(num_outputs);

            for (size_t i = 0; i < num_inputs; ++i) {
                auto type_info = session_->GetInputTypeInfo(i);
                auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
                input_types_.push_back(tensor_info.GetElementType());
                input_dims_ = tensor_info.GetShape();
            }

            for (size_t i = 0; i < num_outputs; ++i) {
                auto type_info = session_->GetOutputTypeInfo(i);
                auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
                output_types_.push_back(tensor_info.GetElementType());
                output_dims_ = tensor_info.GetShape();
            }

            use_onnx_ = true;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "[GeodesicNeuralODE] ONNX load failed: " << e.what() << "\n";
            use_onnx_ = false;
            return false;
        }
#else
        (void)path;
        return false;
#endif
    }
};

GeodesicNeuralODE::GeodesicNeuralODE(int)
    : pImpl_(std::make_unique<Impl>()) {}

GeodesicNeuralODE::~GeodesicNeuralODE() = default;

bool GeodesicNeuralODE::loadONNXModel(const std::string& onnx_path,
                                      bool force_python_fallback) {
    auto* base = pImpl_.get();
    base->model_path_ = onnx_path;
    std::string norm_path = onnx_path;
    auto pos = norm_path.find_last_of("/\\");
    if (pos != std::string::npos) {
        norm_path.replace(pos + 1, std::string::npos, "normalization.json");
    } else {
        norm_path = "models/geodesic_ode/normalization.json";
    }
    base->norm_ = LoadNormalization(norm_path);

    if (!force_python_fallback && base->tryLoadONNX(onnx_path)) {
        base->loaded_ = true;
        return true;
    }

    base->loaded_ = true;
    return true;
}

bool GeodesicNeuralODE::isLoaded() const noexcept {
    return pImpl_->loaded_;
}

std::vector<Event4D> GeodesicNeuralODE::predict(
        const Event4D& initial_event,
        const std::array<double,4>& initial_velocity,
        const std::vector<double>& metric_params,
        double tau) const {
    if (!isLoaded()) {
        return {Event4D()};
    }

    const auto& base = *pImpl_;

#ifdef HAVE_ONNX
    if (base.use_onnx_ && base.session_) {
        try {
            std::vector<float> input_data;
            input_data.reserve(8 + metric_params.size());
            input_data.push_back(static_cast<float>(initial_event.t));
            input_data.push_back(static_cast<float>(initial_event.x));
            input_data.push_back(static_cast<float>(initial_event.y));
            input_data.push_back(static_cast<float>(initial_event.z));
            input_data.push_back(static_cast<float>(initial_velocity[0]));
            input_data.push_back(static_cast<float>(initial_velocity[1]));
            input_data.push_back(static_cast<float>(initial_velocity[2]));
            input_data.push_back(static_cast<float>(initial_velocity[3]));
            for (double p : metric_params) {
                input_data.push_back(static_cast<float>(p));
            }

            auto normed = NormalizeInput(input_data, base.norm_);

            Ort::MemoryInfo mem_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
            Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
                mem_info,
                normed.data(),
                normed.size(),
                base.input_dims_.data(),
                base.input_dims_.size()
            );

            std::vector<const char*> input_names_ptrs;
            input_names_ptrs.reserve(base.input_names_.size());
            for (const auto& name : base.input_names_) {
                input_names_ptrs.push_back(name.c_str());
            }

            std::vector<const char*> output_names_ptrs;
            output_names_ptrs.reserve(base.output_names_.size());
            for (const auto& name : base.output_names_) {
                output_names_ptrs.push_back(name.c_str());
            }

            std::vector<Ort::Value> outputs = base.session_->Run(
                Ort::RunOptions{},
                input_names_ptrs.data(),
                &input_tensor,
                1,
                output_names_ptrs.data(),
                output_names_ptrs.size()
            );

            if (!outputs.empty()) {
                auto* out_ptr = outputs[0].GetTensorMutableData<float>();
                size_t out_size = outputs[0].GetTensorTypeAndShapeInfo().GetElementCount();

                auto denormed = DenormalizeOutput(
                    std::vector<float>(out_ptr, out_ptr + out_size),
                    base.norm_
                );

                double dt = static_cast<double>(denormed[0]);
                double dx = static_cast<double>(denormed[1]);
                double dy = static_cast<double>(denormed[2]);
                double dz = static_cast<double>(denormed[3]);

                return {Event4D(
                    initial_event.t + dt,
                    initial_event.x + dx,
                    initial_event.y + dy,
                    initial_event.z + dz
                )};
            }
        } catch (const Ort::Exception& e) {
            std::cerr << "[GeodesicNeuralODE] ONNX inference failed: " << e.what() << "\n";
        }
    }
#endif

    Event4D final(
        initial_event.t + initial_velocity[0] * tau,
        initial_event.x + initial_velocity[1] * tau,
        initial_event.y + initial_velocity[2] * tau,
        initial_event.z + initial_velocity[3] * tau
    );
    return {final};
}

std::vector<std::vector<Event4D>> GeodesicNeuralODE::predictBundle(
        const std::vector<Event4D>& initial_events,
        const std::vector<std::array<double,4>>& initial_velocities,
        const std::vector<double>& metric_params,
        double tau,
        utils::ThreadPool& pool) const {
    const size_t N = initial_events.size();
    std::vector<std::vector<Event4D>> results(N);

    auto predictOne = [&](size_t i) {
        if (i < N) {
            results[i] = predict(initial_events[i], initial_velocities[i], metric_params, tau);
        }
    };

    pool.parallel_for(0, N, predictOne);
    return results;
}

} // namespace ml
} // namespace quantumverse
