#pragma once

#include "spacetime/DilatonMetric.h"

namespace quantumverse {

/**
 * @class DilatonBlackHole
 * @brief Represents a dilaton black hole, possibly formed by collapsing matter.
 *
 * This class wraps DilatonMetric and provides higher-level operations:
 * - Construction with mass and sigma.
 * - Horizon detection.
 * - Singularity detection.
 * - Hawking temperature and entropy.
 *
 * Currently, it models a static eternal black hole. The collapsing scenario
 * can be added by evolving the mass parameter as a function of advanced time.
 */
class DilatonBlackHole {
public:
    /** @brief Pi constant for calculations. */
    static constexpr double PI = 3.14159265358979323846;

    /**
     * @brief Construct a black hole with given mass and sigma.
     * @param mass Black hole mass M (>0). Use 0 for vacuum (no black hole).
     * @param sigma Dilaton vacuum parameter σ (default 0 for λ=1).
     */
    DilatonBlackHole(double mass = 0.0, double sigma = 0.0) noexcept;

    /** @brief Destructor. */
    ~DilatonBlackHole() = default;

    // Delete copy, allow move
    DilatonBlackHole(const DilatonBlackHole&) = delete;
    DilatonBlackHole& operator=(const DilatonBlackHole&) = delete;
    DilatonBlackHole(DilatonBlackHole&&) noexcept = default;
    DilatonBlackHole& operator=(DilatonBlackHole&&) noexcept = default;

    /**
     * @brief Check if a spacetime point lies on the apparent horizon.
     * @param x_plus Advanced time.
     * @param x_minus Retarded time.
     * @param tol Tolerance.
     * @return true if on horizon.
     */
    bool isHorizon(double x_plus, double x_minus, double tol = 1e-10) const noexcept;

    /**
     * @brief Check if a point is at the singularity.
     */
    bool isSingularity(double x_plus, double x_minus, double tol = 1e-10) const noexcept;

    /**
     * @brief Get the underlying metric.
     */
    const DilatonMetric& getMetric() const noexcept;

    /**
     * @brief Get the mass.
     */
    double getMass() const noexcept;

    /**
     * @brief Get the dilaton vacuum parameter σ.
     */
    double getSigma() const noexcept;

    /**
     * @brief Compute the Hawking temperature T = κ/(2π).
     * For this black hole, the surface gravity κ = 1/(2M) (in units where c=1).
     * @return T = 1/(4π M). For M=0, returns 0.
     */
    double hawkingTemperature() const noexcept;

    /**
     * @brief Compute the Bekenstein-Hawking entropy S = 2π M (in units where G=c=1, and λ=1).
     * @return S = 2π M.
     */
    double entropy() const noexcept;

private:
    DilatonMetric metric;
};

} // namespace quantumverse
