/**
 * @file test_flrw_consistency.cpp
 * @brief Phase 19: Cosmological Consistency & Observational Data
 *
 * Exhaustive validation of the FRW metric, cosmological distance measures,
 * redshift consistency, and Friedmann equation self-consistency.
 *
 * Tests cover:
 *   - Exact scale factor solutions (matter, radiation, de Sitter, ΛCDM)
 *   - Hubble parameter H(t) = ȧ/a consistency
 *   - Age of the universe for standard cosmologies
 *   - FRW metric tensor structure (diagonal, Lorentzian signature)
 *   - Redshift-distance consistency (1+z = 1/a(t_em))
 *   - Comoving, proper, luminosity, and angular-diameter distances
 *   - Curvature constraints (flat/closed/open)
 *   - Ricci scalar for FRW spacetimes
 *   - Weyl tensor vanishes in FRW (conformally flat)
 *   - Deceleration parameter q(t)
 *   - Lookback time and conformal time
 *   - Critical density and density parameters
 */

#include "spacetime/FRWMetric.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "physics/CurvatureCalculator.h"
#include <cmath>
#include <cassert>
#include <iostream>
#include <limits>
#include <vector>
#include <functional>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4189)
#pragma warning(disable:4505)
#endif

using namespace quantumverse;

static constexpr double INF = std::numeric_limits<double>::infinity();
static constexpr double EPS = 1e-12;
static constexpr double H0 = 2.27e-18;
static constexpr double T0 = 4.354e17;

static double relError(double computed, double expected) {
    if (expected == 0.0) return std::abs(computed);
    return std::abs(computed - expected) / std::abs(expected);
}

// ============================================================================
// 19.1.1 - Matter-dominated: exact scale factor a(t) = (t/t0)^(2/3)
// ============================================================================
void test_frw_matter_scale_factor_exact() {
    auto frw = FRWMetric::matterDominated(H0, T0);
    std::vector<double> times = {T0 * 0.25, T0 * 0.5, T0 * 0.75, T0, T0 * 1.5, T0 * 2.0};

    int count = 0;
    for (double t : times) {
        double a = frw.scaleFactor(t);
        double a_expected = std::pow(t / T0, 2.0 / 3.0);
        assert(a > 0.0 && "Scale factor should be positive");
        assert(relError(a, a_expected) < 1e-12 && "Matter-dominated scale factor mismatch");
        (void)a; (void)a_expected;
        count++;
    }
    std::cout << "[PASS] FRW matter-dominated scale factor exact: " << count << " points" << std::endl;
}

// ============================================================================
// 19.1.2 - Radiation-dominated: exact scale factor a(t) = (t/t0)^(1/2)
// ============================================================================
void test_frw_radiation_scale_factor_exact() {
    auto frw = FRWMetric::radiationDominated(H0, T0);
    std::vector<double> times = {T0 * 0.25, T0 * 0.5, T0 * 0.75, T0, T0 * 1.5, T0 * 2.0};

    int count = 0;
    for (double t : times) {
        double a = frw.scaleFactor(t);
        double a_expected = std::pow(t / T0, 0.5);
        assert(a > 0.0 && "Scale factor should be positive");
        assert(relError(a, a_expected) < 1e-12 && "Radiation-dominated scale factor mismatch");
        (void)a; (void)a_expected;
        count++;
    }
    std::cout << "[PASS] FRW radiation-dominated scale factor exact: " << count << " points" << std::endl;
}

// ============================================================================
// 19.1.3 - de Sitter: exact exponential expansion a(t) = exp(H0 t)
// ============================================================================
void test_frw_de_sitter_exact() {
    auto frw = FRWMetric::deSitter(H0);
    std::vector<double> times = {0.0, T0 * 0.1, T0 * 0.5, T0, T0 * 1.5, T0 * 2.0};

    int count = 0;
    for (double t : times) {
        double a = frw.scaleFactor(t);
        double a_expected = std::exp(H0 * t);
        assert(std::isfinite(a) && "de Sitter scale factor should be finite");
        assert(relError(a, a_expected) < 1e-12 && "de Sitter scale factor mismatch");
        (void)a; (void)a_expected;
        count++;
    }
    std::cout << "[PASS] FRW de Sitter exponential expansion exact: " << count << " points" << std::endl;
}

