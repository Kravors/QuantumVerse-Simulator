/**
 * @file GLDebug.h
 * @brief OpenGL debug context callback for error detection and validation
 *
 * Provides a GL debug callback that logs all OpenGL errors, warnings,
 * and performance hints. Essential for catching rendering issues early.
 */

#pragma once

#include <string>
#include <functional>

namespace quantumverse {

// Forward declare GL types to avoid header conflicts
// These will be defined in the .cpp with GL headers
using GLDEBUG_ENUM = unsigned int;

/**
 * @brief OpenGL debug callback handler
 *
 * Installs a debug callback that receives all OpenGL messages including:
 * - Errors (GL_DEBUG_TYPE_ERROR)
 * - Deprecated behavior (GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR)
 * - Undefined behavior (GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
 * - Performance warnings (GL_DEBUG_TYPE_PERFORMANCE)
 * - Other messages (GL_DEBUG_TYPE_OTHER)
 */
class GLDebug {
public:
    /**
     * @brief Get singleton instance
     */
    static GLDebug& instance();

    /**
     * @brief Initialize GL debug callback (call after GL context creation)
     * @return true if debug callback was successfully installed
     */
    bool initialize();

    /**
     * @brief Check if debug callback is active
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief Set custom log callback
     * @param callback Function to receive log messages
     */
    void setLogCallback(std::function<void(const std::string& message)> callback);

    /**
     * @brief Get error count since last reset
     */
    int getErrorCount() const { return m_errorCount; }

    /**
     * @brief Get warning count since last reset
     */
    int getWarningCount() const { return m_warningCount; }

    /**
     * @brief Reset counters
     */
    void resetCounters();

    /**
     * @brief Log a GL error (for use with CHECK_GL_ERROR macro)
     */
    static void logGLError(GLDEBUG_ENUM error, const char* file, int line, const char* context);

private:
    GLDebug() = default;
    ~GLDebug() = default;

    // GL debug callback function (C-style for GL API)
    static void debugCallback(
        GLDEBUG_ENUM source,
        GLDEBUG_ENUM type,
        GLDEBUG_ENUM id,
        GLDEBUG_ENUM severity,
        int length,
        const char* message,
        const void* userParam
    );

    bool m_enabled = false;
    int m_errorCount = 0;
    int m_warningCount = 0;
    std::function<void(const std::string&)> m_logCallback;
};

} // namespace quantumverse