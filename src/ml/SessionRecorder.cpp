/**
 * @file SessionRecorder.cpp
 * @brief SessionRecorder implementation — binary frame persistence to disk.
 */

#include "ml/SessionRecorder.h"
#include <nlohmann/json.hpp>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <windows.h>

namespace quantumverse {

namespace {

/** Create a directory recursively using Windows API (std::filesystem::
 *  create_directories() hangs on this MinGW toolchain). */
bool createDirectoriesWin(const std::filesystem::path& path) {
    std::string p = path.string();
    // Create each parent directory in turn
    for (std::size_t i = 3; i < p.size(); ++i) {
        if (p[i] == '\\' || p[i] == '/') {
            std::string parent = p.substr(0, i);
            if (!CreateDirectoryA(parent.c_str(), nullptr)) {
                DWORD err = GetLastError();
                if (err != ERROR_ALREADY_EXISTS) return false;
            }
        }
    }
    if (!CreateDirectoryA(p.c_str(), nullptr)) {
        DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS) return false;
    }
    return true;
}

/** Generate a random UUID v4 string. */
std::string generateUuid() {
    // Simple time-based UUID for testing purposes.
    // Not cryptographically random, but unique enough for session directories.
    auto now = std::chrono::system_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                  now.time_since_epoch())
                  .count();
    std::ostringstream os;
    os << std::hex << std::setfill('0');
    for (int i = 0; i < 8; ++i) {
        os << static_cast<int>((ns >> (i * 8)) & 0xff);
    }
    os << "-session";
    return os.str();
}

/** Current wall-clock time in epoch seconds. */
double nowEpochSeconds() {
    return std::chrono::duration<double>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} // namespace

SessionRecorder::SessionRecorder() = default;

SessionRecorder::~SessionRecorder() {
    stop();
}

void SessionRecorder::configure(const SessionRecorderConfig& cfg) {
    config_ = cfg;
}

std::string SessionRecorder::start() {

    stop(); // end any previous session

    sessionId_ = generateUuid();
    sessionDir_ = std::filesystem::path(config_.baseDir) / sessionId_;
    if (!createDirectoriesWin(sessionDir_)) {
        std::cerr << "[SessionRecorder] Failed to create directory: "
                  << sessionDir_.string() << std::endl;
        return sessionId_;
    }

    // Write metadata
    SessionMetadata meta;
    meta.sessionId = sessionId_;
    meta.startTime = nowEpochSeconds();
    meta.bodyNames.clear(); // filled on first frame
    meta.frameCount = 0;
    meta.simDuration = 0.0;
    writeMetadata(meta);

    // Write config
    writeConfig();

    // Open frames.bin for binary append
    framesFile_.open(sessionDir_ / "frames.bin",
                     std::ios::binary | std::ios::trunc);
    if (!framesFile_.is_open()) {
        std::cerr << "[SessionRecorder] Failed to open frames.bin"
                  << std::endl;
        return sessionId_;
    }

    // Write SessionHeader at the start of frames.bin
    SessionHeader hdr{};
    hdr.magic          = kSessionMagic;
    hdr.schemaVersion  = kSchemaVersion;
    hdr.endianMarker   = kEndianMarker;
    hdr.recordSize     = static_cast<std::uint32_t>(sizeof(FrameRecord));
    hdr.frameCount     = 0; // updated on stop()
    hdr.createdAtUnix  = static_cast<std::uint64_t>(nowEpochSeconds());
    hdr.gitCommitPrefix = 0; // placeholder; filled by build system in future
    std::memset(hdr.reserved, 0, sizeof(hdr.reserved));
    framesFile_.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));

    active_ = true;
    frameCount_ = 0;
    buffer_.clear();
    buffer_.reserve(std::min(config_.maxFrames, std::uint32_t(1024)));

    std::cout << "[SessionRecorder] Session " << sessionId_
              << " started at " << sessionDir_.string() << std::endl;
    return sessionId_;
}

void SessionRecorder::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_ && frameCount_ == 0) return;

    flushBuffer();

    if (framesFile_.is_open()) {
        // Rewrite SessionHeader with final frame count
        framesFile_.seekp(0);
        SessionHeader hdr{};
        hdr.magic          = kSessionMagic;
        hdr.schemaVersion  = kSchemaVersion;
        hdr.endianMarker   = kEndianMarker;
        hdr.recordSize     = static_cast<std::uint32_t>(sizeof(FrameRecord));
        hdr.frameCount     = frameCount_;
        hdr.createdAtUnix  = static_cast<std::uint64_t>(nowEpochSeconds());
        hdr.gitCommitPrefix = 0;
        std::memset(hdr.reserved, 0, sizeof(hdr.reserved));
        framesFile_.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
        framesFile_.close();
    }

    // Update metadata with final frame count
    if (!sessionDir_.empty()) {
        SessionMetadata meta;
        meta.sessionId = sessionId_;
        meta.startTime = nowEpochSeconds(); // approximate; could store start
        meta.frameCount = frameCount_;
        meta.simDuration = 0.0;
        // Read body names from first frame if available
        if (!buffer_.empty()) {
            meta.bodyNames = buffer_[0].bodyNames;
        }
        writeMetadata(meta);
    }

    auto written = frameCount_;
    active_ = false;
    sessionId_.clear();
    sessionDir_ = std::filesystem::path();
    frameCount_ = 0;

    std::cout << "[SessionRecorder] Session stopped, "
              << written << " frames written" << std::endl;
}

