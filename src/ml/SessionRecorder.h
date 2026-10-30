/**
 * @file SessionRecorder.h
 * @brief Persists simulation frames to disk for ML world-model training.
 *
 * Each session is stored in data/sessions/<uuid>/ with:
 *   metadata.json  - session UUID, timestamps, body names, frame count
 *   config.json    - recording configuration (maxFrames, frameInterval, etc.)
 *   frames.bin     - binary packed frame data (time, positions, velocities,
 *                    camera state, grid Z-range, black hole mass)
 *
 * Binary frame layout (little-endian, native double):
 *   double  time                          - simulation time (s)
 *   uint32  bodyCount                     - number of bodies
 *   per body: uint32 nameLen + char[]     - body name
 *             double pos[3]               - x, y, z (m)
 *             double vel[3]               - vx, vy, vz (m/s)
 *   double  cameraPos[4]                  - t, x, y, z
 *   double  cameraLookAt[4]               - t, x, y, z
 *   double  blackHoleMassKg               - active BH mass (kg)
 *
 * Grid Z-range (gridZmin/gridZmax) is NOT in schemaVersion 1:
 *   CurvatureRenderer::deformGrid() returns sentinel "-1..-1" when all
 *   grid vertices fall inside the event horizon (gtt >= 0 or non-finite),
   *   so recording it would bake a constant sentinel into training data.
 *   Re-add in schemaVersion 2 once gridZRange varies with mass is verified.
 */

#pragma once

#include <string>
#include <vector>
#include <array>
#include <cstdint>
#include <memory>
#include <mutex>
#include <fstream>
#include <filesystem>
#include <optional>
#include <random>
#include <chrono>
#include "spacetime/Event4D.h"
#include "ui4d/UI4D.h"

namespace quantumverse {

/**
 * @brief Binary header for frames.bin — versioned so schema changes are
 *        caught loudly at read time instead of silently misinterpreting bytes.
 *
 * 64 bytes exactly (cache-line aligned, easy to spot in a hex dump).
 * All multi-byte fields are little-endian; kEndianMarker lets a reader
 * detect big/little-endian mismatch before trusting any other field.
 */
struct SessionHeader {
    std::uint32_t magic;            // kSessionMagic ("QVFR")
    std::uint32_t schemaVersion;    // kSchemaVersion
    std::uint32_t endianMarker;     // kEndianMarker
    std::uint32_t recordSize;       // sizeof(FrameRecord) — catches packing drift
    std::uint64_t frameCount;       // written at stop(), 0 until then
    std::uint64_t createdAtUnix;    // seconds since epoch
    std::uint64_t gitCommitPrefix;  // first 8 hex chars of commit, parsed to uint64
    std::uint8_t  reserved[24];     // future flags, zero-filled
};
static_assert(sizeof(SessionHeader) == 64,
              "SessionHeader must be 64 bytes");

constexpr std::uint32_t kSessionMagic   = 0x51564652;  // "QVFR"
constexpr std::uint32_t kSchemaVersion  = 1;
constexpr std::uint32_t kEndianMarker   = 0x01020304;

/**
 * @brief A single simulation frame captured for training.
 *
 * Contains all state needed to reconstruct a single timestep:
 * body positions/velocities, camera state, grid deformation, and
 * black hole mass.
 */
struct FrameRecord {
    double time = 0.0;                                    // simulation time (s)
    std::vector<std::string> bodyNames;                   // body names
    std::vector<double> positions;                        // x,y,z interleaved (m)
    std::vector<double> velocities;                       // vx,vy,vz interleaved (m/s)
    double totalEnergy = 0.0;                             // J
    double kineticEnergy = 0.0;                           // J
    double potentialEnergy = 0.0;                         // J
    std::array<double, 4> cameraPosition{};               // t,x,y,z
    std::array<double, 4> cameraLookAt{};                 // t,x,y,z
    double blackHoleMassKg = 0.0;                         // active BH mass (kg)
};

/**
 * @brief Session metadata written to metadata.json.
 */
struct SessionMetadata {
    std::string sessionId;                                // UUID
    double startTime = 0.0;                               // wall clock (epoch seconds)
    std::vector<std::string> bodyNames;                   // body names in order
    std::uint32_t frameCount = 0;                         // total frames recorded
    double simDuration = 0.0;                             // total sim time covered (s)
};

/**
 * @brief SessionRecorder configuration.
 */
struct SessionRecorderConfig {
    std::string baseDir = "data/sessions";                // root for all sessions
    std::uint32_t maxFrames = 6000;                       // cap per session
    std::uint32_t frameInterval = 1;                      // record every N sim frames
};

/**
 * @brief Persists simulation frames to disk in a session directory.
 *
 * Thread-safe: start/stop/recordFrame are guarded by an internal mutex.
 * Frames are buffered in memory and flushed on stop() or when the buffer
 * reaches maxFrames.
 */
class SessionRecorder {
public:
    SessionRecorder();
    ~SessionRecorder();

    // Non-copyable
    SessionRecorder(const SessionRecorder&) = delete;
    SessionRecorder& operator=(const SessionRecorder&) = delete;

    /**
     * @brief Configure recording parameters.
     */
    void configure(const SessionRecorderConfig& cfg);

    /**
     * @brief Start a new recording session. Creates a UUID-named directory
     *        under baseDir and writes metadata.json + config.json.
     * @return The session UUID.
     */
    std::string start();

    /**
     * @brief Stop recording, flush remaining frames to frames.bin, and
     *        update metadata.json with final frame count.
     */
    void stop();

    /**
     * @brief Record a single frame. Called from the simulation loop.
     *        Thread-safe; copies all data into the internal buffer.
     */
    void recordFrame(const FrameRecord& frame);

    /**
     * @brief Check if recording is active.
     */
    bool isActive() const noexcept { return active_; }

    /**
     * @brief Get the current session UUID (empty if not recording).
     */
    const std::string& sessionId() const noexcept { return sessionId_; }

    /**
     * @brief Get the number of frames recorded so far.
     */
    std::uint32_t frameCount() const noexcept { return frameCount_; }

private:
    SessionRecorderConfig config_;
    std::mutex mutex_;
    bool active_ = false;
    std::string sessionId_;
    std::filesystem::path sessionDir_;
    std::uint32_t frameCount_ = 0;
    std::vector<FrameRecord> buffer_;
    std::ofstream framesFile_;

    bool writeMetadata(const SessionMetadata& meta) const;
    bool writeConfig() const;
    bool appendFrameToFile(const FrameRecord& frame);
    void flushBuffer();

public:
    /**
     * @brief Read all frames from frames.bin in the given session directory.
     *        Used by round-trip tests to verify write/read fidelity.
     */
    static std::vector<FrameRecord> readFrames(const std::filesystem::path& sessionDir);

    /**
     * @brief Read a single session's metadata.json.
     */
    static std::unique_ptr<SessionMetadata> readMetadata(
        const std::filesystem::path& sessionDir);

    /**
     * @brief Read the SessionHeader from frames.bin.
     *        Used by round-trip tests to verify header fidelity.
     * @return Header with all fields, or empty std::optional on failure.
     */
    static std::optional<SessionHeader> readHeader(
        const std::filesystem::path& sessionDir);
};

} // namespace quantumverse
