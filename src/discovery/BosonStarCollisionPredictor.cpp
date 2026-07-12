/**
 * @file BosonStarCollisionPredictor.cpp
 * @brief Implementation of the boson star collision predictor
 */

#include "BosonStarCollisionPredictor.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

BosonStarCollisionPredictor::BosonStarCollisionPredictor()
{
    setParameter("strain_threshold", 1e-7);
    setParameter("boson_mass_ev", 1e-12);
}

std::vector<InstrumentFinding> BosonStarCollisionPredictor::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    std::vector<InstrumentFinding> findings;
    if (trajectory.size() < 10) return findings;

    double strainThreshold = getParameter("strain_threshold");
    double bosonMass = getParameter("boson_mass_ev");

    // Extract (time, strain). Convention: t = time, z = GW strain.
    std::vector<double> times, strains;
    for (const auto& ev : trajectory) {
        times.push_back(ev.t);
        strains.push_back(ev.z);
    }
    size_t n = times.size();
    if (n < 10) return findings;

    // Locate the merger: time of peak strain amplitude.
    double peakStrain = 0.0;
    double mergerTime = 0.0;
    for (size_t i = 0; i < n; ++i) {
        double a = std::abs(strains[i]);
        if (a > peakStrain) {
            peakStrain = a;
            mergerTime = times[i];
        }
    }
    if (peakStrain <= strainThreshold) return findings;

    // Estimate the ringdown frequency from the post-merger segment via a DFT.
    double ringdownFreq = 0.0;
    double dt = times[1] - times[0];
    if (dt > 0) {
        std::vector<double> rt, rx;
        for (size_t i = 0; i < n; ++i) {
            if (times[i] >= mergerTime) {
                rt.push_back(times[i]);
                rx.push_back(strains[i]);
            }
        }
        size_t m = rx.size();
        if (m >= 4) {
            double bestMag = 0.0;
            for (size_t k = 1; k <= m / 2; ++k) {
                double freq = static_cast<double>(k) /
                              (static_cast<double>(m) * dt); // Hz
                double re = 0.0, im = 0.0;
                for (size_t i = 0; i < m; ++i) {
                    double ph = 2.0 * M_PI * freq * rt[i];
                    re += rx[i] * std::cos(ph);
                    im += rx[i] * std::sin(ph);
                }
                double mag = std::sqrt(re * re + im * im);
                if (mag > bestMag) {
                    bestMag = mag;
                    ringdownFreq = freq;
                }
            }
        }
    }

    double confidence = std::min(1.0, peakStrain / (10.0 * strainThreshold));

    InstrumentFinding finding;
    finding.id = "BSCP_" + std::to_string(getTotalFindings());
    finding.instrumentName = getName();
    finding.severity = confidenceToSeverity(confidence);
    finding.confidence = confidence;
    finding.description = "Boson star merger detected at t = " +
        std::to_string(mergerTime) + " with peak strain " + std::to_string(peakStrain) +
        " and ringdown frequency " + std::to_string(ringdownFreq) +
        " Hz, indicating a boson star collision signature.";
    finding.location = location;
    finding.timestamp = mergerTime;
    finding.parameters["merger_time"] = mergerTime;
    finding.parameters["peak_strain"] = peakStrain;
    finding.parameters["ringdown_freq_hz"] = ringdownFreq;
    finding.parameters["boson_mass_ev"] = bosonMass;
    addFinding(finding);
    findings.push_back(finding);

    return findings;
}

} // namespace quantumverse
