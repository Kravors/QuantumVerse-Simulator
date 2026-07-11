#pragma once
#include <vector>
#include <string>
#include <array>
#include <memory>
#include "spacetime/Event4D.h"
#include "utils/ThreadPool.h"

namespace quantumverse {
namespace ml {

class GeodesicNeuralODE {
public:
    explicit GeodesicNeuralODE(int num_metric_params = 1);
    ~GeodesicNeuralODE();

    bool loadONNXModel(const std::string& onnx_path,
                       bool force_python_fallback = false);
    [[nodiscard]] bool isLoaded() const noexcept;

    std::vector<Event4D> predict(
        const Event4D& initial_event,
        const std::array<double,4>& initial_velocity,
        const std::vector<double>& metric_params,
        double tau) const;

    // Parallel integration of multiple geodesics
    std::vector<std::vector<Event4D>> predictBundle(
        const std::vector<Event4D>& initial_events,
        const std::vector<std::array<double,4>>& initial_velocities,
        const std::vector<double>& metric_params,
        double tau,
        utils::ThreadPool& pool) const;

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace ml
} // namespace quantumverse
