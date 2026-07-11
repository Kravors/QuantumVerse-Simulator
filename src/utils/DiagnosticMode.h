/**
 * @file DiagnosticMode.h
 * @brief Diagnostic mode for QuantumVerse - validates deployment integrity
 *
 * This module provides a --diag command-line option that performs:
 * - Shader directory validation
 * - OpenGL capability checks
 * - FBO creation tests
 * - Embedded shader fallback verification
 */

#pragma once

#include <string>
#include <vector>

namespace quantumverse {
namespace utils {

/**
 * @brief Diagnostic result structure
 */
struct DiagnosticResult {
    bool passed;
    std::string testName;
    std::string message;
    std::string details;
};

/**
 * @brief Diagnostic mode runner
 * Checks deployment integrity and reports issues
 */
class DiagnosticMode {
public:
    /**
     * @brief Run all diagnostic tests
     * @return Vector of diagnostic results
     */
    static std::vector<DiagnosticResult> runAllTests();

    /**
     * @brief Check if shader directory exists and has required files
     */
    static DiagnosticResult checkShaderDirectory();

    /**
     * @brief Check OpenGL version and extensions
     */
    static DiagnosticResult checkOpenGLCapabilities();

    /**
     * @brief Test FBO creation with the format used in CurvatureRenderer
     */
    static DiagnosticResult testFramebufferCreation();

    /**
     * @brief Test embedded shader fallback
     */
    static DiagnosticResult testEmbeddedShaders();

    /**
     * @brief Test trivial shader rendering to FBO
     */
    static DiagnosticResult testShaderRender();

    /**
     * @brief Print diagnostic report to stdout
     */
    static void printReport(const std::vector<DiagnosticResult>& results);

    /**
     * @brief Check if running in diagnostic mode
     */
    static bool isDiagnosticMode();

    /**
     * @brief Set diagnostic mode flag
     */
    static bool s_diagnosticMode;

private:
};

} // namespace utils
} // namespace quantumverse