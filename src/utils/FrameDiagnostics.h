/**
 * @file FrameDiagnostics.h
 * @brief Per-frame diagnostics collection and JSON output
 *
 * Collects frame-level timing and state information for performance
 * regression detection and runtime observability.
 */

#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <chrono>

namespace quantumverse {
namespace utils {

/**
 * @brief Per-frame diagnostics snapshot
 */
struct FrameSnapshot {
    double frame_time_ms = 0.0;
    double physics_time_ms = 0.0;
    double render_time_ms = 0.0;
    double ui_time_ms = 0.0;
    int active_geodesics = 0;
    double vram_used_mb = 0.0;
    int gl_error_count = 0;
    int frame_number = 0;
    double timestamp_s = 0.0;
};

/**
 * @brief Frame-level diagnostics collector
 *
 * Thread-safe collector that records per-frame timing and state snapshots.
 * On shutdown, writes the collected data as a JSON array to a file.
 */
class FrameDiagnostics {
public:
    FrameDiagnostics();
    ~FrameDiagnostics();

    FrameDiagnostics(const FrameDiagnostics&) = delete;
    FrameDiagnostics& operator=(const FrameDiagnostics&) = delete;

    /**
     * @brief Record a new frame snapshot
     * @param snapshot Frame data to record
     */
    void recordFrame(const FrameSnapshot& snapshot);

    /**
     * @brief Reset all collected data
     */
    void reset();

    /**
     * @brief Write collected frames as JSON array to file
     * @param filepath Output file path
     * @return true on success, false on failure
     */
    bool writeJson(const std::string& filepath) const;

    /**
     * @brief Get number of recorded frames
     */
    size_t frameCount() const;

    /**
     * @brief Check if diagnostics collection is enabled
     */
    bool isEnabled() const { return enabled_; }

    /**
     * @brief Enable or disable diagnostics collection
     */
    void setEnabled(bool enabled) { enabled_ = enabled; }

private:
    mutable std::mutex mutex_;
    std::vector<FrameSnapshot> frames_;
    bool enabled_ = false;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace utils
} // namespace quantumverse
