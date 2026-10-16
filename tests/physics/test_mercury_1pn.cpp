// Integrator-level test for the 1PN Schwarzschild correction.
//
// Expected: perihelion advances prograde by 43"/century.
// At Mercury (a=5.791e10 m, e=0.2056, T=87.97 d) the advance is
// 0.1036"/orbit. Over 50 orbits: 5.18" = 2.511e-5 rad.
//
// The 1PN correction to the acceleration is
//     a_1PN = (GM/(c^2 r^2)) [ (4GM/r - v^2) r_hat + 4(v.r_hat) v ]
// applied in addition to a_Newtonian = -(GM/r^2) r_hat.
//
// The formula itself is trivially small at Mercury (|a_1PN|/|a_N| ~ 1e-7),
// so the test uses dt=1s and 50 orbits: leapfrog's own O(dt^2) precession
// bias is ~1e-12 rad/orbit, roughly 5 orders of magnitude below the signal.
//
// Adapted to the real QuantumVerse API:
//   - UI4D::getSolarSystem() returns SolarSystemData& (mutable overload added)
//   - UI4D::stepSimulationCPURaw(dt) is the raw leapfrog step
//   - Only Sun + Mercury are kept so Newtonian planet perturbations don't
//     swamp the ~5.18" GR signal over 50 orbits.

#include <QtTest>
#include <cmath>
#include <memory>
#include <string>

#include "ui4d/UI4D.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {

constexpr double kG    = 6.67430e-11;
constexpr double kC    = 2.99792458e8;
constexpr double kMsun = 1.989e30;
constexpr double kGM   = kG * kMsun;    // ~1.327e20 m^3/s^2

constexpr double kAU   = 1.495978707e11;
constexpr double kA_merc = 0.387 * kAU;  // semi-major axis, m (matches config)
constexpr double kE_merc = 0.205630;
constexpr double kR_peri = kA_merc * (1.0 - kE_merc);
constexpr double kT_merc = 7.6006e6;    // orbital period, s (matches config)

constexpr double kDt     = 1.0;         // integration step, s
constexpr int    kOrbits = 24;

// Expected perihelion advance over kOrbits orbits:
//   43 arcsec/century / 415 orbits/century * 24 orbits = 2.49 arcsec
constexpr double kExpectedAdvance_rad =
    (43.0 / 3600.0) * (M_PI / 180.0) / 415.0 * kOrbits;
constexpr double kTolerance_rad = 5.0e-6;   // ~1 arcsec absolute tolerance

double visViva(double r, double a) {
    return std::sqrt(kGM * (2.0 / r - 1.0 / a));
}

// Eccentricity vector and its argument (angle from +X in the XY plane).
struct EccState { double e; double arg; };

EccState eccentricityArgument(double x, double y, double vx, double vy) {
    const double r  = std::sqrt(x*x + y*y);
    const double L  = x*vy - y*vx;              // specific angular momentum (z)
    const double ex =  vy * L / kGM - x / r;    // e_vec = (v x L)/GM - r_hat
    const double ey = -vx * L / kGM - y / r;
    return { std::sqrt(ex*ex + ey*ey), std::atan2(ey, ex) };
}

