/**
 * @file BlackHoleJetAnomalyRecogniser.cpp
 * @brief Implementation of the non-Kerr jet anomaly recogniser
 */

#include "BlackHoleJetAnomalyRecogniser.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace quantumverse {

BlackHoleJetAnomalyRecogniser::BlackHoleJetAnomalyRecogniser()
{
    setParameter("max_spin_parameter", 0.998);
    setParameter("magnetic_field_gauss", 1e4);
    setParameter("black_hole_mass_msun", 10.0);
    setParameter("jet_collimation_threshold_deg", 15.0);
    setParameter("precession_detection_sigma", 3.0);
}

std::vector<InstrumentFinding> BlackHoleJetAnomalyRecogniser::analyze(
    const MetricTensor& metric, const Event4D& location,
    const std::vector<Event4D>& trajectory)
{
    std::vector<InstrumentFinding> findings;

    double maxSpin = getParameter("max_spin_parameter");
    double bField = getParameter("magnetic_field_gauss");
    double mass = getParameter("black_hole_mass_msun");
    double collimThreshold = getParameter("jet_collimation_threshold_deg");

    if (trajectory.size() < 4) return findings;

    // Analyze jet launching region near the horizon
    std::vector<double> jetAngles;
    std::vector<double> radii;

    for (const auto& event : trajectory) {
        double r = std::sqrt(event.x * event.x + event.y * event.y + event.z * event.z);
        if (r > 0.01) {
            radii.push_back(r);
            double theta = std::acos(event.z / r);
            jetAngles.push_back(theta * 180.0 / M_PI);
        }
    }

    if (jetAngles.size() < 3) return findings;

    // Compute BZ power and jet collimation
    double jetPower = computeBlandfordZnajekPower(maxSpin * 0.9, bField, mass);
    double collimationAngle = computeJetCollimationAngle(maxSpin * 0.9);

    // Check for jet precession
    bool precessing = detectJetPrecession(jetAngles);

    // Estimate spin from observed jet properties
    double estimatedSpin = estimateSpinFromJet(jetPower, mass);

    // Anomaly: deviation from expected Kerr jet
    double spinDeviation = std::abs(estimatedSpin - maxSpin * 0.9);
    bool nonKerrJet = spinDeviation > 0.1 || collimationAngle < collimThreshold;

    if (nonKerrJet || precessing) {
        double confidence = std::min(1.0, spinDeviation * 2.0 +
            (precessing ? 0.3 : 0.0) +
            (collimationAngle < collimThreshold ? 0.2 : 0.0));

        InstrumentFinding finding;
        finding.id = "BHJA_" + std::to_string(getTotalFindings());
        finding.instrumentName = getName();
        finding.severity = confidenceToSeverity(confidence);
        finding.confidence = confidence;
        finding.description = "Non-Kerr jet anomaly: spin_deviation=" +
            std::to_string(spinDeviation) +
            ", collimation_angle=" + std::to_string(collimationAngle) +
            " deg, precessing=" + std::string(precessing ? "yes" : "no") +
            ", BZ_power=" + std::to_string(jetPower);
        finding.location = location;
        finding.timestamp = location.t;
        finding.parameters["jet_power_erg_s"] = jetPower;
        finding.parameters["collimation_angle_deg"] = collimationAngle;
        finding.parameters["estimated_spin"] = estimatedSpin;
        finding.parameters["spin_deviation"] = spinDeviation;
        finding.parameters["precessing"] = precessing ? 1.0 : 0.0;
        addFinding(finding);
        findings.push_back(finding);
    }

    return findings;
}

double BlackHoleJetAnomalyRecogniser::computeBlandfordZnajekPower(
    double spin, double magneticField, double mass)
{
    // P_BZ ~ (kappa/4πc) * B^2 * r_g^2 * c * f(spin)
    // Simplified: P ~ 10^45 * (B/10^4 G)^2 * (M/10 Msun)^2 * f(a) erg/s
    double r_g = mass * 1.477; // gravitational radius in km
    double f_spin = spin * spin; // approximate spin function

    double power = 1e45 * (magneticField / 1e4) * (magneticField / 1e4) *
        (mass / 10.0) * (mass / 10.0) * f_spin;

    return power;
}

double BlackHoleJetAnomalyRecogniser::computeJetCollimationAngle(double spinParameter)
{
    // Jet half-opening angle ~ few degrees for rapidly spinning BH
    // theta_j ~ 10 * (1 - a)^0.5 degrees approximately
    double theta = 10.0 * std::sqrt(1.0 - spinParameter + 0.01);
    return theta;
}

bool BlackHoleJetAnomalyRecogniser::detectJetPrecession(
    const std::vector<double>& jetAngles)
{
    if (jetAngles.size() < 5) return false;

    // Look for sinusoidal variation in jet angle (precession)
    double meanAngle = 0.0;
    for (double a : jetAngles) meanAngle += a;
    meanAngle /= jetAngles.size();

    double variance = 0.0;
    for (double a : jetAngles) variance += (a - meanAngle) * (a - meanAngle);
    variance /= jetAngles.size();
    double stddev = std::sqrt(variance);

    // Precession signature: regular oscillation with amplitude > 2*stddev
    int directionChanges = 0;
    for (size_t i = 1; i < jetAngles.size(); ++i) {
        if ((jetAngles[i] - meanAngle) * (jetAngles[i - 1] - meanAngle) < 0) {
            directionChanges++;
        }
    }

    return stddev > 2.0 && directionChanges >= 2;
}

double BlackHoleJetAnomalyRecogniser::estimateSpinFromJet(double jetPower, double mass)
{
    // Invert BZ relation to estimate spin
    // P ~ 10^45 * (M/10)^2 * f(a) -> f(a) ~ P / (10^45 * (M/10)^2)
    double normalizedPower = jetPower / (1e45 * (mass / 10.0) * (mass / 10.0));
    double spinEstimate = std::sqrt(std::min(1.0, normalizedPower));

    return spinEstimate;
}

} // namespace quantumverse