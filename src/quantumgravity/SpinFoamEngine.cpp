/**
 * @file SpinFoamEngine.cpp
 * @brief Implementation of Spin Foam Engine for LQG
 */

#define _USE_MATH_DEFINES
#include <cmath>

// Ensure M_PI is available on all platforms (MSVC <cmath> may not define it)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "SpinFoamEngine.h"
#include "SpinNetwork.h"
#include <random>
#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace quantumverse {
namespace quantumgravity {

SpinFoamEngine::SpinFoamEngine(
    double gamma_,
    double lambda_,
    int maxSpin_,
    int samples_
) : gamma(gamma_),
    lambda(lambda_),
    maxSpin(maxSpin_),
    numSamples(samples_),
    spectralDimension(4.0),  // Classical at large scales
    hausdorffDimension(4.0),
    averageAmplitude(0.0),
    rng(std::random_device{}()),
    spinDist(0, maxSpin)
{
    foam = std::make_unique<SpinFoam>(gamma);
    
    // Initialize with a simple foam
    SpinNetwork initial;
    SpinNetwork final;
    // Populate with some default spins
    // (In practice, these would be set by the user or simulation)
    foam = buildSimpleFoam(initial, final, gamma);
}

std::string SpinFoamEngine::getName() const {
    return "Spin Foam (EPRL)";
}

std::string SpinFoamEngine::getDescription() const {
    return "Loop Quantum Gravity via spin foam path integral. Implements the EPRL vertex amplitude with Immirzi parameter γ = " +
           std::to_string(gamma) + ". Predicts discrete quantum geometry, Big Bounce, and Planck-scale foam structure.";
}

std::string SpinFoamEngine::getFieldEquation() const {
    return "Z = Σ_{j_f, i_v} ∏_f (2j_f+1) ∏_v A_v(j_f, i_v; γ)\n"
           "where A_v is the EPRL vertex amplitude (15j symbol in large spin limit)";
}

std::string SpinFoamEngine::getPlanckScaleEffects() const {
    return "Discrete quantum geometry at Planck scale (l_P ≈ " + std::to_string(lambda) + 
           " m). Area and volume operators have discrete spectra. Classical spacetime emerges via coarse-graining. "
           "Singularities resolved (Big Bounce instead of Big Bang).";
}

MetricTensor SpinFoamEngine::computeMetric(
    const Event4D& location,
    const std::map<std::string, double>& parameters
) const {
    // For now, return a classical metric with small quantum corrections.
    // A full spin foam simulation would compute the expectation value
    // of the metric operator in a coherent state.
    
    // Extract mass if provided (for black hole type solutions)
    double M = 1.0e30;  // default 1 solar mass
    auto it = parameters.find("mass");
    if (it != parameters.end()) {
        M = it->second;
    }
    
    // Compute Schwarzschild radius
    double r_s = 2.0 * Event4D::G * M / (Event4D::C * Event4D::C);
    
    // Get radial coordinate
    double dx = location.x - 0.0;  // Assume singularity at origin
    double dy = location.y - 0.0;
    double dz = location.z - 0.0;
    double r = std::sqrt(dx*dx + dy*dy + dz*dz);
    double theta = M_PI / 2.0;  // Equatorial
    double phi = 0.0;
    
    // Quantum correction factor: effective metric becomes regular at r=0
    // Using a simple polymer-like correction: r^2 -> r^2 + λ^2
    double r_eff = std::sqrt(r*r + lambda*lambda);
    
    // Compute f(r) for a modified Schwarzschild-like metric
    double f = 1.0 - r_s / r_eff;
    
    MetricTensor metric;
    metric.g[0][0] = -f;
    metric.g[1][1] = 1.0 / f;
    metric.g[2][2] = r_eff * r_eff;
    metric.g[3][3] = r_eff * r_eff * std::sin(theta) * std::sin(theta);
    
    // Off-diagonals zero
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (i != j) metric.g[i][j] = 0.0;
        }
    }
    
    return metric;
}

