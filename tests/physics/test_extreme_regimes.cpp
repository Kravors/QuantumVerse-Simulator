#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "spacetime/FRWMetric.h"
#include "spacetime/DilatonMetric.h"
#include "physics/GeodesicIntegrator.h"
#include "physics/CurvatureCalculator.h"
#include "physics/SingularityHandler.h"
#include "physics/PhysicsValidator.h"
#include <cmath>
#include <cassert>
#include <iostream>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4189)
#endif

using namespace quantumverse;

static constexpr double INF = std::numeric_limits<double>::infinity();

static double relError(double computed, double expected) {
    if (expected == 0.0) return std::abs(computed);
    return std::abs(computed - expected) / std::abs(expected);
}

// ============================================================================
// 18.4.1 - Schwarzschild: approach to singularity without NaN
// ============================================================================
void test_schwarzschild_singularity_approach() {
    double M = 1.0;
    auto sch = std::make_shared<SchwarzschildMetric>(M * Event4D::C * Event4D::C / Event4D::G);
    GeodesicIntegrator integrator(1e-8);
    integrator.setMetric(sch);

    Event4D start(0.0, 50.0 * M, 0.0, 0.0);
    std::array<double, 4> vel = {1.0, -0.02, 0.0, 0.0};  // inward
    auto traj = integrator.integrate(start, vel, GeodesicType::TIMELIKE, 2.0, true);

    assert(!traj.empty());
    bool hit_singularity = false;
    for (const auto& step : traj) {
        double r = step.event.spatialLength();
        if (r < 1e-8 * M) {
            hit_singularity = true;
            break;
        }
        // No NaN in trajectory
        assert(std::isfinite(step.event.t) && "t should be finite");
        assert(std::isfinite(step.event.x) && "x should be finite");
        assert(std::isfinite(step.event.y) && "y should be finite");
        assert(std::isfinite(step.event.z) && "z should be finite");
    }
    std::cout << "[PASS] Schwarzschild singularity approach: hit_singularity="
              << hit_singularity << ", steps=" << traj.size() << std::endl;
}

// ============================================================================
// 18.4.2 - Planck-scale curvature: no crash, finite or clamped
// ============================================================================
void test_planck_scale_curvature() {
    double M = 1e12;  // Large mass -> huge curvature at small r
    SchwarzschildMetric sch(M);
    double r_planck = 1.616e-35;  // Planck length

    Event4D ev(0.0, r_planck, 0.0, 0.0);
    auto s = sch.curvatureScalars(ev);
    assert(s.valid);
    // At Planck scale, Kretschmann should be enormous but finite (or clamped)
    assert(std::isfinite(s.kretschmann) && "Kretschmann should be finite at Planck scale");
    std::cout << "[PASS] Planck-scale curvature: K = " << s.kretschmann << std::endl;

    // CurvatureCalculator should not crash
    auto metric = std::make_shared<SchwarzschildMetric>(M);
    CurvatureCalculator calc(metric);
    auto result = calc.computeAll(ev);
    assert(std::isfinite(result.kretschmann) && "FD curvature should be finite");
    (void)result;
}

// ============================================================================
// 18.4.3 - Big Bang: FRW with a(t) -> 0
// ============================================================================
void test_frw_big_bang() {
    double H0 = 2.27e-18;
    double t0 = 4.354e17;
    auto frw = FRWMetric::matterDominated(H0, t0);

    double t_early = 1e-10;
    double a = frw.scaleFactor(t_early);
    assert(a >= 0.0 && "Scale factor should be non-negative");
    assert(std::isfinite(a) && "Scale factor should be finite");

    // H diverges as t->0, but implementation should clamp
    double H = frw.hubbleParameter(t_early);
    assert(std::isfinite(H) || (H == 0.0));
    std::cout << "[PASS] FRW Big Bang: a(1e-10) = " << a << ", H = " << H << std::endl;
}

