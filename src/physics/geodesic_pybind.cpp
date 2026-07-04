// geodesic_pybind.cpp
// Pybind11 module for fast geodesic integration (Schwarzschild, Kerr, Reissner-Nordstrom)
// Provides Python-callable RK4 integrator for dataset generation
//
// Author:  QuantumVerse Phase 3 Implementation
// Date: 2026-05-13
//
// Supports:
//   - Schwarzschild:      ds^2 = -(1-2M/r)dt^2 + (1-2M/r)^{-1}dr^2 + r^2 dOmega^2
//   - Kerr:               ds^2 = -(1-2Mr/sigma)dt^2 - 4Mar sin^2(theta)/sigma dt dphi
//                         + sigma/delta dr^2 + sigma dtheta^2 + (r^2+a^2+2Ma^2r sin^2/sigma) sin^2 dphi^2
//   - Reissner-Nordstrom: ds^2 = -(1-2M/r+Q^2/r^2)dt^2 + (1-2M/r+Q^2/r^2)^{-1}dr^2 + r^2 dOmega^2

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <cmath>
#include <array>
#include <string>
#include <stdexcept>
#include <vector>

namespace py = pybind11;

// ---------------------------------------------------------------------------
// Schwarzschild Christoffel symbols (analytical, equatorial theta=pi/2)
// ---------------------------------------------------------------------------
static void christoffel_schwarzschild(double M, double r,
    std::array<std::array<std::array<double, 4>, 4>, 4>& Gamma)
{
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                Gamma[i][j][k] = 0.0;

    double rs = 2.0 * M;
    if (r <= rs) r = rs + 1e-10;

    double inv_r = 1.0 / r;
    double factor = 1.0 - rs / r;

    // Gamma^t_{tr} = Gamma^t_{rt} = rs / (2r(r-rs))
    double Gt_tr = rs / (2.0 * r * (r - rs));
    Gamma[0][0][1] = Gt_tr;
    Gamma[0][1][0] = Gt_tr;

    // Gamma^r_{tt} = rs(r-rs) / (2r^3)
    Gamma[1][0][0] = (rs * (r - rs)) / (2.0 * r * r * r);

    // Gamma^r_{rr} = -rs / (2r(r-rs))
    Gamma[1][1][1] = -rs / (2.0 * r * (r - rs));

    // Gamma^r_{theta,theta} = -(r - rs)
    Gamma[1][2][2] = -(r - rs);

    // Gamma^r_{phi,phi} = -(r - rs)  (equatorial: sin^2 theta = 1)
    Gamma[1][3][3] = -(r - rs);

    // Gamma^theta_{r theta} = Gamma^theta_{theta r} = 1/r
    Gamma[2][1][2] = inv_r;
    Gamma[2][2][1] = inv_r;

    // Gamma^phi_{r phi} = Gamma^phi_{phi r} = 1/r
    Gamma[3][1][3] = inv_r;
    Gamma[3][3][1] = inv_r;
}

