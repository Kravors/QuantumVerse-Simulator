/**
 * @file LightCone.h
 * @brief Light cone visualization for 4D spacetime
 *
 * Generates null geodesic meshes representing future/past light cones
 * from any spacetime event. Used for causal structure visualization.
 *
 * References:
 * - Plan 1 Phase 1.9: Light cone visualisation + causal graph panel
 * - Rovelli (2004) "Quantum Gravity" - causal structure concepts
 */

#ifndef LIGHTCONE_H
#define LIGHTCONE_H

// Ensure M_PI is available on all platforms (MSVC <cmath> may not define it)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <vector>
#include <array>
#include <memory>
#include <cmath>

#include "../spacetime/Event4D.h"
#include "../spacetime/MetricTensor.h"

namespace quantumverse {

/**
 * @brief Light cone type enumeration
 */
enum class LightConeType {
    FUTURE,   ///< Future light cone (t > t0)
    PAST,     ///< Past light cone (t < t0)
    BOTH      ///< Both future and past
};

/**
 * @brief Vertex data for light cone mesh rendering
 */
struct LightConeVertex {
    float position[3];      ///< 3D projected position
    float color[4];         ///< RGBA color
    float intensity;        ///< Brightness factor
};

/**
 * @brief LightCone generates and manages light cone geometry
 *
 * For a given event in spacetime, generates the null surface
 * by integrating null geodesics in all directions. In flat
 * spacetime, this is simply a cone; in curved spacetime,
 * the cone deforms according to the metric.
 */
class LightCone {
public:
    /**
     * @brief Construct a light cone at the given event
     * @param origin Spacetime origin event
     * @param type Future, past, or both cones
     * @param maxAngle Maximum opening angle (radians, default pi/4)
     * @param resolution Angular resolution (number of radial divisions)
     */
    LightCone(
        const Event4D& origin,
        LightConeType type = LightConeType::FUTURE,
        double maxAngle = M_PI / 4.0,
        int resolution = 32
    );

    ~LightCone() = default;

    /**
     * @brief Set the metric tensor for curved spacetime propagation
     */
    void setMetric(std::shared_ptr<MetricTensor> metric);

    /**
     * @brief Set the origin event for the light cone
     */
    void setOrigin(const Event4D& origin);

    /**
     * @brief Set the cone type (future/past/both)
     */
    void setType(LightConeType type);

    /**
     * @brief Set the maximum opening angle
     * @param angle Maximum angle from the light cone axis (radians)
     */
    void setMaxAngle(double angle);

    /**
     * @brief Set angular resolution for mesh generation
     * @param res Number of radial divisions (higher = smoother)
     */
    void setResolution(int res);

    /**
     * @brief Regenerate the light cone mesh
     *
     * Integrates null geodesics from the origin event outward.
     * In flat spacetime, uses analytic cone formula.
     * In curved spacetime, uses numerical null geodesic integration.
     */
    void generateMesh();

    /**
     * @brief Get the generated vertex data for rendering
     */
    const std::vector<LightConeVertex>& getVertices() const { return vertices; }

    /**
     * @brief Get the index data for triangle rendering
     */
    const std::vector<unsigned int>& getIndices() const { return indices; }

    /**
     * @brief Get the origin event
     */
    const Event4D& getOrigin() const { return origin; }

    /**
     * @brief Get the maximum extent of the cone in spatial units
     */
    double getMaxExtent() const { return maxExtent; }

    /**
     * @brief Get number of vertices
     */
    size_t getVertexCount() const { return vertices.size(); }

    /**
     * @brief Get number of triangle indices
     */
    size_t getIndexCount() const { return indices.size(); }

    /**
     * @brief Set RGBA color for the light cone
     */
    void setColor(float r, float g, float b, float a = 0.3f);

    /**
     * @brief Set whether to show the solid surface or wireframe
     */
    void setWireframe(bool wf) { wireframe = wf; }

    /**
     * @brief Check if this light cone is valid (has geometry)
     */
    bool isValid() const { return !vertices.empty(); }

public:
     /**
      * @brief Integrate a single null geodesic ray
      *
      * @param direction Unit 3-vector direction of the ray
      * @param maxAffineParam Maximum affine parameter to integrate
      * @return Vector of events along the null ray
      */
     std::vector<Event4D> integrateNullRay(
         const std::array<double, 3>& direction,
         double maxAffineParam
     ) const;

private:
     /**
      * @brief Generate flat spacetime light cone (analytic)
     *
     * For Minkowski metric, the light cone is a perfect cone:
     *   |x - x0| = c * |t - t0|
     */
    void generateFlatCone();

    /**
     * @brief Generate curved spacetime light cone via null geodesic integration
     *
     * Shoots null rays in all directions and integrates using
     * the geodesic equation with the current metric.
     */
    void generateCurvedCone();

    /**
      * @brief Build triangle index buffer from vertex grid
     *
     * @param nTheta Number of theta divisions
     * @param nPhi Number of phi divisions
     */
    void buildIndices(int nTheta, int nPhi);

    // Configuration
    Event4D origin;                    ///< Spacetime origin of the cone
    LightConeType type;                ///< Future / past / both
    double maxAngle;                   ///< Maximum opening angle (radians)
    int resolution;                    ///< Angular resolution
    std::shared_ptr<MetricTensor> metric;  ///< Metric for curved propagation

    // Generated geometry
    std::vector<LightConeVertex> vertices;  ///< Vertex buffer
    std::vector<unsigned int> indices;      ///< Index buffer
    double maxExtent;                       ///< Max spatial extent of cone

    // Rendering options
    float colorRGBA[4];  ///< RGBA color
    bool wireframe = false;

    // Constants
    static const int MAX_RAYS = 512;     ///< Maximum null rays to shoot
    static const double AFFINE_STEP;     ///< Integration step size
    static const double AFFINE_MAX;      ///< Max affine parameter
};

} // namespace quantumverse

#endif // LIGHTCONE_H