#include "ml/BVHAcceleratedMetricQuery.h"
#include "ml/MetricGNN.h"
#include <algorithm>

namespace quantumverse {
namespace ml {

void BVHAcceleratedMetricQuery::buildIndex(const std::vector<std::array<double,3>>& query_points) {
    for (const auto& p : query_points) {
        bvh_.insert(p);
    }
    bvh_.rebuild();
}

std::vector<std::vector<std::vector<double>>> BVHAcceleratedMetricQuery::queryMetrics(
    const std::vector<std::array<double,3>>& mass_positions,
    const std::vector<double>& mass_values,
    const std::vector<std::array<double,3>>& query_points) const {
    
    if (!metric_gnn_) {
        return {};
    }
    
    // For now, just call the GNN directly
    // In production, use BVH to cull queries to relevant regions
    auto full_result = metric_gnn_->predict(mass_positions, mass_values, query_points);
    
    return full_result;
}

} // namespace ml
} // namespace quantumverse