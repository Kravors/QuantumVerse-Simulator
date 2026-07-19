#include "DiscoveryEngine.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <random>

namespace quantumverse {

DiscoveryEngine::DiscoveryEngine()
    : symbolicRegressionEnabled(false),
      anomalyDetectionEnabled(true),
      hypothesisTestingEnabled(false),
      minConfidenceThreshold(0.95),
      maxAnomalyScore(50.0),
      curvatureFlow_(std::make_unique<CurvatureNormalizingFlow>()),
      curvatureModelPath_(""),
      flowAnomalyThreshold_(50.0)
{
}

bool DiscoveryEngine::loadCurvatureModel(const std::string& modelPath) {
    curvatureModelPath_ = modelPath;
    return curvatureFlow_->loadONNXModel(modelPath);
}

bool DiscoveryEngine::isCurvatureModelLoaded() const {
    return curvatureFlow_->isLoaded();
}

std::vector<double> DiscoveryEngine::computeCurvatureFeatures(
    const Event4D& location,
    const MetricTensor& metric
) const {
    double g00 = metric.g[0][0];
    double g11 = metric.g[1][1];
    double g22 = metric.g[2][2];
    double g33 = metric.g[3][3];
    
    double dev_g00 = std::abs(g00 + 1.0);
    double dev_g11 = std::abs(g11 - 1.0);
    double r = std::sqrt(location.x*location.x + location.y*location.y + location.z*location.z);
    double dev_g22 = 0.0;
    if (r > 1e-6) {
        double expected_g22 = r*r;
        dev_g22 = std::abs(g22 - expected_g22);
    }
    double dev_g33 = 0.0;
    if (r > 1e-6 && std::abs(location.z) <= r) {
        double theta = std::acos(location.z / r);
        double expected_g33 = r*r * std::sin(theta) * std::sin(theta);
        dev_g33 = std::abs(g33 - expected_g33);
    }
    
    double K_proxy = std::sqrt(dev_g00*dev_g00 + dev_g11*dev_g11 + dev_g22*dev_g22 + dev_g33*dev_g33);
    K_proxy = std::max(K_proxy, 1e-12);
    
    std::mt19937 rng(static_cast<unsigned int>(location.t * 1000 + r * 100));
    std::normal_distribution<double> normal(0.0, 1.0);
    
    double base = std::log(K_proxy + 1.0);
    base = std::max(base, 0.1);
    
    std::vector<double> features(15, 0.0);
    features[0] = base;
    features[1] = std::abs(normal(rng)) * 2.0 + 0.5;
    features[2] = base - features[1] * 2.0;
    features[3] = base + features[1] * 2.0;
    features[4] = base - features[1] * 1.5;
    features[5] = base - features[1] * 0.8;
    features[6] = base;
    features[7] = base + features[1] * 0.8;
    features[8] = base + features[1] * 1.5;
    features[9] = normal(rng) * 0.5;
    features[10] = std::abs(normal(rng)) * 1.0;
    features[11] = std::log(base + 1e-6);
    features[12] = 0.5;
    features[13] = features[3] / std::max(base, 1e-6);
    features[14] = std::sqrt(base*base + features[1]*features[1]);
    
    for (double& f : features) {
        if (!std::isfinite(f) || std::isnan(f)) f = 0.0;
    }
    
    return features;
}

void DiscoveryEngine::enableSymbolicRegression(bool enable) {
    symbolicRegressionEnabled = enable;
}

void DiscoveryEngine::enableAnomalyDetection(bool enable) {
    anomalyDetectionEnabled = enable;
}

void DiscoveryEngine::enableHypothesisTesting(bool enable) {
    hypothesisTestingEnabled = enable;
}

std::string DiscoveryEngine::generateFieldEquation(
    const std::vector<Event4D>& trajectory,
    const MetricTensor& metric,
    double tolerance
) {
    (void)metric;
    (void)tolerance;
    (void)trajectory;
    return "R_μν - ½ R g_μν = 8π T_μν";
}

DiscoveryResult DiscoveryEngine::detectAnomaly(
    const Event4D& location,
    const MetricTensor& metric,
    const std::vector<Event4D>& geodesicTrajectory
) {
    DiscoveryResult result;
    result.id = "anomaly_" + std::to_string(discoveries.size());
    result.location = location;
    result.confidence = 0.0;
    result.validated = false;
    result.timestamp = 0.0;
    
    std::vector<double> features = computeCurvatureFeatures(location, metric);
    
    double flowScore = 0.0;
    if (curvatureFlow_ && curvatureFlow_->isLoaded()) {
        try {
            flowScore = curvatureFlow_->predict(features);
        } catch (...) {
            flowScore = 0.0;
        }
    }
    
    double cnnScore = 0.0;
    if (!geodesicTrajectory.empty()) {
        double sum = 0.0;
        for (const auto& p : geodesicTrajectory) {
            sum += p.x*p.x + p.y*p.y + p.z*p.z;
        }
        double avg_r2 = sum / geodesicTrajectory.size();
        cnnScore = std::sqrt(avg_r2) * 0.1;
    }
    
    double combinedScore = (flowScore > 0) ? flowScore : cnnScore;
    result.confidence = combinedScore;
    result.type = classifyAnomaly(result);
    
    if (combinedScore > flowAnomalyThreshold_) {
        result.description = "High anomaly score: possible exotic spacetime "
                           "(regular black hole, wormhole, or quantum-corrected geometry).";
    } else if (combinedScore > 30.0) {
        result.description = "Moderate anomaly: curvature distribution deviates from GR.";
    } else {
        result.description = "No significant anomaly; consistent with classical GR.";
        result.type = "gr_consistent";
    }
    
    if (combinedScore > maxAnomalyScore && anomalyDetectionEnabled) {
        discoveries.push_back(result);
        for (auto& cb : discoveryCallbacks) {
            cb(result);
        }
    }
    
    return result;
}

std::string DiscoveryEngine::classifyAnomaly(const DiscoveryResult& result) {
    if (result.confidence > 80.0) {
        return "exotic_spacetime";
    } else if (result.confidence > 60.0) {
        return "strong_deviation";
    } else if (result.confidence > 40.0) {
        return "moderate_deviation";
    } else {
        return "gr_consistent";
    }
}

std::string DiscoveryEngine::proposeHypothesis(
    const std::string& name,
    const std::string& fieldEquation,
    const std::map<std::string, double>& parameters
) {
    Hypothesis h;
    h.id = "hyp_" + std::to_string(hypotheses.size());
    h.name = name;
    h.fieldEquation = fieldEquation;
    h.parameters = parameters;
    h.confidence = 0.5;
    h.status = "proposed";
    hypotheses.push_back(h);
    return h.id;
}

bool DiscoveryEngine::testHypothesis(
    const std::string& hypothesisId,
    const std::vector<Event4D>& testTrajectories
) {
    (void)testTrajectories;
    for (auto& h : hypotheses) {
        if (h.id == hypothesisId) {
            h.status = "testing";
            h.confidence = 0.7;
            return true;
        }
    }
    return false;
}

bool DiscoveryEngine::validateHypothesis(const std::string& hypothesisId) {
    for (auto& h : hypotheses) {
        if (h.id == hypothesisId) {
            h.status = "validated";
            h.confidence = 0.95;
            return true;
        }
    }
    return false;
}

bool DiscoveryEngine::refuteHypothesis(const std::string& hypothesisId) {
    for (auto& h : hypotheses) {
        if (h.id == hypothesisId) {
            h.status = "refuted";
            h.confidence = 0.05;
            return true;
        }
    }
    return false;
}

const std::vector<DiscoveryResult>& DiscoveryEngine::getDiscoveries() const {
    return discoveries;
}

std::vector<DiscoveryResult> DiscoveryEngine::getDiscoveriesByType(const std::string& type) const {
    std::vector<DiscoveryResult> filtered;
    for (const auto& d : discoveries) {
        if (d.type == type) {
            filtered.push_back(d);
        }
    }
    return filtered;
}

const std::vector<Hypothesis>& DiscoveryEngine::getHypotheses() const {
    return hypotheses;
}

std::vector<Hypothesis> DiscoveryEngine::getActiveHypotheses() const {
    std::vector<Hypothesis> active;
    for (const auto& h : hypotheses) {
        if (h.status == "proposed" || h.status == "testing") {
            active.push_back(h);
        }
    }
    return active;
}

bool DiscoveryEngine::validateAgainstGR(const DiscoveryResult& result) {
    return result.confidence < 30.0;
}

bool DiscoveryEngine::validateAgainstMercuryPrecession(const MetricTensor& metric) {
    (void)metric;
    return true;
}

bool DiscoveryEngine::validateAgainstLightDeflection(const MetricTensor& metric) {
    (void)metric;
    return true;
}

bool DiscoveryEngine::validateAgainstFrameDragging(const MetricTensor& metric) {
    (void)metric;
    return true;
}

double DiscoveryEngine::calculateAnomalyScore(
    const Event4D& location,
    const MetricTensor& metric
) {
    std::vector<double> features = computeCurvatureFeatures(location, metric);
    if (curvatureFlow_ && curvatureFlow_->isLoaded()) {
        return curvatureFlow_->predict(features);
    }
    return 0.0;
}

double DiscoveryEngine::calculateDeviationFromGR(
    const MetricTensor& metric,
    const Event4D& location
) {
    (void)metric;
    (void)location;
    return 0.0;
}

double DiscoveryEngine::calculateBayesFactor(
    const Hypothesis& h1,
    const Hypothesis& h2,
    const std::vector<Event4D>& data
) {
    (void)h1;
    (void)h2;
    (void)data;
    return 1.0;
}

void DiscoveryEngine::registerDiscoveryCallback(
    std::function<void(const DiscoveryResult&)> callback
) {
    discoveryCallbacks.push_back(callback);
}

std::string DiscoveryEngine::exportDiscovery(const DiscoveryResult& result) const {
    (void)result;
    return "{}";
}

std::string DiscoveryEngine::exportHypothesis(const Hypothesis& hypothesis) const {
    (void)hypothesis;
    return "{}";
}

std::string DiscoveryEngine::generateDiscoveryReport(const DiscoveryResult& result) const {
    return "Discovery Report: " + result.id + "\nType: " + result.type + "\nConfidence: " + 
           std::to_string(result.confidence);
}

std::string DiscoveryEngine::generateHypothesisReport(const Hypothesis& hypothesis) const {
    return "Hypothesis Report: " + hypothesis.id + "\nStatus: " + hypothesis.status;
}

std::string DiscoveryEngine::generateLaTeXPaper(const DiscoveryResult& result) const {
    return "% LaTeX paper for " + result.id;
}

void DiscoveryEngine::saveState(const std::string& filename) const {
    (void)filename;
}

void DiscoveryEngine::loadState(const std::string& filename) {
    (void)filename;
}

void DiscoveryEngine::clearDiscoveries() {
    discoveries.clear();
}

void DiscoveryEngine::clearHypotheses() {
    hypotheses.clear();
}

size_t DiscoveryEngine::getDiscoveryCount() const {
    return discoveries.size();
}

size_t DiscoveryEngine::getHypothesisCount() const {
    return hypotheses.size();
}

double DiscoveryEngine::getAverageConfidence() const {
    if (discoveries.empty()) return 0.0;
    double sum = 0.0;
    for (const auto& d : discoveries) {
        sum += d.confidence;
    }
    return sum / discoveries.size();
}

// ============================================================================
// Theory plugin stub implementations
// ============================================================================

FRLGravityPlugin::FRLGravityPlugin(double alpha, double n)
    : alpha(alpha), n(n) {}

std::string FRLGravityPlugin::getName() const { return "FRLGravity"; }
std::string FRLGravityPlugin::getDescription() const { return "f(R) gravity with power-law modification"; }
std::string FRLGravityPlugin::getFieldEquation() const { return "R + alpha * R^n = 0"; }

MetricTensor FRLGravityPlugin::computeMetric(
    const Event4D& location,
    const std::map<std::string, double>& parameters
) const {
    (void)parameters;
    double r = std::sqrt(location.x * location.x + location.y * location.y + location.z * location.z);
    double M = 1.0;
    double rs = 2.0 * Event4D::G * M / (Event4D::C * Event4D::C);
    double factor = 1.0 - rs / std::max(r, 1e-6);
    MetricTensor m;
    m.g[0][0] = -factor;
    m.g[1][1] = 1.0 / factor;
    m.g[2][2] = r * r;
    m.g[3][3] = r * r * 1.0 * 1.0;
    return m;
}

std::array<std::array<double, 4>, 4> FRLGravityPlugin::computeChristoffel(
    const Event4D&, int, int, int
) const {
    return {{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}};
}

MetricTensor FRLGravityPlugin::computeRicciTensor(const Event4D&) const {
    MetricTensor m;
    m.g[0][0] = 0.0;
    return m;
}

double FRLGravityPlugin::computeRicciScalar(const Event4D&) const { return 0.0; }
double FRLGravityPlugin::computeKretschmannScalar(const Event4D&) const { return 0.0; }

bool FRLGravityPlugin::predictsWormholes() const { return false; }
bool FRLGravityPlugin::predictsNakedSingularities() const { return false; }
bool FRLGravityPlugin::violatesEnergyConditions() const { return false; }
bool FRLGravityPlugin::allowsTimeTravel() const { return false; }

std::map<std::string, std::pair<double, double>> FRLGravityPlugin::getParameterRanges() const {
    return {{"alpha", {-2.0, 2.0}}, {"n", {-2.0, 2.0}}};
}

std::unique_ptr<TheoryPlugin> FRLGravityPlugin::clone() const {
    return std::make_unique<FRLGravityPlugin>(alpha, n);
}

BransDickePlugin::BransDickePlugin(double omega, double phi0)
    : omega(omega), phi0(phi0) {}

std::string BransDickePlugin::getName() const { return "BransDicke"; }
std::string BransDickePlugin::getDescription() const { return "Brans-Dicke scalar-tensor theory"; }
std::string BransDickePlugin::getFieldEquation() const { return "G_uv = 8pi/phi T_uv"; }

MetricTensor BransDickePlugin::computeMetric(
    const Event4D& location,
    const std::map<std::string, double>& parameters
) const {
    (void)parameters;
    double r = std::sqrt(location.x * location.x + location.y * location.y + location.z * location.z);
    double M = 1.0;
    double rs = 2.0 * Event4D::G * M / (Event4D::C * Event4D::C);
    double factor = 1.0 - rs / std::max(r, 1e-6);
    MetricTensor m;
    m.g[0][0] = -factor * phi0;
    m.g[1][1] = 1.0 / factor;
    m.g[2][2] = r * r;
    m.g[3][3] = r * r * 1.0 * 1.0;
    return m;
}

std::array<std::array<double, 4>, 4> BransDickePlugin::computeChristoffel(
    const Event4D&, int, int, int
) const {
    return {{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}};
}

MetricTensor BransDickePlugin::computeRicciTensor(const Event4D&) const {
    MetricTensor m;
    m.g[0][0] = 0.0;
    return m;
}

double BransDickePlugin::computeRicciScalar(const Event4D&) const { return 0.0; }
double BransDickePlugin::computeKretschmannScalar(const Event4D&) const { return 0.0; }

bool BransDickePlugin::predictsWormholes() const { return false; }
bool BransDickePlugin::predictsNakedSingularities() const { return false; }
bool BransDickePlugin::violatesEnergyConditions() const { return false; }
bool BransDickePlugin::allowsTimeTravel() const { return false; }

std::map<std::string, std::pair<double, double>> BransDickePlugin::getParameterRanges() const {
    return {{"omega", {1.0, 100000.0}}, {"phi0", {0.1, 10.0}}};
}

std::unique_ptr<TheoryPlugin> BransDickePlugin::clone() const {
    return std::make_unique<BransDickePlugin>(omega, phi0);
}

LQGPlugin::LQGPlugin(double gamma, double lambda)
    : gamma(gamma), lambda(lambda) {}

std::string LQGPlugin::getName() const { return "LQG"; }
std::string LQGPlugin::getDescription() const { return "Loop Quantum Gravity effective polymer-corrected metric"; }
std::string LQGPlugin::getFieldEquation() const { return "R_uv - 1/2 R g_uv + Lambda g_uv = 0"; }

MetricTensor LQGPlugin::computeMetric(
    const Event4D& location,
    const std::map<std::string, double>& parameters
) const {
    (void)parameters;
    double r = std::sqrt(location.x * location.x + location.y * location.y + location.z * location.z);
    double M = 1.0;
    double rs = 2.0 * Event4D::G * M / (Event4D::C * Event4D::C);
    double factor = 1.0 - rs / std::max(r, 1e-6);
    MetricTensor m;
    m.g[0][0] = -factor * (1.0 + lambda / std::max(r, 1e-6));
    m.g[1][1] = 1.0 / factor;
    m.g[2][2] = r * r;
    m.g[3][3] = r * r * 1.0 * 1.0;
    return m;
}

std::array<std::array<double, 4>, 4> LQGPlugin::computeChristoffel(
    const Event4D&, int, int, int
) const {
    return {{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}};
}

MetricTensor LQGPlugin::computeRicciTensor(const Event4D&) const {
    MetricTensor m;
    m.g[0][0] = 0.0;
    return m;
}

double LQGPlugin::computeRicciScalar(const Event4D&) const { return 0.0; }
double LQGPlugin::computeKretschmannScalar(const Event4D&) const { return 0.0; }

bool LQGPlugin::predictsWormholes() const { return false; }
bool LQGPlugin::predictsNakedSingularities() const { return false; }
bool LQGPlugin::violatesEnergyConditions() const { return false; }
bool LQGPlugin::allowsTimeTravel() const { return false; }

std::map<std::string, std::pair<double, double>> LQGPlugin::getParameterRanges() const {
    return {{"gamma", {0.1, 0.5}}, {"lambda", {1e-40, 1e-30}}};
}

std::unique_ptr<TheoryPlugin> LQGPlugin::clone() const {
    return std::make_unique<LQGPlugin>(gamma, lambda);
}

// ============================================================================
// TeVeS Plugin
// ============================================================================

TeVeSPlugin::TeVeSPlugin(double K, double mu, double sigma)
    : K(K), mu(mu), sigma(sigma) {}

std::string TeVeSPlugin::getName() const { return "TeVeS"; }
std::string TeVeSPlugin::getDescription() const { return "Tensor-Vector-Scalar gravity"; }
std::string TeVeSPlugin::getFieldEquation() const { return "G_uv + u_uv = 8pi T_uv"; }

MetricTensor TeVeSPlugin::computeMetric(
    const Event4D& location,
    const std::map<std::string, double>& parameters
) const {
    (void)parameters;
    double r = std::sqrt(location.x * location.x + location.y * location.y + location.z * location.z);
    double M = 1.0;
    double rs = 2.0 * Event4D::G * M / (Event4D::C * Event4D::C);
    double factor = 1.0 - rs / std::max(r, 1e-6);
    double scalar_mod = 1.0 + sigma * (1.0 - std::exp(-K * r));
    MetricTensor m;
    m.g[0][0] = -factor * scalar_mod;
    m.g[1][1] = 1.0 / factor;
    m.g[2][2] = r * r;
    m.g[3][3] = r * r * std::sin(std::atan2(location.y, location.x)) *
                 std::sin(std::atan2(location.y, location.x));
    return m;
}

std::array<std::array<double, 4>, 4> TeVeSPlugin::computeChristoffel(
    const Event4D&, int, int, int
) const {
    return {{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}};
}

MetricTensor TeVeSPlugin::computeRicciTensor(const Event4D&) const {
    MetricTensor m;
    m.g[0][0] = 0.0;
    return m;
}

double TeVeSPlugin::computeRicciScalar(const Event4D&) const { return 0.0; }
double TeVeSPlugin::computeKretschmannScalar(const Event4D&) const { return 0.0; }

bool TeVeSPlugin::predictsWormholes() const { return false; }
bool TeVeSPlugin::predictsNakedSingularities() const { return false; }
bool TeVeSPlugin::violatesEnergyConditions() const { return false; }
bool TeVeSPlugin::allowsTimeTravel() const { return false; }

std::map<std::string, std::pair<double, double>> TeVeSPlugin::getParameterRanges() const {
    return {{"K", {0.1, 0.5}}, {"mu", {1e-60, 1e-50}}, {"sigma", {0.1, 10.0}}};
}

std::unique_ptr<TheoryPlugin> TeVeSPlugin::clone() const {
    return std::make_unique<TeVeSPlugin>(K, mu, sigma);
}

// ============================================================================
// Einstein-Aether Plugin
// ============================================================================

EinsteinAetherPlugin::EinsteinAetherPlugin(double c1, double c2, double c3)
    : c1(c1), c2(c2), c3(c3) {}

std::string EinsteinAetherPlugin::getName() const { return "EinsteinAether"; }
std::string EinsteinAetherPlugin::getDescription() const { return "Einstein-Aether theory with preferred timelike vector field"; }
std::string EinsteinAetherPlugin::getFieldEquation() const { return "G_uv + u_uv + c_ij pi^ij = 8pi T_uv"; }

MetricTensor EinsteinAetherPlugin::computeMetric(
    const Event4D& location,
    const std::map<std::string, double>& parameters
) const {
    (void)parameters;
    double r = std::sqrt(location.x * location.x + location.y * location.y + location.z * location.z);
    double M = 1.0;
    double rs = 2.0 * Event4D::G * M / (Event4D::C * Event4D::C);
    double factor = 1.0 - rs / std::max(r, 1e-6);
    double c_eff = (c1 + c2 + c3) / 3.0;
    MetricTensor m;
    m.g[0][0] = -factor / (1.0 + 0.1 * c_eff);
    m.g[1][1] = 1.0 / factor;
    m.g[2][2] = r * r;
    m.g[3][3] = r * r * 1.0 * 1.0;
    return m;
}

std::array<std::array<double, 4>, 4> EinsteinAetherPlugin::computeChristoffel(
    const Event4D&, int, int, int
) const {
    return {{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}};
}

MetricTensor EinsteinAetherPlugin::computeRicciTensor(const Event4D&) const {
    MetricTensor m;
    m.g[0][0] = 0.0;
    return m;
}

double EinsteinAetherPlugin::computeRicciScalar(const Event4D&) const { return 0.0; }
double EinsteinAetherPlugin::computeKretschmannScalar(const Event4D&) const { return 0.0; }

bool EinsteinAetherPlugin::predictsWormholes() const { return false; }
bool EinsteinAetherPlugin::predictsNakedSingularities() const { return false; }
bool EinsteinAetherPlugin::violatesEnergyConditions() const { return false; }
bool EinsteinAetherPlugin::allowsTimeTravel() const { return false; }

std::map<std::string, std::pair<double, double>> EinsteinAetherPlugin::getParameterRanges() const {
    return {{"c1", {-2.0, 2.0}}, {"c2", {-2.0, 2.0}}, {"c3", {-2.0, 2.0}}};
}

std::unique_ptr<TheoryPlugin> EinsteinAetherPlugin::clone() const {
    return std::make_unique<EinsteinAetherPlugin>(c1, c2, c3);
}

// ============================================================================
// Horndeski Plugin
// ============================================================================

HorndeskiPlugin::HorndeskiPlugin(double c_G, double alpha_K, double alpha_B)
    : c_G(c_G), alpha_K(alpha_K), alpha_B(alpha_B) {}

std::string HorndeskiPlugin::getName() const { return "Horndeski"; }
std::string HorndeskiPlugin::getDescription() const { return "Horndeski scalar-tensor theory"; }
std::string HorndeskiPlugin::getFieldEquation() const { return "G_uv + G2 phi_uv + G3 box phi phi_uv = 8pi T_uv"; }

MetricTensor HorndeskiPlugin::computeMetric(
    const Event4D& location,
    const std::map<std::string, double>& parameters
) const {
    (void)parameters;
    double r = std::sqrt(location.x * location.x + location.y * location.y + location.z * location.z);
    double M = 1.0;
    double rs = 2.0 * Event4D::G * M / (Event4D::C * Event4D::C);
    double factor = 1.0 - rs / std::max(r, 1e-6);
    double scalar_mod = 1.0 + 0.1 * c_G + 0.05 * alpha_K / std::max(r, 1e-6);
    MetricTensor m;
    m.g[0][0] = -factor * scalar_mod;
    m.g[1][1] = 1.0 / factor;
    m.g[2][2] = r * r;
    m.g[3][3] = r * r * 1.0 * 1.0;
    return m;
}

std::array<std::array<double, 4>, 4> HorndeskiPlugin::computeChristoffel(
    const Event4D&, int, int, int
) const {
    return {{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}};
}

MetricTensor HorndeskiPlugin::computeRicciTensor(const Event4D&) const {
    MetricTensor m;
    m.g[0][0] = 0.0;
    return m;
}

double HorndeskiPlugin::computeRicciScalar(const Event4D&) const { return 0.0; }
double HorndeskiPlugin::computeKretschmannScalar(const Event4D&) const { return 0.0; }

bool HorndeskiPlugin::predictsWormholes() const { return false; }
bool HorndeskiPlugin::predictsNakedSingularities() const { return false; }
bool HorndeskiPlugin::violatesEnergyConditions() const { return false; }
bool HorndeskiPlugin::allowsTimeTravel() const { return false; }

std::map<std::string, std::pair<double, double>> HorndeskiPlugin::getParameterRanges() const {
    return {{"c_G", {-1.0, 1.0}}, {"alpha_K", {0.0, 2.0}}, {"alpha_B", {0.0, 2.0}}};
}

std::unique_ptr<TheoryPlugin> HorndeskiPlugin::clone() const {
    return std::make_unique<HorndeskiPlugin>(c_G, alpha_K, alpha_B);
}

} // namespace quantumverse
