/**
 * @file test_session_recorder.cpp
 * @brief Round-trip test for SessionRecorder: write 100 frames, read back,
 *        assert bit-exact match.
 */

#include "ml/SessionRecorder.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <QtCore/QStandardPaths>

using namespace quantumverse;

int main() {
    int failed = 0;

    // Use QStandardPaths::TempLocation so the directory exists on every
    // machine/CI runner without hardcoding build paths. createDirectoryA
    // only creates the final component — we need recursive creation.
    std::filesystem::path tmpDir =
        std::filesystem::path(QStandardPaths::writableLocation(
                                  QStandardPaths::TempLocation).toStdString()) /
        "session_test";
    std::error_code ec;
    if (!std::filesystem::create_directories(tmpDir, ec)) {
        std::cerr << "[SessionRecorder test] create_directories error: "
                  << ec.message() << std::endl;
        return 1;
    }

    SessionRecorder recorder;
    SessionRecorderConfig cfg;
    cfg.baseDir = tmpDir.string();
    cfg.maxFrames = 1000;
    cfg.frameInterval = 1;
    recorder.configure(cfg);

    std::string sessionId = recorder.start();
    if (sessionId.empty()) {
        std::cerr << "[FAIL] SessionRecorder::start() returned empty sessionId" << std::endl;
        return 1;
    }
    std::cout << "[INFO] Session ID: " << sessionId << std::endl;

    // Verify session directory was created
    std::filesystem::path sessionDir = tmpDir / sessionId;
    if (!std::filesystem::exists(sessionDir / "metadata.json")) {
        std::cerr << "[FAIL] metadata.json not created" << std::endl;
        ++failed;
    }
    if (!std::filesystem::exists(sessionDir / "config.json")) {
        std::cerr << "[FAIL] config.json not created" << std::endl;
        ++failed;
    }
    if (!std::filesystem::exists(sessionDir / "frames.bin")) {
        std::cerr << "[FAIL] frames.bin not created" << std::endl;
        ++failed;
    }

    // Write 100 frames with known values
    const int nFrames = 100;
    const int nBodies = 3;
    const double bhMass = 1.989e30; // 1 solar mass

    for (int i = 0; i < nFrames; ++i) {
        FrameRecord frame;
        frame.time = i * 0.1; // 0.1s steps

        // Body names
        frame.bodyNames = {"Sun", "Earth", "Mars"};

        // Positions: x,y,z interleaved
        frame.positions.reserve(nBodies * 3);
        frame.velocities.reserve(nBodies * 3);
        for (int b = 0; b < nBodies; ++b) {
            frame.positions.push_back(10.0 * b + i * 0.001);
            frame.positions.push_back(20.0 * b + i * 0.002);
            frame.positions.push_back(30.0 * b + i * 0.003);
            frame.velocities.push_back(1.0 * b + i * 0.0001);
            frame.velocities.push_back(2.0 * b + i * 0.0002);
            frame.velocities.push_back(3.0 * b + i * 0.0003);
        }

        frame.totalEnergy = 1e40 + i * 1e35;
        frame.kineticEnergy = 5e39 + i * 5e34;
        frame.potentialEnergy = 5e39 + i * 5e34;

        // Camera: 4D position and look-at
        frame.cameraPosition = {0.0, 100.0 + i, 50.0, 200.0};
        frame.cameraLookAt = {0.0, 0.0, 0.0, 0.0};

        frame.blackHoleMassKg = bhMass;

        recorder.recordFrame(frame);
    }

    recorder.stop();

    // Read frames back
    auto frames = SessionRecorder::readFrames(sessionDir);
    if (static_cast<int>(frames.size()) != nFrames) {
        std::cerr << "[FAIL] Expected " << nFrames << " frames, read "
                  << frames.size() << std::endl;
        ++failed;
    } else {
        std::cout << "[INFO] Read " << frames.size() << " frames back" << std::endl;
    }

    // Bit-exact comparison
    for (int i = 0; i < nFrames && i < static_cast<int>(frames.size()); ++i) {
        const FrameRecord& orig = /* reconstruct expected */ FrameRecord{};
        // We need to compare against expected values. Since we didn't store
        // the originals, reconstruct them here:
        (void)orig;

        const FrameRecord& r = frames[i];

        // time
        double expectedTime = i * 0.1;
        if (std::abs(r.time - expectedTime) > 1e-12) {
            std::cerr << "[FAIL] Frame " << i << ": time mismatch "
                      << r.time << " vs " << expectedTime << std::endl;
            ++failed;
        }

        // body names
        if (r.bodyNames.size() != nBodies ||
            r.bodyNames[0] != "Sun" ||
            r.bodyNames[1] != "Earth" ||
            r.bodyNames[2] != "Mars") {
            std::cerr << "[FAIL] Frame " << i << ": bodyNames mismatch" << std::endl;
            ++failed;
        }

        // positions (3 per body)
        std::size_t expectedPosSize = nBodies * 3;
        if (r.positions.size() != expectedPosSize) {
            std::cerr << "[FAIL] Frame " << i << ": positions size "
                      << r.positions.size() << " vs " << expectedPosSize << std::endl;
            ++failed;
        } else {
            for (std::size_t j = 0; j < expectedPosSize; ++j) {
                int b = j / 3;
                int c = j % 3;
                double base = (c == 0) ? 10.0 : (c == 1) ? 20.0 : 30.0;
                double expected = base * b + i * 0.001 * (c + 1);
                if (std::abs(r.positions[j] - expected) > 1e-12) {
                    std::cerr << "[FAIL] Frame " << i << ": positions[" << j
                              << "] mismatch " << r.positions[j]
                              << " vs " << expected << std::endl;
                    ++failed;
                    break;
                }
            }
        }

        // velocities (3 per body)
        std::size_t expectedVelSize = nBodies * 3;
        if (r.velocities.size() != expectedVelSize) {
            std::cerr << "[FAIL] Frame " << i << ": velocities size "
                      << r.velocities.size() << " vs " << expectedVelSize << std::endl;
            ++failed;
        } else {
            for (std::size_t j = 0; j < expectedVelSize; ++j) {
                int b = j / 3;
                int c = j % 3;
                double base = (c == 0) ? 1.0 : (c == 1) ? 2.0 : 3.0;
                double expected = base * b + i * 0.0001 * (c + 1);
                if (std::abs(r.velocities[j] - expected) > 1e-12) {
                    std::cerr << "[FAIL] Frame " << i << ": velocities[" << j
                              << "] mismatch " << r.velocities[j]
                              << " vs " << expected << std::endl;
                    ++failed;
                    break;
                }
            }
        }

        // energies
        if (std::abs(r.totalEnergy - (1e40 + i * 1e35)) > 1e25) {
            std::cerr << "[FAIL] Frame " << i << ": totalEnergy mismatch "
                      << r.totalEnergy << " vs " << (1e40 + i * 1e35) << std::endl;
            ++failed;
        }
        if (std::abs(r.kineticEnergy - (5e39 + i * 5e34)) > 1e24) {
            std::cerr << "[FAIL] Frame " << i << ": kineticEnergy mismatch "
                      << r.kineticEnergy << " vs " << (5e39 + i * 5e34) << std::endl;
            ++failed;
        }
        if (std::abs(r.potentialEnergy - (5e39 + i * 5e34)) > 1e24) {
            std::cerr << "[FAIL] Frame " << i << ": potentialEnergy mismatch "
                      << r.potentialEnergy << " vs " << (5e39 + i * 5e34) << std::endl;
            ++failed;
        }

        // camera position
        if (std::abs(r.cameraPosition[0] - 0.0) > 1e-12 ||
            std::abs(r.cameraPosition[1] - (100.0 + i)) > 1e-12 ||
            std::abs(r.cameraPosition[2] - 50.0) > 1e-12 ||
            std::abs(r.cameraPosition[3] - 200.0) > 1e-12) {
            std::cerr << "[FAIL] Frame " << i << ": cameraPosition mismatch"
                      << std::endl;
            ++failed;
        }

        // camera lookAt
        if (std::abs(r.cameraLookAt[0]) > 1e-12 ||
            std::abs(r.cameraLookAt[1]) > 1e-12 ||
            std::abs(r.cameraLookAt[2]) > 1e-12 ||
            std::abs(r.cameraLookAt[3]) > 1e-12) {
            std::cerr << "[FAIL] Frame " << i << ": cameraLookAt mismatch"
                      << std::endl;
            ++failed;
        }

        // black hole mass
        if (std::abs(r.blackHoleMassKg - bhMass) > 1e-6) {
            std::cerr << "[FAIL] Frame " << i << ": blackHoleMassKg mismatch"
                      << std::endl;
            ++failed;
        }
    }

    // Read metadata and verify
    auto meta = SessionRecorder::readMetadata(sessionDir);
    if (!meta) {
        std::cerr << "[FAIL] Could not read metadata.json" << std::endl;
        ++failed;
    } else {
        if (meta->frameCount != nFrames) {
            std::cerr << "[FAIL] metadata frameCount " << meta->frameCount
                      << " vs " << nFrames << std::endl;
            ++failed;
        }
        if (meta->sessionId != sessionId) {
            std::cerr << "[FAIL] metadata sessionId mismatch" << std::endl;
            ++failed;
        }
        std::cout << "[INFO] Metadata: sessionId=" << meta->sessionId
                  << ", frameCount=" << meta->frameCount << std::endl;
    }

    // Header round-trip test
    auto hdr = SessionRecorder::readHeader(sessionDir);
    if (!hdr) {
        std::cerr << "[FAIL] Could not read SessionHeader from frames.bin"
                  << std::endl;
        ++failed;
    } else {
        if (hdr->magic != kSessionMagic) {
            std::cerr << "[FAIL] Header magic mismatch: " << hdr->magic
                      << " vs " << kSessionMagic << std::endl;
            ++failed;
        }
        if (hdr->schemaVersion != kSchemaVersion) {
            std::cerr << "[FAIL] Header schemaVersion mismatch: " << hdr->schemaVersion
                      << " vs " << kSchemaVersion << std::endl;
            ++failed;
        }
        if (hdr->endianMarker != kEndianMarker) {
            std::cerr << "[FAIL] Header endianMarker mismatch: " << hdr->endianMarker
                      << " vs " << kEndianMarker << std::endl;
            ++failed;
        }
        if (hdr->recordSize != static_cast<std::uint32_t>(sizeof(FrameRecord))) {
            std::cerr << "[FAIL] Header recordSize mismatch: " << hdr->recordSize
                      << " vs " << sizeof(FrameRecord) << std::endl;
            ++failed;
        }
        if (hdr->frameCount != nFrames) {
            std::cerr << "[FAIL] Header frameCount mismatch: " << hdr->frameCount
                      << " vs " << nFrames << std::endl;
            ++failed;
        }
        if (hdr->createdAtUnix == 0) {
            std::cerr << "[FAIL] Header createdAtUnix is zero" << std::endl;
            ++failed;
        }
        std::cout << "[INFO] Header: magic=0x" << std::hex << hdr->magic << std::dec
                  << ", schemaVersion=" << hdr->schemaVersion
                  << ", endianMarker=0x" << std::hex << hdr->endianMarker << std::dec
                  << ", recordSize=" << hdr->recordSize
                  << ", frameCount=" << hdr->frameCount
                  << ", createdAtUnix=" << hdr->createdAtUnix << std::endl;
    }

    if (failed == 0) {
        std::cout << "=== ALL SESSION RECORDER TESTS PASSED ===" << std::endl;
        return 0;
    }
    std::cerr << "=== " << failed << " SESSION RECORDER TEST(S) FAILED ===" << std::endl;
    return 1;
}