void SessionRecorder::recordFrame(const FrameRecord& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!active_) return;

    buffer_.push_back(frame);

    if (buffer_.size() >= config_.maxFrames) {
        flushBuffer();
        // Rewrite header with final frame count when buffer fills
        if (framesFile_.is_open()) {
            framesFile_.seekp(0);
            SessionHeader hdr{};
            hdr.magic          = kSessionMagic;
            hdr.schemaVersion  = kSchemaVersion;
            hdr.endianMarker   = kEndianMarker;
            hdr.recordSize     = static_cast<std::uint32_t>(sizeof(FrameRecord));
            hdr.frameCount     = frameCount_;
            hdr.createdAtUnix  = static_cast<std::uint64_t>(nowEpochSeconds());
            hdr.gitCommitPrefix = 0;
            std::memset(hdr.reserved, 0, sizeof(hdr.reserved));
            framesFile_.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
            framesFile_.close();
        }
        active_ = false;
        std::cout << "[SessionRecorder] Buffer full (" << config_.maxFrames
                  << " frames), stopping" << std::endl;
    }
}

bool SessionRecorder::writeMetadata(const SessionMetadata& meta) const {
    try {
        nlohmann::json j;
        j["sessionId"] = meta.sessionId;
        j["startTime"] = meta.startTime;
        j["bodyNames"] = meta.bodyNames;
        j["frameCount"] = meta.frameCount;
        j["simDuration"] = meta.simDuration;
        j["schemaVersion"] = kSchemaVersion;

        if (!createDirectoriesWin(sessionDir_)) return false;
        std::ofstream f(sessionDir_ / "metadata.json");
        if (!f.is_open()) return false;
        f << j.dump(2);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[SessionRecorder] writeMetadata exception: "
                  << e.what() << std::endl;
        return false;
    }
}

bool SessionRecorder::writeConfig() const {
    try {
        nlohmann::json j;
        j["maxFrames"] = config_.maxFrames;
        j["frameInterval"] = config_.frameInterval;
        j["baseDir"] = config_.baseDir;

        if (!createDirectoriesWin(sessionDir_)) return false;
        std::ofstream f(sessionDir_ / "config.json");
        if (!f.is_open()) return false;
        f << j.dump(2);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[SessionRecorder] writeConfig exception: "
                  << e.what() << std::endl;
        return false;
    }
}

void SessionRecorder::flushBuffer() {
    if (buffer_.empty()) return;

    for (const auto& frame : buffer_) {
        appendFrameToFile(frame);
        ++frameCount_;
    }
    buffer_.clear();
}

bool SessionRecorder::appendFrameToFile(const FrameRecord& f) {
    if (!framesFile_.is_open()) return false;

    // Calculate serialized size
    std::size_t nameBytes = 0;
    for (const auto& name : f.bodyNames) {
        nameBytes += 4 + name.size(); // uint32 len + chars
    }
    std::size_t frameSize = 8 + 4 + // time + bodyCount
                            nameBytes +
                            f.positions.size() * sizeof(double) +
                            f.velocities.size() * sizeof(double) +
                            4 * sizeof(double) + // cameraPosition
                            4 * sizeof(double) + // cameraLookAt
                            sizeof(double) +     // blackHoleMassKg
                            3 * sizeof(double);  // totalEnergy, kineticEnergy, potentialEnergy

    // Write frame size prefix
    std::uint32_t len = static_cast<std::uint32_t>(frameSize);
    framesFile_.write(reinterpret_cast<const char*>(&len), sizeof(len));

    // Write time
    framesFile_.write(reinterpret_cast<const char*>(&f.time), sizeof(f.time));

    // Write body count
    std::uint32_t bodyCount = static_cast<std::uint32_t>(f.bodyNames.size());
    framesFile_.write(reinterpret_cast<const char*>(&bodyCount), sizeof(bodyCount));

    // Write body names
    for (const auto& name : f.bodyNames) {
        std::uint32_t nameLen = static_cast<std::uint32_t>(name.size());
        framesFile_.write(reinterpret_cast<const char*>(&nameLen), sizeof(nameLen));
        framesFile_.write(name.data(), nameLen);
    }

    // Write positions
    if (!f.positions.empty()) {
        framesFile_.write(reinterpret_cast<const char*>(f.positions.data()),
                          f.positions.size() * sizeof(double));
    }

    // Write velocities
    if (!f.velocities.empty()) {
        framesFile_.write(reinterpret_cast<const char*>(f.velocities.data()),
                          f.velocities.size() * sizeof(double));
    }

    // Write camera position
    framesFile_.write(reinterpret_cast<const char*>(f.cameraPosition.data()),
                      4 * sizeof(double));

    // Write camera lookAt
    framesFile_.write(reinterpret_cast<const char*>(f.cameraLookAt.data()),
                      4 * sizeof(double));

    // Write black hole mass
    framesFile_.write(reinterpret_cast<const char*>(&f.blackHoleMassKg),
                      sizeof(f.blackHoleMassKg));

    // Write energies
    framesFile_.write(reinterpret_cast<const char*>(&f.totalEnergy),
                      sizeof(f.totalEnergy));
    framesFile_.write(reinterpret_cast<const char*>(&f.kineticEnergy),
                      sizeof(f.kineticEnergy));
    framesFile_.write(reinterpret_cast<const char*>(&f.potentialEnergy),
                      sizeof(f.potentialEnergy));

    return framesFile_.good();
}

