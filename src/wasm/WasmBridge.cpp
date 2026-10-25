#include "WasmBridge.h"
#include "../discovery/RLDiscoveryAgent.h"
#include "WasmLLMCoPilot.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <memory>
#include <functional>
#include <string>
#include <emscripten/bind.h>

using namespace quantumverse;

struct WasmBridge {
    std::shared_ptr<PhysicsValidator> validator;
    std::shared_ptr<discovery::RLDiscoveryAgent> agent;
    std::unique_ptr<WasmLLMCoPilot> llmCoPilot;
    discovery::PolicyNetwork policy;
    bool physicsWarning = false;
    LLMCallback llmCallback = nullptr;
    double mass = 1.0;
    double spin = 0.0;
    double charge = 0.0;
    std::shared_ptr<KerrMetric> metric;
};

// Static callback storage for LLM
static WasmBridge* g_bridgeWithCallback = nullptr;
static LLMCallback g_llmCallback = nullptr;

extern "C" {

WasmBridge* qv_bridge_create(void)
{
    auto* b = new WasmBridge();
    b->metric = std::make_shared<KerrMetric>(1.0, 0.0);
    b->validator = std::make_shared<PhysicsValidator>(b->metric);
    b->agent = std::make_shared<discovery::RLDiscoveryAgent>(
        3, std::vector<std::pair<double, double>>{
            {0.1, 100.0},
            {0.0, 1.5},
            {0.0, 1.5},
        });
    b->llmCoPilot = std::make_unique<WasmLLMCoPilot>();
    b->policy = discovery::PolicyNetwork(8, 3);

    b->llmCoPilot->setAnalysisCallback([b](const std::string& explanation) {
        if (b->llmCallback) {
            b->llmCallback(explanation.c_str());
        }
    });

    g_bridgeWithCallback = b;
    g_llmCallback = b->llmCallback;

    return b;
}

void qv_bridge_destroy(WasmBridge* bridge)
{
    delete bridge;
}

const char* qv_bridge_create_metric(WasmBridge* bridge, double mass, double spin)
{
    (void)bridge;
    auto metric = std::make_shared<KerrMetric>(mass, spin);
    static thread_local std::string result;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << "{\"mass\":" << metric->mass()
        << ",\"spin\":" << metric->spin()
        << ",\"outerHorizonRadius\":" << metric->outerHorizonRadius()
        << ",\"innerHorizonRadius\":" << metric->innerHorizonRadius()
        << ",\"schwarzschildRadius\":" << metric->schwarzschildRadius()
        << "}";
    result = oss.str();
    return result.c_str();
}

const char* qv_bridge_validate_state(WasmBridge* bridge, double mass, double spin, double charge)
{
    bridge->metric = std::make_shared<KerrMetric>(mass, spin);
    bridge->validator->setMetric(bridge->metric);

    auto report = bridge->validator->validate(Event4D(0.0, 0.0, 0.0, 0.0));
    bool valid = report.failedChecks == 0;
    bool warning = !valid;

    bridge->physicsWarning = warning;

    static thread_local std::string result;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    oss << "{";
    oss << "\"valid\":" << (valid ? "true" : "false") << ",";
    oss << "\"failedChecks\":" << report.failedChecks << ",";
    oss << "\"totalChecks\":" << report.totalChecks << ",";
    oss << "\"message\":\"";
    if (valid) {
        oss << "Spacetime valid — all physics checks passed.";
    } else {
        oss << "Physics validation failed: " << report.failedChecks
            << " of " << report.totalChecks << " checks failed.";
        for (const auto& f : report.failures) {
            oss << " [" << f.name << ": " << f.message << "]";
        }
    }
    oss << "\"}";
    result = oss.str();
    return result.c_str();
}

int qv_bridge_physics_warning(WasmBridge* bridge)
{
    return bridge->physicsWarning ? 1 : 0;
}

const char* qv_bridge_run_discovery_step(WasmBridge* bridge, double mass, double spin, double charge)
{
    std::vector<double> state(8, 0.0);
    state[0] = mass;
    state[1] = spin;
    state[2] = charge;

    double epsilon = 0.1;
    auto action = bridge->policy.sampleAction(state, epsilon);

    std::vector<double> params = {mass, spin, charge};
    action.apply(params);

    double newMass = params[0];
    double newSpin = params[1];
    double newCharge = params[2];

    newMass = std::clamp(newMass, 0.1, 100.0);
    newSpin = std::clamp(newSpin, 0.0, 1.5);
    newCharge = std::clamp(newCharge, 0.0, 1.5);

    double deltaM = newMass - mass;
    double deltaA = newSpin - spin;
    double deltaQ = newCharge - charge;

    double extremality = (newSpin * newSpin + newCharge * newCharge) /
        (newMass * newMass);

    auto testMetric = std::make_shared<KerrMetric>(newMass, newSpin);
    bridge->validator->setMetric(testMetric);
    auto report = bridge->validator->validate(Event4D(0.0, 0.0, 0.0, 0.0));
    bool isValid = report.failedChecks == 0;

    static thread_local std::string result;

    if (!isValid) {
        bridge->physicsWarning = true;

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(4);
        oss << "VETO: Proposed state M=" << newMass
            << ", a=" << newSpin
            << ", Q=" << newCharge
            << ". Violates cosmic censorship (naked singularity). "
            << "PhysicsValidator reported " << report.failedChecks
            << " failed checks.";
        bridge->llmCoPilot->analyzeAnomaly(oss.str());

        result.clear();
        result += "{";
        result += "\"deltaM\":0.0,\"deltaA\":0.0,\"deltaQ\":0.0,";
        result += "\"vetoed\":true,";
        result += "\"reason\":\"Physics validation failed: ";
        result += std::to_string(report.failedChecks);
        result += " checks failed\"";
        result += "}";
        return result.c_str();
    }

    bridge->physicsWarning = false;

    result.clear();
    result += "{";
    result += "\"deltaM\":" + std::to_string(deltaM) + ",";
    result += "\"deltaA\":" + std::to_string(deltaA) + ",";
    result += "\"deltaQ\":" + std::to_string(deltaQ) + ",";
    result += "\"vetoed\":false,";
    result += "\"extremality\":" + std::to_string(extremality);
    result += "}";
    return result.c_str();
}

void qv_bridge_analyze_anomaly(WasmBridge* bridge, const char* anomalyData)
{
    bridge->llmCoPilot->analyzeAnomaly(anomalyData ? anomalyData : "");
}

void qv_bridge_set_llm_callback(WasmBridge* bridge, LLMCallback callback)
{
    bridge->llmCallback = callback;
}

void qv_bridge_set_mass(WasmBridge* bridge, double mass)
{
    bridge->mass = mass;
    bridge->metric = std::make_shared<KerrMetric>(mass, bridge->spin);
}

void qv_bridge_set_spin(WasmBridge* bridge, double spin)
{
    bridge->spin = spin;
    bridge->metric = std::make_shared<KerrMetric>(bridge->mass, spin);
}

void qv_bridge_set_charge(WasmBridge* bridge, double charge)
{
    bridge->charge = charge;
}

double qv_bridge_get_mass(WasmBridge* bridge)
{
    return bridge->mass;
}

double qv_bridge_get_spin(WasmBridge* bridge)
{
    return bridge->spin;
}

double qv_bridge_get_charge(WasmBridge* bridge)
{
    return bridge->charge;
}

} // extern "C"