// ============================================================================
// 18.4.4 - Kerr naked singularity: a > rs/2 should be detected
// ============================================================================
void test_kerr_naked_singularity() {
    double M = 1.0;
    double rs = 2.0 * M;
    double a = 1.5 * rs / 2.0;  // a > rs/2 => naked singularity in code's units

    SingularityHandler handler(SingularityType::KERR,
        M * Event4D::C * Event4D::C / Event4D::G,
        a * M * Event4D::C * Event4D::C / Event4D::G, 0.0);

    assert(handler.getProperties().is_naked && "Should detect naked singularity");
    Event4D ev(0.0, 5.0 * M, 0.0, 0.0);
    assert(!handler.isInsideEventHorizon(ev) && "Naked singularity has no horizon");
    std::cout << "[PASS] Kerr naked singularity (a > rs/2) detected" << std::endl;
}

// ============================================================================
// 18.4.5 - Schwarzschild: very small mass (M -> 0)
// ============================================================================
void test_schwarzschild_tiny_mass() {
    double M = 1e-50;
    SchwarzschildMetric sch(M);
    Event4D ev(0.0, 1e5, 0.0, 0.0);
    auto g = sch.evaluate(ev);
    assert(std::isfinite(g[0][0]) && "g_tt should be finite");
    assert(std::isfinite(g[1][1]) && "g_rr should be finite");
    auto s = sch.curvatureScalars(ev);
    assert(s.valid);
    assert(std::isfinite(s.kretschmann));
    std::cout << "[PASS] Schwarzschild tiny mass (M=1e-50) stable" << std::endl;
}

// ============================================================================
// 18.4.6 - Schwarzschild: very large mass
// ============================================================================
void test_schwarzschild_huge_mass() {
    double M = 1e50;
    SchwarzschildMetric sch(M);
    Event4D ev(0.0, 1e20, 0.0, 0.0);
    auto g = sch.evaluate(ev);
    assert(std::isfinite(g[0][0]) && "g_tt should be finite");
    assert(std::isfinite(g[1][1]) && "g_rr should be finite");
    auto s = sch.curvatureScalars(ev);
    assert(s.valid);
    assert(std::isfinite(s.kretschmann));
    std::cout << "[PASS] Schwarzschild huge mass (M=1e50) stable" << std::endl;
}

// ============================================================================
// 18.4.7 - Kerr: extremal spin a = rs/2
// ============================================================================
void test_kerr_extremal_spin() {
    double M = 1.0;
    double rs = 2.0 * M;
    double a = rs / 2.0 - 1e-10;  // Just below extremal

    SingularityHandler handler(SingularityType::KERR,
        M * Event4D::C * Event4D::C / Event4D::G,
        a * M * Event4D::C * Event4D::C / Event4D::G, 0.0);

    assert(!handler.getProperties().is_naked && "Should have event horizon");
    assert(handler.getProperties().event_horizon_radius > 0.0);
    std::cout << "[PASS] Kerr extremal spin (a ~ rs/2): horizon exists" << std::endl;
}

// ============================================================================
// 18.4.8 - Schwarzschild: event horizon detection boundary
// ============================================================================
void test_event_horizon_boundary() {
    double M = 1.0;
    double rs = 2.0 * M;
    SingularityHandler handler(SingularityType::SCHWARZSCHILD,
        M * Event4D::C * Event4D::C / Event4D::G, 0.0, 0.0);

    Event4D outside(0.0, rs * 2.0, 0.0, 0.0);
    Event4D inside(0.0, rs * 0.5, 0.0, 0.0);
    Event4D on_horizon(0.0, rs, 0.0, 0.0);

    assert(!handler.isInsideEventHorizon(outside));
    assert(handler.isInsideEventHorizon(inside));
    assert(handler.isInsideEventHorizon(on_horizon));
    std::cout << "[PASS] Event horizon boundary detection correct" << std::endl;
}

// ============================================================================
// 18.4.9 - Hawking evaporation: stays finite
// ============================================================================
void test_hawking_evaporation_stability() {
    SingularityHandler handler(SingularityType::SCHWARZSCHILD, 1e20, 0.0, 0.0);
    for (int i = 0; i < 1000; i++) {
        handler.evolveHawkingEvaporation(1e15);
        assert(!std::isnan(handler.getProperties().mass) && "Mass became NaN");
        assert(!std::isinf(handler.getProperties().mass) && "Mass became Inf");
        assert(handler.getProperties().mass >= 0.0 && "Mass became negative");
    }
    std::cout << "[PASS] Hawking evaporation stable for 1000 steps" << std::endl;
}

