#pragma once

#include "spacetime/DilatonMetric.h"
#include <memory>
#include <cmath>

namespace quantumverse {

/**
 * @class HawkingCalculator
 * @brief Computes Hawking radiation quantities for a given dilaton black hole.
 *
 * This class provides methods to calculate:
 * - The renormalized stress tensor ⟨:T_{--}:⟩ in the Unruh vacuum.
 * - The Bogoliubov coefficients relating in/out modes.
 * - The asymptotic Hawking flux at infinity.
 *
 * The implementation uses the known analytic results for the CGHS model:
 *   ⟨:T_{--}:⟩ = 1/48   (for one matter field, in the exterior region)
 * The flux is constant along x⁻ outside the horizon.
 */
class HawkingCalculator {
public:
    /**
     * @brief Construct with a reference to the black hole metric.
     * @param blackHole The dilaton black hole metric (shared ownership).
     */
    explicit HawkingCalculator(std::shared_ptr<DilatonMetric> blackHole) noexcept;

    /** @brief Destructor. */
    ~HawkingCalculator() = default;

    // Allow move but not copy
    HawkingCalculator(const HawkingCalculator&) = delete;
    HawkingCalculator& operator=(const HawkingCalculator&) = delete;
    HawkingCalculator(HawkingCalculator&&) noexcept = default;
    HawkingCalculator& operator=(HawkingCalculator&&) noexcept = default;

    /**
     * @brief Compute the renormalized stress tensor component ⟨:T_{--}:⟩ at a point.
     * @param x_plus Advanced time.
     * @param x_minus Retarded time.
     * @return The value of ⟨:T_{--}:⟩.
     *
     * In the Unruh vacuum, for the CGHS model with one matter field:
     *   ⟨:T_{--}:⟩ = 1/48   in the exterior region (x⁻ < 0 for the right universe).
     * Inside the horizon or in the vacuum, it is 0.
     */
    double computeTMinusMinus(double x_plus, double x_minus) const noexcept;

    /**
     * @brief Get the asymptotic Hawking flux at future null infinity (x⁻ → -∞).
     * @return Constant flux 1/48 for one matter field.
     */
    double asymptoticFlux() const noexcept;

    /**
     * @brief Compute the Bogoliubov coefficient |β_ωω'|² for a given frequency.
     * This is a placeholder; the full calculation requires mode solutions.
     * For now, we return the thermal occupation number at temperature T.
     * @param omega Frequency ω.
     * @return |β|² ≈ 1/(e^{ω/T} - 1)  (Planck distribution).
     */
    double bogoliubovBetaSquared(double omega) const noexcept;

    /**
     * @brief Get the Hawking temperature T = κ/(2π).
     * @return Temperature.
     */
    double temperature() const noexcept;

private:
    std::shared_ptr<DilatonMetric> blackHole;
};

} // namespace quantumverse
