#ifndef QUANTUMVERSE_GEOMETRY_BVH_H
#define QUANTUMVERSE_GEOMETRY_BVH_H

#include <array>
#include <vector>

namespace geometry {

template <std::size_t N>
class BoundingVolumeHierarchy {
   public:
    BoundingVolumeHierarchy() = default;
    ~BoundingVolumeHierarchy() = default;

    void insert(const std::array<double, N>& point) {}
    void rebuild() {}
    void clear() {}

    template <typename QueryType>
    std::vector<std::vector<std::vector<double>>> query(const QueryType& query_points) const {
        return {};
    }
};

}  // namespace geometry

#endif  // QUANTUMVERSE_GEOMETRY_BVH_H