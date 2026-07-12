// QuantumVerse Test: GL_CHECK gate (P3-3 TDD contract test)
//
// Verifies the compile-time guarantee that GL_CHECK() is compiled out in
// Release builds (NDEBUG defined) and active in Debug builds. The macro
// body is stringized and inspected: a Release build must not reference the
// GL error query, while a Debug build must.

#include <cstdlib>
#include <iostream>
#include <string>

#include "rendering/gl_check.h"

// Two-level stringize so the macro is fully expanded before stringizing.
#define STRINGIZE(x) #x
#define EXPAND_STRINGIZE(x) STRINGIZE(x)

int main() {
    std::cout << "=== GL_CHECK Gate Test ===" << std::endl;

    const std::string expanded = EXPAND_STRINGIZE(GL_CHECK());
    std::cout << "  GL_CHECK() expands to: " << expanded << std::endl;

#if defined(NDEBUG) || defined(QUANTUMVERSE_GL_DEBUG)
    // Release / debug-build: macro must NOT query GL errors (no-op).
    const bool hasGlQuery = expanded.find("glGetError") != std::string::npos ||
                            expanded.find("glad_glGetError") != std::string::npos;
    if (!hasGlQuery) {
        std::cout << "  [PASS] GL_CHECK() is a no-op (no GL error query) in Release" << std::endl;
        return 0;
    }
    std::cerr << "  [FAIL] GL_CHECK() still references GL error query in Release" << std::endl;
    return 1;
#else
    // Debug build: macro MUST be active and query GL errors.
    const bool hasGlQuery = expanded.find("glGetError") != std::string::npos ||
                            expanded.find("glad_glGetError") != std::string::npos;
    if (hasGlQuery) {
        std::cout << "  [PASS] GL_CHECK() is active (GL error query present) in Debug" << std::endl;
        return 0;
    }
    std::cerr << "  [FAIL] GL_CHECK() is not active in Debug build" << std::endl;
    return 1;
#endif
}
