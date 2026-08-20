/**
 * @file PBHMicrolensingScanner.h
 * @brief Primordial black-hole microlensing scanner
 *
 * Searches stellar light curves for a primordial black hole (PBH) acting as a
 * dark, compact gravitational lens.  A PBH passing close to the line of sight
 * produces a symmetric, achromatic magnification of a background star described
 * by the Paczynski curve:
 *
 *   A(u) = (u² + 2) / (u * sqrt(u² + 4)),
 *   u(t) = sqrt(u0² + ((t - t0) / tE)²),
 *
 * where u0 is the impact parameter (in Einstein radii), tE the Einstein-radius
 * crossing time, and t0 the peak time.  Unlike variable stars or flares, the
 * event is a smooth, symmetric brightening that returns to baseline, with no
 * visible lens star (non-self-lensing).  The instrument grid-fits the Paczynski
 * curve and flags events where the fit explains the baseline variance.
 *
 * References:
 * - Paczynski (1986) - Gravitational microlensing by the galactic halo
 * - Griest (1991) - Galactic microlensing as a method of detecting massive
 *   compact halo objects
 */

#ifndef QUANTUMVERSE_PBH_MICROLENSING_SCANNER_H
#define QUANTUMVERSE_PBH_MICROLENSING_SCANNER_H

#include "DiscoveryInstrument.h"
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief Primordial black-hole microlensing scanner
 *
 * Analyzes a light curve (each sample: `x` = observed flux, `t` = time) for a
 * symmetric Paczynski magnification bump.  A dark, compact lens consistent
 * with a PBH is reported as a candidate event, with fitted parameters
 * (t0, tE, u0, peak magnification) recorded.
 */
class PBHMicrolensingScanner : public DiscoveryInstrument
{
public:
    PBHMicrolensingScanner();
    ~PBHMicrolensingScanner() override = default;

    std::vector<InstrumentFinding> analyze(const MetricTensor& metric, const Event4D& location,
        const std::vector<Event4D>& trajectory = {}) override;

    std::string getName() const override { return "PBHMicrolensingScanner"; }
    std::string getDescription() const override
    {
        return "Primordial black-hole microlensing scanner. Searches stellar "
               "light curves for a symmetric, achromatic Paczynski magnification "
               "bump (a dark, compact lens), flagging candidate PBH events.";
    }
    std::string getCategory() const override { return "Gravitational Lensing"; }
    AlertSeverity getDefaultSeverity() const override { return AlertSeverity::HIGH; }

    std::map<std::string, std::pair<double, double>> getParameterRanges() const override;

    /**
     * @brief Physical estimate of the lensing object derived from a microlensing fit.
     */
    struct PBHEstimate {
        double mass_solar = 0.0;          ///< Lens mass in solar masses
        double velocity_kms = 0.0;        ///< Assumed lens transverse velocity (km/s)
        double einstein_radius_au = 0.0;  ///< Physical Einstein radius (AU)
        double lens_distance_kpc = 0.0;   ///< Lens (PBH) distance from observer (kpc)
        double source_distance_kpc = 0.0; ///< Source distance from observer (kpc)
        double confidence = 0.0;          ///< Fit quality (0-1)
    };

    /**
     * @brief Convert a fitted Einstein crossing time into a PBH mass/velocity estimate.
     * @param tE Einstein-radius crossing time, in seconds
     * @param u0 Impact parameter in Einstein radii (retained for the record)
     * @param lensDistanceKpc Lens (PBH) distance from the observer, in kpc
     * @param sourceDistanceKpc Source distance from the observer, in kpc
     * @param lensVelocityKms Assumed lens transverse velocity, in km/s
     * @param confidence Fit quality (0-1)
     *
     * Uses the standard microlensing relations
     *   R_E = sqrt( (4 G M / c^2) * (D_l D_ls / D_s) ),   t_E = R_E / v_t,
     * solved for M given an assumed transverse velocity v_t.  The returned
     * velocity is the assumed v_t, since tE alone cannot break the M/v_t
     * degeneracy; supplying a prior on v_t is the standard Galactic practice.
     */
    static PBHEstimate estimatePBH(double tE, double u0, double lensDistanceKpc,
        double sourceDistanceKpc = 8.0, double lensVelocityKms = 220.0,
        double confidence = 0.0);

private:
    static constexpr double kMinTrajectorySize = 24;

    static double paczynskiMagnification(double u);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_PBH_MICROLENSING_SCANNER_H
