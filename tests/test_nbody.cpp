#include <iostream>
#include <cmath>
#include <cassert>
#include "ui4d/UI4D.h"

int main() {
    quantumverse::UI4D ui4d;
    ui4d.initializeSolarSystem();

    const auto& bodies = ui4d.getSolarSystem().bodies;
    std::cout << "N-Body Test: " << bodies.size() << " bodies initialized" << std::endl;

    // Find Earth
    auto earthIt = bodies.find("Earth");
    if (earthIt == bodies.end()) {
        std::cerr << "Earth not found!" << std::endl;
        return 1;
    }

    const auto& earth = earthIt->second;
    double initialX = earth.position.x;
    double initialZ = earth.position.z;
    double initialDist = std::sqrt(initialX * initialX + initialZ * initialZ);

    std::cout << "Earth initial position: (" << initialX << ", " << earth.position.y << ", " << initialZ << ")" << std::endl;
    std::cout << "Earth initial distance from Sun: " << initialDist << " m" << std::endl;
    std::cout << "Earth initial velocity: (" << earth.velocity.x << ", " << earth.velocity.y << ", " << earth.velocity.z << ")" << std::endl;

    // Run simulation for 1 orbit period (~365 days)
    double dt = 3600.0; // 1 hour timesteps
    int steps = 365 * 24; // 1 year
    for (int i = 0; i < steps; ++i) {
        ui4d.stepSimulation(dt, 10);
    }

    const auto& bodiesAfter = ui4d.getSolarSystem().bodies;
    auto earthAfterIt = bodiesAfter.find("Earth");
    if (earthAfterIt == bodiesAfter.end()) {
        std::cerr << "Earth not found after simulation!" << std::endl;
        return 1;
    }

    const auto& earthAfter = earthAfterIt->second;
    double finalX = earthAfter.position.x;
    double finalZ = earthAfter.position.z;
    double finalDist = std::sqrt(finalX * finalX + finalZ * finalZ);

    std::cout << "After 1 year:" << std::endl;
    std::cout << "Earth final position: (" << finalX << ", " << earthAfter.position.y << ", " << finalZ << ")" << std::endl;
    std::cout << "Earth final distance from Sun: " << finalDist << " m" << std::endl;

    // Check that Earth remained at roughly the same distance from Sun (stable orbit)
    double drift = std::abs(finalDist - initialDist) / initialDist;
    std::cout << "Orbital radius drift: " << drift * 100.0 << "%" << std::endl;

    // A stable orbit should have <5% drift over 1 year
    if (drift < 0.05) {
        std::cout << "N-Body test PASSED: Stable orbit maintained" << std::endl;
    } else {
        std::cerr << "N-Body test FAILED: Excessive orbital drift" << std::endl;
        return 1;
    }

    // Test asteroid injection
    std::cout << "\n--- Asteroid Injection Test ---" << std::endl;
    int initialCount = ui4d.getSolarSystem().bodies.size();
    ui4d.injectAsteroid(1.5e11, 0, 0, 0, 0, 30000, 1e15);
    int afterCount = ui4d.getSolarSystem().bodies.size();
    if (afterCount != initialCount + 1) {
        std::cerr << "Asteroid injection FAILED: body count mismatch" << std::endl;
        return 1;
    }
    std::cout << "Asteroid injected. Body count: " << initialCount << " -> " << afterCount << std::endl;

    // Run simulation with asteroid
    for (int i = 0; i < 100; ++i) {
        ui4d.stepSimulation(3600.0, 10);
    }
    std::cout << "Simulation with asteroid ran successfully" << std::endl;

    // Clear asteroids
    ui4d.clearAsteroids();
    int finalCount = ui4d.getSolarSystem().bodies.size();
    if (finalCount != initialCount) {
        std::cerr << "Clear asteroids FAILED: body count mismatch" << std::endl;
        return 1;
    }
    std::cout << "Asteroids cleared. Body count: " << finalCount << std::endl;
    std::cout << "Asteroid injection test PASSED" << std::endl;

    return 0;
}
