#include "math/Vector4D.h"
#include "math/Matrix4x4.h"
#include "physics/GeodesicIntegrator.h"
#include "Event4D.h"
#include "MetricTensor.h"
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace quantumverse;

extern "C" {
    int LLVMFuzzerInitialize(int* argc, char*** argv) { return 0; }
    
    int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
        if (size < sizeof(double) * 22) return 0;
        
        const double* vals = reinterpret_cast<const double*>(data);
        
        Vector4d start(vals[0], vals[1], vals[2], vals[3]);
        Matrix4x4<double> metric;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                metric(i, j) = vals[4 + i * 4 + j];
            }
        }
        
        double properTime = std::abs(vals[20]);
        int steps = static_cast<int>(std::abs(vals[21])) % 1000 + 10;
        
        Vector4d cppResult = GeodesicIntegrator::integrate(start, metric, properTime, steps);
        Vector4d pythonResult = GeodesicIntegrator::integratePythonSurrogate(start, metric, properTime, steps);
        
        double maxDiff = 0.0;
        for (int i = 0; i < 4; i++) {
            double scale = std::max(std::abs(cppResult[i]), 1.0);
            double relDiff = std::abs(cppResult[i] - pythonResult[i]) / scale;
            if (relDiff > maxDiff) maxDiff = relDiff;
        }
        
        if (maxDiff > 1e-3) {
            fprintf(stderr, "DIFFERENTIAL FAILURE: rel_diff=%f\n", maxDiff);
            fprintf(stderr, "  start=(%f,%f,%f,%f)\n", start[0], start[1], start[2], start[3]);
            fprintf(stderr, "  properTime=%f, steps=%d\n", properTime, steps);
            fprintf(stderr, "  cpp=(%f,%f,%f,%f)\n", cppResult[0], cppResult[1], cppResult[2], cppResult[3]);
            fprintf(stderr, "  py=(%f,%f,%f,%f)\n", pythonResult[0], pythonResult[1], pythonResult[2], pythonResult[3]);
            __builtin_trap();
            return 1;
        }
        
        return 0;
    }
}