// ---------------------------------------------------------------------------
// Kerr Christoffel symbols (analytical, equatorial theta=pi/2)
// a = J/(M*c)  dimensionless spin parameter
// ---------------------------------------------------------------------------
static void christoffel_kerr(double M, double a, double r,
    std::array<std::array<std::array<double, 4>, 4>, 4>& Gamma)
{
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                Gamma[i][j][k] = 0.0;

    double rs = 2.0 * M;
    double r2 = r * r;
    double a2 = a * a;
    double sigma = r2 + a2;          // theta=pi/2 => cos^2=0
    double delta = r2 - rs * r + a2;

    if (sigma < 1e-12) sigma = 1e-12;
    if (std::abs(delta) < 1e-12) delta = 1e-12;

    // Metric components (equatorial, theta=pi/2, sin theta = 1)
    double gtt = -(1.0 - rs * r / sigma);
    double gtf = -rs * r * a / sigma;          // g_{t phi}
    double grr = sigma / delta;
    double gthth = sigma;
    double gff = (r2 + a2 + rs * r * a2 / sigma);  // g_{phi phi}

    // Inverse metric components (2x2 block for t,phi)
    double det_tf = gtt * gff - gtf * gtf;
    if (std::abs(det_tf) < 1e-15) det_tf = 1e-15;
    double gtt_inv = gff / det_tf;
    double gtf_inv = -gtf / det_tf;
    double gff_inv = gtt / det_tf;

    // Derivatives of metric components w.r.t. r
    double dgtt_dr = -rs * (sigma - r * (2.0 * r)) / (sigma * sigma);
    // More precisely: d/dr[ -(1 - 2Mr/sigma) ] = -2M*(sigma - r*2r)/sigma^2
    // sigma = r^2 + a^2, d sigma/dr = 2r
    dgtt_dr = -rs * (sigma - 2.0 * r * r) / (sigma * sigma);

    double dgtf_dr = -rs * a * (sigma - r * 2.0 * r) / (sigma * sigma);

    double dgrr_dr = (2.0 * r * delta - sigma * (2.0 * r - rs)) / (delta * delta);

    double dgthth_dr = 2.0 * r;

    double dgff_dr = 2.0 * r + rs * a2 * (sigma - 2.0 * r * r) / (sigma * sigma);

    // Christoffel symbols via Gamma^lambda_{mu nu} = 1/2 g^{lambda sigma}(d_mu g_{nu sigma} + d_nu g_{mu sigma} - d_sigma g_{mu nu})
    // Only non-zero derivatives are w.r.t. r (mu=1)

    // Helper: Gamma^lambda_{mu nu} for cases where derivative is in r direction
    // d_r g_{mu nu} terms
    auto compute_Gamma = [&](int lam, int mu, int nu, double dg_r) -> double {
        // Only r-derivatives contribute (mu or nu = 1)
        // Gamma^lam_{mu nu} = 0.5 * g^{lam sig} * (d_mu g_{nu sig} + d_nu g_{mu sig} - d_sig g_{mu nu})
        // For equatorial Kerr, only g^{tt}, g^{tphi}, g^{phiphi}, g^{rr}, g^{thth} are non-zero
        double result = 0.0;

        // Contribution from inverse metric components
        // g^{tt} component
        double dg_nu_t = 0, dg_mu_t = 0, dg_t_munu = 0;
        if (nu == 0) dg_nu_t = dg_r;  // d_r g_{mu t} when nu=r
        if (mu == 0) dg_mu_t = dg_r;
        if (0 == 1) dg_t_munu = dg_r; // d_t g_{mu nu} = 0 (stationary)
        // This is getting complex; use direct formula

        return result;
    };

    // Direct computation of non-zero Christoffel symbols for Kerr (equatorial)
    // Using standard formulas from Chandrasekhar / Carroll

    // Gamma^t_{tr} = Gamma^t_{rt}
    Gamma[0][0][1] = (rs * (r2 - a2)) / (2.0 * sigma * sigma);
    Gamma[0][1][0] = Gamma[0][0][1];

    // Gamma^t_{phi r} = Gamma^t_{r phi}
    Gamma[0][3][1] = rs * a * (r2 - a2 - rs * r) / (2.0 * sigma * sigma * delta) * delta;
    // Simplified:
    Gamma[0][3][1] = rs * a / (2.0 * sigma * sigma) * (r2 - a2 - rs * r + rs * r);
    // Actually let me use the standard result more carefully

    // Recompute using standard Kerr Christoffel symbols
    // From standard references (e.g., Carroll GR, Wald)
    // Coordinates: x^0 = t, x^1 = r, x^2 = theta, x^3 = phi

    double r3 = r2 * r;

    // Gamma^t_{tr}
    Gamma[0][0][1] = (rs * (r2 - a2)) / (2.0 * sigma * sigma);
    Gamma[0][1][0] = Gamma[0][0][1];

    // Gamma^t_{r phi} (symmetric in lower indices)
    // d_r g_{t phi} = dgtf_dr
    Gamma[0][3][1] = 0.5 * gtt_inv * dgtf_dr;
    Gamma[0][1][3] = Gamma[0][3][1];

    // Gamma^r_{tt}
    Gamma[1][0][0] = 0.5 * grr * (-dgtt_dr);
    // Wait, sign: Gamma^r_{tt} = -1/2 g^{rr} dg_tt/dr (since g_{tt,r} is the only r-dep in g_{tt})
    // Actually: Gamma^r_{tt} = 1/2 g^{r alpha}(2 d_t g_{t alpha} - d_alpha g_{tt})
    //           = 1/2 g^{rr}(-d_r g_{tt}) = -1/2 * (sigma/delta) * dgtt_dr
    Gamma[1][0][0] = -0.5 * grr * dgtt_dr;

    // Gamma^r_{t phi}
    Gamma[1][0][3] = 0.5 * grr * (-dgtf_dr);
    Gamma[1][3][0] = Gamma[1][0][3];

    // Gamma^r_{rr}
    Gamma[1][1][1] = 0.5 * grr * dgrr_dr;

    // Gamma^r_{theta theta}
    Gamma[1][2][2] = -0.5 * grr * dgthth_dr;

    // Gamma^r_{phi phi}
    Gamma[1][3][3] = -0.5 * grr * dgff_dr;

    // Gamma^theta_{r theta} = Gamma^theta_{theta r}
    Gamma[2][1][2] = 1.0 / r;
    Gamma[2][2][1] = Gamma[2][1][2];

    // Gamma^phi_{r phi} = Gamma^phi_{phi r}
    Gamma[3][1][3] = 1.0 / r;
    Gamma[3][3][1] = Gamma[3][1][3];

    // Gamma^phi_{t r} = Gamma^phi_{r t}
    // Gamma^phi_{tr} = 1/2 g^{phi alpha}(d_t g_{r alpha} + d_r g_{t alpha} - d_alpha g_{tr})
    // Since g_{tr}=0 and d_t of anything = 0:
    // = 1/2 g^{phi phi} d_r g_{t phi} = 1/2 * gff_inv * dgtf_dr
    Gamma[3][0][1] = 0.5 * gff_inv * dgtf_dr;
    Gamma[3][1][0] = Gamma[3][0][1];

    // Gamma^phi_{t t}
    // = 1/2 g^{phi phi}(-d_phi g_{tt}) = 0 (stationary, axisymmetric)
    // But also: 1/2 g^{phi alpha}(2 d_t g_{t alpha} - d_alpha g_{tt})
    // d_phi g_{tt} = 0, so Gamma^phi_{tt} = 0
    Gamma[3][0][0] = 0.0;

    // Gamma^r_{t t} correction - also need g^{t phi} contribution
    // Gamma^r_{tt} = 1/2 g^{r alpha}(2 d_t g_{t alpha} - d_alpha g_{tt})
    //             = 1/2 g^{rr}(-d_r g_{tt}) [only r derivative matters]
    // Already computed above

    // Additional mixed terms from inverse metric off-diagonal
    // Gamma^phi_{rr} = 0 (no phi dependence in g_{rr})
    // Gamma^t_{rr} = 0
    // Gamma^t_{tt} = 0
}

