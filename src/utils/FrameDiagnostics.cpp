/**
 * @file FrameDiagnostics.cpp
 * @brief Implementation of per-frame diagnostics collection
 */

#include "FrameDiagnostics.h"
#include <iomanip>
#include <sstream>

namespace quantumverse {
namespace utils {

FrameDiagnostics::FrameDiagnostics()
    : start_time_(std::chrono::steady_clock::now())
{
}

FrameDiagnostics::~FrameDiagnostics() = default;

void FrameDiagnostics::recordFrame(const FrameSnapshot& snapshot) {
    if (!enabled_) return;
    std::lock_guard<std::mutex> lock(mutex_);
    frames_.push_back(snapshot);
}

void FrameDiagnostics::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    frames_.clear();
    start_time_ = std::chrono::steady_clock::now();
}

size_t FrameDiagnostics::frameCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return frames_.size();
}

static std::string frameToJson(const FrameSnapshot& f) {
    std::ostringstream json;
    json << "{";
    json << "\"frame\":" << f.frame_number << ",";
    json << "\"timestamp_s\":" << std::fixed << std::setprecision(6) << f.timestamp_s << ",";
    json << "\"frame_time_ms\":" << f.frame_time_ms << ",";
    json << "\"physics_time_ms\":" << f.physics_time_ms << ",";
    json << "\"render_time_ms\":" << f.render_time_ms << ",";
    json << "\"ui_time_ms\":" << f.ui_time_ms << ",";
    json << "\"active_geodesics\":" << f.active_geodesics << ",";
    json << "\"vram_used_mb\":" << f.vram_used_mb << ",";
    json << "\"gl_error_count\":" << f.gl_error_count;
    json << "}";
    return json.str();
}

bool FrameDiagnostics::writeJson(const std::string& filepath) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ofstream file(filepath);
    if (!file.is_open()) {
        return false;
    }

    file << "{\n";
    file << "  \"frames\": [\n";

    for (size_t i = 0; i < frames_.size(); ++i) {
        file << "    " << frameToJson(frames_[i]);
        if (i + 1 < frames_.size()) {
            file << ",";
        }
        file << "\n";
    }

    file << "  ],\n";
    file << "  \"total_frames\": " << frames_.size() << ",\n";

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_).count();
    double elapsed_s = elapsed / 1000.0;

    double avg_frame_ms = 0.0;
    double max_frame_ms = 0.0;
    double min_frame_ms = 1e9;
    double total_physics_ms = 0.0;
    double total_render_ms = 0.0;

    if (!frames_.empty()) {
        for (const auto& f : frames_) {
            avg_frame_ms += f.frame_time_ms;
            total_physics_ms += f.physics_time_ms;
            total_render_ms += f.render_time_ms;
            if (f.frame_time_ms > max_frame_ms) max_frame_ms = f.frame_time_ms;
            if (f.frame_time_ms < min_frame_ms) min_frame_ms = f.frame_time_ms;
        }
        avg_frame_ms /= frames_.size();
    }

    double avg_fps = avg_frame_ms > 0.0 ? 1000.0 / avg_frame_ms : 0.0;

    file << "  \"summary\": {\n";
    file << "    \"elapsed_s\": " << std::fixed << std::setprecision(3) << elapsed_s << ",\n";
    file << "    \"avg_frame_ms\": " << avg_frame_ms << ",\n";
    file << "    \"min_frame_ms\": " << (min_frame_ms < 1e9 ? min_frame_ms : 0.0) << ",\n";
    file << "    \"max_frame_ms\": " << max_frame_ms << ",\n";
    file << "    \"avg_fps\": " << avg_fps << ",\n";
    file << "    \"total_physics_ms\": " << total_physics_ms << ",\n";
    file << "    \"total_render_ms\": " << total_render_ms << ",\n";
    file << "    \"total_frames\": " << frames_.size() << "\n";
    file << "  }\n";
    file << "}\n";

    return true;
}

} // namespace utils
} // namespace quantumverse
