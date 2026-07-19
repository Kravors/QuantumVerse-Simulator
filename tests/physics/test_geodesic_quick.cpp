#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"
#include "physics/GeodesicIntegrator.h"
#include <iostream>

using namespace quantumverse;

int main() {
    double M = 1.0;
    auto sch = std::make_shared<SchwarzschildMetric>(M * Event4D::C * Event4D::C / Event4D::G);
    GeodesicIntegrator integrator(1e-6);
    integrator.setMetric(sch);

    Event4D start(0.0, 20.0 * M, 0.0, 0.0);
    std::array<double, 4> vel = {0.9, 0.0, 0.2, 0.0};

    auto g0 = sch->evaluate(start);
    double norm0 = 0.0;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            norm0 += g0[i][j] * vel[i] * vel[j];
    double scale = std::sqrt(-1.0 / norm0);
    for (int i = 0; i < 4; i++) vel[i] *= scale;

    std::cout << "Starting integration..." << std::endl;
    auto traj = integrator.integrate(start, vel, GeodesicType::TIMELIKE, 1.0, true);
    std::cout << "Done. Steps: " << traj.size() << std::endl;
    return 0;
}
