/**
 * @file GFTEngine.cpp
 * @brief Implementation of Group Field Theory Engine
 */

#define _USE_MATH_DEFINES
#include <cmath>

#include "GFTEngine.h"
#include <algorithm>
#include <numeric>
#include <random>

// Ensure M_PI is available on all platforms (MSVC <cmath> may not define it)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {
namespace quantumgravity {

GFTEngine::GFTEngine(
    double coupling_,
    double mass_squared_,
    double cutoff_,
    int grid_size_
) : coupling(coupling_),
    mass_squared(mass_squared_),
    cutoff(cutoff_),
    grid_size(grid_size_),
    spectralDimension(4.0),
    effectiveVolume(0.0),
    twoPointCorrelation(0.0),
    is_condensate(false),
    condensate_magnitude(0.0),
    rng(std::random_device{}()),
    normalDist(0.0, 1.0)
{
    // Allocate field: φ(g1,g2,g3,g4) on SU(2)^4 grid
    // Simplified: use linear array of size grid_size^4
    // In full implementation, would use group representation theory
    field_values.resize(grid_size * grid_size * grid_size * grid_size);

    // Initialize with small random fluctuations
    for (auto& val : field_values) {
        val = 0.01 * normalDist(rng);
    }
}

std::string GFTEngine::getName() const {
    return "Group Field Theory (GFT)";
}

std::string GFTEngine::getDescription() const {
    return "Quantum gravity via field theory on group manifolds. Uses SU(2)^4 fields for 4-simplices. Predicts emergent spacetime from condensates, with spectral dimension running from 4 to 2.";
}

std::string GFTEngine::getFieldEquation() const {
    return "S[φ] = ∫ dg φ^2 + m² φ^2 + λ ∫ dg1...dg5 φ(g1)...φ(g5) δ(g1...g5)\n"
           "Path integral: Z = ∫ Dφ exp(-S[φ])";
}

std::string GFTEngine::getPlanckScaleEffects() const {
    return "Spacetime emerges from GFT condensate. At Planck scale, spectral dimension d_s → 2. "
           "Geometric observables (area, volume) have discrete spectra via group representation theory. "
           "Big Bang replaced by condensate nucleation (Big Bounce possible).";
}

MetricTensor GFTEngine::computeMetric(
    const Event4D& location,
    const std::map<std::string, double>& parameters
) const {
    // GFT effective metric from condensate two-point function
    // In mean-field approximation: G(g) ≈ ⟨φ(g)⟩² / (1 - λ ⟨φ⟩²)
    // The metric emerges from the two-point function's dependence on group elements

    // For a homogeneous isotropic condensate, the effective metric is FLRW-like
    // with scale factor a(t) determined by condensate evolution

    // Extract radial distance
    double r = std::sqrt(location.x*location.x + location.y*location.y + location.z*location.z);
    double t = location.t;

    // Condensate-driven scale factor: a(t) ∝ exp(H t) for de Sitter-like phase
    // or a(t) ∝ t^p for power-law evolution
    double H = 1.0 / (cutoff * cutoff);  // Hubble from Planck scale
    double a = std::exp(H * std::abs(t));  // Simplified: always expanding

    // Effective FLRW metric: ds² = -dt² + a(t)² (dr²/(1-kr²) + r² dΩ²)
    // Here k=0 for simplicity
    double dr2 = 1.0;  // g_rr = a(t)²
    double angular = a * a * r * r;

    MetricTensor metric;
    metric.g[0][0] = -1.0;  // cosmic time
    metric.g[1][1] = a * a / (1.0);  // radial (k=0)
    metric.g[2][2] = angular;  // θ
    metric.g[3][3] = angular * std::sin(M_PI/2.0) * std::sin(M_PI/2.0);  // φ

    // Off-diagonals zero
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (i != j) metric.g[i][j] = 0.0;
        }
    }

    return metric;
}

std::array<std::array<double, 4>, 4> GFTEngine::computeChristoffel(
    const Event4D& location,
    int rho, int mu, int nu
) const {
    // For FLRW metric, Christoffel symbols are known analytically
    // But we return zeros as placeholder (would need numerical derivatives)
    std::array<std::array<double, 4>, 4> result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result[i][j] = 0.0;
        }
    }
    return result;
}

