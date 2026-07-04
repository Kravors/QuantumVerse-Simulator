/**
 * @file TraceLogger.h
 * @brief Structured JSON logging for crash analysis and debugging
 *
 * Provides timestamped, structured logging to both file and stderr.
 * Each log entry is a single-line JSON object for easy parsing.
 */

#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
    #include <windows.h>
    #include <dbghelp.h>
#endif

namespace quantumverse {
namespace utils {

/**
 * @brief Log entry categories for filtering
 */
enum class LogCategory {
    INIT,           ///< Initialization steps
    RENDER,         ///< Rendering operations
    GL,             ///< OpenGL specific
    THREAD,         ///< Thread operations
    ERR,            ///< Error conditions
    CRASH           ///< Crash information
};

/**
 * @brief Central trace logger for structured JSON logging
 */
class TraceLogger {
public:
    /**
     * @brief Get singleton instance
     */
    static TraceLogger& instance();

    /**
     * @brief Initialize the logger with output file
     * @param logPath Path to the log file
     */
    void initialize(const std::string& logPath);

    /**
     * @brief Log a structured message
     * @param category Log category
     * @param message Log message
     * @param data Optional JSON data string
     */
    void log(LogCategory category, const std::string& message, const std::string& data = "");

    /**
     * @brief Log an OpenGL error
     * @param file Source file name
     * @param line Source line number
     * @param glError OpenGL error code
     */
    void logGLError(const char* file, int line, unsigned int glError);

    /**
     * @brief Log thread information
     * @param context Thread context description
     */
    void logThread(const std::string& context);

    /**
     * @brief Flush the log buffer
     */
    void flush();

    /**
     * @brief Shutdown the logger
     */
    void shutdown();

private:
    TraceLogger() = default;
    ~TraceLogger();

    std::ofstream m_logFile;
    std::unique_ptr<std::mutex> m_mutex;  // Lazy initialization to avoid static init crash
    bool m_initialized{false};

    /**
     * @brief Get the mutex (lazy initialization)
     */
    std::mutex& getMutex();

    /**
     * @brief Get current ISO 8601 timestamp
     */
    std::string getTimestamp();

    /**
     * @brief Escape JSON string
     */
    std::string escapeJson(const std::string& str);
};

/**
 * @brief GL error checking macro
 */
#define CHECK_GL_ERROR() \
    do { \
        unsigned int err = glGetError(); \
        if (err != GL_NO_ERROR) { \
            quantumverse::utils::TraceLogger::instance().logGLError(__FILE__, __LINE__, err); \
        } \
    } while(0)

/**
 * @brief Convenience logging macros
 */
#define TRACE_INIT(msg) quantumverse::utils::TraceLogger::instance().log(quantumverse::utils::LogCategory::INIT, msg)
#define TRACE_RENDER(msg) quantumverse::utils::TraceLogger::instance().log(quantumverse::utils::LogCategory::RENDER, msg)
#define TRACE_GL(msg) quantumverse::utils::TraceLogger::instance().log(quantumverse::utils::LogCategory::GL, msg)
#define TRACE_THREAD(msg) quantumverse::utils::TraceLogger::instance().log(quantumverse::utils::LogCategory::THREAD, msg)
#define TRACE_ERROR(msg) quantumverse::utils::TraceLogger::instance().log(quantumverse::utils::LogCategory::ERR, msg)

} // namespace utils
} // namespace quantumverse