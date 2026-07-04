/**
 * @file CausalSet.cpp
 * @brief Implementation of Causal Set Dynamics
 */

#include "CausalSet.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <queue>

namespace quantumverse {
namespace quantumgravity {

// ========== CausalSet implementation ==========

// Default Planck volume constant (l_P^3 in natural units)
static constexpr double default_planck_volume = 1.0;

CausalSet::CausalSet() : next_id(0) {}

int CausalSet::addElement(double birth_time, const std::set<int>& past_ids) {
    CausalElement elem(next_id, birth_time);
    elem.past = past_ids;
    elem.volume = default_planck_volume;  // Default Planck volume

    // Update future sets of past elements
    for (int pid : past_ids) {
        auto it = id_to_index.find(pid);
        if (it != id_to_index.end()) {
            elements[it->second].future.insert(next_id);
        }
    }

    elements.push_back(elem);
    id_to_index[next_id] = elements.size() - 1;
    return next_id++;
}

bool CausalSet::precedes(int x, int y) const {
    // Check if x < y (x precedes y)
    // Direct check: x in past of y
    auto it_y = id_to_index.find(y);
    if (it_y == id_to_index.end()) return false;

    const CausalElement& ey = elements[it_y->second];
    if (ey.past.find(x) != ey.past.end()) return true;

    // Transitive closure: check if there's a path x -> ... -> y
    // BFS from x to see if y is reachable
    std::set<int> visited;
    std::queue<int> queue;
    queue.push(x);
    visited.insert(x);

    while (!queue.empty()) {
        int current = queue.front();
        queue.pop();

        auto it = id_to_index.find(current);
        if (it == id_to_index.end()) continue;

        const CausalElement& ec = elements[it->second];
        for (int neighbor : ec.future) {
            if (neighbor == y) return true;
            if (visited.find(neighbor) == visited.end()) {
                visited.insert(neighbor);
                queue.push(neighbor);
            }
        }
    }

    return false;
}

std::set<int> CausalSet::getPast(int id) const {
    auto it = id_to_index.find(id);
    if (it == id_to_index.end()) return {};
    return elements[it->second].past;
}

std::set<int> CausalSet::getFuture(int id) const {
    auto it = id_to_index.find(id);
    if (it == id_to_index.end()) return {};
    return elements[it->second].future;
}

const CausalElement& CausalSet::getElement(int id) const {
    auto it = id_to_index.find(id);
    if (it == id_to_index.end()) {
        throw std::out_of_range("Element ID not found");
    }
    return elements[it->second];
}

std::vector<int> CausalSet::getAllIDs() const {
    std::vector<int> ids;
    for (const auto& elem : elements) {
        ids.push_back(elem.id);
    }
    return ids;
}

int CausalSet::intervalSize(int x, int y) const {
    // Number of elements z such that x < z < y
    if (!precedes(x, y)) return 0;

    auto past_y = getPast(y);
    auto future_x = getFuture(x);

    std::set<int> interval;
    for (int z : future_x) {
        if (past_y.find(z) != past_y.end()) {
            interval.insert(z);
        }
    }
    return interval.size();
}

bool CausalSet::isTree() const {
    // Check if each element has at most one immediate past (parent)
    for (const auto& elem : elements) {
        // Count minimal past elements (those with no other past element between)
        int parent_count = 0;
        for (int pid : elem.past) {
            bool is_minimal = true;
            for (int other : elem.past) {
                if (other != pid && precedes(other, pid)) {
                    is_minimal = false;
                    break;
                }
            }
            if (is_minimal) parent_count++;
        }
        if (parent_count > 1) return false;  // Not a tree (branching)
    }
    return true;
}

double CausalSet::density() const {
    if (elements.empty()) return 0.0;

    // Compute bounding box of birth times
    double min_t = elements[0].birth_time;
    double max_t = elements[0].birth_time;
    for (const auto& elem : elements) {
        min_t = std::min(min_t, elem.birth_time);
        max_t = std::max(max_t, elem.birth_time);
    }

    double time_span = max_t - min_t;
    if (time_span < 1e-10) return elements.size();

    // Density = elements per unit "volume" (here just time for 1D)
    return elements.size() / time_span;
}

std::vector<std::vector<int>> CausalSet::adjacencyMatrix() const {
    int n = elements.size();
    std::vector<std::vector<int>> adj(n, std::vector<int>(n, 0));

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i != j && precedes(elements[i].id, elements[j].id)) {
                adj[i][j] = 1;
            }
        }
    }
    return adj;
}