std::array<std::array<double, 4>, 4> SpinFoamEngine::computeChristoffel(
    const Event4D& location,
    int rho, int mu, int nu
) const {
    // Compute metric at this location
    MetricTensor metric = computeMetric(location, {});
    
    // Compute inverse metric
    double inv[4][4];
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            inv[i][j] = (i == j) ? 1.0 / metric.g[i][i] : 0.0;  // Diagonal only (simplified)
        }
    }
    
    // Compute Christoffel symbols: Γ^ρ_μν = 1/2 g^ρσ (∂_μ g_σν + ∂_ν g_σμ - ∂_σ g_μν)
    // For a static spherical metric, we can compute analytically.
    // But here we return a placeholder (zeros) - would need numerical derivatives.
    
    std::array<std::array<double, 4>, 4> result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result[i][j] = 0.0;
        }
    }
    
    return result;
}

MetricTensor SpinFoamEngine::computeRicciTensor(const Event4D& location) const {
    // For a vacuum solution (like quantum-corrected Schwarzschild),
    // Ricci tensor should be zero outside matter.
    // Return zero tensor.
    MetricTensor zero;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            zero.g[i][j] = 0.0;
        }
    }
    return zero;
}

double SpinFoamEngine::computeRicciScalar(const Event4D& location) const {
    // R = 0 for vacuum
    return 0.0;
}

double SpinFoamEngine::computeKretschmannScalar(const Event4D& location) const {
    // Compute K = R_μνρσ R^μνρσ
    // For quantum-corrected Schwarzschild, K is finite at r=0.
    // Use approximate formula: K ≈ 48 M^2 / (r^2 + λ^2)^3
    double dx = location.x;
    double dy = location.y;
    double dz = location.z;
    double r = std::sqrt(dx*dx + dy*dy + dz*dz);
    
    // Assume M from parameters or default
    double M = 1.0e30;
    double r_eff = std::sqrt(r*r + lambda*lambda);
    return 48.0 * M * M / (r_eff * r_eff * r_eff * r_eff * r_eff * r_eff);
}

double SpinFoamEngine::computeAmplitude(
    const Event4D& from,
    const Event4D& to
) const {
    // Compute transition amplitude between two events in the spin foam path integral.
    // This is highly simplified: we treat the spacetime interval as a "spin" and
    // compute a basic amplitude.
    
    // Compute proper distance (simplified)
    double dt = to.t - from.t;
    double dx = to.x - from.x;
    double dy = to.y - from.y;
    double dz = to.z - from.z;
    double ds2 = -Event4D::C2 * dt*dt + dx*dx + dy*dy + dz*dz;
    double properTime = std::sqrt(std::abs(ds2)) / Event4D::C;
    
    // Simple amplitude: exp(i * S / ħ) where S ~ proper time * (c^4 / (8πG))
    // But we return real part as a probability amplitude magnitude.
    // Use a Gaussian damping: exp(-properTime^2 / (2 σ^2))
    double sigma = lambda;  // Planck scale
    double amp = std::exp(-properTime*properTime / (2.0 * sigma * sigma));
    
    return amp;
}

double SpinFoamEngine::computeSpectralDimension(double mu) const {
    // Spectral dimension of spin foam models:
    // - At large scales (μ → 0): d_s → 4 (classical spacetime)
    // - At Planck scale (μ → ∞): d_s → 2 (quantum foam)
    // 
    // Use a simple running: d_s(μ) = 2 + 2 / (1 + (μ/μ_c)^α)
    // where μ_c is crossover scale ~ 1/lambda, α ~ 1.
    
    double mu_c = 1.0 / lambda;  // Planck energy scale
    double alpha = 1.0;
    double d_s = 2.0 + 2.0 / (1.0 + std::pow(mu / mu_c, alpha));
    
    return d_s;
}

double SpinFoamEngine::computeHausdorffDimension() const {
    // Hausdorff dimension for LQG is 4 at large scales, but may be scale-dependent.
    // Return effective value ~ 4.
    return 4.0;
}

