/**
 * @file TheoryManager.cpp
 * @brief Implementation of TheoryManager for plugin-based physics theories
 * 
 * Manages registration, selection, and invocation of alternative gravity theories.
 * Integrates with DiscoveryEngine for anomaly detection and hypothesis testing.
 */

#include "DiscoveryEngine.h"
#include <algorithm>
#include <sstream>

namespace quantumverse {

TheoryManager::TheoryManager() : activeTheory("") {}

bool TheoryManager::registerPlugin(std::unique_ptr<TheoryPlugin> plugin) {
    if (!plugin) return false;
    
    std::string name = plugin->getName();
    if (plugins.find(name) != plugins.end()) {
        // Already registered
        return false;
    }
    
    plugins[name] = std::move(plugin);
    return true;
}

bool TheoryManager::unregisterPlugin(const std::string& name) {
    auto it = plugins.find(name);
    if (it != plugins.end()) {
        plugins.erase(it);
        if (activeTheory == name) {
            activeTheory = "";
        }
        return true;
    }
    return false;
}

TheoryPlugin* TheoryManager::getPlugin(const std::string& name) {
    auto it = plugins.find(name);
    if (it != plugins.end()) {
        return it->second.get();
    }
    return nullptr;
}

const TheoryPlugin* TheoryManager::getPlugin(const std::string& name) const {
    auto it = plugins.find(name);
    if (it != plugins.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool TheoryManager::setActiveTheory(const std::string& name) {
    if (plugins.find(name) != plugins.end()) {
        activeTheory = name;
        return true;
    }
    return false;
}

std::string TheoryManager::getActiveTheory() const {
    return activeTheory;
}

MetricTensor TheoryManager::computeMetric(
    const Event4D& location,
    const std::map<std::string, double>& parameters
) const {
    if (activeTheory.empty()) {
        // Return Minkowski
        MetricTensor minkowski;
        minkowski.setToMinkowski();
        return minkowski;
    }
    
    auto it = plugins.find(activeTheory);
    if (it == plugins.end()) {
        MetricTensor minkowski;
        minkowski.setToMinkowski();
        return minkowski;
    }
    
    return it->second->computeMetric(location, parameters);
}

std::vector<std::string> TheoryManager::getAvailableTheories() const {
    std::vector<std::string> names;
    for (const auto& pair : plugins) {
        names.push_back(pair.first);
    }
    std::sort(names.begin(), names.end());
    return names;
}

std::string TheoryManager::getTheoryDescription(const std::string& name) const {
    auto it = plugins.find(name);
    if (it != plugins.end()) {
        return it->second->getDescription();
    }
    return "";
}

std::map<std::string, double> TheoryManager::compareTheories(
    const Event4D& location,
    const std::map<std::string, double>& parameters
) const {
    std::map<std::string, double> results;
    for (const auto& pair : plugins) {
        MetricTensor m = pair.second->computeMetric(location, parameters);
        // Simple comparison: use g_tt component as a scalar measure
        results[pair.first] = m.g[0][0];
    }
    return results;
}

std::string TheoryManager::exportComparison(
    const std::vector<std::string>& theoryNames,
    const Event4D& location
) const {
    std::ostringstream oss;
    oss << "Theory Comparison at location (t=" << location.t
        << ", x=" << location.x << ", y=" << location.y << ", z=" << location.z << ")\n";
    oss << "==============================================\n";
    
    for (const auto& name : theoryNames) {
        auto it = plugins.find(name);
        if (it != plugins.end()) {
            MetricTensor m = it->second->computeMetric(location, {});
            oss << name << ":\n";
            oss << "  g_tt = " << m.g[0][0] << "\n";
            oss << "  g_rr = " << m.g[1][1] << "\n";
            oss << "  g_θθ = " << m.g[2][2] << "\n";
            oss << "  g_φφ = " << m.g[3][3] << "\n";
        }
    }
    return oss.str();
}

} // namespace quantumverse