// ========== CausalSetDynamics implementation ==========

CausalSetDynamics::CausalSetDynamics(CausalSet& cs_, double alpha_, double beta_)
    : cs(cs_), alpha(alpha_), beta(beta_), uniformDist(0.0, 1.0) {}

double CausalSetDynamics::birthProbability(const std::set<int>& past, double current_time) const {
    // Probability of adding element with given past set
    // CSG model: P(past) ∝ exp(-α * |past| + β * number_of_pairs_in_past)
    // Simplified: P ∝ (α)^|past| * (1-β)^(pairs)

    int n_past = past.size();
    int n_pairs = 0;
    if (n_past >= 2) {
        // Count pairs (i,j) where both in past and i<j
        // For simplicity, assume all pairs are correlated
        n_pairs = n_past * (n_past - 1) / 2;
    }

    double logP = -alpha * n_past + beta * n_pairs;
    return std::exp(logP);
}

int CausalSetDynamics::growStep(double birth_time) {
    // Get all possible past sets (all subsets of existing elements)
    // In practice, we sample from a distribution rather than enumerate all subsets
    // Here: use a simplified "sprinkling" model where each existing element
    // independently gets included in the past with probability p

    const auto& existing = cs.getAllIDs();
    if (existing.empty()) {
        // First element
        return cs.addElement(birth_time, {});
    }

    // Determine past set size from Poisson-like distribution
    std::uniform_int_distribution<int> countDist(0, existing.size());
    int k = countDist(rng);

    // Randomly select k elements as past
    std::vector<int> existing_vec(existing.begin(), existing.end());
    std::shuffle(existing_vec.begin(), existing_vec.end(), rng);

    std::set<int> past;
    for (int i = 0; i < k && i < (int)existing_vec.size(); ++i) {
        past.insert(existing_vec[i]);
    }

    // Ensure transitivity: if we include x and y, and x<y already, that's fine
    // (The poset structure will be updated automatically)

    return cs.addElement(birth_time, past);
}

void CausalSetDynamics::grow(int n, double max_time) {
    double dt = max_time / (n > 1 ? n - 1 : 1);
    for (int i = 0; i < n; ++i) {
        double t = i * dt;
        growStep(t);
        element_count_history.push_back(cs.size());
        volume_history.push_back(cs.size() * default_planck_volume);
    }
}

// ========== CausalSetEngine implementation ==========

CausalSetEngine::CausalSetEngine(
    double alpha_,
    double beta_,
    double planck_vol_,
    int initial_elements_
) : alpha(alpha_),
    beta(beta_),
    planck_volume(planck_vol_),
    num_elements(initial_elements_),
    spectralDimension(2.0),  // Causal sets have d_s = 2 in some formulations
    dimensionality(4.0),     // Target 4D spacetime
    sprinkling_density(0.0),
    rng(std::random_device{}())
{
    causal_set = std::make_unique<CausalSet>();
    dynamics = std::make_unique<CausalSetDynamics>(*causal_set, alpha, beta);

    // Initialize with some elements
    dynamics->grow(initial_elements_, 1.0);
    num_elements = causal_set->size();
    sprinkling_density = causal_set->density();
}

std::string CausalSetEngine::getName() const {
    return "Causal Set Theory";
}