std::vector<FrameRecord> SessionRecorder::readFrames(
    const std::filesystem::path& sessionDir)
{
    std::vector<FrameRecord> frames;
    std::ifstream f(sessionDir / "frames.bin", std::ios::binary);
    if (!f.is_open()) return frames;

    // Read and verify SessionHeader
    SessionHeader hdr;
    f.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (f.gcount() != sizeof(hdr)) return frames;
    if (hdr.magic != kSessionMagic) return frames;
    if (hdr.schemaVersion != kSchemaVersion) return frames;
    if (hdr.endianMarker != kEndianMarker) return frames;

    while (f.good()) {
        // Read frame size prefix
        std::uint32_t len = 0;
        f.read(reinterpret_cast<char*>(&len), sizeof(len));
        if (!f.good() || len == 0) break;

        std::vector<char> buf(len);
        f.read(buf.data(), len);
        if (!f.good()) break;

        const char* p = buf.data();

        FrameRecord frame;

        // time
        std::memcpy(&frame.time, p, sizeof(double));
        p += sizeof(double);

        // body count
        std::uint32_t bodyCount = 0;
        std::memcpy(&bodyCount, p, sizeof(std::uint32_t));
        p += sizeof(std::uint32_t);

        // body names
        frame.bodyNames.reserve(bodyCount);
        for (std::uint32_t i = 0; i < bodyCount; ++i) {
            std::uint32_t nameLen = 0;
            std::memcpy(&nameLen, p, sizeof(std::uint32_t));
            p += sizeof(std::uint32_t);
            frame.bodyNames.emplace_back(p, nameLen);
            p += nameLen;
        }

        // positions (3 doubles per body)
        std::size_t posCount = bodyCount * 3;
        frame.positions.resize(posCount);
        std::memcpy(frame.positions.data(), p, posCount * sizeof(double));
        p += posCount * sizeof(double);

        // velocities (3 doubles per body)
        std::size_t velCount = bodyCount * 3;
        frame.velocities.resize(velCount);
        std::memcpy(frame.velocities.data(), p, velCount * sizeof(double));
        p += velCount * sizeof(double);

        // camera position (4 doubles)
        std::memcpy(frame.cameraPosition.data(), p, 4 * sizeof(double));
        p += 4 * sizeof(double);

        // camera lookAt (4 doubles)
        std::memcpy(frame.cameraLookAt.data(), p, 4 * sizeof(double));
        p += 4 * sizeof(double);

        // grid Z range NOT in schemaVersion 1 — skipped

        // black hole mass
        std::memcpy(&frame.blackHoleMassKg, p, sizeof(double));
        p += sizeof(double);

        // energies
        std::memcpy(&frame.totalEnergy, p, sizeof(double));
        p += sizeof(double);
        std::memcpy(&frame.kineticEnergy, p, sizeof(double));
        p += sizeof(double);
        std::memcpy(&frame.potentialEnergy, p, sizeof(double));
        p += sizeof(double);

        frames.push_back(std::move(frame));
    }

    return frames;
}

std::unique_ptr<SessionMetadata> SessionRecorder::readMetadata(
    const std::filesystem::path& sessionDir)
{
    try {
        std::ifstream f(sessionDir / "metadata.json");
        if (!f.is_open()) return nullptr;
        nlohmann::json j;
        f >> j;

        auto meta = std::make_unique<SessionMetadata>();
        meta->sessionId = j.value("sessionId", std::string{});
        meta->startTime = j.value("startTime", 0.0);
        meta->frameCount = j.value("frameCount", std::uint32_t(0));
        meta->simDuration = j.value("simDuration", 0.0);
        if (j.contains("bodyNames") && j["bodyNames"].is_array()) {
            for (const auto& name : j["bodyNames"]) {
                meta->bodyNames.push_back(name.get<std::string>());
            }
        }
        return meta;
    } catch (const std::exception& e) {
        std::cerr << "[SessionRecorder] readMetadata exception: "
                  << e.what() << std::endl;
        return nullptr;
    }
}

std::optional<SessionHeader> SessionRecorder::readHeader(
    const std::filesystem::path& sessionDir)
{
    std::ifstream f(sessionDir / "frames.bin", std::ios::binary);
    if (!f.is_open()) return std::nullopt;

    SessionHeader hdr;
    f.read(reinterpret_cast<char*>(&hdr), sizeof(hdr));
    if (f.gcount() != sizeof(hdr)) return std::nullopt;
    return hdr;
}

} // namespace quantumverse