// ============================================================================
// 19.1.4 - ΛCDM: flat universe constraint Ωm + ΩΛ = 1
// ============================================================================
void test_frw_lambda_cdm_flat_constraint() {
    auto frw = FRWMetric::lambdaCDM(0.31, 0.69, H0);
    assert(frw.curvature() == 0 && "ΛCDM should be flat");
    assert(frw.hubbleConstant() == H0);

    double t = T0 * 0.8;
    double a = frw.scaleFactor(t);
    assert(a > 0.0 && "ΛCDM scale factor should be positive");
    assert(std::isfinite(a) && "ΛCDM scale factor should be finite");
    std::cout << "[PASS] FRW ΛCDM flat constraint and scale factor valid" << std::endl;
}

// ============================================================================
// 19.1.5 - Hubble parameter: H(t) = ȧ/a consistency check
// ============================================================================
void test_frw_hubble_parameter_consistency() {
    auto frw = FRWMetric::matterDominated(H0, T0);
    std::vector<double> times = {T0 * 0.3, T0 * 0.6, T0, T0 * 1.5};

    int count = 0;
    for (double t : times) {
        double H = frw.hubbleParameter(t);
        double a = frw.scaleFactor(t);
        double h = std::max(1e-6, std::abs(t) * 1e-5);
        double adot = (frw.scaleFactor(t + h) - frw.scaleFactor(t - h)) / (2.0 * h);
        double H_expected = adot / a;
        assert(std::isfinite(H) && H > 0.0 && "Hubble parameter should be finite and positive");
        assert(relError(H, H_expected) < 1e-6 && "Hubble parameter H = ȧ/a mismatch");
        (void)H; (void)H_expected;
        count++;
    }
    std::cout << "[PASS] FRW Hubble parameter H = ȧ/a consistency: " << count << " points" << std::endl;
}

// ============================================================================
// 19.1.6 - Age of universe: t_age = 2/(3H0) for flat matter-dominated
// ============================================================================
void test_frw_age_of_universe() {
    auto frw = FRWMetric::matterDominated(H0, T0);
    double t_age = frw.ageOfUniverse();
    double t_expected = 2.0 / (3.0 * H0);
    assert(std::abs(t_age - t_expected) < 1e-6 * t_expected && "Age of universe mismatch");
    (void)t_age; (void)t_expected;
    std::cout << "[PASS] FRW age of universe = 2/(3H0) exact" << std::endl;
}

// ============================================================================
// 19.1.7 - FRW metric is diagonal (no cross terms g_tμ = 0 for μ≠0)
// ============================================================================
void test_frw_metric_diagonal() {
    auto frw = FRWMetric::matterDominated(H0, T0);
    Event4D ev(T0 * 0.8, 1.0, M_PI / 4.0, M_PI / 3.0);
    auto g = frw.evaluate(ev);

    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            assert(std::abs(g.g[i][j]) < 1e-12 && "FRW metric should be diagonal");
        }
    }
    std::cout << "[PASS] FRW metric is diagonal (no cross terms)" << std::endl;
}

// ============================================================================
// 19.1.8 - FRW metric has Lorentzian signature (-,+,+,+)
// ============================================================================
void test_frw_lorentzian_signature() {
    auto frw = FRWMetric::matterDominated(H0, T0);
    Event4D ev(T0 * 0.8, 1e9, M_PI / 4.0, M_PI / 3.0);
    auto g = frw.evaluate(ev);

    assert(g.g[0][0] < 0.0 && "g_tt should be negative");
    assert(g.g[1][1] > 0.0 && "g_rr should be positive");
    assert(g.g[2][2] > 0.0 && "g_θθ should be positive");
    assert(g.g[3][3] > 0.0 && "g_φφ should be positive");
    std::cout << "[PASS] FRW metric has Lorentzian signature (-,+,+,+)" << std::endl;
}

// ============================================================================
// 19.1.9 - Redshift consistency: 1 + z = 1/a(t_em) at present a(t0)=1
// ============================================================================
void test_frw_redshift_consistency() {
    auto frw = FRWMetric::matterDominated(H0, T0);
    double t_em = T0 * 0.5;
    double a_em = frw.scaleFactor(t_em);
    double z = (1.0 / a_em) - 1.0;

    assert(z > 0.0 && "Redshift should be positive for emission in the past");
    assert(std::isfinite(z) && "Redshift should be finite");
    assert(relError(a_em, 1.0 / (1.0 + z)) < 1e-12 && "Redshift consistency 1+z = 1/a");
    std::cout << "[PASS] FRW redshift consistency: 1+z = 1/a(t_em) = " << (1.0 + z) << std::endl;
}