std::string CausalSetEngine::getDescription() const {
    return "Quantum gravity via discrete spacetime posets. Spacetime is a causal set where order encodes causality. Emergent continuum via coarse-graining. Predicts finite spacetime discreteness at Planck scale.";
}

std::string CausalSetEngine::getFieldEquation() const {
    return "No field equations in traditional sense.\n"
           "Dynamics via sequential growth: P(past) ∝ exp(-α|past| + β #pairs)\n"
           "Continuum limit: n → ∞, λ → 0 with nλ⁴ = constant (4D)";
}

MetricTensor CausalSetEngine::computeMetric(
    const Event4D& location,
    const std::map<std::string, double>& parameters
) const {
    // Causal set metric is fundamentally discrete
    // For visualization, return a coarse-grained effective metric
    // Approximate as Minkowski or FLRW depending on growth

    double r = std::sqrt(location.x*location.x + location.y*location.y + location.z*location.z);
    double t = location.t;

    // For a sprinkling of causal set elements, the effective metric is approximately flat
    // but with small stochastic fluctuations at Planck scale
    // Here return Minkowski as baseline

    MetricTensor metric;
    metric.g[0][0] = -1.0;
    metric.g[1][1] = 1.0;
    metric.g[2][2] = 1.0;
    metric.g[3][3] = 1.0;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (i != j) metric.g[i][j] = 0.0;
        }
    }

    return metric;
}

std::array<std::array<double, 4>, 4> CausalSetEngine::computeChristoffel(
    const Event4D& location,
    int rho, int mu, int nu
) const {
    // Flat spacetime: Christoffel symbols vanish
    std::array<std::array<double, 4>, 4> result;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result[i][j] = 0.0;
        }
    }
    return result;
}

MetricTensor CausalSetEngine::computeRicciTensor(const Event4D& location) const {
    MetricTensor zero;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            zero.g[i][j] = 0.0;
        }
    }
    return zero;
}

double CausalSetEngine::computeRicciScalar(const Event4D& location) const {
    return 0.0;  // Flat
}

double CausalSetEngine::computeKretschmannScalar(const Event4D& location) const {
    return 0.0;  // Flat
}

double CausalSetEngine::computeAmplitude(
    const Event4D& from,
    const Event4D& to
) const {
    // Causal set amplitude: probability that to is in the future of from
    // In a random causal set, this is proportional to the number of paths
    // Simplified: use exponential decay with proper time

    double dt = to.t - from.t;
    if (dt < 0) return 0.0;  // Cannot go backward in time

    double dx = to.x - from.x;
    double dy = to.y - from.y;
    double dz = to.z - from.z;
    double ds2 = -Event4D::C2 * dt*dt + dx*dx + dy*dy + dz*dz;

    if (ds2 < 0) {
        // Spacelike separation: probability very small
        return std::exp(-std::sqrt(-ds2) / planck_volume);
    } else {
        // Timelike or lightlike: higher probability
        double proper_time = std::sqrt(ds2) / Event4D::C;
        return std::exp(-proper_time / (10.0 * planck_volume));
    }
}

double CausalSetEngine::computeSpectralDimension(double mu) const {
    // Causal sets exhibit dimensional reduction: d_s → 2 at small scales
    // Use same running as other quantum gravity approaches
    // At large scales (mu → 0): d_s → 4
    // At small scales (mu → ∞): d_s → 2
    double mu_c = 1.0 / std::sqrt(planck_volume);  // Planck scale
    double alpha = 1.0;
    double d_s = 2.0 + 2.0 / (1.0 + std::pow(mu / mu_c, alpha));
    return d_s;
}

double CausalSetEngine::computeHausdorffDimension() const {
    // Estimate from growth dynamics
    return dimensionality;
}

