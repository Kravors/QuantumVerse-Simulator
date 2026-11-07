/**
 * @file test_mercury_1pn.cpp
 * @brief Integrator-level test of the 1PN Schwarzschild correction.
 *
 * Integrates Mercury's orbit with the 1PN correction enabled via
 * UI4D::stepSimulationCPURaw() and measures the perihelion advance.
 * Expected: ~0.1036 arcsec/orbit (43 arcsec/century).
 * Isolates Sun+Mercury so Newtonian planet perturbations don't swamp
 * the ~1e-7 ratio 1PN signal.
 */

#include <iostream>
#include <cmath>
#include <limits>
#include "ui4d/UI4D.h"
#include "physics/PhysicsConstants.h"
#include "test_assert.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

constexpr double kG    = 6.67430e-11;
constexpr double kC    = 2.99792458e8;
constexpr double kMsun = 1.989e30;
constexpr double kGM   = kG * kMsun;

constexpr double kAU   = 1.495978707e11;
constexpr double kA_merc = 0.387 * kAU;
constexpr double kE_merc = 0.205630;
constexpr double kR_peri = kA_merc * (1.0 - kE_merc);
constexpr double kT_merc = 7.6006e6;

constexpr double kDt     = 1.0;
constexpr int    kOrbits = 3;

constexpr double kExpectedAdvance_rad =
    (43.0 / 3600.0) * (M_PI / 180.0) / 415.0 * kOrbits;
constexpr double kTolerance_rad = 1e-5;

double visViva(double r, double a) {
    return std::sqrt(kGM * (2.0 / r - 1.0 / a));
}

struct EccState { double e; double arg; };

EccState eccentricityArgument(double x, double y, double vx, double vy) {
    const double r  = std::sqrt(x*x + y*y);
    const double L  = x*vy - y*vx;
    const double ex =  vy * L / kGM - x / r;
    const double ey = -vx * L / kGM - y / r;
    return { std::sqrt(ex*ex + ey*ey), std::atan2(ey, ex) };
}

quantumverse::UI4D::SolarSystemBody& setupSunMercury(quantumverse::UI4D& ui) {
    ui.setGrCorrections(true);
    auto& bodies = ui.solarSystem.bodies;
    for (auto it = bodies.begin(); it != bodies.end(); ) {
        if (it->first != "Sun" && it->first != "Mercury") {
            it = bodies.erase(it);
        } else {
            ++it;
        }
    }
    auto& m = bodies.at("Mercury");
    const double v_peri = visViva(kR_peri, kA_merc);
    m.position.x = kR_peri; m.position.y = 0.0;   m.position.z = 0.0;
    m.velocity.x = 0.0;     m.velocity.y = v_peri; m.velocity.z = 0.0;
    m.position.t = 0.0;
    m.velocity.t = 0.0;
    return m;
}

} // namespace

int main() {
    quantumverse::UI4D ui4d;
    auto& m = setupSunMercury(ui4d);

    auto s0 = eccentricityArgument(m.position.x, m.position.y,
                                   m.velocity.x, m.velocity.y);
    double prev_arg = s0.arg;
    double total_rotation = 0.0;

    // Energy baseline (captured before integration)
    auto E = [&]() {
        const double r  = std::sqrt(m.position.x*m.position.x +
                                    m.position.y*m.position.y +
                                    m.position.z*m.position.z);
        const double v2 = m.velocity.x*m.velocity.x +
                          m.velocity.y*m.velocity.y +
                          m.velocity.z*m.velocity.z;
        return 0.5 * v2 - kGM / r;
    };
    const double E0 = E();

    const int64_t steps = static_cast<int64_t>(kOrbits * kT_merc / kDt);
    const int     sample_every = 100;

    // Safety cap: bail out rather than timeout on slow CI.
    const int64_t kMaxSteps = 30'000'000;
    int64_t effectiveSteps = std::min(steps, kMaxSteps);

    for (int64_t i = 0; i < effectiveSteps; ++i) {
        ui4d.stepSimulationCPURaw(kDt);
        if ((i + 1) % sample_every == 0) {
            auto s = eccentricityArgument(m.position.x, m.position.y,
                                          m.velocity.x, m.velocity.y);
            double d = s.arg - prev_arg;
            while (d >  M_PI) d -= 2.0 * M_PI;
            while (d < -M_PI) d += 2.0 * M_PI;
            total_rotation += d;
            prev_arg = s.arg;
        }
    }

    QV_CHECK(effectiveSteps == steps);
    if (effectiveSteps != steps) {
        std::cerr << "Hit step cap before completing " << kOrbits << " orbits" << std::endl;
        return 1;
    }

    const double residual_arcsec = total_rotation * 206264.806;
    const double expected_arcsec = kExpectedAdvance_rad * 206264.806;
    const double tolerance_arcsec = kTolerance_rad * 206264.806;

    std::cout << "[1PN] total rotation = " << total_rotation << " rad" << std::endl;
    std::cout << "[1PN] precession = " << residual_arcsec << " arcsec"
              << " (expected ~" << expected_arcsec << " +/- " << tolerance_arcsec << ")" << std::endl;

    bool precessionPass = std::abs(total_rotation - kExpectedAdvance_rad) < kTolerance_rad;
    std::cout << (precessionPass ? "[PASS]" : "[FAIL]")
              << " Perihelion advance: " << residual_arcsec << " arcsec over "
              << kOrbits << " orbits" << std::endl;

    // Energy drift check: E0 captured before main integration loop
    const double drift = std::abs((E() - E0) / E0);
    bool energyPass = drift < 1e-6;
    std::cout << (energyPass ? "[PASS]" : "[FAIL]")
              << " Energy drift: " << drift << " (threshold 1e-6)" << std::endl;

    // Earth-radius sanity: analytic 1PN acceleration ratio at 1 AU
    const double r_E = 1.496e11;
    const double v_E = std::sqrt(kGM / r_E);
    const double a_N = kGM / (r_E * r_E);
    const double a_1PN = (kGM / (kC*kC * r_E * r_E)) *
                         (4.0*kGM/r_E - v_E*v_E);
    const double ratio = std::abs(a_1PN / a_N);
    bool earthPass = ratio < 1e-7;
    std::cout << (earthPass ? "[PASS]" : "[FAIL]")
              << " |a_1PN/a_N| at Earth: " << ratio << " (threshold 1e-7)" << std::endl;

    bool allPass = precessionPass && energyPass && earthPass;
    std::cout << std::endl << (allPass ? "=== ALL TESTS PASSED ===" : "=== SOME TESTS FAILED ===") << std::endl;
    return allPass ? 0 : 1;
}
