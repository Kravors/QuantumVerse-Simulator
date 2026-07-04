// GWPolarization.h
// Gravitational Wave Polarization Tomography
//
// This module implements GW polarization analysis for detecting
// deviations from General Relativity:
// - Plus and cross polarizations (GR)
// - Extra polarizations (scalar, vector, breathing)
// - Detector response functions
// - Parameter estimation for modified gravity
//
// Author: QuantumVerse Team
// Date: 2026-06-07

#pragma once

#include <vector>
#include <array>
#include <complex>
#include "../spacetime/Event4D.h"

namespace quantumverse {

/**
 * @brief GW polarization modes.
 */
enum class PolarizationMode {
    Plus,           ///< h_+ (GR tensor)
    Cross,          ///< h_x (GR tensor)
    Breathing,      ///< h_b (scalar)
    Longitudinal,   ///< h_l (scalar)
    VectorX,        ///< h_x (vector)
    VectorY         ///< h_y (vector)
};

/**
 * @brief GW polarization state.
 */
struct GWPolarizationState {
    std::vector<std::pair<double, double>> waveform;  ///< Time series (time, strain)
    PolarizationMode mode;                            ///< Polarization type
    double amplitude = 0.0;                         ///< Overall amplitude
    double frequency = 0.0;                         ///< Central frequency
    double phase = 0.0;                             ///< Initial phase
    double sky_location[2] = {0.0, 0.0};            ///< (ra, dec) in radians
    double polarization_angle = 0.0;                ///< Polarization angle
    
    /**
     * @brief Get strain at time t.
     */
    double strainAt(double t) const;
};

/**
 * @brief GW polarization tomography analyzer.
 */
class GWPolarization {
public:
    /**
     * @brief Analyze detector data for polarization content.
     * @param detector1 First detector strain data
     * @param detector2 Second detector strain data
     * @param time_delay Time delay between detectors
     * @return Vector of detected polarizations
     */
    static std::vector<GWPolarizationState> analyze(
        const std::vector<double>& detector1,
        const std::vector<double>& detector2,
        double time_delay);
    
    /**
     * @brief Generate waveform for given polarization.
     * @param mode Polarization mode
     * @param amplitude Amplitude
     * @param frequency Frequency
     * @param duration Duration
     * @param sample_rate Sample rate
     * @return Waveform
     */
    static GWPolarizationState generateWaveform(
        PolarizationMode mode,
        double amplitude,
        double frequency,
        double duration,
        double sample_rate = 4096.0);
    
    /**
     * @brief Compute detector response.
     * @param polarization GW polarization
     * @param detector_arm1 First detector arm direction
     * @param detector_arm2 Second detector arm direction
     * @return Detector response
     */
    static double detectorResponse(
        const GWPolarizationState& polarization,
        const std::array<double, 3>& detector_arm1,
        const std::array<double, 3>& detector_arm2);
    
    /**
     * @brief Test for GR consistency.
     * @param polarizations Detected polarizations
     * @return true if consistent with GR
     */
    static bool isGRConsistent(
        const std::vector<GWPolarizationState>& polarizations);
    
    /**
     * @brief Compute modified gravity parameters.
     * @param polarizations Detected polarizations
     * @return Parameter deviation from GR
     */
    static double modifiedGravityParameter(
        const std::vector<GWPolarizationState>& polarizations);
};

/**
 * @brief GW detector network.
 */
class GWDetectorNetwork {
public:
    struct Detector {
        std::string name;
        std::array<double, 3> arm1;     ///< First arm direction
        std::array<double, 3> arm2;     ///< Second arm direction
        double latitude;                ///< Latitude in radians
        double longitude;               ///< Longitude in radians
    };
    
    std::vector<Detector> detectors;
    
    /**
     * @brief Add a detector to the network.
     */
    void addDetector(const Detector& det) {
        detectors.push_back(det);
    }
    
    /**
     * @brief Get network response to a GW.
     * @param polarization GW polarization
     * @return Vector of responses
     */
    std::vector<double> networkResponse(
        const GWPolarizationState& polarization) const;
    
    /**
     * @brief Sky localization from network.
     * @param responses Detector responses
     * @return Sky position (ra, dec)
     */
    std::array<double, 2> skyLocalization(
        const std::vector<double>& responses) const;
};

} // namespace quantumverse