// ---------------------------------------------------------------------------
// Reissner-Nordstrom Christoffel symbols (analytical, equatorial)
// ds^2 = -(1 - 2M/r + Q^2/r^2) dt^2 + (1 - 2M/r + Q^2/r^2)^{-1} dr^2 + r^2 dOmega^2
// ---------------------------------------------------------------------------
static void christoffel_reissner_nordstrom(double M, double Q, double r,
    std::array<std::array<std::array<double, 4>, 4>, 4>& Gamma)
{
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                Gamma[i][j][k] = 0.0;

    // f(r) = 1 - 2M/r + Q^2/r^2
    double f = 1.0 - 2.0 * M / r + Q * Q / (r * r);
    if (std::abs(f) < 1e-12) f = 1e-12;

    // f'(r) = 2M/r^2 - 2Q^2/r^3
    double df_dr = 2.0 * M / (r * r) - 2.0 * Q * Q / (r * r * r);

    double inv_r = 1.0 / r;

    // Gamma^t_{tr} = Gamma^t_{rt} = f'/(2f)
    double Gt_tr = df_dr / (2.0 * f);
    Gamma[0][0][1] = Gt_tr;
    Gamma[0][1][0] = Gt_tr;

    // Gamma^r_{tt} = f * f' / 2
    Gamma[1][0][0] = 0.5 * f * df_dr;

    // Gamma^r_{rr} = -f'/(2f)
    Gamma[1][1][1] = -df_dr / (2.0 * f);

    // Gamma^r_{theta theta} = -r * f
    Gamma[1][2][2] = -r * f;

    // Gamma^r_{phi phi} = -r * f  (equatorial: sin^2 theta = 1)
    Gamma[1][3][3] = -r * f;

    // Gamma^theta_{r theta} = Gamma^theta_{theta r} = 1/r
    Gamma[2][1][2] = inv_r;
    Gamma[2][2][1] = inv_r;

    // Gamma^phi_{r phi} = Gamma^phi_{phi r} = 1/r
    Gamma[3][1][3] = inv_r;
    Gamma[3][3][1] = inv_r;
}

