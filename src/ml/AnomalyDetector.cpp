/**
 * @file AnomalyDetector.cpp
 * @brief Lightweight autoencoder anomaly detector implementation
 */

#include "ml/AnomalyDetector.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace quantumverse {

AnomalyDetector::AnomalyDetector() = default;

bool AnomalyDetector::load(const std::string& path) {
    try {
        std::ifstream f(path);
        if (!f.is_open()) return false;
        nlohmann::json j;
        f >> j;
        return loadFromString(j.dump());
    } catch (const std::exception& e) {
        std::cerr << "AnomalyDetector::load exception: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "AnomalyDetector::load unknown exception" << std::endl;
        return false;
    }
}

bool AnomalyDetector::loadFromString(const std::string& json_str) {
    try {
        nlohmann::json j = nlohmann::json::parse(json_str);

        metadata_.feature_dim = j.value("feature_dim", 20);
        metadata_.threshold = j.value("threshold", 0.05);
        metadata_.mean = j.value("mean", std::vector<double>());
        metadata_.std = j.value("std", std::vector<double>());

        if (!metadata_.mean.empty() && metadata_.mean.size() != metadata_.feature_dim) {
            throw std::invalid_argument("Normalization mean size mismatch");
        }
        if (!metadata_.std.empty() && metadata_.std.size() != metadata_.feature_dim) {
            throw std::invalid_argument("Normalization std size mismatch");
        }

        auto loadLayers = [](const nlohmann::json& arr, std::vector<Layer>& out) {
            out.clear();
            for (const auto& elem : arr) {
                Layer l;
                l.in = elem.value("in", 0);
                l.out = elem.value("out", 0);
                l.weights = elem.value("weights", std::vector<std::vector<double>>());
                l.bias = elem.value("bias", std::vector<double>());
                if (static_cast<int>(l.weights.size()) != l.out) {
                    throw std::invalid_argument("Layer weight rows != out");
                }
                for (const auto& row : l.weights) {
                    if (static_cast<int>(row.size()) != l.in) {
                        throw std::invalid_argument("Layer weight cols != in");
                    }
                }
                if (static_cast<int>(l.bias.size()) != l.out) {
                    throw std::invalid_argument("Layer bias size != out");
                }
                out.push_back(std::move(l));
            }
        };

        loadLayers(j.value("encoder", nlohmann::json::array()), metadata_.encoder);
        loadLayers(j.value("decoder", nlohmann::json::array()), metadata_.decoder);

        if (metadata_.encoder.empty() || metadata_.decoder.empty()) {
            return false;
        }

        loaded_ = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "AnomalyDetector::loadFromString exception: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "AnomalyDetector::loadFromString unknown exception" << std::endl;
        return false;
    }
}

std::vector<double> AnomalyDetector::forward(
    const std::vector<double>& input,
    const Layer& layer,
    bool relu
) {
    std::vector<double> out(layer.out, 0.0);
    for (int i = 0; i < layer.out; ++i) {
        double sum = layer.bias[i];
        for (int j = 0; j < layer.in; ++j) {
            sum += layer.weights[i][j] * input[j];
        }
        if (relu) {
            sum = std::max(0.0, sum);
        }
        out[i] = sum;
    }
    return out;
}

double AnomalyDetector::mse(const std::vector<double>& a, const std::vector<double>& b) {
    if (a.size() != b.size()) {
        throw std::invalid_argument("MSE vector size mismatch");
    }
    double sum = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        double d = a[i] - b[i];
        sum += d * d;
    }
    return sum / static_cast<double>(a.size());
}

std::vector<double> AnomalyDetector::normalize(const std::vector<double>& raw, const std::vector<double>& mean, const std::vector<double>& std) {
    if (raw.size() != mean.size() || raw.size() != std.size()) {
        throw std::invalid_argument("Normalization size mismatch");
    }
    std::vector<double> out(raw.size());
    for (std::size_t i = 0; i < raw.size(); ++i) {
        out[i] = (raw[i] - mean[i]) / std[i];
    }
    return out;
}

double AnomalyDetector::score(const FeatureVector& features) const {
    if (!loaded_) {
        throw std::logic_error("AnomalyDetector: model not loaded");
    }
    if (static_cast<int>(features.toVector().size()) != metadata_.feature_dim) {
        throw std::invalid_argument("FeatureVector dimension mismatch");
    }

    auto raw = features.toVector();
    auto x = metadata_.mean.empty() ? raw : normalize(raw, metadata_.mean, metadata_.std);

    // Encoder: 20 -> 10 (ReLU) -> 2 (linear)
    auto h1 = forward(x, metadata_.encoder[0], true);
    auto z  = forward(h1, metadata_.encoder[1], false);

    // Decoder: 2 -> 10 (ReLU) -> 20 (linear)
    auto h2 = forward(z, metadata_.decoder[0], true);
    auto xr = forward(h2, metadata_.decoder[1], false);

    return mse(x, xr);
}

bool AnomalyDetector::isAnomalous(const FeatureVector& features) const {
    if (!loaded_) return false;
    return score(features) > metadata_.threshold;
}

} // namespace quantumverse
