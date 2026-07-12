// QuantumVerse Test: Performance Gate (TDD contract test)
//
// Verifies the performance regression gate:
//   1. The parse/threshold logic accepts a good log and rejects bad ones.
//   2. Thresholds can be overridden via environment variables.
//   3. The qml_performance_baseline executable, when present, returns exit 0
//      for a passing run (best-effort: skipped if the binary is not built).

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "perf_gate.h"

namespace {

int g_failures = 0;

void check(bool condition, const char* label) {
    if (condition) {
        std::cout << "  [PASS] " << label << std::endl;
    } else {
        std::cerr << "  [FAIL] " << label << std::endl;
        ++g_failures;
    }
}

#if defined(_WIN32)
int setEnv(const char* name, const char* value) {
    return _putenv_s(name, value);
}
#else
int setEnv(const char* name, const char* value) {
    return setenv(name, value, 1);
}
#endif

}  // namespace

int main() {
    std::cout << "=== Performance Gate Test ===" << std::endl;

    const double avgTh = 60.0;
    const double maxTh = 80.0;

    const std::string goodLog =
        "Some preamble\nAverage frame time: 16.2 ms, Max: 31.5 ms, Frames: 50\nDone\n";
    const std::string badAvgLog =
        "Average frame time: 75.0 ms, Max: 31.5 ms, Frames: 50\n";
    const std::string badMaxLog =
        "Average frame time: 16.2 ms, Max: 95.0 ms, Frames: 50\n";
    const std::string malformedLog = "no statistics here\n";
    const std::string nanLog =
        "Average frame time: nan ms, Max: 31.5 ms, Frames: 50\n";

    check(perf_gate::evaluate(goodLog, avgTh, maxTh) == perf_gate::Eval::Pass,
          "good log passes");
    check(perf_gate::evaluate(badAvgLog, avgTh, maxTh) == perf_gate::Eval::AvgExceeded,
          "avg over threshold fails");
    check(perf_gate::evaluate(badMaxLog, avgTh, maxTh) == perf_gate::Eval::MaxExceeded,
          "max over threshold fails");
    check(perf_gate::evaluate(malformedLog, avgTh, maxTh) == perf_gate::Eval::ParseError,
          "malformed log fails to parse");
    check(perf_gate::evaluate(nanLog, avgTh, maxTh) == perf_gate::Eval::BadValue,
          "non-finite value is rejected");

    // Boundary: exactly at threshold must fail (>= comparison).
    const std::string boundaryLog =
        "Average frame time: 60.0 ms, Max: 80.0 ms, Frames: 50\n";
    check(perf_gate::evaluate(boundaryLog, avgTh, maxTh) == perf_gate::Eval::AvgExceeded,
          "value exactly at threshold fails");

    // Environment override tightens the gate.
    setEnv("QUANTUMVERSE_PERF_AVG_THRESHOLD_MS", "10");
    setEnv("QUANTUMVERSE_PERF_MAX_THRESHOLD_MS", "40");
    const double tightAvg = perf_gate::thresholdFromEnv("QUANTUMVERSE_PERF_AVG_THRESHOLD_MS", avgTh);
    const double tightMax = perf_gate::thresholdFromEnv("QUANTUMVERSE_PERF_MAX_THRESHOLD_MS", maxTh);
    check(tightAvg == 10.0 && tightMax == 40.0,
          "env vars override thresholds");
    check(perf_gate::evaluate(goodLog, tightAvg, tightMax) == perf_gate::Eval::AvgExceeded,
          "good log fails under tighter env threshold");
    // Restore defaults for any subsequent logic.
    setEnv("QUANTUMVERSE_PERF_AVG_THRESHOLD_MS", "60");
    setEnv("QUANTUMVERSE_PERF_MAX_THRESHOLD_MS", "80");

    // Executable contract: if the baseline binary is built and available in the
    // working directory, invoking it must yield exit code 0 for a passing run.
#ifdef _WIN32
    const char* baselineName = "qml_performance_baseline.exe";
#else
    const char* baselineName = "qml_performance_baseline";
#endif
    bool baselineBuilt = false;
    {
        std::ifstream probe(baselineName);
        baselineBuilt = probe.is_open();
    }
    if (baselineBuilt) {
        // P3-1 contract: the per-frame renderer_render.log must NOT be
        // created when PERF_TRACE is off (the default / gate build).
        std::remove("renderer_render.log");
        const std::string invoke = std::string("\"") + baselineName + "\"";
        const int ret = std::system(invoke.c_str());
        check(ret == 0, "baseline executable invoked and returned exit 0");
        bool logExists = false;
        {
            std::ifstream log("renderer_render.log");
            logExists = log.is_open();
        }
        check(!logExists, "renderer_render.log not created when PERF_TRACE is off");
    } else {
        std::cout << "  [SKIP] baseline binary not present in working directory" << std::endl;
    }

    if (g_failures == 0) {
        std::cout << "All performance gate checks passed." << std::endl;
        return 0;
    }
    std::cerr << g_failures << " performance gate check(s) failed." << std::endl;
    return 1;
}