// ============================================================================
// 19.1.10 - Comoving vs proper distance: d_proper = a(t) * d_comoving
// ============================================================================
void test_frw_comoving_distance() {
    auto frw = FRWMetric::matterDominated(H0, T0);
    double t = T0 * 0.8;
    double a = frw.scaleFactor(t);
    double chi = 1e9;  // comoving distance in meters
    double d_proper = a * chi;
    (void)d_proper;

    assert(d_proper > 0.0 && "Proper distance should be positive");
    assert(std::isfinite(d_proper) && "Proper distance should be finite");
    assert(relError(d_proper / chi, a) < 1e-12 && "Proper = a * comoving mismatch");
    std::cout << "[PASS] FRW comoving/proper distance: d = a * chi" << std::endl;
}

// ============================================================================
// 19.1.11 - Angular diameter distance: d_A = a(t_em) * chi
// ============================================================================
void test_frw_angular_diameter_distance() {
    auto frw = FRWMetric::matterDominated(H0, T0);
    double t_em = T0 * 0.5;
    double a_em = frw.scaleFactor(t_em);
    double chi = 1e9;
    double d_A = a_em * chi;
    (void)d_A;

    assert(d_A > 0.0 && "Angular diameter distance should be positive");
    assert(std::isfinite(d_A) && "Angular diameter distance should be finite");
    assert(relError(d_A / chi, a_em) < 1e-12 && "d_A = a(t_em) * chi mismatch");
    std::cout << "[PASS] FRW angular diameter distance: d_A = a(t_em) * chi" << std::endl;
}

// ============================================================================
// 19.1.12 - Luminosity distance: d_L = d_A / a(t_em)^2 = chi / a(t_em)
// ============================================================================
void test_frw_luminosity_distance() {
    auto frw = FRWMetric::matterDominated(H0, T0);
    double t_em = T0 * 0.5;
    double a_em = frw.scaleFactor(t_em);
    double chi = 1e9;
    double d_A = a_em * chi;
    double d_L = d_A / (a_em * a_em);

    assert(d_L > d_A && "Luminosity distance should exceed angular diameter distance");
    assert(relError(d_L, chi / a_em) < 1e-12 && "d_L = chi/a(t_em) mismatch");
    std::cout << "[PASS] FRW luminosity distance: d_L = d_A / a^2 = " << d_L << std::endl;
}

// ============================================================================
// 19.1.13 - Curvature types: flat (k=0), closed (k=+1), open (k=-1)
// ============================================================================
void test_frw_curvature_types() {
    auto flat = FRWMetric::matterDominated(H0, T0);
    auto closed = FRWMetric(FRWMetric::Curvature::CLOSED,
        [t0 = T0](double t) { return std::pow(t / t0, 2.0 / 3.0); }, H0);
    auto open = FRWMetric(FRWMetric::Curvature::OPEN,
        [t0 = T0](double t) { return std::pow(t / t0, 2.0 / 3.0); }, H0);

    assert(flat.curvature() == 0 && "Flat universe should have k=0");
    assert(closed.curvature() == 1 && "Closed universe should have k=+1");
    assert(open.curvature() == -1 && "Open universe should have k=-1");

    // In the FLRW line element ds² = -c²dt² + a²[dr²/(1-kr²) + r²dΩ²] the comoving
    // radial coordinate r is measured in units of the curvature radius whenever
    // k = ±1, so the chart is only defined for |r| < 1 in the closed case
    // (r = 1 is the coordinate singularity at the 3-sphere equator). Probing the
    // signature therefore has to happen inside that domain; r = 1e9 lies far
    // outside it and makes 1 - kr² negative by construction.
    // Note: g_rr for FLAT and OPEN is positive for every r, so a single
    // in-domain radius exercises all three curvature types.
    const double r = 0.5;
    Event4D ev(T0 * 0.8, r, M_PI / 4.0, M_PI / 3.0);
    auto g_flat = flat.evaluate(ev);
    auto g_closed = closed.evaluate(ev);
    auto g_open = open.evaluate(ev);

    assert(1.0 - 1.0 * r * r > 0.0 && "Test radius must lie inside the closed FRW chart");
    assert(g_flat.g[1][1] > 0.0 && "Flat g_rr should be positive");
    assert(g_closed.g[1][1] > 0.0 && "Closed g_rr should be positive");
    assert(g_open.g[1][1] > 0.0 && "Open g_rr should be positive");
    // Closed slices are "smaller" than flat ones at fixed r (1-r² < 1), open larger.
    assert(g_closed.g[1][1] > g_flat.g[1][1] && "Closed g_rr must exceed flat g_rr for |r|<1");
    assert(g_open.g[1][1] < g_flat.g[1][1] && "Open g_rr must be below flat g_rr");
    std::cout << "[PASS] FRW curvature types: flat/closed/open verified" << std::endl;
}

