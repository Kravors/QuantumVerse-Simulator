// QuantumVerse Test: ML dead-UI fix (AnomalyMonitor modelLoaded gating)
// Verifies that AnomalyMonitor returns -1.0 score when model not loaded,
// preventing garbage panic scores (5.76e14) from reaching the UI.

#include <QCoreApplication>
#include <cmath>
#include <iostream>
#include <cstdlib>

#include "ml/AnomalyMonitor.h"
#include "ml/AnomalyDetector.h"

using namespace quantumverse;

#define CHECK(cond, msg) do {                                              \
    if (!(cond)) {                                                         \
        std::cerr << "FAIL: " << msg << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::exit(1);                                                      \
    }                                                                      \
    std::cout << "[PASS] " << msg << std::endl;                            \
    std::cout.flush();                                                     \
} while (0)

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    std::cout << "=== ML Dead-UI Fix Tests (AnomalyMonitor modelLoaded) ===" << std::endl;
    std::cout.flush();

    // -----------------------------------------------------------------------
    // Test 1: Default state — no detector → modelLoaded=false, lastScore=0.0
    // -----------------------------------------------------------------------
    {
        AnomalyMonitor monitor;
        CHECK(!monitor.isModelLoaded(), "Default modelLoaded should be false");
        CHECK(monitor.lastScore() == 0.0, "Default lastScore should be 0.0 before sampling");
    }

    // -----------------------------------------------------------------------
    // Test 2: Null detector → modelLoaded=false
    // -----------------------------------------------------------------------
    {
        AnomalyMonitor monitor;
        monitor.setAnomalyDetector(nullptr);
        CHECK(!monitor.isModelLoaded(), "Null detector → modelLoaded=false");
    }

    // -----------------------------------------------------------------------
    // Test 3: Unloaded detector → modelLoaded=false
    // -----------------------------------------------------------------------
    {
        AnomalyMonitor monitor;
        auto emptyDetector = std::make_shared<AnomalyDetector>();
        CHECK(!emptyDetector->isLoaded(), "Empty detector should not be loaded");
        monitor.setAnomalyDetector(emptyDetector);
        CHECK(!monitor.isModelLoaded(), "Unloaded detector → modelLoaded=false");
    }

    // -----------------------------------------------------------------------
    // Test 4: setModelLoaded(true) → modelLoaded=true
    // -----------------------------------------------------------------------
    {
        AnomalyMonitor monitor;
        monitor.setModelLoaded(true);
        CHECK(monitor.isModelLoaded(), "setModelLoaded(true) → modelLoaded=true");
    }

    // -----------------------------------------------------------------------
    // Test 5: processSample with no detector → lastScore=-1.0
    // -----------------------------------------------------------------------
    {
        AnomalyMonitor monitor;
        monitor.processSample();
        CHECK(monitor.lastScore() == -1.0, "processSample without detector → lastScore=-1.0");
        CHECK(!monitor.isModelLoaded(), "modelLoaded stays false");
    }

    // -----------------------------------------------------------------------
    // Test 6: processSample with unloaded detector → lastScore=-1.0
    // -----------------------------------------------------------------------
    {
        AnomalyMonitor monitor;
        auto emptyDetector = std::make_shared<AnomalyDetector>();
        monitor.setAnomalyDetector(emptyDetector);
        monitor.processSample();
        CHECK(monitor.lastScore() == -1.0, "Unloaded detector → lastScore=-1.0");
        CHECK(!monitor.isModelLoaded(), "modelLoaded stays false");
    }

    // -----------------------------------------------------------------------
    // Test 7: setModelLoaded(false) explicitly resets lastScore to -1.0
    // -----------------------------------------------------------------------
    {
        AnomalyMonitor monitor;
        monitor.setModelLoaded(true);
        CHECK(monitor.isModelLoaded(), "Manual setModelLoaded(true)");
        monitor.setModelLoaded(false);
        CHECK(!monitor.isModelLoaded(), "setModelLoaded(false)");
        CHECK(monitor.lastScore() == -1.0, "setModelLoaded(false) → lastScore=-1.0");
    }

    std::cout << "=== All ML dead-UI tests passed ===" << std::endl;
    return 0;
}