MetricTensor GFTEngine::computeRicciTensor(const Event4D& location) const {
    // For FLRW with a(t) = exp(Ht), Ricci tensor is that of de Sitter space
    // R_μν = 3H² diag(-1, 1, 1, 1) for k=0
    MetricTensor zero;
    double H = 1.0 / (cutoff * cutoff);
    zero.g[0][0] = -3.0 * H * H;
    zero.g[1][1] = 3.0 * H * H;
    zero.g[2][2] = 3.0 * H * H;
    zero.g[3][3] = 3.0 * H * H;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (i != j) zero.g[i][j] = 0.0;
        }
    }
    return zero;
}

double GFTEngine::computeRicciScalar(const Event4D& location) const {
    // R = 12 H² for de Sitter
    double H = 1.0 / (cutoff * cutoff);
    return 12.0 * H * H;
}

double GFTEngine::computeKretschmannScalar(const Event4D& location) const {
    // K = R_μνρσ R^μνρσ = 12 H⁴ for de Sitter
    double H = 1.0 / (cutoff * cutoff);
    return 12.0 * H * H * H * H;
}

double GFTEngine::computeAmplitude(
    const Event4D& from,
    const Event4D& to
) const {
    // GFT transition amplitude between two 4-geometries
    // In path integral: A = ∫ Dφ exp(-S[φ]) with boundary conditions
    // Simplified: use saddle-point approximation around condensate

    // Compute "distance" in group space (very simplified)
    double dt = to.t - from.t;
    double dx = to.x - from.x;
    double dy = to.y - from.y;
    double dz = to.z - from.z;
    double ds2 = -Event4D::C2 * dt*dt + dx*dx + dy*dy + dz*dz;
    double distance = std::sqrt(std::abs(ds2));

    // Amplitude from effective action: exp(-S_eff / ħ)
    // S_eff ~ m² ⟨φ⟩² V + λ ⟨φ⟩⁴ V  (mean-field)
    double S_eff = mass_squared * condensate_magnitude * condensate_magnitude * distance
                 + coupling * condensate_magnitude * condensate_magnitude * condensate_magnitude * condensate_magnitude * distance;

    return std::exp(-S_eff);
}

double GFTEngine::computeSpectralDimension(double mu) const {
    // GFT spectral dimension: d_s(μ) = 2 + 2/(1 + (μ/μ_c)^α)
    // Same running as spin foam (both LQG approaches)
    // At large scales (mu → 0): d_s → 4
    // At small scales (mu → ∞): d_s → 2
    double mu_c = cutoff;  // Crossover at cutoff scale
    double alpha = 1.0;
    double d_s = 2.0 + 2.0 / (1.0 + std::pow(mu / mu_c, alpha));
    return d_s;
}

double GFTEngine::computeHausdorffDimension() const {
    // In GFT condensate phase, Hausdorff dimension = 4 (classical)
    // In quantum phase, may be scale-dependent
    return 4.0;
}

std::vector<double> GFTEngine::getGeometryVertices() const {
    // Return vertices representing a 4-simplex (5 vertices in 4D)
    std::vector<double> verts;

    // 4-simplex vertices in 4D spacetime (t, x, y, z)
    // Place at t=0 for simplicity
    double a = 1.0;  // scale
    std::vector<std::array<double,4>> positions = {
        {0.0,  a,  a,  a},
        {0.0,  a, -a, -a},
        {0.0, -a,  a, -a},
        {0.0, -a, -a,  a},
        {0.0,  0,  0,  0}   // center
    };

    for (const auto& pos : positions) {
        verts.push_back(pos[0]);  // t
        verts.push_back(pos[1]);  // x
        verts.push_back(pos[2]);  // y
        verts.push_back(pos[3]);  // z
    }

    return verts;
}

std::vector<int> GFTEngine::getGeometryEdges() const {
    // 4-simplex has 10 edges connecting 5 vertices
    // Connect each vertex to all others (complete graph K5)
    return {
        0,1, 0,2, 0,3, 0,4,
        1,2, 1,3, 1,4,
        2,3, 2,4,
        3,4
    };
}

std::vector<double> GFTEngine::getGeometryColors() const {
    // Color vertices by field value at that location
    // For now, return uniform color
    return {0.0, 0.5, 1.0, 1.0};  // Blue
}