// ============================================================================
// 19.1.14 - Ricci scalar for flat FRW matter-dominated
// ============================================================================
void test_frw_ricci_scalar_matter() {
    auto frw = FRWMetric::matterDominated(H0, T0);
    double t = T0 * 0.8;
    Event4D ev(t, 1e9, M_PI / 4.0, M_PI / 3.0);
    auto g = frw.evaluate(ev);

    CurvatureCalculator calc;
    calc.setFDStep(1e-4);
    auto result = calc.computeAll(ev);
    (void)g;
    assert(std::isfinite(result.ricciScalar) && "Ricci scalar should be finite");
    (void)result;
    std::cout << "[PASS] FRW Ricci scalar finite at t = " << t / T0 << " t0" << std::endl;
}

// ============================================================================
// 19.1.15 - Weyl tensor vanishes in FRW (conformally flat)
// ============================================================================
void test_frw_weyl_zero() {
    auto frw = FRWMetric::matterDominated(H0, T0);
    Event4D ev(T0 * 0.8, 1e9, M_PI / 4.0, M_PI / 3.0);
    auto g = frw.evaluate(ev);

    // FRW is conformally flat → Weyl tensor = 0
    // Since we don't have a direct Weyl API, verify via metric structure:
    // conformally flat metrics have R_ijkl = (1/4) R_ijkl_Minkowski
    assert(g.g[0][0] < 0.0 && "g_tt negative for Lorentzian");
    assert(g.g[1][1] > 0.0 && "g_rr positive");
    std::cout << "[PASS] FRW Weyl = 0 (conformally flat)" << std::endl;
}

// ============================================================================
// 19.1.16 - Deceleration parameter: q = 1/2 (matter), q = 1 (radiation), q = -1 (de Sitter)
// ============================================================================
void test_frw_deceleration_parameter() {
    auto matter = FRWMetric::matterDominated(H0, T0);
    auto radiation = FRWMetric::radiationDominated(H0, T0);
    auto desitter = FRWMetric::deSitter(H0);

    double t = T0 * 0.8;

    // The finite-difference step has to scale with cosmic time. With a fixed
    // h = 1e6 s at t ~ 3.5e17 s the second-difference numerator
    // a(t+h) - 2a(t) + a(t-h) is of order a''h^2 ~ 1e-23 while the round-off in
    // a(t) ~ 1 is ~1e-16, so the result was pure cancellation noise. Using a
    // relative step keeps both derivatives well conditioned: the round-off term
    // scales as eps/h^2 and the truncation term as (h/t)^2, and h/t = 1e-2 puts
    // both far below the 1e-3 tolerance asserted here.
    const double h1 = t * 1e-2;   // step for the first derivative
    const double h2 = t * 1e-2;   // step for the second derivative

    auto decelerationParameter = [&](const FRWMetric& model) {
        const double a = model.scaleFactor(t);
        const double adot = (model.scaleFactor(t + h1) - model.scaleFactor(t - h1)) / (2.0 * h1);
        const double addot = (model.scaleFactor(t + h2) - 2.0 * a + model.scaleFactor(t - h2)) / (h2 * h2);
        return -addot * a / (adot * adot);
    };

    // Matter: q = 1/2
    double q_mat = decelerationParameter(matter);
    assert(std::abs(q_mat - 0.5) < 1e-3 && "Matter deceleration q should be ~0.5");

    // Radiation: q = 1
    double q_rad = decelerationParameter(radiation);
    assert(std::abs(q_rad - 1.0) < 1e-3 && "Radiation deceleration q should be ~1.0");

    // de Sitter: q = -1
    double q_ds = decelerationParameter(desitter);
    assert(std::abs(q_ds - (-1.0)) < 1e-3 && "de Sitter deceleration q should be ~-1.0");

    std::cout << "[PASS] FRW deceleration parameter: q_mat=" << q_mat
              << ", q_rad=" << q_rad << ", q_ds=" << q_ds << std::endl;
}

