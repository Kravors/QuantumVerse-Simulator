// Phase 5.6: Crash helper executable
// Initializes the QuantumVerse crash handler and deliberately crashes
// so the test can verify a dump file is produced.

#include "utils/CrashHandler.h"
#include <cstdlib>
#include <windows.h>

int main()
{
    // Write crash dumps next to the executable so the test can find them.
    char exePath[MAX_PATH] = {0};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string dumpDir = exePath;
    auto pos = dumpDir.find_last_of("\\/");
    if (pos != std::string::npos) {
        dumpDir = dumpDir.substr(0, pos);
    }

    quantumverse::utils::initializeCrashHandler(dumpDir);

    // Deliberately cause an access violation.
    int* p = nullptr;
    *p = 42;

    return EXIT_FAILURE;
}
