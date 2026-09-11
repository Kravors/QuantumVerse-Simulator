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

#include "EducationalTour.h"

namespace quantumverse {

class TourManager {
public:
    static TourManager& instance() {
        static TourManager inst;
        return inst;
    }

    bool initialize(const std::string& tour_dir = "data/tours/");

    std::vector<std::string> listTours() const;
    std::vector<std::string> listTourNames() const;

    bool hasTour(const std::string& name) const;
    std::shared_ptr<EducationalTour> getTour(const std::string& name);
    std::shared_ptr<EducationalTour> getTourByFilename(const std::string& filename);

    bool loadTour(const std::string& filename);
    bool loadAllTours();

    void reload();
    void clear();

    const std::string& tourDir() const { return tour_dir_; }
    size_t count() const { return tours_.size(); }

private:
    TourManager() = default;

    bool doLoadAllTours();

    std::string tour_dir_ = "data/tours/";
    std::unordered_map<std::string, std::shared_ptr<EducationalTour>> tours_;
    std::unordered_map<std::string, std::string> name_to_filename_;
    mutable std::mutex mutex_;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_TOUR_MANAGER_H