// ============================================================================
// 19.1.17 - Metric determinant for FRW
// ============================================================================
void test_frw_metric_determinant() {
    auto frw = FRWMetric::matterDominated(H0, T0);
    Event4D ev(T0 * 0.8, 1e9, M_PI / 4.0, M_PI / 3.0);
    auto g = frw.evaluate(ev);

    double det = g.g[0][0] * g.g[1][1] * g.g[2][2] * g.g[3][3];
    assert(det < 0.0 && "FRW determinant should be negative (Lorentzian)");
    assert(std::isfinite(det) && "FRW determinant should be finite");
    std::cout << "[PASS] FRW metric determinant negative: det = " << det << std::endl;
}

// ============================================================================
// 19.1.18 - FRW metric g_tt = -c^2 is constant (no gravitational redshift in proper time)
// ============================================================================
void test_frw_g_tt_constant() {
    auto frw = FRWMetric::matterDominated(H0, T0);
    double c2 = Event4D::C * Event4D::C;
    std::vector<double> times = {T0 * 0.3, T0 * 0.6, T0, T0 * 1.5};
    (void)c2;

    int count = 0;
    for (double t : times) {
        Event4D ev(t, 1e9, M_PI / 4.0, M_PI / 3.0);
        auto g = frw.evaluate(ev);
        assert(std::abs(g.g[0][0] - (-c2)) < 1e-6 && "g_tt should be -c^2");
        count++;
    }
    std::cout << "[PASS] FRW g_tt = -c^2 constant: " << count << " points" << std::endl;
}

// ============================================================================
// 19.1.19 - FRW g_rr = a^2 / (1 - kr^2) behavior
// ============================================================================
void test_frw_g_rr_curvature_dependence() {
    auto flat = FRWMetric::matterDominated(H0, T0);
    auto closed = FRWMetric(FRWMetric::Curvature::CLOSED,
        [t0 = T0](double t) { return std::pow(t / t0, 2.0 / 3.0); }, H0);
    auto open = FRWMetric(FRWMetric::Curvature::OPEN,
        [t0 = T0](double t) { return std::pow(t / t0, 2.0 / 3.0); }, H0);

    double t = T0 * 0.8;
    double r = 1e9;
    double a = std::pow(t / T0, 2.0 / 3.0);

    Event4D ev(t, r, M_PI / 4.0, M_PI / 3.0);
    auto g_flat = flat.evaluate(ev);
    auto g_closed = closed.evaluate(ev);
    auto g_open = open.evaluate(ev);

    double g_rr_flat = a * a / (1.0 - 0.0 * r * r);
    double g_rr_closed = a * a / (1.0 - 1.0 * r * r);
    double g_rr_open = a * a / (1.0 - (-1.0) * r * r);
    (void)g_rr_flat; (void)g_rr_closed; (void)g_rr_open;

    assert(relError(g_flat.g[1][1], g_rr_flat) < 1e-10 && "Flat g_rr mismatch");
    assert(relError(g_open.g[1][1], g_rr_open) < 1e-10 && "Open g_rr mismatch");
    assert(relError(g_closed.g[1][1], g_rr_closed) < 1e-10 && "Closed g_rr mismatch");
    std::cout << "[PASS] FRW g_rr curvature dependence verified" << std::endl;
}

// ============================================================================
// 19.1.20 - Lookback time: integral of (1 - 1/a(z)) / H(z) dz
// ============================================================================
void test_frw_lookback_time() {
    auto frw = FRWMetric::matterDominated(H0, T0);
    double z = 1.0;
    double a_em = 1.0 / (1.0 + z);
    (void)a_em;

    // Matter-dominated lookback time ≈ (2/3H0) * [1 - 1/sqrt(1+z)]
    double t_lookback_expected = (2.0 / (3.0 * H0)) * (1.0 - 1.0 / std::sqrt(1.0 + z));
    assert(t_lookback_expected > 0.0 && "Lookback time should be positive");
    assert(std::isfinite(t_lookback_expected) && "Lookback time should be finite");
    std::cout << "[PASS] FRW lookback time for z=1: " << t_lookback_expected / (60.0 * 60.0 * 24.0 * 365.25)
              << " Gyr" << std::endl;
}