// ---------------------------------------------------------------------------
// Embind bindings: expose a JS-friendly WasmBridge class so app.js can use
// new Module.WasmBridge() and method calls directly.
// ---------------------------------------------------------------------------

namespace {

std::function<void(const std::string&)> g_llmCallbackBinding;

void llmCallbackTrampoline(const char* explanation)
{
    if (g_llmCallbackBinding && explanation) {
        g_llmCallbackBinding(explanation);
    }
}

} // namespace

class WasmBridgeJS {
public:
    WasmBridgeJS() : bridge_(qv_bridge_create()) {}
    ~WasmBridgeJS() { qv_bridge_destroy(bridge_); }

    std::string createMetric(double mass, double spin)
    {
        return qv_bridge_create_metric(bridge_, mass, spin);
    }

    std::string validateState(double mass, double spin, double charge)
    {
        return qv_bridge_validate_state(bridge_, mass, spin, charge);
    }

    int physicsWarning() const
    {
        return qv_bridge_physics_warning(bridge_);
    }

    std::string runDiscoveryStep(double mass, double spin, double charge)
    {
        return qv_bridge_run_discovery_step(bridge_, mass, spin, charge);
    }

    void analyzeAnomaly(const std::string& anomalyData)
    {
        qv_bridge_analyze_anomaly(bridge_, anomalyData.c_str());
    }

    void setLLMCallback(std::function<void(const std::string&)> cb)
    {
        g_llmCallbackBinding = std::move(cb);
        qv_bridge_set_llm_callback(bridge_, llmCallbackTrampoline);
    }

    void setMass(double m)
    {
        qv_bridge_set_mass(bridge_, m);
    }

    void setSpin(double s)
    {
        qv_bridge_set_spin(bridge_, s);
    }

    void setCharge(double c)
    {
        qv_bridge_set_charge(bridge_, c);
    }

    double getMass() const
    {
        return qv_bridge_get_mass(bridge_);
    }

    double getSpin() const
    {
        return qv_bridge_get_spin(bridge_);
    }

    double getCharge() const
    {
        return qv_bridge_get_charge(bridge_);
    }

private:
    WasmBridge* bridge_;
};

EMSCRIPTEN_BINDINGS(quantumverse_wasm)
{
    using namespace emscripten;

    class_<WasmBridgeJS>("WasmBridge")
        .constructor()
        .function("createMetric", &WasmBridgeJS::createMetric)
        .function("validateState", &WasmBridgeJS::validateState)
        .function("physicsWarning", &WasmBridgeJS::physicsWarning)
        .function("runDiscoveryStep", &WasmBridgeJS::runDiscoveryStep)
        .function("analyzeAnomaly", &WasmBridgeJS::analyzeAnomaly)
        .function("setLLMCallback", &WasmBridgeJS::setLLMCallback)
        .function("setMass", &WasmBridgeJS::setMass)
        .function("setSpin", &WasmBridgeJS::setSpin)
        .function("setCharge", &WasmBridgeJS::setCharge)
        .function("getMass", &WasmBridgeJS::getMass)
        .function("getSpin", &WasmBridgeJS::getSpin)
        .function("getCharge", &WasmBridgeJS::getCharge);
}
