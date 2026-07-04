/**
 * @file TraceLogger.cpp
 * @brief Implementation of structured JSON logging
 */

#include "TraceLogger.h"
#include <iostream>
#include <thread>
#include <memory>

// Include GL header for error codes - must be after glad if used
#ifdef _WIN32
    #include <windows.h>
    #include <dbghelp.h>
    #pragma comment(lib, "dbghelp.lib")
#endif

// For OpenGL error codes
#define GL_NO_ERROR 0
#define GL_INVALID_ENUM 0x0500
#define GL_INVALID_VALUE 0x0501
#define GL_INVALID_OPERATION 0x0502
#define GL_OUT_OF_MEMORY 0x0505
#define GL_STACK_UNDERFLOW 0x0504
#define GL_STACK_OVERFLOW 0x0503

namespace quantumverse {
namespace utils {

TraceLogger& TraceLogger::instance() {
    static TraceLogger instance;
    return instance;
}

TraceLogger::~TraceLogger() {
    shutdown();
}

std::mutex& TraceLogger::getMutex() {
    if (!m_mutex) {
        m_mutex = std::make_unique<std::mutex>();
    }
    return *m_mutex;
}

void TraceLogger::initialize(const std::string& logPath) {
    // Open file first without locking (no other threads can access yet)
    m_logFile.open(logPath, std::ios::out | std::ios::trunc);
    m_initialized = m_logFile.is_open();
    
    if (m_initialized) {
        // Log without locking - we're the only thread at this point
        std::string jsonLine = "{\"timestamp\":\"" + getTimestamp() + "\","
                               "\"category\":\"INIT\","
                               "\"thread\":\"" + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + "\","
                               "\"message\":\"TraceLogger initialized\",\"data\":{\"logPath\":\"" + logPath + "\"}}\n";
        m_logFile << jsonLine;
        m_logFile.flush();
        std::cerr << jsonLine;
    }
}

void TraceLogger::log(LogCategory category, const std::string& message, const std::string& data) {
    std::lock_guard<std::mutex> lock(getMutex());
    
    std::string categoryStr;
    switch (category) {
        case LogCategory::INIT: categoryStr = "INIT"; break;
        case LogCategory::RENDER: categoryStr = "RENDER"; break;
        case LogCategory::GL: categoryStr = "GL"; break;
        case LogCategory::THREAD: categoryStr = "THREAD"; break;
        case LogCategory::ERR: categoryStr = "ERROR"; break;
        case LogCategory::CRASH: categoryStr = "CRASH"; break;
    }
    
    std::string threadId;
    {
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        threadId = oss.str();
    }
    
    std::string jsonLine = "{\"timestamp\":\"" + getTimestamp() + "\","
                           "\"category\":\"" + categoryStr + "\","
                           "\"thread\":\"" + threadId + "\","
                           "\"message\":\"" + escapeJson(message) + "\"";
    
    if (!data.empty()) {
        jsonLine += ",\"data\":" + data;
    }
    jsonLine += "}\n";
    
    // Write to file
    if (m_initialized && m_logFile.good()) {
        m_logFile << jsonLine;
        m_logFile.flush();
    }
    
    // Also write to stderr for immediate visibility
    std::cerr << jsonLine;
}

void TraceLogger::logGLError(const char* file, int line, unsigned int glError) {
    std::string errorName;
    switch (glError) {
        case GL_INVALID_ENUM: errorName = "GL_INVALID_ENUM"; break;
        case GL_INVALID_VALUE: errorName = "GL_INVALID_VALUE"; break;
        case GL_INVALID_OPERATION: errorName = "GL_INVALID_OPERATION"; break;
        case GL_OUT_OF_MEMORY: errorName = "GL_OUT_OF_MEMORY"; break;
        case GL_STACK_UNDERFLOW: errorName = "GL_STACK_UNDERFLOW"; break;
        case GL_STACK_OVERFLOW: errorName = "GL_STACK_OVERFLOW"; break;
        default: errorName = "UNKNOWN(" + std::to_string(glError) + ")"; break;
    }
    
    std::string data = "{\"file\":\"" + std::string(file) + "\","
                       "\"line\":" + std::to_string(line) + ","
                       "\"errorCode\":\"" + errorName + "\"}";
    
    log(LogCategory::GL, "OpenGL error detected", data);
}

void TraceLogger::logThread(const std::string& context) {
    std::string threadId;
    {
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        threadId = oss.str();
    }
    
    std::string data = "{\"context\":\"" + escapeJson(context) + "\","
                       "\"threadId\":\"" + threadId + "\"}";
    
    log(LogCategory::THREAD, "Thread context", data);
}

void TraceLogger::flush() {
    std::lock_guard<std::mutex> lock(getMutex());
    if (m_initialized && m_logFile.good()) {
        m_logFile.flush();
    }
}

void TraceLogger::shutdown() {
    std::lock_guard<std::mutex> lock(getMutex());
    if (m_logFile.is_open()) {
        log(LogCategory::INIT, "TraceLogger shutting down");
        m_logFile.close();
    }
    m_initialized = false;
}

std::string TraceLogger::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return oss.str();
}

std::string TraceLogger::escapeJson(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

} // namespace utils
} // namespace quantumverse