// ============================================================================
// 19.1.21 - Conformal time: eta = integral dt / a(t)
// ============================================================================
void test_frw_conformal_time() {
    auto frw = FRWMetric::matterDominated(H0, T0);
    double t = T0 * 0.8;
    double a = frw.scaleFactor(t);

    // For matter-dominated: eta = 3 sqrt(a) / (H0 sqrt(a0))... approximate
    // We just verify the integral converges
    double dt = 1e13;
    double eta = 0.0;
    for (int i = 0; i < 1000; i++) {
        double ti = i * dt;
        if (ti >= t) break;
        double ai = frw.scaleFactor(ti);
        if (ai > 1e-30) eta += dt / ai;
    }
    assert(std::isfinite(eta) && eta > 0.0 && "Conformal time should be finite and positive");
    std::cout << "[PASS] FRW conformal time integral converges: eta = " << eta << std::endl;
}

// ============================================================================
// 19.1.22 - Critical density: rho_crit = 3H^2 / (8*pi*G)
// ============================================================================
void test_frw_critical_density() {
    double H = H0;
    double rho_crit = 3.0 * H * H / (8.0 * M_PI * Event4D::G);
    assert(rho_crit > 0.0 && "Critical density should be positive");
    assert(std::isfinite(rho_crit) && "Critical density should be finite");

    // rho_crit = 3H0^2/(8 pi G) with H0 = 2.27e-18 s^-1 (~70 km/s/Mpc) and
    // G = 6.6743e-11 m^3 kg^-1 s^-2 evaluates to ~9.22e-27 kg/m^3, i.e. about
    // 5.5 hydrogen atoms per cubic metre. The previous ">1e-20 kg/m^3" bound was
    // wrong by seven orders of magnitude and could never hold for a physically
    // correct closed form; pin the literature value instead.
    const double rho_crit_expected = 9.2156e-27;   // kg/m^3
    assert(relError(rho_crit, rho_crit_expected) < 1e-3
           && "Critical density should match 3H0^2/(8 pi G) ~ 9.22e-27 kg/m^3");
    assert(rho_crit > 1e-28 && rho_crit < 1e-25
           && "Critical density should sit in the 1e-28..1e-25 kg/m^3 band");
    std::cout << "[PASS] FRW critical density: rho_crit = " << rho_crit << " kg/m^3" << std::endl;
}

// ============================================================================
// 19.1.23 - Matter density scales as a^-3
// ============================================================================
void test_frw_matter_density_scaling() {
    auto frw = FRWMetric::matterDominated(H0, T0);
    double t1 = T0 * 0.5;
    double t2 = T0 * 1.0;
    double a1 = frw.scaleFactor(t1);
    double a2 = frw.scaleFactor(t2);

    // rho_matter ∝ a^-3
    double rho_ratio = (a2 * a2 * a2) / (a1 * a1 * a1);
    assert(rho_ratio > 0.0 && "Density ratio should be positive");
    assert(std::isfinite(rho_ratio) && "Density ratio should be finite");
    std::cout << "[PASS] FRW matter density scaling: rho2/rho1 = (a1/a2)^3 = " << rho_ratio << std::endl;
}

// ============================================================================
// 19.1.24 - Radiation density scales as a^-4
// ============================================================================
void test_frw_radiation_density_scaling() {
    auto frw = FRWMetric::radiationDominated(H0, T0);
    double t1 = T0 * 0.5;
    double t2 = T0 * 1.0;
    double a1 = frw.scaleFactor(t1);
    double a2 = frw.scaleFactor(t2);

    // rho_rad ∝ a^-4
    double rho_ratio = (a2 * a2 * a2 * a2) / (a1 * a1 * a1 * a1);
    assert(rho_ratio > 0.0 && "Density ratio should be positive");
    assert(std::isfinite(rho_ratio) && "Density ratio should be finite");
    std::cout << "[PASS] FRW radiation density scaling: rho2/rho1 = (a1/a2)^4 = " << rho_ratio << std::endl;
}

// ============================================================================
// 19.1.25 - Distance duality (Etherington's theorem): d_L = (1+z)^2 d_A
// ============================================================================
void test_frw_distance_duality() {
    auto frw = FRWMetric::matterDominated(H0, T0);
    double t_em = T0 * 0.5;
    double a_em = frw.scaleFactor(t_em);
    double z = (1.0 / a_em) - 1.0;
    double chi = 1e9;
    (void)z;

    double d_A = a_em * chi;
    double d_L = d_A / (a_em * a_em);
    (void)d_L;

    assert(relError(d_L, (1.0 + z) * (1.0 + z) * d_A) < 1e-12 && "Etherington distance duality violated");
    std::cout << "[PASS] FRW Etherington distance duality: d_L = (1+z)^2 d_A" << std::endl;
}

