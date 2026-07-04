#pragma once
#include <memory>
#include <string>
#include <vector>
#include <array>

namespace quantumverse {
namespace spacetime {
struct Event4D;
} // namespace spacetime

namespace utils {
class ThreadPool;
} // namespace utils

namespace ml {

using MetricTensorField = std::vector<std::vector<std::vector<double>>>;

struct SurrogateConfig {
    bool enable_geodesic_neural_ode = false;
    bool enable_metric_gnn = false;
    double max_relative_error = 0.01;
    double fallback_timeout_ms = 1.0;
};

class SurrogateIntegration {
public:
    explicit SurrogateIntegration(SurrogateConfig cfg);
    ~SurrogateIntegration();

    bool loadSurrogates(const std::string& model_dir);

    bool predictGeodesicIfReady(
        const spacetime::Event4D& initial_event,
        const std::array<double,4>& initial_velocity,
        const std::vector<double>& metric_params,
        double tau,
        spacetime::Event4D& out_event) const;

    // Parallel geodesic bundle integration
    std::vector<std::vector<spacetime::Event4D>> predictGeodesicBundleIfReady(
        const std::vector<spacetime::Event4D>& initial_events,
        const std::vector<std::array<double,4>>& initial_velocities,
        const std::vector<double>& metric_params,
        double tau,
        utils::ThreadPool& pool) const;

    bool predictMetricIfReady(
        const std::vector<std::array<double,3>>& mass_positions,
        const std::vector<double>& mass_values,
        const std::vector<std::array<double,3>>& query_points,
        MetricTensorField& out_metric) const;

    bool isGeodesicSurrogateReady() const noexcept;
    bool isMetricSurrogateReady() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace ml
} // namespace quantumverse