std::vector<double> SpinFoamEngine::getGeometryVertices() const {
    // Return vertices from a sample spin network for visualization.
    // For now, generate a simple tetrahedral wireframe.
    std::vector<double> verts;
    
    // Simple tetrahedron vertices in 4D (project to 3D for visualization)
    // Using coordinates in 4D spacetime (t, x, y, z)
    double a = 1.0;
    // 4 vertices of a tetrahedron in 3D space (x,y,z), t=0
    std::vector<std::array<double,3>> positions = {
        { a,  a,  a},
        { a, -a, -a},
        {-a,  a, -a},
        {-a, -a,  a}
    };
    
    for (const auto& pos : positions) {
        verts.push_back(0.0);      // t
        verts.push_back(pos[0]);   // x
        verts.push_back(pos[1]);   // y
        verts.push_back(pos[2]);   // z
    }
    
    return verts;
}

std::vector<int> SpinFoamEngine::getGeometryEdges() const {
    // Edges connecting tetrahedron vertices (6 edges)
    return {0,1, 0,2, 0,3, 1,2, 1,3, 2,3};
}

std::vector<double> SpinFoamEngine::getGeometryColors() const {
     // Color vertices by spin value. Each vertex gets 4 RGBA values.
     // 4 tetrahedral vertices, each with RGBA color.
     // Vertex 0: red-ish (high spin)
     // Vertex 1: green-ish (medium spin)
     // Vertex 2: blue-ish (low spin)
     // Vertex 3: yellow-ish (mixed spin)
     return {
         1.0, 0.3, 0.3, 1.0,   // Vertex 0: red
         0.3, 1.0, 0.3, 1.0,   // Vertex 1: green
         0.3, 0.3, 1.0, 1.0,   // Vertex 2: blue
         1.0, 1.0, 0.3, 1.0    // Vertex 3: yellow
     };
 }

std::map<std::string, std::pair<double, double>> SpinFoamEngine::getParameterRanges() const {
    return {
        {"gamma", {0.1, 1.0}},      // Immirzi parameter range
        {"lambda", {1.0e-35, 1.0e-33}},  // Planck length scale
        {"maxSpin", {1, 10}},        // Maximum spin quantum number
        {"mass", {1.0e-8, 1.0e40}}  // Mass range (from Planck to supermassive)
    };
}

std::unique_ptr<TheoryPlugin> SpinFoamEngine::clone() const {
    return std::make_unique<SpinFoamEngine>(gamma, lambda, maxSpin, numSamples);
}

// Internal methods

SpinNetwork SpinFoamEngine::generateRandomBoundary(int numVertices, double avgSpin) const {
    std::vector<Vertex> vertices;
    std::vector<Edge> edges;
    
    // Create vertices
    for (int i = 0; i < numVertices; ++i) {
        vertices.emplace_back();
    }
    
    // Randomly connect vertices with spins around avgSpin
    std::uniform_int_distribution<int> vertexDist(0, numVertices-1);
    std::uniform_real_distribution<double> spinOffset(-0.5, 0.5);
    
    for (int i = 0; i < numVertices; ++i) {
        for (int j = i+1; j < numVertices; ++j) {
            if (rng() % 2 == 0) {  // 50% chance of edge
                Spin s = static_cast<Spin>(2 * (avgSpin + spinOffset(rng)));
                s = std::max(0, std::min(s, maxSpin));
                edges.emplace_back(i, j, s, 0, 0);
                vertices[i].incident_edges.push_back(edges.size()-1);
                vertices[j].incident_edges.push_back(edges.size()-1);
            }
        }
    }
    
    return SpinNetwork(edges, vertices);
}

double SpinFoamEngine::computeVertexAmplitude(const std::array<Spin, 4>& spins) const {
    return computeEPRLVertexAmplitude(spins, gamma);
}

double SpinFoamEngine::computeFaceAmplitude(Spin spin) const {
    double j = spin / 2.0;
    return 2.0 * j + 1.0;  // Dimension of representation
}