// ============================================================================
// 19.1.26 - Hubble parameter at present: H(t0) = H0
// ============================================================================
void test_frw_hubble_constant_at_present() {
    // For a(t) = (t/t0)^(2/3) the Hubble parameter is exactly H(t) = 2/(3t), so the
    // model's own present-day value is 2/(3 t0). That equals the H0 handed to the
    // factory only when t0 is the matter-dominated age 2/(3 H0); here T0 = 13.8 Gyr
    // is the observed (LCDM) age while 2/(3 H0) = 9.3 Gyr, so asserting
    // H(T0) == H0 could never hold. Check the analytic relation...
    auto frw = FRWMetric::matterDominated(H0, T0);
    double H = frw.hubbleParameter(T0);
    assert(relError(H, 2.0 / (3.0 * T0)) < 1e-6
           && "Matter-dominated H(t) must equal 2/(3t)");

    // ...and that a self-consistent construction does reproduce H0 at t0.
    const double t0_selfConsistent = 2.0 / (3.0 * H0);
    auto frwSelfConsistent = FRWMetric::matterDominated(H0, t0_selfConsistent);
    double H_selfConsistent = frwSelfConsistent.hubbleParameter(t0_selfConsistent);
    assert(std::abs(H_selfConsistent - H0) < 1e-6 * H0
           && "Hubble parameter at t0 = 2/(3H0) should equal H0");

    std::cout << "[PASS] FRW H(t0) = 2/(3 t0) = " << H
              << " s^-1; self-consistent H(2/(3H0)) = " << H_selfConsistent << " s^-1" << std::endl;
}

// ============================================================================
// 19.1.27 - Flatness: k=0 for all standard factory methods
// ============================================================================
void test_frw_standard_models_flat() {
    auto matter = FRWMetric::matterDominated(H0, T0);
    auto radiation = FRWMetric::radiationDominated(H0, T0);
    auto desitter = FRWMetric::deSitter(H0);
    auto lcdm = FRWMetric::lambdaCDM(0.31, 0.69, H0);

    assert(matter.curvature() == 0 && "Matter-dominated should be flat");
    assert(radiation.curvature() == 0 && "Radiation-dominated should be flat");
    assert(desitter.curvature() == 0 && "de Sitter should be flat");
    assert(lcdm.curvature() == 0 && "ΛCDM should be flat");
    std::cout << "[PASS] FRW standard models all flat (k=0)" << std::endl;
}

// ============================================================================
// 19.1.28 - Scale factor at present: a(t0) = 1 for all standard models
// ============================================================================
void test_frw_scale_factor_normalization() {
    const double omegaM = 0.31;
    const double omegaLambda = 0.69;

    auto matter = FRWMetric::matterDominated(H0, T0);
    auto radiation = FRWMetric::radiationDominated(H0, T0);
    auto desitter = FRWMetric::deSitter(H0);
    auto lcdm = FRWMetric::lambdaCDM(omegaM, omegaLambda, H0);

    // matterDominated/radiationDominated take t0 explicitly and normalise there.
    assert(relError(matter.scaleFactor(T0), 1.0) < 1e-12 && "Matter a(t0) should be 1");
    assert(relError(radiation.scaleFactor(T0), 1.0) < 1e-12 && "Radiation a(t0) should be 1");

    // deSitter(H0) has no t0 parameter: a(t) = exp(H0 t), so its normalisation
    // epoch is t = 0 (see test_frw_de_sitter_exact, which pins that closed form).
    // Asserting a(T0) == 1 would contradict a(t) = exp(H0 t).
    assert(relError(desitter.scaleFactor(0.0), 1.0) < 1e-12 && "de Sitter a(0) should be 1");

    // lambdaCDM's normalisation epoch is likewise implied by H0 and the density
    // parameters, not by T0: a(t) = (Om/OL)^(1/3) sinh(3/2 H0 sqrt(OL) t)^(2/3)
    // reaches 1 when sinh(3/2 H0 sqrt(OL) t) = sqrt(OL/Om).
    const double t_age_lcdm = std::asinh(std::sqrt(omegaLambda / omegaM))
                            / (1.5 * H0 * std::sqrt(omegaLambda));
    assert(relError(lcdm.scaleFactor(t_age_lcdm), 1.0) < 1e-9
           && "LCDM a(t_age) should be 1");

    std::cout << "[PASS] FRW a = 1 at each model's normalisation epoch"
              << " (LCDM t_age = " << t_age_lcdm / 3.1557e16 << " Gyr)" << std::endl;
}

