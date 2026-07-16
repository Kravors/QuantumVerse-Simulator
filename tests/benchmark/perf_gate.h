#pragma once

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <regex>
#include <string>

namespace perf_gate {

/// Outcome of evaluating a performance log against thresholds.
enum class Eval {
    Pass,         ///< avg and max within thresholds
    NoLog,        ///< log file could not be opened
    ParseError,   ///< statistics line could not be parsed
    BadValue,     ///< parsed values are non-finite
    AvgExceeded,  ///< average frame time at/over threshold
    MaxExceeded   ///< maximum frame time at/over threshold
};

inline const char* evalToString(Eval e) {
    switch (e) {
        case Eval::Pass:        return "pass";
        case Eval::NoLog:       return "log file missing";
        case Eval::ParseError:  return "could not parse statistics";
        case Eval::BadValue:    return "non-finite frame time";
        case Eval::AvgExceeded: return "average frame time exceeded threshold";
        case Eval::MaxExceeded: return "maximum frame time exceeded threshold";
    }
    return "unknown";
}

/// Parse the `Average frame time: <avg> ms, Max: <max> ms` line from a log.
inline bool parseLog(const std::string& log, double& avg, double& max) {
    static const std::regex avg_regex(
        R"(Average frame time:\s+([^\s,]+)\s*ms,\s+Max:\s+([^\s,]+)\s*ms)");
    std::smatch match;
    if (std::regex_search(log, match, avg_regex)) {
        try {
            avg = std::stod(match[1]);
            max = std::stod(match[2]);
        } catch (...) {
            return false;
        }
        return true;
    }
    return false;
}

/// Read a double threshold from an environment variable, falling back to a
/// compile-time default when unset or unparseable. This lets CI adjust the
/// gate without recompiling.
inline double thresholdFromEnv(const char* name, double fallback) {
    double value = fallback;
#ifdef _MSC_VER
    char* e = nullptr;
    size_t envsz = 0;
    if (_dupenv_s(&e, &envsz, name) == 0 && e && *e) {
        try {
            value = std::stod(e);
        } catch (...) {
        }
        free(e);
    }
#else
    const char* e = std::getenv(name);
    if (e && *e) {
        try {
            value = std::stod(e);
        } catch (...) {
        }
    }
#endif
    return value;
}

/// Evaluate log content against the supplied thresholds.
inline Eval evaluate(const std::string& log, double avgThreshold, double maxThreshold) {
    double avg = 0.0;
    double max = 0.0;
    if (!parseLog(log, avg, max)) {
        return Eval::ParseError;
    }
    if (!std::isfinite(avg) || !std::isfinite(max)) {
        return Eval::BadValue;
    }
    if (avg >= avgThreshold) {
        return Eval::AvgExceeded;
    }
    if (max >= maxThreshold) {
        return Eval::MaxExceeded;
    }
    return Eval::Pass;
}

/// Convenience: 0 when within thresholds, 1 otherwise.
inline int exitCode(const std::string& log, double avgThreshold, double maxThreshold) {
    return evaluate(log, avgThreshold, maxThreshold) == Eval::Pass ? 0 : 1;
}

}  // namespace perf_gate
