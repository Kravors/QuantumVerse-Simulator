/**
 * @file CrashHandler.h
 * @brief Windows crash dump generation and handling
 *
 * Provides crash dump generation for Windows platforms to aid in debugging.
 */

#pragma once

#include <string>

#ifdef _WIN32
    #include <windows.h>
    #include <dbghelp.h>
#endif

namespace quantumverse {
namespace utils {

/**
 * @brief Initialize crash handling for the application
 * @param dumpPath Path to write crash dump files
 */
void initializeCrashHandler(const std::string& dumpPath);

/**
 * @brief Check if running under a debugger
 * @return true if debugger is attached
 */
bool isDebuggerPresent();

/**
 * @brief Get the last crash dump path (if any)
 * @return Path to the most recent crash dump, or empty string if none
 */
std::string getLastCrashDumpPath();

} // namespace utils
} // namespace quantumverse