void SpinFoamEngine::runMonteCarlo(int numSteps) {
    amplitudeHistory.clear();
    
    for (int step = 0; step < numSteps; ++step) {
        // Generate a random spin foam configuration
        // For now, just generate a random boundary and compute amplitude
        SpinNetwork boundary = generateRandomBoundary(4, 2.0);  // 4 vertices, avg spin 2
        
        // Compute amplitude (simplified: product of vertex amplitudes)
        double totalAmp = 1.0;
        for (const auto& v : boundary.getVertices()) {
            std::array<Spin, 4> spins;
            size_t n = std::min<size_t>(4, v.incident_edges.size());
            for (size_t i = 0; i < n; ++i) {
                spins[i] = boundary.getEdges()[v.incident_edges[i]].spin;
            }
            totalAmp *= computeVertexAmplitude(spins);
        }
        
        amplitudeHistory.push_back(totalAmp);
    }
    
    // Compute average
    if (!amplitudeHistory.empty()) {
        averageAmplitude = std::accumulate(amplitudeHistory.begin(), amplitudeHistory.end(), 0.0) / amplitudeHistory.size();
    }
}

MetricTensor SpinFoamEngine::computeEffectiveMetric(const Event4D& location) const {
    // Compute effective metric from spin foam expectation values.
    // Uses the idea that geometry emerges from collective behavior of many spins.
    // 
    // For a spherically symmetric situation, the effective metric can be
    // approximated by a "mean field" where the area operator expectation
    // gives an effective radial coordinate.
    
    double r = std::sqrt(location.x*location.x + location.y*location.y + location.z*location.z);
    
    // Effective area of sphere of radius r: <A> = 4π <r^2>_op
    // In LQG, area operator eigenvalues are discrete: A = 8πγ l_P^2 Σ sqrt(j(j+1))
    // For a coherent state peaked at large j, <A> ≈ 4π r^2 (classical) + quantum corrections.
    
    // Quantum correction: r^2 -> r^2 + γ^2 λ^2 log(r/λ)  (from asymptotic expansions)
    // Simplified: r_eff^2 = r^2 + λ^2 (1 + γ^2)
    
    double r_eff_sq = r*r + lambda*lambda * (1.0 + gamma*gamma);
    double r_eff = std::sqrt(r_eff_sq);
    
    // Build an effective Schwarzschild-like metric with this effective radius
    double M = 1.0e30;  // default mass
    double r_s = 2.0 * Event4D::G * M / (Event4D::C * Event4D::C);
    double f = 1.0 - r_s / r_eff;
    
    MetricTensor metric;
    metric.g[0][0] = -f;
    metric.g[1][1] = 1.0 / f;
    metric.g[2][2] = r_eff_sq;
    metric.g[3][3] = r_eff_sq * std::sin(M_PI/2.0) * std::sin(M_PI/2.0);
    
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (i != j) metric.g[i][j] = 0.0;
        }
    }
    
    return metric;
}

double SpinFoamEngine::computeAreaExpectation(double radius) const {
    // Expectation value of area of a sphere of radius r in LQG
    // A = 8πγ l_P^2 Σ sqrt(j(j+1)) over punctures
    // For a smooth classical limit with N punctures, <A> ≈ 4π r^2
    // plus quantum corrections of order l_P^2.
    
    double classical = 4.0 * M_PI * radius * radius;
    double quantum_correction = 8.0 * M_PI * gamma * lambda * lambda;
    
    return classical + quantum_correction;
}

double SpinFoamEngine::computeVolumeExpectation(double radius) const {
    // Expectation value of volume of a ball of radius r
    // In LQG, volume operator has discrete spectrum.
    // Classical: V = (4/3)π r^3
    // Quantum correction: V -> V + O(λ^3)
    
    double classical = (4.0/3.0) * M_PI * radius * radius * radius;
    double quantum_correction = std::pow(lambda, 3);
    
    return classical + quantum_correction;
}

} // namespace quantumgravity
} // namespace quantumverse