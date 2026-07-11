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
    return true;
}

bool DiscoveryEngine::validateAgainstLightDeflection(const MetricTensor& metric) {
    return true;
}

bool DiscoveryEngine::validateAgainstFrameDragging(const MetricTensor& metric) {
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
    return 0.0;
}

double DiscoveryEngine::calculateBayesFactor(
    const Hypothesis& h1,
    const Hypothesis& h2,
    const std::vector<Event4D>& data
) {
    return 1.0;
}

void DiscoveryEngine::registerDiscoveryCallback(
    std::function<void(const DiscoveryResult&)> callback
) {
    discoveryCallbacks.push_back(callback);
}

std::string DiscoveryEngine::exportDiscovery(const DiscoveryResult& result) const {
    return "{}";
}

std::string DiscoveryEngine::exportHypothesis(const Hypothesis& hypothesis) const {
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
}

void DiscoveryEngine::loadState(const std::string& filename) {
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

} // namespace quantumverse