// ---------------------------------------------------------------------------
// Geodesic derivative: dy/dtau = f(y)
// y = [t, r, theta, phi, ut, ur, utheta, uphi]
// ---------------------------------------------------------------------------
static void geodesic_derivative(const double y[8], double M, double a, double Q,
    const std::string& metric_type, double /*tau*/, double dydtau[8])
{
    double r = y[1];
    double ut = y[4], ur = y[5], uth = y[6], uph = y[7];

    // Position derivatives: dx^mu/dtau = u^mu
    dydtau[0] = ut;
    dydtau[1] = ur;
    dydtau[2] = uth;
    dydtau[3] = uph;

    // Compute Christoffel symbols
    std::array<std::array<std::array<double, 4>, 4>, 4> Gamma;

    if (metric_type == "kerr") {
        christoffel_kerr(M, a, r, Gamma);
    } else if (metric_type == "reissner-nordstrom") {
        christoffel_reissner_nordstrom(M, Q, r, Gamma);
    } else {
        // Default: Schwarzschild
        christoffel_schwarzschild(M, r, Gamma);
    }

    // Velocity derivatives: du^lambda/dtau = -Gamma^lambda_{mu nu} u^mu u^nu
    for (int lam = 0; lam < 4; lam++) {
        double acc = 0.0;
        for (int mu = 0; mu < 4; mu++) {
            for (int nu = 0; nu < 4; nu++) {
                acc -= Gamma[lam][mu][nu] * y[4 + mu] * y[4 + nu];
            }
        }
        dydtau[4 + lam] = acc;
    }
}

// ---------------------------------------------------------------------------
// RK4 integration step
// ---------------------------------------------------------------------------
static void rk4_step(const double y[8], double M, double a, double Q,
    const std::string& metric_type, double h, double y_new[8])
{
    double k1[8], k2[8], k3[8], k4[8];
    double y_temp[8];

    geodesic_derivative(y, M, a, Q, metric_type, 0.0, k1);

    for (int i = 0; i < 8; i++) y_temp[i] = y[i] + 0.5 * h * k1[i];
    geodesic_derivative(y_temp, M, a, Q, metric_type, 0.0, k2);

    for (int i = 0; i < 8; i++) y_temp[i] = y[i] + 0.5 * h * k2[i];
    geodesic_derivative(y_temp, M, a, Q, metric_type, 0.0, k3);

    for (int i = 0; i < 8; i++) y_temp[i] = y[i] + h * k3[i];
    geodesic_derivative(y_temp, M, a, Q, metric_type, 0.0, k4);

    for (int i = 0; i < 8; i++) {
        y_new[i] = y[i] + (h / 6.0) * (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]);
    }
}

// ---------------------------------------------------------------------------
// Full integration: returns final state (8-vector)
// ---------------------------------------------------------------------------
static py::array_t<double> integrate_geodesic(
    const py::array_t<double>& initial_event,
    const py::array_t<double>& initial_velocity,
    double M, double a, double Q,
    const std::string& metric_type,
    double tau_max, int num_steps)
{
    py::buffer_info event_buf = initial_event.request();
    py::buffer_info vel_buf = initial_velocity.request();

    if (event_buf.size != 4 || vel_buf.size != 4) {
        throw std::runtime_error("initial_event and initial_velocity must be length 4");
    }

    if (metric_type != "schwarzschild" && metric_type != "kerr" &&
        metric_type != "reissner-nordstrom") {
        throw std::invalid_argument("Unknown metric_type: " + metric_type +
            ". Must be 'schwarzschild', 'kerr', or 'reissner-nordstrom'");
    }

    // Validate Kerr parameters
    if (metric_type == "kerr") {
        double rs = 2.0 * M;
        double a_abs = std::abs(a);
        if (a_abs > M) {
            throw std::invalid_argument("Kerr spin parameter |a|=" + std::to_string(a_abs) +
                " exceeds M=" + std::to_string(M) + " (would be naked singularity)");
        }
    }

    // Validate RN parameters
    if (metric_type == "reissner-nordstrom") {
        if (Q > M) {
            throw std::invalid_argument("RN charge Q=" + std::to_string(Q) +
                " exceeds M=" + std::to_string(M) + " (would be naked singularity)");
        }
    }

    double* event_ptr = static_cast<double*>(event_buf.ptr);
    double* vel_ptr = static_cast<double*>(vel_buf.ptr);

    double y[8];
    for (int i = 0; i < 4; i++) y[i] = event_ptr[i];
    for (int i = 0; i < 4; i++) y[4 + i] = vel_ptr[i];

    double h = tau_max / num_steps;
    double y_next[8];

    for (int step = 0; step < num_steps; step++) {
        rk4_step(y, M, a, Q, metric_type, h, y_next);
        for (int i = 0; i < 8; i++) y[i] = y_next[i];
    }

    auto result = py::array_t<double>(8);
    py::buffer_info result_buf = result.request();
    double* result_ptr = static_cast<double*>(result_buf.ptr);
    for (int i = 0; i < 8; i++) {
        result_ptr[i] = y[i];
    }

    return result;
}