std::map<std::string, std::pair<double, double>> GFTEngine::getParameterRanges() const {
    return {
        {"coupling", {0.01, 1.0}},          // Interaction strength λ
        {"mass_squared", {-1.0, 10.0}},     // Can be tachyonic for condensate
        {"cutoff", {1.0e30, 1.0e40}},       // Planck scale cutoff
        {"grid_size", {5, 20}},             // Discretization resolution
        {"condensate_magnitude", {0.0, 10.0}}
    };
}

std::unique_ptr<TheoryPlugin> GFTEngine::clone() const {
    return std::make_unique<GFTEngine>(coupling, mass_squared, cutoff, grid_size);
}

// Internal methods

double GFTEngine::computeAction(const std::vector<double>& field) const {
    // GFT action: S[φ] = ∫ dg (1/2 m² φ² + 1/2 (∇φ)²) + λ ∫ dg1...dg5 φ⁵ δ(g1...g5)
    // Discretized version:

    double kinetic = 0.0;
    double mass_term = 0.0;
    double interaction = 0.0;

    // Mass term: Σ_i m² φ_i²
    for (double val : field) {
        mass_term += mass_squared * val * val;
    }

    // Kinetic term: simplified as (φ(i+1) - φ(i))² (needs proper group derivative)
    // For now, omit or use simple difference
    for (size_t i = 0; i < field.size() - 1; ++i) {
        double diff = field[i+1] - field[i];
        kinetic += diff * diff;
    }

    // Interaction term: λ Σ_{simplices} φ(g1)...φ(g5)
    // Simplified: sum over all 5-tuples (brute force, very expensive)
    // In practice, use Monte Carlo or tensor network techniques
    interaction = coupling * std::pow(std::accumulate(field.begin(), field.end(), 0.0) / field.size(), 5);

    return 0.5 * (kinetic + mass_term) + interaction;
}

std::vector<double> GFTEngine::randomFieldConfiguration() const {
    std::vector<double> config = field_values;
    for (auto& val : config) {
        val += 0.1 * normalDist(rng);  // Small perturbation
    }
    return config;
}

double GFTEngine::computeTwoPointFunction(const std::vector<double>& field, int separation) const {
    // Two-point function: G(x,y) = ⟨φ(x) φ(y)⟩
    // For homogeneous case, depends only on |x-y|
    if (separation >= static_cast<int>(field.size())) return 0.0;

    double sum = 0.0;
    for (size_t i = 0; i + separation < field.size(); ++i) {
        sum += field[i] * field[i + separation];
    }
    return sum / (field.size() - separation);
}

double GFTEngine::groupVolumeSU2() const {
    // Volume of SU(2) group manifold: ∫ dg = 8π²
    return 8.0 * M_PI * M_PI;
}

void GFTEngine::setCondensate(bool enable, double magnitude) {
    is_condensate = enable;
    condensate_magnitude = magnitude;
    if (enable) {
        // Set field to uniform condensate value
        std::fill(field_values.begin(), field_values.end(), magnitude);
    }
}

void GFTEngine::evolveMonteCarlo(int steps) {
    // Simple Metropolis-Hastings for GFT field configuration
    for (int step = 0; step < steps; ++step) {
        std::vector<double> proposal = randomFieldConfiguration();
        double S_old = computeAction(field_values);
        double S_new = computeAction(proposal);

        double deltaS = S_new - S_old;
        double accept_prob = std::exp(-deltaS);

        if (normalDist(rng) < accept_prob) {
            field_values = proposal;
        }
    }

    // Update observables
    effectiveVolume = computeEffectiveAction();
    twoPointCorrelation = computeTwoPointFunction(field_values, 1);
}

double GFTEngine::computeCondensateFraction() const {
    // Fraction of field in zero-momentum mode (condensate indicator)
    double sum = std::accumulate(field_values.begin(), field_values.end(), 0.0);
    double mean = sum / field_values.size();

    double variance = 0.0;
    for (double val : field_values) {
        variance += (val - mean) * (val - mean);
    }
    variance /= field_values.size();

    // Condensate fraction ~ |mean| / sqrt(variance)
    if (variance < 1e-10) return 1.0;
    return std::abs(mean) / std::sqrt(variance);
}

double GFTEngine::computeEffectiveAction() const {
    // Effective action from mean-field: S_eff = V [m² φ₀² + λ φ₀⁴]
    double V = groupVolumeSU2();  // Simplified: single group volume
    return V * (mass_squared * condensate_magnitude * condensate_magnitude
                + coupling * std::pow(condensate_magnitude, 4));
}

} // namespace quantumgravity
} // namespace quantumverse