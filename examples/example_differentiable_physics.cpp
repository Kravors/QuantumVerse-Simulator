#define _USE_MATH_DEFINES
#include <iostream>
#include <iomanip>
#include "../src/physics/DifferentiableGeodesicIntegrator.h"
#include "../src/physics/DifferentiableCurvature.h"
#include "../src/spacetime/MetricTensor.h"
#include "../src/spacetime/Event4D.h"
#include "../src/math/AutoDiff.h"

using namespace quantumverse;

// Schwarzschild metric with position-dependent evaluate
class SchwarzschildMetricAD : public MetricTensor {
public:
    double M;

    SchwarzschildMetricAD(double mass = 1.0) : M(mass) {
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                g[i][j] = (i == j) ? (i == 0 ? -1.0 : 1.0) : 0.0;
    }

    std::array<std::array<double, 4>, 4> evaluate(const Event4D& event) const override {
        double r = std::sqrt(event.x*event.x + event.y*event.y + event.z*event.z);
        double theta = std::atan2(std::sqrt(event.x*event.x + event.y*event.y), event.z);
        if (std::abs(event.z) < 1e-10) theta = M_PI / 2.0;
        double sinTheta = std::sin(theta);
        double rs = 2.0 * M;

        std::array<std::array<double, 4>, 4> metric;
        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
                metric[i][j] = 0.0;

        if (r > rs * 1.001) {
            double f = 1.0 - rs / r;
            metric[0][0] = -f;
            metric[1][1] = 1.0 / f;
            metric[2][2] = r * r;
            metric[3][3] = r * r * sinTheta * sinTheta;
        } else {
            metric[0][0] = -1.0;
            metric[1][1] = 1.0;
            metric[2][2] = 1.0;
            metric[3][3] = 1.0;
        }
        return metric;
    }
};

// Test integrator with parameter perturbation
class TestDiffIntegrator : public DifferentiableGeodesicIntegrator {
public:
    double baseMass;

    TestDiffIntegrator() : DifferentiableGeodesicIntegrator(1e-6, 1e-10, 0.1, 0.9, 50000), baseMass(1.0) {}
    TestDiffIntegrator(double tol, double minStep, double maxStep, double safety, int maxIter)
        : DifferentiableGeodesicIntegrator(tol, minStep, maxStep, safety, maxIter), baseMass(1.0) {}

    std::shared_ptr<MetricTensor> createPerturbedMetric(int, double delta) const override {
        return std::make_shared<SchwarzschildMetricAD>(baseMass + delta);
    }
};

void printSeparator(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(70, '=') << "\n";
}

int main() {
    std::cout << std::fixed << std::setprecision(8);

    // ============================================================
    // EXAMPLE 1: Differentiable Geodesic Integration (N=1)
    // ============================================================
    printSeparator("EXAMPLE 1: Gradient of Geodesic w.r.t. Mass (N=1)");

    SchwarzschildMetricAD metric(5.0);
    TestDiffIntegrator diffInt;
    diffInt.baseMass = 5.0;
    diffInt.setMetric(std::make_shared<SchwarzschildMetricAD>(metric));

    Event4D start{0, 10.0, 0.0, 0.0};
    std::array<double, 4> vel = {1.0, 0.0, 0.0, 0.5};

    auto result = diffInt.integrateAD1(start, vel, GeodesicType::TIMELIKE, 1.0, 5.0, false);

    std::cout << "Initial event:  r = 10.0\n";
    std::cout << "Final position: r = " << result.finalPosition[1].getValue() << "\n";
    std::cout << "Gradient dr_final/dM = " << result.finalPosition[1].getDerivs()[0] << "\n";

    // ============================================================
    // EXAMPLE 2: Curvature Gradients
    // ============================================================
    printSeparator("EXAMPLE 2: Curvature Tensor Gradients");

    DifferentiableCurvature curvature(1e-5);
    curvature.setMetric(std::make_shared<SchwarzschildMetricAD>(metric));

    std::array<ADVariable<1>, 4> pos_AD;
    double r_val = 10.0;
    pos_AD[0] = ADVariable<1>(0.0, std::array<double,1>{0.0});
    pos_AD[1] = ADVariable<1>(r_val, std::array<double,1>{0.0});
    pos_AD[2] = ADVariable<1>(M_PI/4, std::array<double,1>{0.0});
    pos_AD[3] = ADVariable<1>(0.0, std::array<double,1>{0.0});

    auto K = curvature.computeKretschmannAD<1>(pos_AD);

    Event4D pos_event{0, 10.0, M_PI/4, 0.0};
    std::array<double, 1> params = {5.0};
    auto K_grad = curvature.computeKretschmannGradient<1>(pos_event, params);

    std::cout << "At r = 10.0 (Schwarzschild, M=5.0):\n";
    std::cout << "  Kretschmann K = " << K.getValue() << "\n";
    std::cout << "  dK/dM (FD) = " << K_grad[0] << "\n";
    std::cout << "  Analytic: dK/dM = 96M/r^6 = " << 96.0*5.0/std::pow(10.0,6) << "\n";

    // ============================================================
    // EXAMPLE 3: Ricci Scalar (Vacuum Check)
    // ============================================================
    printSeparator("EXAMPLE 3: Ricci Scalar in Schwarzschild Vacuum");

    auto R = curvature.computeRicciScalarAD<1>(pos_AD);
    std::cout << "Ricci scalar R = " << R.getValue() << "\n";
    std::cout << "Expected: R ~ 0 (Schwarzschild is vacuum solution)\n";

    // ============================================================
    // EXAMPLE 4: Light Deflection Gradient
    // ============================================================
    printSeparator("EXAMPLE 4: Light Deflection Gradient");

    SchwarzschildMetricAD sunMetric(1.477e3);
    TestDiffIntegrator lightDiff;
    lightDiff.baseMass = 1.477e3;
    lightDiff.setMetric(std::make_shared<SchwarzschildMetricAD>(sunMetric));

    double b = 6.96e8;
    Event4D lightStart{0, -100.0 * b, b, 0.0};
    std::array<double, 4> lightVel = {1.0, 1.0, 0.0, 0.0};

    auto lightResult = lightDiff.integrateAD1(lightStart, lightVel, GeodesicType::LIGHTLIKE, 2000.0, 1.477e3, false);

    double finalY = lightResult.finalPosition[2].getValue();
    double dYdM = lightResult.finalPosition[2].getDerivs()[0];
    std::cout << "Impact parameter b = " << b << "\n";
    std::cout << "Deflection dy = " << (finalY - b) << " m\n";
    std::cout << "Gradient dy/dM = " << dYdM << "\n";
    std::cout << "Analytic dAlpha/dM = 4/b = " << 4.0/b << "\n";

    printSeparator("END OF DIFFERENTIABLE PHYSICS DEMO");
    std::cout << "\nAll examples completed successfully!\n";

    return 0;
}