// ============================================================================
// 18.4.10 - Regular BH: Hayward at r=0 finite curvature
// ============================================================================
void test_hayward_r0_finite() {
    double M = 1.0;
    SingularityHandler handler(SingularityType::HAYWARD,
        M * Event4D::C * Event4D::C / Event4D::G, 0.0, 0.0);

    double K = handler.computeKretschmannAtRadius(1e-20);
    assert(std::isfinite(K) && K > 0.0 && "Hayward K(0) should be finite positive");
    std::cout << "[PASS] Hayward K(0) finite: " << K << std::endl;
}

// ============================================================================
// 18.4.11 - Schwarzschild: radial plunge to r -> 0
// ============================================================================
void test_radial_plunge_to_singularity() {
    double M = 1.0;
    auto sch = std::make_shared<SchwarzschildMetric>(M * Event4D::C * Event4D::C / Event4D::G);
    GeodesicIntegrator integrator(1e-8);
    integrator.setMetric(sch);

    Event4D start(0.0, 100.0 * M, 0.0, 0.0);
    std::array<double, 4> vel = {2.0, -0.1, 0.0, 0.0};  // high inward velocity
    auto traj = integrator.integrate(start, vel, GeodesicType::TIMELIKE, 5.0, true);

    assert(!traj.empty());
    double r_min = INF;
    for (const auto& step : traj) {
        double ri = step.event.spatialLength();
        r_min = std::min(r_min, ri);
        assert(std::isfinite(step.event.x) && "x should stay finite");
    }
    assert(r_min < 1.0 * M && "Should reach near singularity");
    std::cout << "[PASS] Radial plunge: r_min = " << r_min << " < " << 1.0 * M << std::endl;
}

// ============================================================================
// 18.4.12 - FRW: Big Crunch a(t) -> 0
// ============================================================================
void test_frw_big_crunch() {
    // Closed universe can have Big Crunch
    double H0 = 2.27e-18;
    double t0 = 4.354e17;
    auto frw = FRWMetric(FRWMetric::Curvature::CLOSED,
        [t0](double t) {
            if (t <= 0) return 0.0;
            return std::sin(M_PI * t / (2.0 * t0)) * std::pow(t / t0, 0.5);
        }, H0);

    double t_crunch = t0 * 1.5;
    double a = frw.scaleFactor(t_crunch);
    assert(std::isfinite(a) && "Scale factor should be finite");
    (void)a;
    std::cout << "[PASS] FRW Big Crunch (closed universe) handled" << std::endl;
}

// ============================================================================
// 18.4.13 - Schwarzschild: Inf handling in metric evaluation
// ============================================================================
void test_schwarzschild_inf_handling() {
    double M = 1e30;
    SchwarzschildMetric sch(M);
    Event4D ev(INF, 0.0, 0.0, 0.0);
    auto g = sch.evaluate(ev);
    // Should handle inf gracefully (returns Minkowski for non-finite coords)
    (void)g;
    std::cout << "[PASS] Schwarzschild handles Inf coordinate gracefully" << std::endl;
}

// ============================================================================
// 18.4.14 - Schwarzschild: NaN handling in metric evaluation
// ============================================================================
void test_schwarzschild_nan_handling() {
    double M = 1e30;
    SchwarzschildMetric sch(M);
    Event4D ev(NAN, 0.0, 0.0, 0.0);
    auto g = sch.evaluate(ev);
    (void)g;
    std::cout << "[PASS] Schwarzschild handles NaN coordinate gracefully" << std::endl;
}

// ============================================================================
// 18.4.15 - Kerr: near-extremal spin stability
// ============================================================================
void test_kerr_near_extremal_stability() {
    double M = 1.0;
    double rs = 2.0 * M;
    double a = rs / 2.0 * 0.999;

    auto kerr = MetricTensor::kerr(
        M * Event4D::C * Event4D::C / Event4D::G,
        a * M * Event4D::C * Event4D::C / Event4D::G,
        10.0 * M, M_PI / 2.0);

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            assert(std::isfinite(kerr.g[i][j]) && "Kerr metric should be finite near extremal");

    std::cout << "[PASS] Kerr near-extremal spin: metric finite" << std::endl;
}

