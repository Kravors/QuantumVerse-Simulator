/**
 * @file FeatureVector.h
 * @brief 20-dimensional feature extraction for ML anomaly detection
 *
 * Aggregates physics telemetry, solar system kinematics, black hole parameters,
 * and discovery instrument confidences into a fixed-size vector suitable for
 * lightweight autoencoder inference.
 */

#ifndef QUANTUMVERSE_FEATURE_VECTOR_H
#define QUANTUMVERSE_FEATURE_VECTOR_H

#include <array>
#include <vector>
#include <string>
#include <limits>
#include <cmath>
#include "ui4d/UI4D.h"
#include "discovery/DiscoveryInstrument.h"
#include "physics/PhysicsConstants.h"

namespace quantumverse {

/**
 * @brief 20-dimensional feature vector for anomaly detection
 *
 * Layout:
 *   [ 0] totalKineticEnergy
 *   [ 1] totalPotentialEnergy
 *   [ 2] totalEnergy
 *   [ 3] angularMomentumZ
 *   [ 4] earthOrbitalSpeed
 *   [ 5] earthDistance
 *   [ 6] gravitationalWaveStrain
 *   [ 7] gravitationalWaveFreq
 *   [ 8] G
 *   [ 9] c
 *   [10] blackHoleMass
 *   [11] bodyCount
 *   [12] meanBodySpeed
 *   [13] maxBodySpeed
 *   [14] minBodySpeed
 *   [15] stdBodySpeed
 *   [16] meanInstrumentConfidence
 *   [17] maxInstrumentConfidence
 *   [18] anomalyCount
 *   [19] instrumentCount
 */
struct FeatureVector {
    static constexpr std::size_t Dim = 20;
    std::array<double, Dim> data{};

    /**
     * @brief Extract features from live simulation state
     *
     * @param telemetry Current physics telemetry
     * @param blackHoleMassKg Active black hole mass in kg
     * @param bodies Solar system bodies (for velocity statistics)
     * @param findings Latest discovery instrument findings
     * @return Populated feature vector
     */
    static FeatureVector extract(
        const UI4D::PhysicsTelemetry& telemetry,
        double blackHoleMassKg,
        const std::vector<UI4D::SolarSystemBody>& bodies,
        const std::vector<InstrumentFinding>& findings
    ) {
        FeatureVector fv;
        fv.data[0]  = telemetry.totalKineticEnergy;
        fv.data[1]  = telemetry.totalPotentialEnergy;
        fv.data[2]  = telemetry.totalEnergy;
        fv.data[3]  = telemetry.angularMomentumZ;
        fv.data[4]  = telemetry.earthOrbitalSpeed;
        fv.data[5]  = telemetry.earthDistance;
        fv.data[6]  = telemetry.gravitationalWaveStrain;
        fv.data[7]  = telemetry.gravitationalWaveFreq;
        fv.data[8]  = PhysicsConstants::instance().get_G();
        fv.data[9]  = PhysicsConstants::instance().get_c();
        fv.data[10] = blackHoleMassKg;
        fv.data[11] = static_cast<double>(telemetry.bodyCount);

        // Body velocity statistics
        fv.data[12] = meanBodySpeed(bodies);
        fv.data[13] = maxBodySpeed(bodies);
        fv.data[14] = minBodySpeed(bodies);
        fv.data[15] = stdBodySpeed(bodies);

        // Instrument confidence statistics
        fv.data[16] = meanConfidence(findings);
        fv.data[17] = maxConfidence(findings);
        fv.data[18] = static_cast<double>(anomalyCount(findings));
        fv.data[19] = static_cast<double>(findings.size());

        return fv;
    }

    /**
     * @brief Convert to std::vector<double> for inference APIs
     */
    std::vector<double> toVector() const {
        return std::vector<double>(data.begin(), data.end());
    }

    /**
     * @brief Load from std::vector<double> (must match Dim)
     */
    static FeatureVector fromVector(const std::vector<double>& v) {
        FeatureVector fv;
        if (v.size() != Dim) {
            throw std::invalid_argument("FeatureVector dimension mismatch");
        }
        for (std::size_t i = 0; i < Dim; ++i) {
            fv.data[i] = v[i];
        }
        return fv;
    }

private:
    static double bodySpeed(const UI4D::SolarSystemBody& body) {
        double vx = body.velocity.x;
        double vy = body.velocity.y;
        double vz = body.velocity.z;
        return std::sqrt(vx * vx + vy * vy + vz * vz);
    }

    static double meanBodySpeed(const std::vector<UI4D::SolarSystemBody>& bodies) {
        if (bodies.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& b : bodies) sum += bodySpeed(b);
        return sum / static_cast<double>(bodies.size());
    }

    static double maxBodySpeed(const std::vector<UI4D::SolarSystemBody>& bodies) {
        if (bodies.empty()) return 0.0;
        double mx = 0.0;
        for (const auto& b : bodies) mx = std::max(mx, bodySpeed(b));
        return mx;
    }

    static double minBodySpeed(const std::vector<UI4D::SolarSystemBody>& bodies) {
        if (bodies.empty()) return 0.0;
        double mn = std::numeric_limits<double>::max();
        for (const auto& b : bodies) mn = std::min(mn, bodySpeed(b));
        return mn;
    }

    static double stdBodySpeed(const std::vector<UI4D::SolarSystemBody>& bodies) {
        if (bodies.size() < 2) return 0.0;
        double m = meanBodySpeed(bodies);
        double sum = 0.0;
        for (const auto& b : bodies) {
            double d = bodySpeed(b) - m;
            sum += d * d;
        }
        return std::sqrt(sum / static_cast<double>(bodies.size()));
    }

    static double meanConfidence(const std::vector<InstrumentFinding>& findings) {
        if (findings.empty()) return 0.0;
        double sum = 0.0;
        for (const auto& f : findings) sum += f.confidence;
        return sum / static_cast<double>(findings.size());
    }

    static double maxConfidence(const std::vector<InstrumentFinding>& findings) {
        if (findings.empty()) return 0.0;
        double mx = 0.0;
        for (const auto& f : findings) mx = std::max(mx, f.confidence);
        return mx;
    }

    static int anomalyCount(const std::vector<InstrumentFinding>& findings) {
        int cnt = 0;
        for (const auto& f : findings) {
            if (f.isAnomaly) ++cnt;
        }
        return cnt;
    }
};

} // namespace quantumverse

#endif // QUANTUMVERSE_FEATURE_VECTOR_H
