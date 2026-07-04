#include "WorkflowAnalytics.h"
#include <numeric>
#include <algorithm>
#include <sstream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace quantumverse {

WorkflowAnalytics& WorkflowAnalytics::instance() {
    static WorkflowAnalytics instance;
    return instance;
}

void WorkflowAnalytics::recordBuild(const BuildMetrics& metrics) {
    m_build_history.push_back(metrics);
    if (m_build_history.size() > m_max_build_history) {
        m_build_history.erase(m_build_history.begin());
    }
}

void WorkflowAnalytics::recordTest(const TestMetrics& metrics) {
    m_test_history.push_back(metrics);
    if (m_test_history.size() > m_max_test_history) {
        m_test_history.erase(m_test_history.begin());
    }
}

double WorkflowAnalytics::getAverageBuildTime(int last_n) const {
    if (m_build_history.empty()) return 0.0;
    
    int count = std::min<int>(last_n, static_cast<int>(m_build_history.size()));
    double total = 0.0;
    for (int i = 0; i < count; ++i) {
        total += m_build_history[m_build_history.size() - 1 - i].build_time;
    }
    return total / count;
}

double WorkflowAnalytics::getTestPassRate(int last_n) const {
    if (m_test_history.empty()) return 0.0;
    
    int count = std::min<int>(last_n, static_cast<int>(m_test_history.size()));
    int passed = 0;
    for (int i = 0; i < count; ++i) {
        if (m_test_history[m_test_history.size() - 1 - i].passed) {
            ++passed;
        }
    }
    return (100.0 * passed) / count;
}

std::vector<std::string> WorkflowAnalytics::getSlowestTests(int count) const {
    std::vector<TestMetrics> sorted = m_test_history;
    std::sort(sorted.begin(), sorted.end(),
        [](const TestMetrics& a, const TestMetrics& b) {
            return a.duration > b.duration;
        });
    
    std::vector<std::string> result;
    for (int i = 0; i < count && i < static_cast<int>(sorted.size()); ++i) {
        result.push_back(sorted[i].test_name);
    }
    return result;
}

std::vector<std::string> WorkflowAnalytics::getFlakyTests() const {
    std::map<std::string, std::pair<int, int>> test_stats;
    
    for (const auto& test : m_test_history) {
        test_stats[test.test_name].first += test.passed ? 1 : 0;
        test_stats[test.test_name].second += 1;
    }
    
    std::vector<std::string> result;
    for (const auto& kv : test_stats) {
        const std::string& name = kv.first;
        int passed = kv.second.first;
        int total = kv.second.second;
        if (total >= 5) {
            double rate = static_cast<double>(passed) / total;
            if (rate > 0.3 && rate < 0.9) {
                result.push_back(name);
            }
        }
    }
    return result;
}

std::vector<std::string> WorkflowAnalytics::getRecommendations() const {
    std::vector<std::string> recommendations;
    
    // Check build time trends
    if (m_build_history.size() >= 5) {
        double avg = getAverageBuildTime(5);
        if (avg > 120.0) {
            recommendations.push_back(
                "Build time is high (" + std::to_string(avg) + "s). "
                "Consider using ccache or precompiled headers."
            );
        }
    }
    
    // Check test pass rate
    double pass_rate = getTestPassRate(20);
    if (pass_rate < 95.0) {
        recommendations.push_back(
            "Test pass rate is low (" + std::to_string(pass_rate) + "%). "
            "Review failing tests and fix flaky tests."
        );
    }
    
    // Check for slow tests
    auto slow_tests = getSlowestTests(5);
    if (!slow_tests.empty()) {
        recommendations.push_back(
            "Slow tests detected: " + slow_tests[0] + 
            ". Consider optimizing or splitting into smaller tests."
        );
    }
    
    return recommendations;
}

std::string WorkflowAnalytics::exportJSON() const {
    std::ostringstream oss;
    oss << "{\n  \"build_history\": [\n";
    
    for (size_t i = 0; i < m_build_history.size(); ++i) {
        const auto& b = m_build_history[i];
        oss << "    {\"build_time\": " << b.build_time
            << ", \"test_passed\": " << b.test_passed
            << ", \"test_failed\": " << b.test_failed
            << ", \"warnings\": " << b.warnings << "}";
        if (i < m_build_history.size() - 1) oss << ",";
        oss << "\n";
    }
    
    oss << "  ],\n  \"test_history\": [\n";
    
    for (size_t i = 0; i < m_test_history.size(); ++i) {
        const auto& t = m_test_history[i];
        oss << "    {\"name\": \"" << t.test_name
            << "\", \"duration\": " << t.duration
            << ", \"passed\": " << (t.passed ? "true" : "false") << "}";
        if (i < m_test_history.size() - 1) oss << ",";
        oss << "\n";
    }
    
    oss << "  ]\n}";
    return oss.str();
}

// BuildOptimizer implementation
std::vector<BuildOptimizer::OptimizationSuggestion> 
BuildOptimizer::analyze(const std::vector<WorkflowAnalytics::BuildMetrics>& history) const {
    std::vector<OptimizationSuggestion> suggestions;
    
    if (history.empty()) return suggestions;
    
    // Analyze build times
    double avg_time = 0.0;
    for (const auto& m : history) {
        avg_time += m.build_time;
    }
    avg_time /= history.size();
    
    if (avg_time > 60.0) {
        suggestions.push_back({
            "parallelization",
            "Build time is high",
            "Increase parallel jobs to " + std::to_string(getOptimalJobCount()),
            25.0
        });
    }
    
    // Check for warnings
    int total_warnings = 0;
    for (const auto& m : history) {
        total_warnings += m.warnings;
    }
    if (total_warnings > 50) {
        suggestions.push_back({
            "warnings",
            "High warning count detected",
            "Enable stricter warning flags and fix warnings",
            10.0
        });
    }
    
    return suggestions;
}

int BuildOptimizer::getOptimalJobCount() const {
    // Simple heuristic: use number of CPU cores
#ifdef _WIN32
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    int cores = static_cast<int>(sysinfo.dwNumberOfProcessors);
#else
    int cores = sysconf(_SC_NPROCESSORS_ONLN);
#endif
    return std::max<int>(1, cores - 1);
}

std::vector<std::string> BuildOptimizer::getRecommendedFlags(
    const std::string& build_type
) const {
    if (build_type == "Release") {
        return {"-O3", "-march=native", "-DNDEBUG"};
    } else {
        return {"-g", "-O0", "-Wall", "-Wextra"};
    }
}

} // namespace quantumverse