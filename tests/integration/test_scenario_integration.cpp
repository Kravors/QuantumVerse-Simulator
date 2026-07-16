#include "ui4d/SceneGraphManager.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "physics/GeodesicIntegrator.h"
#include "physics/SingularityHandler.h"
#include "discovery/ExoplanetaryTTVFifthForceHunter.h"
#include "discovery/GalacticRotationCurveScanner.h"
#include <cmath>
#include <cassert>
#include <iostream>

using namespace quantumverse;

int main() {
    std::cout << "=== Scenario Integration Test ===" << std::endl;

    double blackHoleMass = 10.0 * 1.989e30;
    auto metric = std::make_shared<SchwarzschildMetric>(blackHoleMass);

    SceneGraphManager sceneGraph(metric);
    sceneGraph.createSolarSystem();
    const auto& scene = sceneGraph.getScene();
    assert(!scene.objects.empty() && "Scene should contain celestial bodies");
    std::cout << "[PASS] Celestial bodies rendered: " << scene.objects.size() << std::endl;

    auto scalar = metric->curvatureScalars(Event4D(0, 1e10, 0, 0));
    assert(scalar.valid);
    assert(scalar.kretschmann > 0.0 && "Curvature should be non-zero away from horizon");
    std::cout << "[PASS] Curvature non-zero: K = " << scalar.kretschmann << std::endl;

    Event4D start(0, 1e10, 0, 0);
    std::array<double,4> vel = {0, 0, 0, 0.1};
    GeodesicIntegrator integrator;
    integrator.setMetric(metric);
    auto trajectory = integrator.integrate(start, vel, GeodesicType::TIMELIKE, 100.0, true);
    assert(!trajectory.empty() && "Geodesic should produce non-empty trajectory");
    std::cout << "[PASS] Geodesic integrated: " << trajectory.size() << " steps" << std::endl;

    ExoplanetaryTTVFifthForceHunter ttvHunter;
    GalacticRotationCurveScanner rotScanner;
    std::vector<Event4D> scanTrajectory;
    for (int i = 0; i < 100; i++) {
        scanTrajectory.push_back(Event4D(
            static_cast<double>(i) * 1e5,
            1e10 + static_cast<double>(i) * 1e6,
            0.0,
            0.0
        ));
    }

    auto ttvFindings = ttvHunter.analyze(*metric, Event4D(0, 0, 0, 0), scanTrajectory);
    auto rotFindings = rotScanner.analyze(*metric, Event4D(0, 0, 0, 0), scanTrajectory);
    size_t totalFindings = ttvFindings.size() + rotFindings.size();
    assert(totalFindings > 0 && "Discovery instruments should produce findings");
    std::cout << "[PASS] Discovery findings produced: " << totalFindings << std::endl;

    std::cout << "=== SCENARIO INTEGRATION TEST PASSED ===" << std::endl;
    return 0;
}
