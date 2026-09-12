// QuantumVerse Test Harness: NDEBUG-safe assertions
//
// The test binaries are built Release by default (CMAKE_BUILD_TYPE is not
// set, and `--config Release` passes /DNDEBUG), so the standard `assert()`
// macro is compiled out and every assertion in the suite silently became a
// no-op.  Every "23/23 passing" from a Release run was meaningless.
//
// These macros throw std::runtime_error instead of calling abort, so they
// remain active under NDEBUG and produce real failure messages.  Convert
// every `assert(cond)` in tests/ to `QV_CHECK(cond)` and every
// `assert(std::abs(a - b) < eps)` to `QV_CHECK_NEAR(a, b, eps)`.

#pragma once

#include <stdexcept>
#include <cmath>
#include <string>

namespace quantumverse {
namespace test {

/// @brief Assert a boolean condition, throwing on failure.
/// @param cond Expression that must be true.
#define QV_CHECK(cond)                                                     \
    do {                                                                   \
        if (!(cond)) {                                                    \
            throw std::runtime_error(                                     \
                std::string("QV_CHECK failed: ") + #cond +                \
                " at " + __FILE__ + ":" + std::to_string(__LINE__));     \
        }                                                                 \
    } while (0)

/// @brief Assert two values agree within an absolute tolerance.
/// @param a First value.
/// @param b Second value.
/// @param eps Absolute tolerance.
#define QV_CHECK_NEAR(a, b, eps)                                           \
    do {                                                                   \
        double _qv_a = static_cast<double>(a);                            \
        double _qv_b = static_cast<double>(b);                            \
        double _qv_eps = static_cast<double>(eps);                        \
        if (std::fabs(_qv_a - _qv_b) > _qv_eps) {                        \
            throw std::runtime_error(                                     \
                std::string("QV_CHECK_NEAR failed: ") + #a + "=" +        \
                std::to_string(_qv_a) + " vs " + #b + "=" +               \
                std::to_string(_qv_b) + " (eps=" +                        \
                std::to_string(_qv_eps) + ") at " + __FILE__ + ":" +      \
                std::to_string(__LINE__));                                \
        }                                                                 \
    } while (0)

}  // namespace test
}  // namespace quantumverse