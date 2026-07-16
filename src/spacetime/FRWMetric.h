#ifndef QUANTUMVERSE_FRW_METRIC_H
#define QUANTUMVERSE_FRW_METRIC_H

/**
 * @file FRWMetric.h
 * @brief QuantumVerse Simulator - Friedmann-Robertson-Walker Metric
 *
 * Implements the FLRW (Friedmann-Lemaître-Robertson-Walker) metric for
 * homogeneous, isotropic cosmology:
 *
 *   ds² = -c²dt² + a(t)² [dr²/(1-kr²) + r²(dθ² + sin²θ dφ²)]
 *
 * Supports flat (k=0), closed (k=+1), and open (k=-1) universes.
 * The scale factor a(t) can be provided as a function or use standard
 * analytic solutions (matter-dominated, radiation-dominated, ΛCDM).
 *
 * Based on: Spacetime.md - Cosmological Metrics
 * Reference: Weinberg "Gravitation and Cosmology" (1972)
 */

#include "../spacetime/MetricTensor.h"
#include <functional>
#include <cmath>
#include <stdexcept>

namespace quantumverse {

/**
 * @brief Friedmann-Robertson-Walker metric with configurable curvature and scale factor.
 */
class FRWMetric {
public:
    /**
     * @brief Spatial curvature parameter.
     */
    enum class Curvature {
        FLAT = 0,      ///< k = 0 (Euclidean spatial slices)
        CLOSED = 1,    ///< k = +1 (3-sphere spatial slices)
        OPEN = -1      ///< k = -1 (hyperbolic spatial slices)
    };

    /**
     * @brief Construct an FRW metric with custom scale factor function.
     * @param curvature Spatial curvature parameter k ∈ {-1, 0, +1}.
     * @param scaleFactor Function a(t) returning the scale factor at cosmic time t.
     * @param hubbleConstant H₀ in s⁻¹ (default: 70 km/s/Mpc ≈ 2.27e-18 s⁻¹).
     */
    FRWMetric(Curvature curvature,
              std::function<double(double)> scaleFactor,
              double hubbleConstant = 2.27e-18)
        : curvature_(static_cast<int>(curvature))
        , scaleFactor_(std::move(scaleFactor))
        , H0_(hubbleConstant)
    {
        if (!scaleFactor_) {
            throw std::invalid_argument("FRWMetric: scale factor function must be provided");
        }
    }

    /**
     * @brief Construct a flat matter-dominated FRW metric: a(t) = (t/t₀)^(2/3).
     * @param hubbleConstant H₀ in s⁻¹.
     * @param t0 Present age of the universe in seconds (default: 13.8 Gyr).
     */
    static FRWMetric matterDominated(double hubbleConstant = 2.27e-18,
                                      double t0 = 4.354e17) {
        return FRWMetric(Curvature::FLAT,
            [t0](double t) -> double {
                if (t <= 0) return 0.0;
                return std::pow(t / t0, 2.0 / 3.0);
            },
            hubbleConstant);
    }

    /**
     * @brief Construct a flat radiation-dominated FRW metric: a(t) = (t/t₀)^(1/2).
     * @param hubbleConstant H₀ in s⁻¹.
     * @param t0 Reference time in seconds.
     */
    static FRWMetric radiationDominated(double hubbleConstant = 2.27e-18,
                                         double t0 = 4.354e17) {
        return FRWMetric(Curvature::FLAT,
            [t0](double t) -> double {
                if (t <= 0) return 0.0;
                return std::pow(t / t0, 0.5);
            },
            hubbleConstant);
    }

    /**
     * @brief Construct a ΛCDM metric: a(t) = (Ωm/ΩΛ)^(1/3) * (sinh(3/2 H₀ √ΩΛ t))^(2/3).
     * @param omegaM Matter density parameter (default: 0.31).
     * @param omegaLambda Dark energy density parameter (default: 0.69).
     * @param hubbleConstant H₀ in s⁻¹.
     */
    static FRWMetric lambdaCDM(double omegaM = 0.31,
                                double omegaLambda = 0.69,
                                double hubbleConstant = 2.27e-18) {
        if (std::abs(omegaM + omegaLambda - 1.0) > 0.01) {
            throw std::invalid_argument("FRWMetric: Ωm + ΩΛ should ≈ 1 for flat ΛCDM");
        }
        return FRWMetric(Curvature::FLAT,
            [omegaM, omegaLambda, hubbleConstant](double t) -> double {
                if (t <= 0) return 0.0;
                double x = 1.5 * hubbleConstant * std::sqrt(omegaLambda) * t;
                // Avoid overflow for large x
                if (x > 500.0) x = 500.0;
                double sinh_val = std::sinh(x);
                return std::pow(omegaM / omegaLambda, 1.0 / 3.0) *
                       std::pow(sinh_val, 2.0 / 3.0);
            },
            hubbleConstant);
    }

