/**
 * @file TourManager.cpp
 * @brief TourManager implementation.
 */

#include "TourManager.h"

#include <filesystem>
#include <algorithm>
#include <iostream>

namespace quantumverse {

namespace fs = std::filesystem;

bool TourManager::initialize(const std::string& tour_dir) {
    return initializeTour(QString::fromStdString(tour_dir));
}

bool TourManager::initializeTour(const QString& tourDir) {
    std::lock_guard<std::mutex> lock(mutex_);
    tour_dir_ = tourDir.toStdString();

    if (!fs::exists(tour_dir_)) {
        std::cerr << "[TourManager] Tour directory not found: " << tour_dir_ << std::endl;
        std::cerr << "[TourManager] Attempting to create: " << tour_dir_ << std::endl;
        try {
            fs::create_directories(tour_dir_);
        } catch (const std::exception& e) {
            std::cerr << "[TourManager] Failed to create directory: " << e.what() << std::endl;
            return false;
        }
    }

    tours_.clear();
    name_to_filename_.clear();
    return doLoadAllTours();
}

QStringList TourManager::listTourIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    QStringList ids;
    for (const auto& pair : name_to_filename_) {
        ids << QString::fromStdString(pair.first);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

bool TourManager::hasTour(const QString& id) const {
    return hasTour(id.toStdString());
}

bool TourManager::loadTour(const QString& filename) {
    return loadTour(filename.toStdString());
}

int TourManager::tourCount() const {
    return static_cast<int>(tours_.size());
}

std::vector<std::string> TourManager::listTours() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    for (const auto& pair : name_to_filename_) {
        names.push_back(pair.first);
    }
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<std::string> TourManager::listTourNames() const {
    return listTours();
}

bool TourManager::hasTour(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return name_to_filename_.find(name) != name_to_filename_.end();
}

std::shared_ptr<EducationalTour> TourManager::getTour(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = name_to_filename_.find(name);
    if (it == name_to_filename_.end()) return nullptr;

    auto sit = tours_.find(it->second);
    if (sit != tours_.end()) return sit->second;

    return nullptr;
}

std::shared_ptr<EducationalTour> TourManager::getTourByFilename(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tours_.find(filename);
    if (it != tours_.end()) return it->second;

    std::string full_path = tour_dir_ + "/" + filename;
    auto tour = std::make_shared<EducationalTour>();
    if (!tour->loadFromFile(full_path)) {
        return nullptr;
    }

    tours_[filename] = tour;
    name_to_filename_[tour->id] = filename;
    return tour;
}

bool TourManager::loadTour(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string full_path = tour_dir_ + "/" + filename;

    auto tour = std::make_shared<EducationalTour>();
    if (!tour->loadFromFile(full_path)) {
        std::cerr << "[TourManager] Failed to load: " << filename << std::endl;
        return false;
    }

    tours_[filename] = tour;
    name_to_filename_[tour->id] = filename;

    std::cout << "[TourManager] Loaded: " << tour->id << " from " << filename << std::endl;
    return true;
}

bool TourManager::loadAllTours() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!fs::exists(tour_dir_)) {
        std::cerr << "[TourManager] Tour directory not found: " << tour_dir_ << std::endl;
        return false;
    }

    tours_.clear();
    name_to_filename_.clear();
    return doLoadAllTours();
}

bool TourManager::doLoadAllTours() {
    if (!fs::exists(tour_dir_)) {
        return false;
    }

    int loaded = 0;

    for (const auto& entry : fs::directory_iterator(tour_dir_)) {
        if (!entry.is_regular_file()) continue;
        auto ext = entry.path().extension().string();
        if (ext != ".json") continue;

        std::string filename = entry.path().filename().string();
        std::string full_path = tour_dir_ + "/" + filename;

        auto tour = std::make_shared<EducationalTour>();
        if (!tour->loadFromFile(full_path)) {
            std::cerr << "[TourManager] Skipping: " << filename << std::endl;
            continue;
        }

        tours_[filename] = tour;
        name_to_filename_[tour->id] = filename;
        loaded++;
    }

    std::cout << "[TourManager] Loaded " << loaded << " tours from " << tour_dir_ << std::endl;
    return loaded > 0;
}

bool TourManager::loadTourById(const QString& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = name_to_filename_.find(id.toStdString());
    if (it == name_to_filename_.end()) {
        std::cerr << "[TourManager] Tour id not found: " << id.toStdString() << std::endl;
        return false;
    }
    const std::string& filename = it->second;
    tours_.erase(filename);
    std::string full_path = tour_dir_ + "/" + filename;
    auto tour = std::make_shared<EducationalTour>();
    if (!tour->loadFromFile(full_path)) {
        std::cerr << "[TourManager] Failed to load by id: " << id.toStdString() << std::endl;
        return false;
    }
    tours_[filename] = tour;
    name_to_filename_[tour->id] = filename;
    return true;
}

QString TourManager::tourDirectory() const {
    return QString::fromStdString(tour_dir_);
}

void TourManager::reload() {
    std::cout << "[TourManager] Reloading all tours..." << std::endl;
    loadAllTours();
}

void TourManager::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    tours_.clear();
    name_to_filename_.clear();
}

} // namespace quantumverse