/**
 * @file PerformanceMonitor.cpp
 * @brief Implementation of performance monitoring utilities
 */

#include "PerformanceMonitor.h"
#include <iostream>

namespace quantumverse {
namespace utils {

PerformanceMonitor::PerformanceMonitor()
    : m_frameCount(0)
    , m_totalTimeMs(0.0)
    , m_drawCalls(0)
    , m_triangleCount(0)
    , m_peakMemoryMB(0)
    , m_peakVRAMMB(0) {
    
    m_currentMetrics.frameTimeMs = 0.0;
    m_currentMetrics.fps = 0.0;
    m_currentMetrics.cpuUsagePercent = 0.0;
    m_currentMetrics.memoryUsedMB = 0;
    m_currentMetrics.vramUsedMB = 0;
    m_currentMetrics.renderTimeMs = 0.0;
    m_currentMetrics.updateTimeMs = 0.0;
    m_currentMetrics.uiTimeMs = 0.0;
    m_currentMetrics.drawCalls = 0;
    m_currentMetrics.triangleCount = 0;
}

void PerformanceMonitor::beginFrame() {
    m_frameStart = PerfClock::now();
    m_drawCalls = 0;
    m_triangleCount = 0;
}

void PerformanceMonitor::endFrame() {
    auto end = PerfClock::now();
    double frameTime = std::chrono::duration<double, std::milli>(end - m_frameStart).count();
    
    m_currentMetrics.frameTimeMs = frameTime;
    m_currentMetrics.fps = 1000.0 / std::max(frameTime, 0.001);
    m_currentMetrics.drawCalls = m_drawCalls;
    m_currentMetrics.triangleCount = m_triangleCount;
    
    // Store in history (keep last 120 frames)
    m_frameTimeHistory.push_back(frameTime);
    m_fpsHistory.push_back(m_currentMetrics.fps);
    
    if (m_frameTimeHistory.size() > 120) {
        m_frameTimeHistory.erase(m_frameTimeHistory.begin());
        m_fpsHistory.erase(m_fpsHistory.begin());
    }
    
    m_frameCount++;
    m_totalTimeMs += frameTime;
}

void PerformanceMonitor::recordEvent(const std::string& name, double durationMs) {
    m_eventTimings.push_back({name, durationMs});
}

double PerformanceMonitor::getAverageFPS(size_t frameCount) const {
    if (m_fpsHistory.empty()) return 0.0;
    
    size_t count = std::min(frameCount, m_fpsHistory.size());
    double sum = 0.0;
    for (size_t i = m_fpsHistory.size() - count; i < m_fpsHistory.size(); ++i) {
        sum += m_fpsHistory[i];
    }
    return sum / static_cast<double>(count);
}

double PerformanceMonitor::getAverageFrameTime(size_t frameCount) const {
    if (m_frameTimeHistory.empty()) return 0.0;
    
    size_t count = std::min(frameCount, m_frameTimeHistory.size());
    double sum = 0.0;
    for (size_t i = m_frameTimeHistory.size() - count; i < m_frameTimeHistory.size(); ++i) {
        sum += m_frameTimeHistory[i];
    }
    return sum / static_cast<double>(count);
}

void PerformanceMonitor::reset() {
    m_frameTimeHistory.clear();
    m_fpsHistory.clear();
    m_eventTimings.clear();
    m_frameCount = 0;
    m_totalTimeMs = 0.0;
}

double PerformanceMonitor::getEventDuration(const std::string& name) const {
    for (const auto& event : m_eventTimings) {
        if (event.first == name) {
            return event.second;
        }
    }
    return 0.0;
}

void PerformanceMonitor::beginUIRender() {
    m_uiStart = PerfClock::now();
}

void PerformanceMonitor::endUIRender() {
    auto end = PerfClock::now();
    double uiTime = std::chrono::duration<double, std::milli>(end - m_uiStart).count();
    m_currentMetrics.uiTimeMs = uiTime;
}

char PerformanceMonitor::getPerformanceGrade() const {
    if (m_currentMetrics.fps >= 60.0) return 'A';
    if (m_currentMetrics.fps >= 45.0) return 'B';
    if (m_currentMetrics.fps >= 30.0) return 'C';
    return 'D';
}

} // namespace utils
} // namespace quantumverse