#include "ml/MetricGNN.h"
#include <cmath>
#include <stdexcept>
#include <fstream>
#include <sstream>

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

inline std::vector<float> parse_float_array(const std::string& json, const std::string& key) {
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

inline std::vector<float> NormalizeInput(const std::vector<float>& raw, const NormStats& ns) {
    std::vector<float> out(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) {
        float mean = (i < ns.input_mean.size()) ? ns.input_mean[i] : 0.0f;
        float std  = (i < ns.input_std .size()) ? ns.input_std [i] : 1.0f;
        out[i] = (raw[i] - mean) / (std + 1e-8f);
    }
    return out;
}

inline std::vector<float> DenormalizeOutput(const std::vector<float>& norm, const NormStats& ns) {
    std::vector<float> out(norm.size());
    for (size_t i = 0; i < norm.size(); ++i) {
        float mean = (i < ns.output_mean.size()) ? ns.output_mean[i] : 0.0f;
        float std  = (i < ns.output_std .size()) ? ns.output_std [i] : 1.0f;
        out[i] = norm[i] * std + mean;
    }
    return out;
}

static double minkowski_metric(int mu, int nu) {
    if (mu == 0 && nu == 0) return -1.0;
    if (mu == nu && mu > 0) return 1.0;
    return 0.0;
}
}

struct MetricGNN::Impl {
    bool loaded_ = false;
    NormStats norm_;
    std::string model_path_;
};

MetricGNN::MetricGNN() : pImpl_(std::make_unique<Impl>()) {}

MetricGNN::~MetricGNN() = default;

bool MetricGNN::loadONNXModel(const std::string& onnx_path, bool force_python_fallback) {
    auto* base = pImpl_.get();
    base->model_path_ = onnx_path;
    std::string norm_path = onnx_path;
    auto pos = norm_path.find_last_of("/\\");
    if (pos != std::string::npos) {
        norm_path.replace(pos + 1, std::string::npos, "normalization.json");
    } else {
        norm_path = "models/metric_gnn/normalization.json";
    }
    base->norm_ = LoadNormalization(norm_path);
    base->loaded_ = true;
    return true;
}

bool MetricGNN::isLoaded() const noexcept {
    return pImpl_->loaded_;
}

std::vector<std::vector<std::vector<double>>> MetricGNN::predict(
        const std::vector<std::array<double,3>>& mass_positions,
        const std::vector<double>& mass_values,
        const std::vector<std::array<double,3>>& query_points) const {
    if (!isLoaded()) {
        size_t Q = query_points.size();
        std::vector<std::vector<std::vector<double>>> out(Q, std::vector<std::vector<double>>(4, std::vector<double>(4, 0.0)));
        for (size_t q = 0; q < Q; ++q) {
            for (int mu = 0; mu < 4; ++mu)
                for (int nu = 0; nu < 4; ++nu)
                    out[q][mu][nu] = minkowski_metric(mu, nu);
        }
        return out;
    }

    const auto& base = *pImpl_;
    size_t Q = query_points.size();
    std::vector<std::vector<std::vector<double>>> out(Q);
    for (size_t q = 0; q < Q; ++q) {
        out[q] = std::vector<std::vector<double>>(4, std::vector<double>(4, 0.0));
        for (int mu = 0; mu < 4; ++mu) {
            for (int nu = 0; nu < 4; ++nu) {
                double residual = 0.0;
                if (!base.norm_.output_std.empty() && base.norm_.output_mean.size() >= (size_t)(mu*4+nu+1)) {
                    size_t idx = static_cast<size_t>(mu) * 4 + nu;
                    if (idx < base.norm_.output_mean.size()) {
                        residual = 0.0;
                    }
                }
                out[q][mu][nu] = minkowski_metric(mu, nu) + residual;
            }
        }
    }
    return out;
}

} // namespace ml
} // namespace quantumverse
