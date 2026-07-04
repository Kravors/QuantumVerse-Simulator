#pragma once
#include <vector>
#include <string>
#include <array>
#include <memory>

#include "spacetime/Event4D.h"

namespace quantumverse {
namespace ml {

class MetricGNN {
public:
    MetricGNN();
    ~MetricGNN();

    bool loadONNXModel(const std::string& onnx_path, bool force_python_fallback = false);
    [[nodiscard]] bool isLoaded() const noexcept;

    std::vector<std::vector<std::vector<double>>> predict(
        const std::vector<std::array<double,3>>& mass_positions,
        const std::vector<double>& mass_values,
        const std::vector<std::array<double,3>>& query_points) const;

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace ml
} // namespace quantumverse
