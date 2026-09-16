/**
 * @file TourManager.h
 * @brief Scans data/tours/ and provides access to educational tours.
 *
 * Mirrors ScenarioManager: singleton, lazy directory scan, name->filename
 * index, reload on demand.  Tours live in data/tours/ alongside
 * data/scenarios/ so the two systems are siblings rather than competitors.
 */

#ifndef QUANTUMVERSE_TOUR_MANAGER_H
#define QUANTUMVERSE_TOUR_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>

#include <QObject>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "EducationalTour.h"

namespace quantumverse {

class TourManager : public QObject {
    Q_OBJECT
public:
    static TourManager& instance() {
        static TourManager* mgr = nullptr;
        if (!mgr) {
            // Heap-allocate and leak intentionally — avoids static destruction
            // order issues with QObject and QML engine shutdown.
            mgr = new TourManager();
        }
        return *mgr;
    }

    Q_INVOKABLE bool initializeTour(const QString& tourDir = "data/tours/");
    Q_INVOKABLE QStringList listTourIds() const;
    Q_INVOKABLE bool hasTour(const QString& id) const;
    Q_INVOKABLE bool loadTour(const QString& filename);
    Q_INVOKABLE bool loadTourById(const QString& id);
    Q_INVOKABLE int tourCount() const;
    Q_INVOKABLE QString tourDirectory() const;
    Q_INVOKABLE QString tourFilePath(const QString& idOrFilename) const;

    bool initialize(const std::string& tour_dir = "data/tours/");
    std::vector<std::string> listTours() const;
    std::vector<std::string> listTourNames() const;

    bool hasTour(const std::string& name) const;
    bool hasTour(const char* id) const {
        return hasTour(QString::fromUtf8(id));
    }
    std::shared_ptr<EducationalTour> getTour(const std::string& name);
    std::shared_ptr<EducationalTour> getTourByFilename(const std::string& filename);

    bool loadTour(const std::string& filename);
    bool loadAllTours();

    void reload();
    void clear();

    const std::string& tourDir() const { return tour_dir_; }
    size_t count() const { return tours_.size(); }
    TourManager() : QObject(nullptr) {}


    bool doLoadAllTours();

    std::string tour_dir_ = "data/tours/";
    std::unordered_map<std::string, std::shared_ptr<EducationalTour>> tours_;
    std::unordered_map<std::string, std::string> name_to_filename_;
    mutable std::mutex mutex_;
};

} // namespace quantumverse

Q_DECLARE_METATYPE(quantumverse::TourManager*)

#endif // QUANTUMVERSE_TOUR_MANAGER_H