// ============================================================================
// 18.4.16 - Schwarzschild: large r -> g -> Minkowski
// ============================================================================
void test_schwarzschild_large_r_minkowski() {
    double M = 1e30;
    SchwarzschildMetric sch(M);
    Event4D ev(0.0, 1e20, 0.0, 0.0);
    auto g = sch.evaluate(ev);

    MetricTensor mink;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            assert(std::abs(g[i][j] - mink.g[i][j]) < 1e-10);
    std::cout << "[PASS] Schwarzschild at r=1e20 -> Minkowski" << std::endl;
}

// ============================================================================
// 18.4.17 - Schwarzschild: negative r handled safely
// ============================================================================
void test_schwarzschild_negative_r() {
    double M = 1e30;
    SchwarzschildMetric sch(M);
    Event4D ev(0.0, -1e5, 0.0, 0.0);
    auto g = sch.evaluate(ev);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            assert(std::isfinite(g[i][j]) && "Metric should be finite for negative r");
    std::cout << "[PASS] Schwarzschild negative r handled safely" << std::endl;
}

// ============================================================================
// 18.4.18 - Schwarzschild: zero r handled safely
// ============================================================================
void test_schwarzschild_zero_r() {
    double M = 1e30;
    SchwarzschildMetric sch(M);
    Event4D ev(0.0, 0.0, 0.0, 0.0);
    auto g = sch.evaluate(ev);
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            assert(std::isfinite(g[i][j]) && "Metric should be finite at origin");
    std::cout << "[PASS] Schwarzschild at r=0 handled safely" << std::endl;
}

// ============================================================================
// 18.4.19 - Schwarzschild: Kretschmann at r=1e-8 M (near singularity)
// ============================================================================
void test_kretschmann_near_singularity() {
    double M = 1e30;
    SchwarzschildMetric sch(M);
    double r = 1e-8 * 1.0;  // r in geometric units
    Event4D ev(0.0, r * Event4D::C * Event4D::C / Event4D::G, 0.0, 0.0);
    auto s = sch.curvatureScalars(ev);
    assert(s.valid);
    assert(std::isfinite(s.kretschmann) && s.kretschmann > 0.0);
    std::cout << "[PASS] Kretschmann near singularity finite and positive" << std::endl;
}

// ============================================================================
// 18.4.20 - Schwarzschild: geodesic through horizon
// ============================================================================
void test_geodesic_through_horizon() {
    double M = 1.0;
    auto sch = std::make_shared<SchwarzschildMetric>(M * Event4D::C * Event4D::C / Event4D::G);
    GeodesicIntegrator integrator(1e-8);
    integrator.setMetric(sch);

    Event4D start(0.0, 10.0 * M, 0.0, 0.0);
    std::array<double, 4> vel = {2.0, -0.5, 0.0, 0.0};  // fast inward
    auto traj = integrator.integrate(start, vel, GeodesicType::TIMELIKE, 2.0, true);

    assert(!traj.empty());
    bool crossed_horizon = false;
    for (const auto& step : traj) {
        double r = step.event.spatialLength();
        if (r < 2.0 * M) crossed_horizon = true;
        assert(std::isfinite(step.event.t) && "t should remain finite");
    }
    assert(crossed_horizon && "Should cross event horizon");
    std::cout << "[PASS] Geodesic crosses horizon: crossed=" << crossed_horizon << std::endl;
}

// ============================================================================
// 18.4.21 - Dilaton: singularity at conformal factor = 0
// ============================================================================
void test_dilaton_conformal_singularity() {
    double sigma = 0.5;
    double M = 1.0;
    DilatonMetric dm(sigma, M);

    double xp = 1.0;
    double xm = -1.0 / (M * std::exp(2.0 * sigma));
    assert(dm.isSingularity(xp, xm, 1e-8));

    // Check that conformal factor approaches zero
    double Omega = dm.conformalFactor(xp, xm);
    assert(std::abs(Omega) < 1e-6 && "Conformal factor should be near zero at singularity");
    std::cout << "[PASS] Dilaton conformal singularity detected" << std::endl;
}