// ---------------------------------------------------------------------------
// Full trajectory integration: returns vector of (t, r, theta, phi) tuples
// ---------------------------------------------------------------------------
static std::vector<std::array<double, 4>> integrate_trajectory(
    const py::array_t<double>& initial_event,
    const py::array_t<double>& initial_velocity,
    double M, double a, double Q,
    const std::string& metric_type,
    double tau_max, int num_steps)
{
    py::buffer_info event_buf = initial_event.request();
    py::buffer_info vel_buf = initial_velocity.request();

    if (event_buf.size != 4 || vel_buf.size != 4) {
        throw std::runtime_error("initial_event and initial_velocity must be length 4");
    }

    if (metric_type != "schwarzschild" && metric_type != "kerr" &&
        metric_type != "reissner-nordstrom") {
        throw std::invalid_argument("Unknown metric_type: " + metric_type);
    }

    double* event_ptr = static_cast<double*>(event_buf.ptr);
    double* vel_ptr = static_cast<double*>(vel_buf.ptr);

    double y[8];
    for (int i = 0; i < 4; i++) y[i] = event_ptr[i];
    for (int i = 0; i < 4; i++) y[4 + i] = vel_ptr[i];

    double h = tau_max / num_steps;
    double y_next[8];

    std::vector<std::array<double, 4>> trajectory;
    trajectory.reserve(num_steps + 1);

    // Store initial point
    trajectory.push_back({y[0], y[1], y[2], y[3]});

    for (int step = 0; step < num_steps; step++) {
        rk4_step(y, M, a, Q, metric_type, h, y_next);
        for (int i = 0; i < 8; i++) y[i] = y_next[i];
        trajectory.push_back({y[0], y[1], y[2], y[3]});

        // Terminate if too close to singularity (r < 1.1 * horizon radius)
        double r = y[1];
        double rs = 2.0 * M;
        if (metric_type == "reissner-nordstrom") {
            double r_horizon = M + std::sqrt(M * M - Q * Q);
            if (r < 1.1 * r_horizon) break;
        } else if (metric_type == "kerr") {
            double r_horizon = M + std::sqrt(M * M - a * a);
            if (r < 1.1 * r_horizon) break;
        } else {
            if (r < 1.1 * rs) break;
        }
    }

    return trajectory;
}

