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
        FRWMetric m(Curvature::FLAT,
            [t0](double t) -> double {
                if (t <= 0) return 0.0;
                return std::pow(t / t0, 2.0 / 3.0);
            },
            hubbleConstant);
        m.omegaM_ = 1.0;
        m.omegaLambda_ = 0.0;
        return m;
    }

    /**
     * @brief Construct a flat radiation-dominated FRW metric: a(t) = (t/t₀)^(1/2).
     * @param hubbleConstant H₀ in s⁻¹.
     * @param t0 Reference time in seconds.
     */
    static FRWMetric radiationDominated(double hubbleConstant = 2.27e-18,
                                         double t0 = 4.354e17) {
        FRWMetric m(Curvature::FLAT,
            [t0](double t) -> double {
                if (t <= 0) return 0.0;
                return std::pow(t / t0, 0.5);
            },
            hubbleConstant);
        m.omegaM_ = 0.0;
        m.omegaLambda_ = 0.0;
        return m;
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
        FRWMetric m(Curvature::FLAT,
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
        m.omegaM_ = omegaM;
        m.omegaLambda_ = omegaLambda;
        return m;
    }

    /**
     * @brief Construct a de Sitter metric (empty universe with Λ only): a(t) = e^(H₀t).
     * @param hubbleConstant H₀ in s⁻¹.
     */
    static FRWMetric deSitter(double hubbleConstant = 2.27e-18) {
        FRWMetric m(Curvature::FLAT,
            [hubbleConstant](double t) -> double {
                return std::exp(hubbleConstant * t);
            },
            hubbleConstant);
        m.omegaM_ = 0.0;
        m.omegaLambda_ = 1.0;
        return m;
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
     * @brief Get the matter density parameter Ωₘ.
     */
    double omegaM() const { return omegaM_; }

    /**
     * @brief Get the dark energy density parameter ΩΛ.
     */
    double omegaLambda() const { return omegaLambda_; }

    /**
     * @brief Get the Hubble parameter H(z) at redshift z.
     *
     * Standard flat-ΛCDM: H(z) = H₀ · √(Ωₘ(1+z)³ + ΩΛ).
     * Returns H(z) in s⁻¹, consistent with hubbleConstant().
     *
     * @param z Redshift (z ≥ 0).
     * @return Hubble parameter at redshift z, in s⁻¹.
     */
    double hubble(double z) const noexcept {
        const double opz = 1.0 + z;
        const double E2  = omegaM_ * opz * opz * opz + omegaLambda_;
        return H0_ * std::sqrt(E2);
    }

    /**
     * @brief Comoving line-of-sight distance χ(z) = c ∫₀^z dz'/H(z').
     *
     * Numerical integral via Simpson's rule with 4096 intervals.
     * Returns distance in Mpc.
     *
     * @param z Redshift (z ≥ 0).
     * @return Comoving distance in Mpc.
     */
    double comovingDistance(double z) const noexcept;

    /**
     * @brief Angular-diameter distance D_A(z) = χ(z)/(1+z) (flat universe).
     *
     * @param z Redshift (z ≥ 0).
     * @return Angular-diameter distance in Mpc.
     */
    double angularDiameterDistance(double z) const noexcept {
        return comovingDistance(z) / (1.0 + z);
    }

    /**
     * @brief Get the scale factor at time t.
     */
    double scaleFactor(double t) const {
        return scaleFactor_(t);
    }

    /**
     * @brief Compute the age of the universe for this model.
     *
     * Finds cosmic time t such that the scale factor a(t) reaches 1
     * (present day) by bisection on the scale factor function. Works for
     * any monotonically increasing scale factor — matter, radiation, ΛCDM,
     * or custom.
     *
     * @param tMax Maximum time to search (default: 200 Gyr).
     * @param steps Number of bisection iterations (default: 200 → ~1e-60 relative precision).
     * @return Estimated age of the universe in seconds.
     */
    double ageOfUniverse(double tMax = 6.31e18, int steps = 200) const;

private:
    int curvature_;  ///< k = -1, 0, or +1
    std::function<double(double)> scaleFactor_;
    double H0_;
    double omegaM_ = 0.0;    ///< Matter density parameter Ωₘ
    double omegaLambda_ = 0.0; ///< Dark energy density parameter ΩΛ

    /**
      * @brief Numerically compute the derivative of the scale factor.
      * Uses relative step size for better numerical precision at large t values.
      */
    double derivativeScaleFactor(double t) const {
        // Use relative step size to handle large t values (cosmic time ~1e17)
        const double h = std::max(1e-6, std::abs(t) * 1e-5);
        double a_plus = scaleFactor_(t + h);
        double a_minus = scaleFactor_(t - h);
        return (a_plus - a_minus) / (2.0 * h);
    }
};

inline double FRWMetric::ageOfUniverse(double tMax, int steps) const {
    // Bisection on t: find t such that scaleFactor(t) = 1 (present day).
    const double aAtZero = scaleFactor_(0.0);
    if (aAtZero >= 1.0) return 0.0; // degenerate: already at present at t=0

    const double aAtMax = scaleFactor_(tMax);
    if (aAtMax <= 1.0) return tMax; // hasn't reached present by tMax

    double lo = 0.0, hi = tMax;
    for (int i = 0; i < steps; ++i) {
        const double mid = 0.5 * (lo + hi);
        const double aMid = scaleFactor_(mid);
        if (aMid < 1.0) lo = mid;
        else hi = mid;
    }
    return 0.5 * (lo + hi);
}

inline double FRWMetric::comovingDistance(double z) const noexcept {
    // χ(z) = c ∫₀^z dz'/H(z')  via Simpson's rule
    // H(z) in s⁻¹, c in m/s → result in metres, convert to Mpc.
    constexpr double C_SI   = 299792458.0;   // m/s
    constexpr double MPC_M  = 3.0856775814913673e22; // m per Mpc

    const int steps = 4096;
    if (z <= 0.0) return 0.0;
    if (steps % 2 != 0) { /* steps already even */ }

    const double h = z / static_cast<double>(steps);
    auto H_inv = [this](double zp) { return 1.0 / hubble(zp); }; // seconds

    double s = H_inv(0.0) + H_inv(z);
    for (int i = 1; i < steps; ++i) {
        const double zp = i * h;
        const double w  = (i % 2 == 0) ? 2.0 : 4.0;
        s += w * H_inv(zp);
    }
    const double integral_seconds = s * h / 3.0; // seconds
    const double distance_m       = C_SI * integral_seconds; // metres
    return distance_m / MPC_M; // Mpc
}

} // namespace quantumverse

#endif // QUANTUMVERSE_FRW_METRIC_H