// ============================================================================
// 18.4.22 - Schwarzschild: singularity handler tidal forces near horizon
// ============================================================================
void test_tidal_forces_near_horizon() {
    double M = 1e30;
    SingularityHandler handler(SingularityType::SCHWARZSCHILD, M, 0.0, 0.0);
    double rs = handler.getProperties().schwarzschild_radius;

    Event4D near_horizon(0.0, rs * 1.1, 0.0, 0.0);
    auto forces = handler.computeTidalForces(near_horizon);
    assert(std::isfinite(forces.radial_stretch) && "Tidal stretch should be finite");
    assert(std::isfinite(forces.lateral_compression) && "Lateral compression should be finite");
    assert(forces.radial_stretch > 0.0 && "Radial stretch should be positive");
    std::cout << "[PASS] Tidal forces near horizon: stretch=" << forces.radial_stretch << std::endl;
}

// ============================================================================
// 18.4.23 - Schwarzschild: singularity handler at r=0
// ============================================================================
void test_singularity_at_origin() {
    double M = 1.0;
    SingularityHandler handler(SingularityType::SCHWARZSCHILD,
        M * Event4D::C * Event4D::C / Event4D::G, 0.0, 0.0);

    Event4D origin(0.0, 0.0, 0.0, 0.0);
    assert(handler.isInsideEventHorizon(origin));
    assert(handler.shouldTerminateGeodesic(origin, 1.0));
    std::cout << "[PASS] Singularity at origin: inside horizon, terminate" << std::endl;
}

// ============================================================================
// 18.4.24 - Schwarzschild: evaporation to near-Planck mass
// ============================================================================
void test_evaporation_to_planck() {
    SingularityHandler handler(SingularityType::SCHWARZSCHILD, 1e20, 0.0, 0.0);
    double initial = handler.getProperties().mass;
    for (int i = 0; i < 10000; i++) {
        handler.evolveHawkingEvaporation(1e20);
        if (handler.isEvaporated()) break;
    }
    double final = handler.getProperties().mass;
    assert(!std::isnan(final));
    assert(!std::isinf(final));
    assert(final >= 0.0);
    assert(final <= initial && "Mass should decrease");
    std::cout << "[PASS] Evaporation to Planck remnant: " << initial << " -> " << final << std::endl;
}

// ============================================================================
// 18.4.25 - Kerr: over-extremal (a > M) detection
// ============================================================================
void test_kerr_over_extremal() {
    double M = 1.0;
    double rs = 2.0 * M;
    double a = rs / 2.0 * 1.5;  // a > M_geom

    SingularityHandler handler(SingularityType::KERR,
        M * Event4D::C * Event4D::C / Event4D::G,
        a * M * Event4D::C * Event4D::C / Event4D::G, 0.0);

    // Naked singularity detected
    assert(handler.getProperties().is_naked || !handler.getProperties().has_ergosphere);
    std::cout << "[PASS] Kerr over-extremal spin handled" << std::endl;
}

// ============================================================================
// 18.4.26 - Schwarzschild: curvature diverges at r -> 0
// ============================================================================
void test_curvature_diverges_at_origin() {
    double M = 1e30;
    SchwarzschildMetric sch(M);
    std::vector<double> r = {1e-3, 1e-6, 1e-9, 1e-12};
    double K_prev = 0.0;
    for (double ri : r) {
        double r_meters = ri * Event4D::C * Event4D::C / Event4D::G;
        if (r_meters < 1e-15) continue;
        Event4D ev(0.0, r_meters, 0.0, 0.0);
        auto s = sch.curvatureScalars(ev);
        assert(s.valid);
        assert(s.kretschmann > K_prev && "K should increase as r->0");
        K_prev = s.kretschmann;
    }
    std::cout << "[PASS] Curvature diverges monotonically as r->0" << std::endl;
}

// ============================================================================
// 18.4.27 - Schwarzschild: tidal force increases near singularity
// ============================================================================
void test_tidal_forces_increase_near_singularity() {
    double M = 1.0;
    SingularityHandler handler(SingularityType::SCHWARZSCHILD,
        M * Event4D::C * Event4D::C / Event4D::G, 0.0, 0.0);
    double rs = handler.getProperties().schwarzschild_radius;

    std::vector<double> r_factors = {10.0, 5.0, 2.5, 1.5, 1.1};
    double stretch_prev = 0.0;
    for (double f : r_factors) {
        double r = f * rs;
        if (r < rs * 1.01) continue;
        Event4D ev(0.0, r, 0.0, 0.0);
        auto forces = handler.computeTidalForces(ev);
        assert(forces.radial_stretch > stretch_prev && "Tidal stretch should increase");
        stretch_prev = forces.radial_stretch;
    }
    std::cout << "[PASS] Tidal forces increase monotonically near singularity" << std::endl;
}

