/**
 * @file LightCone.cpp
 * @brief Implementation of LightCone null geodesic visualization
 */

#include "spacetime/LightCone.h"
#include "../physics/GeodesicIntegrator.h"
#include <cmath>
#include <algorithm>

namespace quantumverse {

// Static constants
const double LightCone::AFFINE_STEP = 0.1;
const double LightCone::AFFINE_MAX = 100.0;

LightCone::LightCone(
    const Event4D& origin_,
    LightConeType type_,
    double maxAngle_,
    int resolution_
) : origin(origin_),
    type(type_),
    maxAngle(maxAngle_),
    resolution(resolution_),
    maxExtent(0.0)
{
    colorRGBA[0] = 1.0f;
    colorRGBA[1] = 0.8f;
    colorRGBA[2] = 0.2f;
    colorRGBA[3] = 0.3f;
}

void LightCone::setMetric(std::shared_ptr<MetricTensor> m)
{
    metric = m;
}

void LightCone::setOrigin(const Event4D& o)
{
    origin = o;
    generateMesh();
}

void LightCone::setType(LightConeType t)
{
    type = t;
    generateMesh();
}

void LightCone::setMaxAngle(double angle)
{
    maxAngle = angle;
    generateMesh();
}

void LightCone::setResolution(int res)
{
    resolution = std::max(4, std::min(128, res));
    generateMesh();
}

void LightCone::setColor(float r, float g, float b, float a)
{
    colorRGBA[0] = r;
    colorRGBA[1] = g;
    colorRGBA[2] = b;
    colorRGBA[3] = a;
}

void LightCone::generateMesh()
{
    if (!metric) {
        generateFlatCone();
    } else {
        generateCurvedCone();
    }
}

void LightCone::generateFlatCone()
{
    vertices.clear();
    indices.clear();

    // For flat spacetime: |x - x0| = c * |t - t0|
    // We parameterize the cone surface using theta (polar) and phi (azimuthal)
    // Only the spatial part is rendered (3D projection)

    int nTheta = resolution;
    int nPhi = resolution * 2;
    double dt = 1.0; // time extent of cone

    // Generate vertices on the cone surface
    for (int i = 0; i <= nTheta; ++i) {
        double theta = maxAngle * i / nTheta;
        double r = dt * std::tan(theta);  // spatial radius at this angle

        for (int j = 0; j <= nPhi; ++j) {
            double phi = 2.0 * M_PI * j / nPhi;

            LightConeVertex v;
            v.position[0] = static_cast<float>(origin.x + r * std::cos(phi));
            v.position[1] = static_cast<float>(origin.y + r * std::sin(phi));
            v.position[2] = static_cast<float>(origin.z + dt);

            // Color based on angle from axis
            float angleFactor = static_cast<float>(theta / maxAngle);
            v.color[0] = colorRGBA[0] * (1.0f - 0.5f * angleFactor);
            v.color[1] = colorRGBA[1] * (1.0f - 0.5f * angleFactor);
            v.color[2] = colorRGBA[2] * (1.0f + 0.5f * angleFactor);
            v.color[3] = colorRGBA[3];

            v.intensity = 1.0f - 0.3f * angleFactor;

            vertices.push_back(v);
        }
    }

    buildIndices(nTheta + 1, nPhi + 1);
    maxExtent = dt * std::tan(maxAngle);
}

void LightCone::generateCurvedCone()
{
    vertices.clear();
    indices.clear();

    // In curved spacetime, integrate null geodesics from origin
    int nTheta = resolution;
    int nPhi = resolution * 2;

    std::vector<Event4D> rayEndpoints;
    rayEndpoints.reserve((nTheta + 1) * (nPhi + 1));

    for (int i = 0; i <= nTheta; ++i) {
        double theta = maxAngle * i / nTheta;
        double stheta = std::sin(theta);
        double ctheta = std::cos(theta);

        for (int j = 0; j <= nPhi; ++j) {
            double phi = 2.0 * M_PI * j / nPhi;
            double sphi = std::sin(phi);
            double cphi = std::cos(phi);

            // Direction in 3D space
            std::array<double, 3> direction = {
                stheta * cphi,
                stheta * sphi,
                ctheta
            };

            // Integrate null ray
            auto ray = integrateNullRay(direction, AFFINE_MAX);

            if (!ray.empty()) {
                rayEndpoints.push_back(ray.back());
            } else {
                // Fallback: use flat approximation
                double r = AFFINE_MAX;
                Event4D endpoint(
                    origin.t + r,
                    origin.x + r * direction[0],
                    origin.y + r * direction[1],
                    origin.z + r * direction[2]
                );
                rayEndpoints.push_back(endpoint);
            }
        }
    }

    // Convert to vertex buffer
    for (size_t i = 0; i < rayEndpoints.size(); ++i) {
        const auto& ep = rayEndpoints[i];
        LightConeVertex v;
        v.position[0] = static_cast<float>(ep.x);
        v.position[1] = static_cast<float>(ep.y);
        v.position[2] = static_cast<float>(ep.z);

        // Color by time component
        float tNorm = static_cast<float>((ep.t - origin.t) / AFFINE_MAX);
        v.color[0] = colorRGBA[0] * (0.5f + 0.5f * tNorm);
        v.color[1] = colorRGBA[1] * (0.5f + 0.5f * tNorm);
        v.color[2] = colorRGBA[2] * (1.0f - 0.3f * tNorm);
        v.color[3] = colorRGBA[3];
        v.intensity = 0.5f + 0.5f * tNorm;

        vertices.push_back(v);
    }

    buildIndices(nTheta + 1, nPhi + 1);

    // Compute max extent
    maxExtent = 0.0;
    for (const auto& ep : rayEndpoints) {
        double dx = ep.x - origin.x;
        double dy = ep.y - origin.y;
        double dz = ep.z - origin.z;
        double r = std::sqrt(dx*dx + dy*dy + dz*dz);
        if (r > maxExtent) maxExtent = r;
    }
}

std::vector<Event4D> LightCone::integrateNullRay(
    const std::array<double, 3>& direction,
    double maxAffineParam
) const
{
    std::vector<Event4D> trajectory;

    if (!metric) {
        // Flat spacetime: analytic solution
        double r = 0.0;
        double step = maxAffineParam / 20.0;
        for (int i = 0; i <= 20; ++i) {
            r = step * i;
            trajectory.push_back(Event4D(
                origin.t + r,
                origin.x + r * direction[0],
                origin.y + r * direction[1],
                origin.z + r * direction[2]
            ));
        }
        return trajectory;
    }

    // Curved spacetime: numerical integration using geodesic equation
    // Initial 4-momentum for null ray: k^mu such that g_{mu nu} k^mu k^nu = 0
    // We use the metric to compute proper null direction

    // Simplified approach: use the metric to compute effective null direction
    // Full implementation would solve the geodesic equation with proper
    // initial conditions satisfying k_mu k^mu = 0

    auto geodIntegrator = std::make_shared<GeodesicIntegrator>(1e-8, 1e-10, 0.01, 0.9, 10000);
    geodIntegrator->setMetric(metric);

    // Approximate null initial velocity
    double step = maxAffineParam / 50.0;
    Event4D current = origin;

    for (int i = 0; i < 50; ++i) {
        trajectory.push_back(current);

        // Compute metric at current position
        auto m = metric->evaluate(Event4D(current.t, current.x, current.y, current.z));

         // Simple null propagation: advance along direction with metric correction
         // This is a simplified approximation; full null geodesic integration
         // would require solving the geodesic equation properly

         double g00 = m[0][0];
        double spatialNorm = std::sqrt(direction[0]*direction[0] +
                                        direction[1]*direction[1] +
                                        direction[2]*direction[2]);
        if (spatialNorm < 1e-10) break;

        // Null condition: g00 * dt^2 + 2*g0i*dt*dx^i + gij*dx^i*dx^j = 0
        // For simplicity, use coordinate speed approximation
        double dt = step;
        double scale = step / spatialNorm;

        current.t += dt;
        current.x += scale * direction[0];
        current.y += scale * direction[1];
        current.z += scale * direction[2];
    }

    return trajectory;
}

void LightCone::buildIndices(int nTheta, int nPhi)
{
    indices.clear();
    indices.reserve((nTheta - 1) * (nPhi - 1) * 6);

    for (int i = 0; i < nTheta - 1; ++i) {
        for (int j = 0; j < nPhi - 1; ++j) {
            int base = i * nPhi + j;
            // Two triangles per quad
            indices.push_back(base);
            indices.push_back(base + nPhi);
            indices.push_back(base + 1);

            indices.push_back(base + 1);
            indices.push_back(base + nPhi);
            indices.push_back(base + nPhi + 1);
        }
    }
}

} // namespace quantumverse