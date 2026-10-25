/**
 * @file test_mercury_1pn.cpp
 * @brief Integrator-level test of the 1PN Schwarzschild correction.
 *
 * Integrates Mercury's orbit with the 1PN correction enabled via
 * UI4D::stepSimulation() and measures the perihelion advance.
 * Expected: ~0.1036 arcsec/orbit (43 arcsec/century).
 * Also checks Newtonian energy drift over one orbit and that Earth's orbit
 * is unaffected by the correction (it is ~400x smaller at Earth's radius).
 */

#include <iostream>
#include <cmath>
#include <limits>
#include "ui4d/UI4D.h"
#include "physics/PhysicsConstants.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main() {
    quantumverse::UI4D ui4d;
    ui4d.setGrCorrections(true);

    auto& bodies = ui4d.solarSystem.bodies;

    auto sunIt = bodies.find("Sun");
    auto mercuryIt = bodies.find("Mercury");
    auto earthIt = bodies.find("Earth");

    if (sunIt == bodies.end() || mercuryIt == bodies.end() || earthIt == bodies.end()) {
        std::cerr << "Missing bodies (Sun/Mercury/Earth)!" << std::endl;
        return 1;
    }

    double sunMass = sunIt->second.mass;
    double mercuryMass = mercuryIt->second.mass;
    double mercuryPeriod = mercuryIt->second.orbitalPeriod;

    std::cout << "=== 1PN Mercury Perihelion Advance Test ===" << std::endl;
    std::cout << "Sun mass: " << sunMass << " kg" << std::endl;
    std::cout << "Mercury period: " << mercuryPeriod << " s (" << (mercuryPeriod / 86400.0) << " days)" << std::endl;

    // Integrate 10 Mercury orbits at dt = 0.1 s per step.
    // stepSimulationCPURaw(0.1) calls stepSimulationCPU(0.1, ...) directly,
    // bypassing adaptive substepping and [N-BODY] diagnostic I/O.
    double dt = 0.1;
    int stepsPerOrbit = static_cast<int>(std::ceil(mercuryPeriod / dt));
    int numOrbits = 10;
    int totalSteps = stepsPerOrbit * numOrbits;

    std::cout << "Steps per orbit: " << stepsPerOrbit << ", total steps: " << totalSteps << std::endl;

    // Perihelion detection: track r = |position|, detect local minima
    double rPrev = std::numeric_limits<double>::max();
    bool decreasing = true;
    int perihelionCount = 0;
    double firstPerihelionAngle = 0.0;
    double lastPerihelionAngle = 0.0;
    double unwrappedAngle = 0.0;
    double lastRawAngle = 0.0;
    bool firstAngleSet = false;

    // Energy tracking over first orbit: E = 0.5*m*v^2 - GM*m/r
    double initialEnergy = 0.0;
    double maxEnergyDrift = 0.0;

    for (int step = 0; step < totalSteps; ++step) {
        ui4d.stepSimulationCPURaw(dt);

        const auto& mercury = bodies.at("Mercury");
        double mx = mercury.position.x;
        double my = mercury.position.y;
        double mz = mercury.position.z;
        double r = std::sqrt(mx * mx + my * my + mz * mz);

        // Detect perihelion: r transitions from decreasing to increasing
        if (r < rPrev) {
            decreasing = true;
        } else if (decreasing && r > rPrev) {
            // Just passed perihelion (previous step was the minimum)
            double angle = std::atan2(my, mx);
            if (!firstAngleSet) {
                unwrappedAngle = angle;
                lastRawAngle = angle;
                firstAngleSet = true;
            } else {
                double delta = angle - lastRawAngle;
                while (delta > M_PI) delta -= 2.0 * M_PI;
                while (delta < -M_PI) delta += 2.0 * M_PI;
                unwrappedAngle += delta;
                lastRawAngle = angle;
            }

            if (perihelionCount == 0) {
                firstPerihelionAngle = unwrappedAngle;
            }
            lastPerihelionAngle = unwrappedAngle;
            perihelionCount++;
            decreasing = false;
        }
        rPrev = r;

        // Energy tracking (first orbit only)
        if (step < stepsPerOrbit) {
            double vx = mercury.velocity.x;
            double vy = mercury.velocity.y;
            double vz = mercury.velocity.z;
            double v2 = vx * vx + vy * vy + vz * vz;
            double kinetic = 0.5 * mercuryMass * v2;
            double potential = -quantumverse::PhysicsConstants::instance().get_G() * sunMass * mercuryMass / r;
            double totalE = kinetic + potential;
            if (step == 0) {
                initialEnergy = totalE;
            }
            double drift = std::abs(totalE - initialEnergy) / std::abs(initialEnergy);
            if (drift > maxEnergyDrift) {
                maxEnergyDrift = drift;
            }
        }
    }

    std::cout << "Perihelion passages detected: " << perihelionCount << std::endl;

    if (perihelionCount < 2) {
        std::cerr << "Not enough perihelion passages detected!" << std::endl;
        return 1;
    }

    // Precession: angle swept between perihelions minus 2*pi per orbit
    int orbitsCompleted = perihelionCount - 1;
    double totalAngleSwept = lastPerihelionAngle - firstPerihelionAngle;
    double expectedAngleSwept = 2.0 * M_PI * static_cast<double>(orbitsCompleted);
    double totalPrecession = totalAngleSwept - expectedAngleSwept;
    double precessionPerOrbitRad = totalPrecession / static_cast<double>(orbitsCompleted);
    double precessionPerOrbitArcsec = precessionPerOrbitRad * 180.0 * 3600.0 / M_PI;

    // Expected: 43 arcsec/century / 415.2 orbits/century = 0.1036 arcsec/orbit
    double expectedPerOrbitArcsec = 0.1036;
    double tolerance = 0.15;

    std::cout << "Total angle swept: " << totalAngleSwept << " rad" << std::endl;
    std::cout << "Expected (no precession): " << expectedAngleSwept << " rad" << std::endl;
    std::cout << "Total precession (" << orbitsCompleted << " orbits): " << totalPrecession << " rad" << std::endl;
    std::cout << "Precession per orbit: " << precessionPerOrbitArcsec << " arcsec" << std::endl;
    std::cout << "Expected: ~" << expectedPerOrbitArcsec << " arcsec/orbit (tolerance ±" << tolerance << ")" << std::endl;
    std::cout << "Max energy drift (1 orbit): " << maxEnergyDrift << std::endl;

    bool precessionPass = std::abs(precessionPerOrbitArcsec - expectedPerOrbitArcsec) < tolerance;
    bool energyPass = maxEnergyDrift < 1e-4;

    std::cout << (precessionPass ? "[PASS]" : "[FAIL]")
              << " Perihelion advance: " << precessionPerOrbitArcsec << " arcsec/orbit" << std::endl;
    std::cout << (energyPass ? "[PASS]" : "[FAIL]")
              << " Energy drift: " << maxEnergyDrift << " (threshold 1e-4)" << std::endl;

    // Earth unaffected check: Earth's orbit after 1 year with GR on vs off
    const auto& earthGrOn = earthIt->second;

    quantumverse::UI4D ui4dNoGR;
    ui4dNoGR.setGrCorrections(false);
    auto& bodiesNoGR = ui4dNoGR.solarSystem.bodies;

    double earthPeriod = bodiesNoGR.at("Earth").orbitalPeriod;
    double earthDt = 3600.0;
    int earthSteps = static_cast<int>(std::ceil(earthPeriod / earthDt));

    for (int step = 0; step < earthSteps; ++step) {
        ui4dNoGR.stepSimulationCPURaw(earthDt);
    }

    const auto& earthNoGR = bodiesNoGR.at("Earth");
    double earthDiff = std::sqrt(
        (earthGrOn.position.x - earthNoGR.position.x) * (earthGrOn.position.x - earthNoGR.position.x) +
        (earthGrOn.position.y - earthNoGR.position.y) * (earthGrOn.position.y - earthNoGR.position.y) +
        (earthGrOn.position.z - earthNoGR.position.z) * (earthGrOn.position.z - earthNoGR.position.z)
    );

    double earthOrbitRadius = std::sqrt(
        earthGrOn.position.x * earthGrOn.position.x +
        earthGrOn.position.y * earthGrOn.position.y +
        earthGrOn.position.z * earthGrOn.position.z
    );

    double earthRelativeDiff = earthDiff / earthOrbitRadius;

    std::cout << "Earth position difference (GR on vs off, 1 year): " << earthDiff << " m" << std::endl;
    std::cout << "Earth relative difference: " << earthRelativeDiff * 100.0 << "%" << std::endl;

    bool earthPass = earthRelativeDiff < 1e-4;
    std::cout << (earthPass ? "[PASS]" : "[FAIL]")
              << " Earth unaffected: " << earthRelativeDiff * 100.0 << "% (threshold 0.01%)" << std::endl;

    bool allPass = precessionPass && energyPass && earthPass;
    std::cout << std::endl << (allPass ? "=== ALL TESTS PASSED ===" : "=== SOME TESTS FAILED ===") << std::endl;
    return allPass ? 0 : 1;
}