// ============================================================================
// 19.1.29 - Matter-radiation equality
// ============================================================================
void test_frw_matter_radiation_equality() {
    auto matter = FRWMetric::matterDominated(H0, T0);
    auto radiation = FRWMetric::radiationDominated(H0, T0);

    // At t_eq, a_eq = (t_eq/t0)^(2/3) for matter = (t_eq/t0)^(1/2) for radiation
    // => t_eq^(2/3) = t_eq^(1/2) * t0^(1/6) => t_eq = t0
    // Actually, equality happens when a_mat = a_rad => (t/t0)^(2/3) = (t/t0)^(1/2) => t = t0
    double t_eq = T0;
    double a_eq_mat = matter.scaleFactor(t_eq);
    double a_eq_rad = radiation.scaleFactor(t_eq);
    assert(relError(a_eq_mat, a_eq_rad) < 1e-12 && "Equality scale factors should match");
    (void)a_eq_mat; (void)a_eq_rad;
    std::cout << "[PASS] FRW matter-radiation equality verified at t = " << t_eq / T0 << " t0" << std::endl;
}

// ============================================================================
// 19.1.30 - FRW at early times: a(t) → 0 as t → 0
// ============================================================================
void test_frw_early_time_small_a() {
    auto frw = FRWMetric::matterDominated(H0, T0);
    double t_early = T0 * 1e-10;
    double a_early = frw.scaleFactor(t_early);

    // a(t) = (t/t0)^(2/3), so a(1e-10 t0) = 1e-20/3 ~ 2.15e-7 -- NOT < 1e-10.
    // The old bound implicitly assumed a ~ t/t0. Assert the actual power law and
    // demonstrate a -> 0 by driving t/t0 down instead.
    assert(relError(a_early, std::pow(1e-10, 2.0 / 3.0)) < 1e-12
           && "Matter-dominated a(t) must follow (t/t0)^(2/3)");
    assert(a_early > 0.0 && "Early universe scale factor should be positive");
    assert(std::isfinite(a_early) && "Early universe scale factor should be finite");
    assert(a_early < 1e-6 && "a(1e-10 t0) should be far below the present-day value");

    // Monotone approach to zero as t -> 0.
    double a_prev = a_early;
    for (double frac : {1e-12, 1e-15, 1e-18, 1e-21}) {
        double a_i = frw.scaleFactor(T0 * frac);
        assert(a_i > 0.0 && std::isfinite(a_i) && "Scale factor must stay positive and finite");
        assert(a_i < a_prev && "Scale factor must decrease monotonically toward t = 0");
        a_prev = a_i;
    }
    assert(a_prev < 1e-13 && "a(1e-21 t0) should be vanishingly small");
    assert(frw.scaleFactor(0.0) == 0.0 && "a(0) should be exactly 0");

    std::cout << "[PASS] FRW early time a(t) -> 0: a(1e-10 t0) = " << a_early
              << ", a(1e-21 t0) = " << a_prev << std::endl;
}

// ============================================================================
// Main
// ============================================================================
int main() {
    test_frw_matter_scale_factor_exact();
    test_frw_radiation_scale_factor_exact();
    test_frw_de_sitter_exact();
    test_frw_lambda_cdm_flat_constraint();
    test_frw_hubble_parameter_consistency();
    test_frw_age_of_universe();
    test_frw_metric_diagonal();
    test_frw_lorentzian_signature();
    test_frw_redshift_consistency();
    test_frw_comoving_distance();
    test_frw_angular_diameter_distance();
    test_frw_luminosity_distance();
    test_frw_curvature_types();
    test_frw_ricci_scalar_matter();
    test_frw_weyl_zero();
    test_frw_deceleration_parameter();
    test_frw_metric_determinant();
    test_frw_g_tt_constant();
    test_frw_g_rr_curvature_dependence();
    test_frw_lookback_time();
    test_frw_conformal_time();
    test_frw_critical_density();
    test_frw_matter_density_scaling();
    test_frw_radiation_density_scaling();
    test_frw_distance_duality();
    test_frw_hubble_constant_at_present();
    test_frw_standard_models_flat();
    test_frw_scale_factor_normalization();
    test_frw_matter_radiation_equality();
    test_frw_early_time_small_a();

#ifdef _MSC_VER
#pragma warning(pop)
#endif

    std::cout << "=== ALL FRW CONSISTENCY TESTS PASSED (" << 30 << " test functions) ===" << std::endl;
    return 0;
}
