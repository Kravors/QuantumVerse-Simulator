#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "perf_gate.h"
#include <QCoreApplication>
#include <QDir>

int main() {
    const double avgThreshold = perf_gate::thresholdFromEnv(
        "QUANTUMVERSE_PERF_AVG_THRESHOLD_MS", PERF_AVG_THRESHOLD_DEFAULT);
    const double maxThreshold = perf_gate::thresholdFromEnv(
        "QUANTUMVERSE_PERF_MAX_THRESHOLD_MS", PERF_MAX_THRESHOLD_DEFAULT);

    const char* logPath = "headless_performance.log";
    std::remove(logPath);

    const QString exePath = QCoreApplication::applicationDirPath() + QDir::separator() + QStringLiteral("quantumverse_qml");
    const std::string command = std::string("\"") + exePath.toStdString() + "\" --headless --frames 50";
    std::cerr << "Running: " << command << std::endl;
    const int ret = std::system(command.c_str());
    if (ret != 0) {
        std::cerr << "Headless run failed with exit code " << ret << std::endl;
        return 1;
    }

    std::ifstream log(logPath);
    if (!log.is_open()) {
        std::cerr << "Could not open " << logPath << std::endl;
        return 1;
    }

    std::string content((std::istreambuf_iterator<char>(log)),
                        std::istreambuf_iterator<char>());
    log.close();

    const perf_gate::Eval result = perf_gate::evaluate(content, avgThreshold, maxThreshold);
    if (result != perf_gate::Eval::Pass) {
        double avg = 0.0;
        double max = 0.0;
        perf_gate::parseLog(content, avg, max);
        std::cerr << "Performance gate failed (" << perf_gate::evalToString(result)
                  << "): avg=" << avg << " ms (threshold " << avgThreshold
                  << "), max=" << max << " ms (threshold " << maxThreshold << ")"
                  << std::endl;
        return 1;
    }

    std::cout << "Performance gate passed (avgThreshold=" << avgThreshold
              << " ms, maxThreshold=" << maxThreshold << " ms)" << std::endl;
    return 0;
}
