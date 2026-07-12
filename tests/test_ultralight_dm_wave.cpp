// QuantumVerse Test: UltralightDMWaveInterferometer
// TDD validation of ultralight dark-matter wave detection on a synthetic
// oscillatory signal. Builds a trajectory whose x-coordinate is a pure sine
// wave and verifies the detector recovers its frequency and amplitude.

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "discovery/UltralightDMWaveInterferometer.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

int main() {
    std::cout << "=== UltralightDMWaveInterferometer Test ===" << std::endl;

    // --- Synthetic ultralight DM wave ---------------------------------------
    // x(t) = A sin(omega t), with omega chosen to align exactly to a DFT
    // frequency bin so the detector recovers it cleanly.
    const double dt = 0.02;
    const int N = 1000;
    const double omega = 2.0 * 3.14159265358979323846 / (static_cast<double>(N) * dt); // = bin 1
    const double amplitude = 1.0;

    std::vector<quantumverse::Event4D> trajectory;
    for (int i = 0; i < N; ++i) {
        double t = i * dt;
        double x = amplitude * std::sin(omega * t);
        trajectory.emplace_back(t, x, 0.0, 0.0);
    }

    quantumverse::MetricTensor metric;
    quantumverse::Event4D location(0.0, 0.0, 0.0, 0.0);

    quantumverse::UltralightDMWaveInterferometer detector;
    auto findings = detector.analyze(metric, location, trajectory);

    assert(!findings.empty() && "No DM wave detected");

    const auto& f = findings.front();
    auto fit = f.parameters.find("oscillation_freq_rad_s");
    assert(fit != f.parameters.end() && "oscillation_freq_rad_s missing");
    assert(std::abs(fit->second - omega) < 1e-3 && "Detected frequency mismatch");

    auto ait = f.parameters.find("signal_amplitude");
    assert(ait != f.parameters.end() && "signal_amplitude missing");
    assert(std::abs(ait->second - amplitude) < 0.01 && "Detected amplitude mismatch");

    std::cout << "Detected DM wave: freq=" << fit->second
              << " rad/s, amplitude=" << ait->second << std::endl;

    // --- Negative case: constant (non-oscillatory) signal ------------------
    std::vector<quantumverse::Event4D> steady;
    for (int i = 0; i < N; ++i) {
        steady.emplace_back(i * dt, 0.5, 0.0, 0.0);
    }
    auto none = detector.analyze(metric, location, steady);
    assert(none.empty() && "Constant signal should not trigger detection");

    // --- Edge case: too few points -----------------------------------------
    std::vector<quantumverse::Event4D> tiny = {
        quantumverse::Event4D(0.0, 1.0, 0.0, 0.0),
        quantumverse::Event4D(0.02, 0.0, 0.0, 0.0),
    };
    auto few = detector.analyze(metric, location, tiny);
    assert(few.empty() && "Too-few-point trajectory should not produce a finding");

    std::cout << "All UltralightDMWaveInterferometer tests passed." << std::endl;
    return 0;
}
