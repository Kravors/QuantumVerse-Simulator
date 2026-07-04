/**
 * @file CrashHandler.cpp
 * @brief Implementation of Windows crash dump handling
 */

#include "CrashHandler.h"
#include "TraceLogger.h"
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
    #include <dbghelp.h>
    #include <tchar.h>
    #pragma comment(lib, "dbghelp.lib")
#endif

namespace quantumverse {
namespace utils {

#ifdef _WIN32
static std::string s_dumpPath;
static std::string s_lastCrashDump;

/**
 * @brief Exception filter for crash handling
 */
LONG WINAPI crashHandler(EXCEPTION_POINTERS* exceptionInfo) {
    // Log the crash
    TraceLogger::instance().log(
        LogCategory::CRASH,
        "Application crash detected",
        "{\"exceptionCode\":\"" + std::to_string(exceptionInfo->ExceptionRecord->ExceptionCode) + "\"}"
    );
    
    // Generate crash dump
    std::string dumpFile = s_dumpPath + "/quantumverse_crash_" + 
        std::to_string(GetCurrentProcessId()) + ".dmp";
    
    HANDLE hFile = CreateFileA(
        dumpFile.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    
    if (hFile != INVALID_HANDLE_VALUE) {
        HANDLE hProcess = GetCurrentProcess();
        DWORD processId = GetCurrentProcessId();
        
        MINIDUMP_EXCEPTION_INFORMATION exceptionInfoDump{};
        exceptionInfoDump.ThreadId = GetCurrentThreadId();
        exceptionInfoDump.ExceptionPointers = exceptionInfo;
        
        BOOL result = MiniDumpWriteDump(
            hProcess,
            processId,
            hFile,
            MiniDumpNormal,
            &exceptionInfoDump,
            nullptr,
            nullptr
        );
        
        s_lastCrashDump = dumpFile;
        CloseHandle(hFile);
    }
    
    return EXCEPTION_EXECUTE_HANDLER;
}

void initializeCrashHandler(const std::string& dumpPath) {
    s_dumpPath = dumpPath;
    
    // Set up unhandled exception filter
    SetUnhandledExceptionFilter(crashHandler);
    
    // Log to stderr directly to avoid potential issues with TraceLogger mutex
    // (TraceLogger may not be fully initialized yet)
    std::cerr << "{\"timestamp\":\"INIT\",\"category\":\"INIT\",\"message\":\"Crash handler initialized\",\"data\":{\"dumpPath\":\"" << dumpPath << "\"}}\n";
    std::cerr.flush();
}

bool isDebuggerPresent() {
    return ::IsDebuggerPresent() == TRUE;
}

std::string getLastCrashDumpPath() {
    return s_lastCrashDump;
}

#else

// Non-Windows implementations
void initializeCrashHandler(const std::string& dumpPath) {
    // No-op on non-Windows
    (void)dumpPath;
}

bool isDebuggerPresent() {
    return false;
}

std::string getLastCrashDumpPath() {
    return "";
}

#endif

} // namespace utils
} // namespace quantumverse