// ---------------------------------------------------------------------------
// Compute conserved quantities along a geodesic
// ---------------------------------------------------------------------------
static py::array_t<double> compute_conserved_quantities(
    const py::array_t<double>& initial_event,
    const py::array_t<double>& initial_velocity,
    double M, double a, double Q,
    const std::string& metric_type)
{
    py::buffer_info event_buf = initial_event.request();
    py::buffer_info vel_buf = initial_velocity.request();

    if (event_buf.size != 4 || vel_buf.size != 4) {
        throw std::runtime_error("initial_event and initial_velocity must be length 4");
    }

    double* event_ptr = static_cast<double*>(event_buf.ptr);
    double* vel_ptr = static_cast<double*>(vel_buf.ptr);

    double r = event_ptr[1];
    double theta = event_ptr[2];
    double rs = 2.0 * M;

    // Metric components at the initial event
    double gtt, gtf, grr, gthth, gff;

    if (metric_type == "kerr") {
        double r2 = r * r;
        double a2 = a * a;
        double sigma = r2 + a2 * std::cos(theta) * std::cos(theta);
        double delta = r2 - rs * r + a2;
        gtt = -(1.0 - rs * r / sigma);
        gtf = -rs * r * a * std::sin(theta) * std::sin(theta) / sigma;
        grr = sigma / delta;
        gthth = sigma;
        gff = (r2 + a2 + rs * r * a2 * std::sin(theta) * std::sin(theta) / sigma) *
              std::sin(theta) * std::sin(theta);
    } else if (metric_type == "reissner-nordstrom") {
        double f = 1.0 - 2.0 * M / r + Q * Q / (r * r);
        gtt = -f;
        gtf = 0.0;
        grr = 1.0 / f;
        gthth = r * r;
        gff = r * r * std::sin(theta) * std::sin(theta);
    } else {
        // Schwarzschild
        double f = 1.0 - rs / r;
        gtt = -f;
        gtf = 0.0;
        grr = 1.0 / f;
        gthth = r * r;
        gff = r * r * std::sin(theta) * std::sin(theta);
    }

    // Conserved quantities:
    // Energy: E = -g_{tt} u^t - g_{t phi} u^phi
    // Angular momentum: L_z = g_{phi phi} u^phi + g_{t phi} u^t
    double ut = vel_ptr[0];
    double uph = vel_ptr[3];

    double E = -(gtt * ut + gtf * uph);
    double Lz = gff * uph + gtf * ut;

    // Carter constant (for Kerr) - simplified equatorial version
    // Q_carter = p_theta^2 + cos^2(theta) * (a^2 (m^2 - E^2) + Lz^2 / sin^2(theta))
    // For equatorial: theta = pi/2, cos(theta) = 0, sin(theta) = 1
    // p_theta = g_{theta theta} u^theta = gthth * uth
    double uth = vel_ptr[2];
    double p_theta = gthth * uth;
    double Q_carter = p_theta * p_theta;  // equatorial simplification

    auto result = py::array_t<double>(3);
    py::buffer_info result_buf = result.request();
    double* result_ptr = static_cast<double*>(result_buf.ptr);
    result_ptr[0] = E;
    result_ptr[1] = Lz;
    result_ptr[2] = Q_carter;

    return result;
}

// ---------------------------------------------------------------------------
// PYBIND11 MODULE DEFINITION
// ---------------------------------------------------------------------------
PYBIND11_MODULE(_geodesic_cpp, m) {
    m.doc() = "QuantumVerse Geodesic Integrator (C++ backend) - Phase 3 Training Data Generator";

    m.def("integrate_schwarzschild", &integrate_geodesic,
          py::arg("initial_event"), py::arg("initial_velocity"),
          py::arg("M"), py::arg("a") = 0.0, py::arg("Q") = 0.0,
          py::arg("metric_type") = "schwarzschild",
          py::arg("tau_max"), py::arg("num_steps") = 1000,
          "Integrate geodesic using RK4 (backward-compatible alias). "
          "Returns final 8-vector [t, r, theta, phi, ut, ur, utheta, uphi].");

    m.def("integrate_geodesic", &integrate_geodesic,
          py::arg("initial_event"), py::arg("initial_velocity"),
          py::arg("M"), py::arg("a") = 0.0, py::arg("Q") = 0.0,
          py::arg("metric_type") = "schwarzschild",
          py::arg("tau_max"), py::arg("num_steps") = 1000,
          "Integrate geodesic for Schwarzschild/Kerr/RN metrics using RK4. "
          "Returns final 8-vector [t, r, theta, phi, ut, ur, utheta, uphi].");

    m.def("integrate_trajectory", &integrate_trajectory,
          py::arg("initial_event"), py::arg("initial_velocity"),
          py::arg("M"), py::arg("a") = 0.0, py::arg("Q") = 0.0,
          py::arg("metric_type") = "schwarzschild",
          py::arg("tau_max"), py::arg("num_steps") = 1000,
          "Integrate full geodesic trajectory. "
          "Returns list of [t, r, theta, phi] arrays.");

    m.def("compute_conserved_quantities", &compute_conserved_quantities,
          py::arg("initial_event"), py::arg("initial_velocity"),
          py::arg("M"), py::arg("a") = 0.0, py::arg("Q") = 0.0,
          py::arg("metric_type") = "schwarzschild",
          "Compute conserved energy E, angular momentum Lz, and Carter constant Q. "
          "Returns 3-vector [E, Lz, Q_carter].");
}
