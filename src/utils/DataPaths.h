/**
 * @file DataPaths.h
 * @brief Runtime data path resolution for QuantumVerse
 *
 * Resolves data/ relative to the executable, supporting both installed
 * layouts (data/ next to exe) and local dev layouts (../data/ from build/Release/).
 */

#pragma once

#include <string>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QCoreApplication>

namespace quantumverse {
namespace utils {

/**
 * @brief Resolve the repository root directory.
 *
 * Search order (furthest-first to avoid matching build/ intermediates):
 *   1. <applicationDirPath>/../../../..
 *   2. <applicationDirPath>/../../..
 *   3. <applicationDirPath>/../..
 *   4. <applicationDirPath>/..
 *   5. <applicationDirPath>
 *
 * @return Absolute path to the repo root directory.
 */
inline std::string repoRoot() {
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + "/../../../..",
        appDir + "/../../..",
        appDir + "/../..",
        appDir + "/..",
        appDir,
    };
    for (const auto& c : candidates) {
        const QString normalized = QDir::cleanPath(c);
        if (QDir(normalized).exists("config")) {
            return normalized.toStdString();
        }
    }
    return QDir(QCoreApplication::applicationDirPath()).path().toStdString();
}

/**
 * @brief Resolve the data directory.
 *
 * Explicit candidate list, furthest-first to avoid matching build/ intermediates:
 *   1. <applicationDirPath>/../../../../data
 *   2. <applicationDirPath>/../../../data
 *   3. <applicationDirPath>/../../data
 *   4. <applicationDirPath>/../data
 *   5. <applicationDirPath>/data
 *
 * @return Absolute path to the data directory.
 */
inline std::string dataRoot() {
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + "/data",
        appDir + "/../data",
        appDir + "/../../data",
        appDir + "/../../../data",
        appDir + "/../../../../data",
    };
    for (const auto& c : candidates) {
        const QString normalized = QDir::cleanPath(c);
        if (QDir(normalized).exists()) {
            return normalized.toStdString();
        }
    }
    return QDir::cleanPath(appDir + "/../data").toStdString();
}

/**
 * @brief Resolve a path relative to the data root.
 *
 * @param relativePath Path relative to data root, e.g. "scenarios/" or "ml/anomaly_model.json"
 * @return Absolute path string.
 */
inline std::string dataPath(const std::string& relativePath) {
    QDir root(QString::fromStdString(dataRoot()));
    return root.filePath(QString::fromStdString(relativePath)).toStdString();
}

} // namespace utils
} // namespace quantumverse
