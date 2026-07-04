#pragma once

#include "geometry/BVH.h"
#include <memory>
#include <vector>

namespace quantumverse {
namespace ml {

class MetricGNN;

class BVHAcceleratedMetricQuery {
public:
    void buildIndex(const std::vector<std::array<double,3>>& query_points);
    
    std::vector<std::vector<std::vector<double>>> queryMetrics(
        const std::vector<std::array<double,3>>& mass_positions,
        const std::vector<double>& mass_values,
        const std::vector<std::array<double,3>>& query_points) const;
    
    void setMetricGNN(MetricGNN* gnn) { metric_gnn_ = gnn; }
    
private:
    geometry::BoundingVolumeHierarchy<3> bvh_;
    MetricGNN* metric_gnn_ = nullptr;
};

} // namespace ml
} // namespace quantumverse