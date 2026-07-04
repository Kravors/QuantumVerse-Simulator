/**
 * @file PerformanceMonitor.h
 * @brief Performance monitoring and profiling utilities for QuantumVerse
 *
 * Provides real-time performance metrics including frame timing,
 * memory usage, and GPU statistics.
 */

#pragma once

#include <chrono>
#include <vector>
#include <string>
#include <cstdint>
#include <algorithm>

namespace quantumverse {
namespace utils {

// Public type aliases for use in ScopedTimer
using PerfClock = std::chrono::high_resolution_clock;
using PerfTimePoint = std::chrono::time_point<PerfClock>;

/**
 * @brief Performance metrics snapshot
 */
struct PerformanceMetrics {
    double frameTimeMs;           ///< Frame time in milliseconds
    double fps;                   ///< Frames per second
    double cpuUsagePercent;       ///< CPU usage percentage
    size_t memoryUsedMB;          ///< Memory used in MB
    size_t vramUsedMB;            ///< VRAM used in MB
    double renderTimeMs;          ///< Rendering time in ms
    double updateTimeMs;          ///< Update time in ms
    double uiTimeMs;              ///< UI render time in ms
    int drawCalls;                ///< Number of draw calls
    int triangleCount;            ///< Number of triangles rendered
};

/**
 * @brief High-resolution performance monitor
 */
class PerformanceMonitor {
public:
    PerformanceMonitor();
    ~PerformanceMonitor() = default;

    /**
     * @brief Start frame timing
     */
    void beginFrame();

    /**
     * @brief End frame timing and record metrics
     */
    void endFrame();

    /**
     * @brief Record a timing event
     * @param name Event name
     * @param durationMs Duration in milliseconds
     */
    void recordEvent(const std::string& name, double durationMs);

    /**
     * @brief Get current performance metrics
     */
    const PerformanceMetrics& getMetrics() const { return m_currentMetrics; }

    /**
     * @brief Get average FPS over last N frames
     * @param frameCount Number of frames to average
     */
    double getAverageFPS(size_t frameCount = 60) const;

    /**
     * @brief Get average frame time over last N frames
     */
    double getAverageFrameTime(size_t frameCount = 60) const;

    /**
     * @brief Reset all statistics
     */
    void reset();

    /**
     * @brief Get event duration by name
     */
    double getEventDuration(const std::string& name) const;

    /**
     * @brief Begin UI render timing section
     */
    void beginUIRender();

    /**
     * @brief End UI render timing section
     */
    void endUIRender();

    /**
     * @brief Get UI render time in milliseconds
     */
    double getUIRenderTimeMs() const { return m_currentMetrics.uiTimeMs; }

    /**
     * @brief Check if performance target is met (≥45 FPS)
     */
    bool isPerformanceTargetMet() const { return m_currentMetrics.fps >= 45.0; }

    /**
     * @brief Get performance grade based on FPS
     * @return 'A' for ≥60 FPS, 'B' for ≥45 FPS, 'C' for ≥30 FPS, 'D' otherwise
     */
    char getPerformanceGrade() const;

private:
    PerfTimePoint m_frameStart;
    PerfTimePoint m_renderStart;
    PerfTimePoint m_updateStart;
    PerfTimePoint m_uiStart;

    PerformanceMetrics m_currentMetrics;

    // History buffers for averaging
    std::vector<double> m_frameTimeHistory;
    std::vector<double> m_fpsHistory;

    // Event timing map
    std::vector<std::pair<std::string, double>> m_eventTimings;

    // Statistics
    size_t m_frameCount;
    double m_totalTimeMs;
    int m_drawCalls;
    int m_triangleCount;

    // Memory tracking
    size_t m_peakMemoryMB;
    size_t m_peakVRAMMB;
};

/**
 * @brief Scoped timer for automatic event timing
 */
class ScopedTimer {
public:
    explicit ScopedTimer(PerformanceMonitor& monitor, const std::string& name)
        : m_monitor(monitor), m_name(name), m_start(PerfClock::now()) {}

    ~ScopedTimer() {
        auto end = PerfClock::now();
        auto duration = std::chrono::duration<double, std::milli>(end - m_start).count();
        m_monitor.recordEvent(m_name, duration);
    }

private:
    PerformanceMonitor& m_monitor;
    std::string m_name;
    PerfTimePoint m_start;
};

/**
 * @brief RAII scope for render timing
 */
class RenderScope {
public:
    explicit RenderScope(PerformanceMonitor& monitor) : m_monitor(monitor) {
        m_monitor.beginFrame();
    }

    ~RenderScope() {
        m_monitor.endFrame();
    }

private:
    PerformanceMonitor& m_monitor;
};

} // namespace utils
} // namespace quantumverse

// Convenience macro for scoped timing
#define QUANTUMVERSE_SCOPED_TIMER(monitor, name) \
    quantumverse::utils::ScopedTimer _timer_##__LINE__(monitor, name)