#include "ml/SurrogateIntegration.h"
#include "ml/GeodesicNeuralODE.h"
#include "ml/MetricGNN.h"
#include "ml/BVHAcceleratedMetricQuery.h"
#include "spacetime/Event4D.h"
#include "geometry/BVH.h"

namespace quantumverse {
namespace ml {

using MetricTensorField = std::vector<std::vector<std::vector<double>>>;

struct SurrogateIntegration::Impl {
    SurrogateConfig cfg_;
    std::unique_ptr<GeodesicNeuralODE> geodesic_ode_;
    std::unique_ptr<MetricGNN> metric_gnn_;
    std::unique_ptr<BVHAcceleratedMetricQuery> bvhQuery_;
    mutable geometry::BoundingVolumeHierarchy<3> bvh_;
    bool geodesic_loaded_ = false;
    bool metric_loaded_ = false;
    mutable bool bvh_built_ = false;
};

SurrogateIntegration::SurrogateIntegration(SurrogateConfig cfg)
    : pImpl_(std::make_unique<Impl>()) {
    pImpl_->cfg_ = cfg;
    pImpl_->bvhQuery_ = std::make_unique<BVHAcceleratedMetricQuery>();
}

SurrogateIntegration::~SurrogateIntegration() = default;

bool SurrogateIntegration::loadSurrogates(const std::string& model_dir) {
    auto* base = pImpl_.get();
    if (base->cfg_.enable_geodesic_neural_ode) {
        base->geodesic_ode_ = std::make_unique<GeodesicNeuralODE>(1);
        std::string path = model_dir + "/geodesic_ode/geodesic_ode.onnx";
        base->geodesic_loaded_ = base->geodesic_ode_->loadONNXModel(path, true);
    }
    if (base->cfg_.enable_metric_gnn) {
        base->metric_gnn_ = std::make_unique<MetricGNN>();
        std::string path = model_dir + "/metric_gnn/metric_gnn.onnx";
        base->metric_loaded_ = base->metric_gnn_->loadONNXModel(path, true);
        pImpl_->bvhQuery_->setMetricGNN(base->metric_gnn_.get());
    }
    return base->geodesic_loaded_ || base->metric_loaded_;
}

bool SurrogateIntegration::predictGeodesicIfReady(
        const Event4D& initial_event,
        const std::array<double,4>& initial_velocity,
        const std::vector<double>& metric_params,
        double tau,
        Event4D& out_event) const {
    const auto* base = pImpl_.get();
    if (!base->geodesic_loaded_ || !base->geodesic_ode_) return false;
    auto out = base->geodesic_ode_->predict(initial_event, initial_velocity, metric_params, tau);
    if (out.empty()) return false;
    out_event = out.front();
    return true;
}

bool SurrogateIntegration::predictMetricIfReady(
        const std::vector<std::array<double,3>>& mass_positions,
        const std::vector<double>& mass_values,
        const std::vector<std::array<double,3>>& query_points,
        MetricTensorField& out_metric) const {
    const auto* base = pImpl_.get();
    if (!base->metric_loaded_ || !base->metric_gnn_) return false;

    // Build BVH index from query points for fast lookups
    if (!base->bvh_built_) {
        base->bvh_.clear();
        for (const auto& p : query_points) {
            std::array<double, 3> point = {p[0], p[1], p[2]};
            base->bvh_.insert(point);
        }
        base->bvh_.rebuild();
        base->bvh_built_ = true;
    }

    out_metric = base->metric_gnn_->predict(mass_positions, mass_values, query_points);
    return !out_metric.empty();
}

bool SurrogateIntegration::isGeodesicSurrogateReady() const noexcept {
    return pImpl_->geodesic_loaded_;
}

bool SurrogateIntegration::isMetricSurrogateReady() const noexcept {
    return pImpl_->metric_loaded_;
}

// Parallel geodesic bundle integration
std::vector<std::vector<Event4D>> SurrogateIntegration::predictGeodesicBundleIfReady(
        const std::vector<Event4D>& initial_events,
        const std::vector<std::array<double,4>>& initial_velocities,
        const std::vector<double>& metric_params,
        double tau,
        utils::ThreadPool& pool) const {
    const auto* base = pImpl_.get();
    if (!base->geodesic_loaded_ || !base->geodesic_ode_) {
        return {};
    }
    return base->geodesic_ode_->predictBundle(initial_events, initial_velocities, metric_params, tau, pool);
}

} // namespace ml
} // namespace quantumverse