std::vector<double> CausalSetEngine::getGeometryVertices() const {
    // Return positions of causal set elements in a 2D projection
    // Each element gets a position based on its birth time and a spatial coordinate
    // For simplicity, place elements on a line with time as x-axis
    std::vector<double> verts;

    const auto& ids = causal_set->getAllIDs();
    for (int id : ids) {
        const CausalElement& elem = causal_set->getElement(id);
        // Position: (t, x, y, z) where spatial coords derived from id
        verts.push_back(elem.birth_time);  // t
        verts.push_back(std::sin(id));     // x (pseudo-random)
        verts.push_back(std::cos(id));     // y
        verts.push_back(0.0);              // z
    }

    return verts;
}

std::vector<int> CausalSetEngine::getGeometryEdges() const {
    // Edges represent causal relations: from past to future
    std::vector<int> edges;
    const auto& ids = causal_set->getAllIDs();

    for (int id : ids) {
        const CausalElement& elem = causal_set->getElement(id);
        for (int future_id : elem.future) {
            // Check if this is an immediate relation (no intermediate element)
            // For simplicity, include all relations (transitive closure)
            edges.push_back(id);
            edges.push_back(future_id);
        }
    }

    return edges;
}

std::vector<double> CausalSetEngine::getGeometryColors() const {
    // Color by birth time (earlier = blue, later = red)
    std::vector<double> colors;
    const auto& ids = causal_set->getAllIDs();

    double min_t = std::numeric_limits<double>::max();
    double max_t = -std::numeric_limits<double>::max();
    for (int id : ids) {
        const CausalElement& elem = causal_set->getElement(id);
        min_t = std::min(min_t, elem.birth_time);
        max_t = std::max(max_t, elem.birth_time);
    }

    for (int id : ids) {
        const CausalElement& elem = causal_set->getElement(id);
        double t_norm = (max_t > min_t) ? (elem.birth_time - min_t) / (max_t - min_t) : 0.5;
        // Gradient from blue (0) to red (1)
        colors.push_back(t_norm);      // R
        colors.push_back(0.2);         // G
        colors.push_back(1.0 - t_norm); // B
        colors.push_back(0.8);         // A
    }

    return colors;
}

std::map<std::string, std::pair<double, double>> CausalSetEngine::getParameterRanges() const {
    return {
        {"alpha", {0.1, 2.0}},           // Growth parameter α
        {"beta", {0.0, 1.0}},            // Growth parameter β
        {"planck_volume", {1.0, 10.0}},  // Planck volume in arbitrary units
        {"num_elements", {10, 10000}}    // Number of elements
    };
}

std::unique_ptr<TheoryPlugin> CausalSetEngine::clone() const {
    return std::make_unique<CausalSetEngine>(alpha, beta, planck_volume, num_elements);
}

std::string CausalSetEngine::getPlanckScaleEffects() const {
    return "Spacetime is fundamentally discrete at Planck scale. Each element represents a Planck-volume atom. "
           "Causal order encodes lightcone structure. Classical continuum emerges via coarse-graining. "
           "Spectral dimension runs from 4 to 2 at Planck scale.";
}

void CausalSetEngine::grow(int n, double max_time) {
    dynamics->grow(n, max_time);
    num_elements = causal_set->size();
    sprinkling_density = causal_set->density();
}

double CausalSetEngine::estimateDimensionality() const {
    // Estimate via scaling of interval sizes
    // In d-dimensional spacetime, average interval size ~ r^d
    // Use a simple estimator from the causal set

    if (causal_set->size() < 10) return 4.0;

    // Compute average number of elements in intervals of various "radii"
    // Here just return current estimate
    return dimensionality;
}

double CausalSetEngine::randomWalkSpectralDimension(int num_walks, int steps) const {
    // Estimate spectral dimension via random walks on the causal set
    // d_s = 2 * (mean squared displacement) / log(steps)  (for diffusion on fractals)

    if (causal_set->size() == 0) return 2.0;

    // Simplified: return stored value
    return spectralDimension;
}

} // namespace quantumgravity
} // namespace quantumverse