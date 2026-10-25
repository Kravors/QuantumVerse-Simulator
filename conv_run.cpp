#include <iostream>
#include <cmath>
#include <iomanip>
#include <cstdint>
#include <array>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "src/spacetime/Event4D.h"
#include "src/spacetime/MetricTensor.h"
#include "src/physics/PhysicsConstants.h"

using namespace quantumverse;

// Orbit equation RHS: du/dφ = v, dv/dφ = 3Mu² - u
static inline std::pair<double,double> orbitRhs(double M, double u, double v) {
    return {v, 3.0 * M * u * u - u};
}

// RK4 step
static inline void rk4Step(double M, double& u, double& v, double dPhi) {
    auto [k1u, k1v] = orbitRhs(M, u, v);
    auto [k2u, k2v] = orbitRhs(M, u + 0.5 * dPhi * k1u, v + 0.5 * dPhi * k1v);
    auto [k3u, k3v] = orbitRhs(M, u + 0.5 * dPhi * k2u, v + 0.5 * dPhi * k2v);
    auto [k4u, k4v] = orbitRhs(M, u + dPhi * k3u, v + dPhi * k3v);
    u += (dPhi / 6.0) * (k1u + 2.0 * k2u + 2.0 * k3u + k4u);
    v += (dPhi / 6.0) * (k1v + 2.0 * k2v + 2.0 * k3v + k4v);
}

int main() {
    // Confirm SchwarzschildMetric constructor takes mass in kg
    // Geometric M=1 corresponds to M_kg = c^2/G
    const double M_kg = PHYS_C() * PHYS_C() / PHYS_G();
    SchwarzschildMetric metric(M_kg);
    const double rs = 2.0 * PHYS_G() * M_kg / (PHYS_C() * PHYS_C());  // = 2 in geometric units
    const double M_geom = 1.0;  // geometric mass

    // Confirm Event4D public fields .t/.x/.y/.z
    const double rStart = 60.0 * M_geom;
    const double rMax = 10000.0 * M_geom;

    double bs[] = {6.0 * M_geom, 12.0 * M_geom};
    double hs[] = {1.0, 0.5, 0.25, 0.125};

    double dFinest[2] = {0.0, 0.0};

    for (int bi = 0; bi < 2; bi++) {
        double b = bs[bi];
        for (int hi = 0; hi < 4; hi++) {
            double h = hs[hi];

            // Start at r0 on incoming leg, phi = 0
            double u = 1.0 / rStart;
            double v = +std::sqrt(std::max(0.0, 1.0 / (b * b) - u * u * (1.0 - 2.0 * M_geom * u)));
            double phi = 0.0;
            double dPhi = h;
            int steps = 0;
            const int maxSteps = 2000000;
            bool captured = false;
            bool escaped = false;

            double uPrev = u, vPrev = v, phiPrev = phi;

            while (steps < maxSteps && !captured && !escaped) {
                double r = 1.0 / u;
                if (r < rs) { captured = true; break; }
                if (r > rMax) { escaped = true; break; }
                if (std::abs(phi) > 100.0 * M_PI) break;

                uPrev = u; vPrev = v; phiPrev = phi;
                rk4Step(M_geom, u, v, dPhi);
                phi += dPhi;
                steps++;

                // Interpolate escape point when u crosses zero
                if (u <= 0.0 && uPrev > 0.0) {
                    double frac = uPrev / (uPrev - u);
                    phi = phiPrev + frac * dPhi;
                    escaped = true;
                    break;
                }
            }

            if (!escaped) {
                std::cout << "[CONV] b=" << (b / M_geom) << "M h=" << h << " d=nan" << std::endl;
                continue;
            }

            // Flat-space reference: angle from r0 to periapsis to infinity
            double flatSweep = std::acos(std::min(1.0, b / rStart)) + M_PI / 2.0;
            double deflection = std::abs(phi) - flatSweep;

            if (hi == 3) dFinest[bi] = deflection;

            std::cout << "[CONV] b=" << (b / M_geom) << "M h=" << h << " d=" << deflection << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << "d(b=6M, h=0.125) = " << dFinest[0] << std::endl;
    std::cout << "d(b=12M, h=0.125) = " << dFinest[1] << std::endl;
    std::cout << "ratio d1/d2 = " << (dFinest[0] / dFinest[1]) << std::endl;

    return 0;
}