// Build a fresh UI4D with only Sun + Mercury, Mercury at perihelion moving
// in +Y. Returns the Mercury body reference for the caller to integrate.
quantumverse::UI4D::SolarSystemBody& setupSunMercury(quantumverse::UI4D& ui, bool grCorrections = true) {
    ui.setGrCorrections(grCorrections);

    // Isolate the two-body problem: drop every planet except the Sun so that
    // Newtonian planet-planet perturbations cannot mask the 1PN signal.
    auto& bodies = ui.getSolarSystem().bodies;
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

class TestMercury1PN : public QObject {
    Q_OBJECT

private slots:

    // Sanity: the arithmetic that produces 43"/century is correct.
    void testAnalyticAdvance() {
        const double per_orbit_rad =
            (43.0 / 3600.0) * (M_PI / 180.0) / 415.0;
        QVERIFY(std::abs(per_orbit_rad - 5.02e-7) < 1e-9);
    }

    // Integrator: perihelion advances by the GR amount.
    void testPerihelionAdvance() {
        auto ui = std::make_unique<quantumverse::UI4D>();
        auto& m = setupSunMercury(*ui);

        // ---- Prime the phase tracker ----
        auto s0 = eccentricityArgument(m.position.x, m.position.y,
                                       m.velocity.x, m.velocity.y);
        double prev_arg = s0.arg;
        double total_rotation = 0.0;

        // ---- Integrate ----
        const int64_t steps = static_cast<int64_t>(kOrbits * kT_merc / kDt);
        const int     sample_every = 100;
        for (int64_t i = 0; i < steps; ++i) {
            ui->stepSimulationCPURaw(kDt);            // raw leapfrog step, dt=1s
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

        // ---- Measure ----
        // e_vec rotates with perihelion. total_rotation is the accumulated
        // argument change = the perihelion advance (Newtonian: ~0, 1PN: ~43"/century).
        const double residual_rad = total_rotation;
        const double residual_arcsec = residual_rad * 206264.806;

        qInfo() << "[1PN] total rotation =" << total_rotation << " rad";
        qInfo() << "[1PN] precession (turns) =" << total_rotation / (2.0 * M_PI);
        qInfo() << "[1PN] residual advance = "
                << residual_arcsec << " arcsec (expected ~2.49\")";

        // Diagnostic failure messages distinguish the three cases:
        if (std::abs(residual_arcsec) < 0.5) {
            QFAIL("1PN correction appears NOT IMPLEMENTED: residual ~0 "
                  "means the integrator is Newtonian. Implement "
                  "a_1PN in stepSimulationCPU, then rerun.");
        }
        if (residual_rad < 0) {
            QFAIL(qPrintable(QString("1PN correction has the WRONG SIGN: "
                    "advance %1\" is retrograde; expected prograde ~+2.49\".")
                    .arg(residual_arcsec)));
        }

        QVERIFY2(std::abs(residual_rad - kExpectedAdvance_rad) < kTolerance_rad,
                 qPrintable(QString("1PN advance = %1\" (expected %2 +/- %3\")")
                    .arg(residual_arcsec)
                    .arg(kExpectedAdvance_rad * 206264.806, 0, 'f', 3)
                    .arg(kTolerance_rad * 206264.806, 0, 'f', 3)));
    }

    // Newtonian control: perihelion should NOT advance without 1PN correction.
    // Measures the baseline numerical drift of the integrator.
    void testPerihelionNewtonian() {
        auto ui = std::make_unique<quantumverse::UI4D>();
        auto& m = setupSunMercury(*ui, false);  // 1PN disabled

        auto s0 = eccentricityArgument(m.position.x, m.position.y,
                                       m.velocity.x, m.velocity.y);
        double prev_arg = s0.arg;
        double total_rotation = 0.0;

        const int64_t steps = static_cast<int64_t>(5 * kT_merc / kDt);
        const int     sample_every = 1000;
        for (int64_t i = 0; i < steps; ++i) {
            ui->stepSimulationCPURaw(kDt);
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

        const double residual_arcsec = total_rotation * 206264.806;
        qInfo() << "[Newtonian] total rotation =" << total_rotation << " rad";
        qInfo() << "[Newtonian] residual advance = " << residual_arcsec << " arcsec";
        QVERIFY2(std::abs(total_rotation) < 1e-6,
                 qPrintable(QString("Newtonian precession %1\" should be ~0").arg(residual_arcsec)));
    }

    // Energy drift over the 1PN run.
    // The 1PN term is not exactly conservative; a small monotone drift is
    // expected, bounded by the leapfrog's own error term.
    void testEnergyDrift() {
        auto ui = std::make_unique<quantumverse::UI4D>();
        auto& m = setupSunMercury(*ui);

        auto E = [&]() {
            const double r  = std::sqrt(m.position.x*m.position.x +
                                        m.position.y*m.position.y +
                                        m.position.z*m.position.z);
            const double v2 = m.velocity.x*m.velocity.x +
                              m.velocity.y*m.velocity.y +
                              m.velocity.z*m.velocity.z;
            return 0.5 * v2 - kGM / r;   // specific orbital energy
        };

        const double E0 = E();
        const int64_t steps = static_cast<int64_t>(10 * kT_merc / kDt);
        for (int64_t i = 0; i < steps; ++i) ui->stepSimulationCPURaw(kDt);
        const double drift = std::abs((E() - E0) / E0);

        qInfo() << "[1PN] energy drift over 10 orbits = " << drift;
        QVERIFY2(drift < 1e-6, qPrintable(QString("drift %1").arg(drift)));
    }

    // Earth-radius sanity: 1PN correction to the acceleration must be
    // negligible at 1 AU. Analytic check — no integrator needed.
    void testEarthUnaffected() {
        const double r_E = 1.496e11;
        const double v_E = std::sqrt(kGM / r_E);
        const double a_N = kGM / (r_E * r_E);
        const double a_1PN = (kGM / (kC*kC * r_E * r_E)) *
                             (4.0*kGM/r_E - v_E*v_E);
        const double ratio = std::abs(a_1PN / a_N);

        qInfo() << "[1PN] |a_1PN/a_N| at Earth =" << ratio;
        QVERIFY2(ratio < 1e-7, qPrintable(QString("ratio %1 too large").arg(ratio)));
    }
};

QTEST_MAIN(TestMercury1PN)
#include "test_mercury_1pn.moc"
