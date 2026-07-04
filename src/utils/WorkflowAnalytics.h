#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <optional>

namespace quantumverse {

/**
 * @brief Build and test analytics collector
 * 
 * Collects performance metrics, test results, and build statistics
 * for AI-driven optimization and trend analysis.
 */
class WorkflowAnalytics {
public:
    struct BuildMetrics {
        double configure_time;
        double build_time;
        double test_time;
        int test_passed;
        int test_failed;
        int warnings;
        int errors;
        std::string build_type;
        std::string platform;
        std::chrono::system_clock::time_point timestamp;
    };

    struct TestMetrics {
        std::string test_name;
        double duration;
        bool passed;
        std::string category;
        std::chrono::system_clock::time_point timestamp;
    };

    /**
     * @brief Get singleton instance
     */
    static WorkflowAnalytics& instance();

    /**
     * @brief Record build metrics
     */
    void recordBuild(const BuildMetrics& metrics);

    /**
     * @brief Record test metrics
     */
    void recordTest(const TestMetrics& metrics);

    /**
     * @brief Get build trend analysis
     * @return Average build time over last N builds
     */
    double getAverageBuildTime(int last_n = 10) const;

    /**
     * @brief Get test pass rate trend
     * @return Pass rate percentage over last N tests
     */
    double getTestPassRate(int last_n = 50) const;

    /**
     * @brief Get slowest tests
     * @param count Number of tests to return
     * @return List of slowest test names
     */
    std::vector<std::string> getSlowestTests(int count = 10) const;

    /**
     * @brief Get flaky tests (inconsistent pass/fail)
     * @return List of potentially flaky test names
     */
    std::vector<std::string> getFlakyTests() const;

    /**
     * @brief Generate optimization recommendations
     * @return List of actionable recommendations
     */
    std::vector<std::string> getRecommendations() const;

    /**
     * @brief Export analytics to JSON
     */
    std::string exportJSON() const;

private:
    WorkflowAnalytics() = default;
    
    std::vector<BuildMetrics> m_build_history;
    std::vector<TestMetrics> m_test_history;
    int m_max_build_history = 100;
    int m_max_test_history = 500;
};

/**
 * @brief AI-driven build optimizer
 * 
 * Analyzes historical build data to suggest:
 * - Optimal parallel job count
 * - Compiler flag improvements
 * - Cache strategies
 */
class BuildOptimizer {
public:
    struct OptimizationSuggestion {
        std::string type;
        std::string description;
        std::string recommendation;
        double expected_improvement_percent;
    };

    /**
     * @brief Analyze build history and suggest optimizations
     */
    std::vector<OptimizationSuggestion> analyze(
        const std::vector<WorkflowAnalytics::BuildMetrics>& history
    ) const;

    /**
     * @brief Get optimal parallel job count based on system
     */
    int getOptimalJobCount() const;

    /**
     * @brief Get recommended compiler flags
     * @param build_type Debug or Release
     */
    std::vector<std::string> getRecommendedFlags(
        const std::string& build_type
    ) const;
};

} // namespace quantumverse