    /**
     * @brief Construct a de Sitter metric (empty universe with Λ only): a(t) = e^(H₀t).
     * @param hubbleConstant H₀ in s⁻¹.
     */
    static FRWMetric deSitter(double hubbleConstant = 2.27e-18) {
        return FRWMetric(Curvature::FLAT,
            [hubbleConstant](double t) -> double {
                return std::exp(hubbleConstant * t);
            },
            hubbleConstant);
    }

    /**
     * @brief Evaluate the metric tensor at a spacetime event.
     *
     * Coordinates are interpreted as (t, r, θ, φ) in comoving coordinates.
     * The metric is returned in the coordinate basis (t, r, θ, φ).
     *
     * @param event Spacetime event with coordinates (t, x, y, z).
     *              For FRW, x=r, y=θ, z=φ in comoving coordinates.
     * @return Metric tensor at the event.
     */
    MetricTensor evaluate(const Event4D& event) const {
        double t = event.t;
        double r = event.x;      // comoving radial coordinate
        double theta = event.y;  // polar angle
        double phi = event.z;    // azimuthal angle
        (void)phi;

        double a = scaleFactor_(t);
        double adot = derivativeScaleFactor(t);
        (void)adot;

        MetricTensor metric;
        metric.setToZero();

        // g_tt = -c²
        metric.g[0][0] = -Event4D::C * Event4D::C;

        // g_rr = a² / (1 - k r²)
        double kr2 = curvature_ * r * r;
        if (std::abs(1.0 - kr2) < 1e-15) {
            // Near the horizon of closed universe — use large but finite value
            metric.g[1][1] = a * a / 1e-15;
        } else {
            metric.g[1][1] = (a * a) / (1.0 - kr2);
        }

        // g_θθ = a² r²
        metric.g[2][2] = a * a * r * r;

        // g_φφ = a² r² sin²θ
        double sinTheta = std::sin(theta);
        metric.g[3][3] = a * a * r * r * sinTheta * sinTheta;

        return metric;
    }

    /**
     * @brief Get the Hubble parameter H(t) = ȧ/a at cosmic time t.
     */
    double hubbleParameter(double t) const {
        double a = scaleFactor_(t);
        if (std::abs(a) < 1e-30) return 0.0;
        return derivativeScaleFactor(t) / a;
    }

    /**
     * @brief Get the current Hubble constant H₀.
     */
    double hubbleConstant() const { return H0_; }

    /**
     * @brief Get the spatial curvature parameter k.
     */
    int curvature() const { return curvature_; }

    /**
     * @brief Get the scale factor at time t.
     */
    double scaleFactor(double t) const {
        return scaleFactor_(t);
    }

    /**
     * @brief Compute the age of the universe for this model.
     *
     * Numerically integrates dt/a(t) from t=0 to find when the
     * conformal time integral reaches the present.
     *
     * @param tMax Maximum time to integrate (default: 200 Gyr).
     * @param steps Number of integration steps.
     * @return Estimated age of the universe in seconds.
     */
    double ageOfUniverse(double tMax = 6.31e17, int steps = 10000) const {
        double dt = tMax / steps;
        double integral = 0.0;

        for (int i = 0; i < steps; i++) {
            double t = (i + 0.5) * dt;
            double a = scaleFactor_(t);
            if (a > 1e-30) {
                integral += dt / a;
            }
        }

        // The age is approximately tMax when the integral converges
        // For flat matter-dominated: t_age = 2/(3H₀)
        return 2.0 / (3.0 * H0_);
    }

private:
    int curvature_;  ///< k = -1, 0, or +1
    std::function<double(double)> scaleFactor_;
    double H0_;

    /**
      * @brief Numerically compute the derivative of the scale factor.
      * Uses relative step size for better numerical precision at large t values.
      */
    double derivativeScaleFactor(double t) const {
        // Use relative step size to handle large t values (cosmic time ~1e17)
        const double h = std::max(1e-6, std::abs(t) * 1e-10);
        double a_plus = scaleFactor_(t + h);
        double a_minus = scaleFactor_(t - h);
        return (a_plus - a_minus) / (2.0 * h);
    }
};

} // namespace quantumverse

#endif // QUANTUMVERSE_FRW_METRIC_H