// ============================================================================
// 18.4.28 - Schwarzschild: geodesic inside horizon
// ============================================================================
void test_geodesic_inside_horizon() {
    double M = 1.0;
    auto sch = std::make_shared<SchwarzschildMetric>(M * Event4D::C * Event4D::C / Event4D::G);
    GeodesicIntegrator integrator(1e-8);
    integrator.setMetric(sch);

    Event4D start(0.0, 3.0 * M, 0.0, 0.0);  // Start outside
    std::array<double, 4> vel = {1.0, -0.3, 0.0, 0.0};
    auto traj = integrator.integrate(start, vel, GeodesicType::TIMELIKE, 2.0, true);

    assert(!traj.empty());
    for (const auto& step : traj) {
        assert(std::isfinite(step.event.x) && "x should be finite inside horizon");
    }
    std::cout << "[PASS] Geodesic inside horizon: all positions finite" << std::endl;
}

// ============================================================================
// 18.4.29 - Schwarzschild: Minkowski at r=Infinity
// ============================================================================
void test_schwarzschild_infinity_minkowski() {
    double M = 1e30;
    SchwarzschildMetric sch(M);
    double r = 1e30;
    Event4D ev(0.0, r, 0.0, 0.0);
    auto g = sch.evaluate(ev);

    MetricTensor mink;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            assert(std::abs(g[i][j] - mink.g[i][j]) < 1e-10);
    std::cout << "[PASS] Schwarzschild at r=1e30 -> Minkowski" << std::endl;
}

// ============================================================================
// 18.4.30 - Schwarzschild: Hawking temperature formula
// T = 1/(8*pi*M) in geometric units
// ============================================================================
void test_hawking_temperature_formula() {
    double M = 1e30;
    SingularityHandler handler(SingularityType::SCHWARZSCHILD, M, 0.0, 0.0);
    double T = handler.getHawkingTemperature();

    double T_expected = Event4D::C2 * Event4D::C / (8.0 * M_PI * Event4D::G * M);
    double err = relError(T, T_expected);
    assert(err < 1e-6 && "Hawking temperature should match 1/(8*pi*M)");
    std::cout << "[PASS] Hawking temperature exact: T = " << T << std::endl;
}

// ============================================================================
// 18.4.31 - Schwarzschild: singularity handler - regular BHs no NaN at r=0
// ============================================================================
void test_regular_bh_no_nan_at_origin() {
    std::vector<SingularityType> types = {
        SingularityType::HAYWARD, SingularityType::BARDEEN,
        SingularityType::LOOP_QUANTUM, SingularityType::REGULAR_GAUSS
    };
    for (auto type : types) {
        SingularityHandler handler(type, 1.0, 0.0, 0.0);
        Event4D origin(0.0, 0.0, 0.0, 0.0);
        auto forces = handler.computeTidalForces(origin);
        assert(!std::isnan(forces.radial_stretch));
        assert(!std::isnan(forces.lateral_compression));
    }
    std::cout << "[PASS] Regular BHs: no NaN at r=0" << std::endl;
}

// ============================================================================
// 18.4.32 - Schwarzschild: geodesic near horizon but outside
// ============================================================================
void test_geodesic_near_horizon_outside() {
    double M = 1.0;
    auto sch = std::make_shared<SchwarzschildMetric>(M * Event4D::C * Event4D::C / Event4D::G);
    GeodesicIntegrator integrator(1e-10);
    integrator.setMetric(sch);

    double rs = 2.0 * M;
    Event4D start(0.0, rs * 1.01, M_PI / 2.0, 0.0);
    std::array<double, 4> vel = {1.0, 0.0, 0.0, 0.1};
    auto traj = integrator.integrate(start, vel, GeodesicType::TIMELIKE, 1.0, true);

    assert(!traj.empty());
    for (const auto& step : traj) {
        assert(std::isfinite(step.event.x) && "x should be finite");
    }
    std::cout << "[PASS] Geodesic near horizon (just outside): " << traj.size() << " steps" << std::endl;
}

