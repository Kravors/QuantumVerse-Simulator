#include "ml/GeodesicNeuralODE.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <stdexcept>

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
}

struct GeodesicNeuralODE::Impl {
    bool loaded_ = false;
    NormStats norm_;
    std::string model_path_;
    std::array<double,4> (*cpp_predict_fn_)(const double*) = nullptr;

    [[nodiscard]] bool tryLoadONNX(const std::string&) {
        return false;
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

std::vector<spacetime::Event4D> GeodesicNeuralODE::predict(
        const spacetime::Event4D& initial_event,
        const std::array<double,4>& initial_velocity,
        const std::vector<double>& metric_params,
        double tau) const {
    if (!isLoaded()) {
        return {spacetime::Event4D()};
    }

    const auto& base = *pImpl_;
    spacetime::Event4D final(
        initial_event.t + initial_velocity[0] * tau,
        initial_event.x + initial_velocity[1] * tau,
        initial_event.y + initial_velocity[2] * tau,
        initial_event.z + initial_velocity[3] * tau
    );
    return {final};
}

std::vector<std::vector<spacetime::Event4D>> GeodesicNeuralODE::predictBundle(
        const std::vector<spacetime::Event4D>& initial_events,
        const std::vector<std::array<double,4>>& initial_velocities,
        const std::vector<double>& metric_params,
        double tau,
        utils::ThreadPool& pool) const {
    const size_t N = initial_events.size();
    std::vector<std::vector<spacetime::Event4D>> results(N);

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