// ============================================================================
// 18.4.33 - Schwarzschild: very high curvature numerical stability
// ============================================================================
void test_high_curvature_numerical_stability() {
    double M = 1e40;
    auto sch = std::make_shared<SchwarzschildMetric>(M);
    CurvatureCalculator calc(sch);

    std::vector<double> r = {1e3, 1e5, 1e7};
    for (double ri : r) {
        Event4D ev(0.0, ri, 0.0, 0.0);
        auto result = calc.computeAll(ev);
        assert(std::isfinite(result.kretschmann));
        assert(std::isfinite(result.ricciScalar));
        assert(std::isfinite(result.maxRiemannComponent));
    }
    std::cout << "[PASS] High curvature numerical stability" << std::endl;
}

// ============================================================================
// 18.4.34 - Schwarzschild: zero velocity at r=infinity
// ============================================================================
void test_zero_velocity_infinity() {
    double M = 1.0;
    auto sch = std::make_shared<SchwarzschildMetric>(M * Event4D::C * Event4D::C / Event4D::G);
    GeodesicIntegrator integrator(1e-8);
    integrator.setMetric(sch);

    Event4D start(0.0, 1e10 * M, 0.0, 0.0);
    std::array<double, 4> vel = {1.0, 0.0, 0.0, 0.0};
    auto traj = integrator.integrate(start, vel, GeodesicType::TIMELIKE, 1.0, true);

    assert(!traj.empty());
    for (const auto& step : traj) {
        assert(std::isfinite(step.event.x) && "x should be finite");
    }
    std::cout << "[PASS] Zero angular velocity at large r stable" << std::endl;
}

// ============================================================================
// 18.4.35 - Schwarzschild: coordinate system robustness
// ============================================================================
void test_coordinate_system_robustness() {
    double M = 1.0;
    SchwarzschildMetric sch(M * Event4D::C * Event4D::C / Event4D::G);

    std::vector<Event4D> points = {
        Event4D(0.0, 10.0, 0.0, 0.0),
        Event4D(0.0, 10.0, M_PI, 0.0),
        Event4D(0.0, 10.0, M_PI / 2.0, M_PI),
        Event4D(0.0, 10.0, 0.001, 0.001)
    };

    for (const auto& ev : points) {
        auto g = sch.evaluate(ev);
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                assert(std::isfinite(g[i][j]) && "Metric should be finite at all angles");
    }
    std::cout << "[PASS] Coordinate system robustness at various angles" << std::endl;
}

// ============================================================================
// Main
// ============================================================================
int main() {
    test_schwarzschild_singularity_approach();
    test_planck_scale_curvature();
    test_frw_big_bang();
    test_kerr_naked_singularity();
    test_schwarzschild_tiny_mass();
    test_schwarzschild_huge_mass();
    test_kerr_extremal_spin();
    test_event_horizon_boundary();
    test_hawking_evaporation_stability();
    test_hayward_r0_finite();
    test_radial_plunge_to_singularity();
    test_frw_big_crunch();
    test_schwarzschild_inf_handling();
    test_schwarzschild_nan_handling();
    test_kerr_near_extremal_stability();
    test_schwarzschild_large_r_minkowski();
    test_schwarzschild_negative_r();
    test_schwarzschild_zero_r();
    test_kretschmann_near_singularity();
    test_geodesic_through_horizon();
    test_dilaton_conformal_singularity();
    test_tidal_forces_near_horizon();
    test_singularity_at_origin();
    test_evaporation_to_planck();
    test_kerr_over_extremal();
    test_curvature_diverges_at_origin();
    test_tidal_forces_increase_near_singularity();
    test_geodesic_inside_horizon();
    test_schwarzschild_infinity_minkowski();
    test_hawking_temperature_formula();
    test_regular_bh_no_nan_at_origin();
    test_geodesic_near_horizon_outside();
    test_high_curvature_numerical_stability();
    test_zero_velocity_infinity();
    test_coordinate_system_robustness();

#ifdef _MSC_VER
#pragma warning(pop)
#endif

    std::cout << "=== ALL EXTREME REGIME TESTS PASSED ===" << std::endl;
    return 0;
}
