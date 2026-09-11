/**
 * @file EducationalTour.cpp
 * @brief EducationalTour implementation: validation and file I/O.
 */

#include "EducationalTour.h"

#include <fstream>
#include <iostream>

namespace quantumverse {

bool EducationalTour::validate(std::string* error_msg) const {
    if (id.empty()) {
        if (error_msg) *error_msg = "Tour id is empty";
        return false;
    }
    if (title.empty()) {
        if (error_msg) *error_msg = "Tour title is empty";
        return false;
    }
    if (steps.empty()) {
        if (error_msg) *error_msg = "Tour has no steps";
        return false;
    }
    for (size_t i = 0; i < steps.size(); i++) {
        const auto& s = steps[i];
        if (s.title.empty()) {
            if (error_msg) *error_msg = "Step " + std::to_string(i) + " has an empty title";
            return false;
        }
        if (s.duration_sec < 0.0) {
            if (error_msg) *error_msg = "Step " + std::to_string(i) + " has negative duration";
            return false;
        }
        for (size_t a = 0; a < s.actions.size(); a++) {
            if (s.actions[a].type.empty()) {
                if (error_msg) *error_msg =
                    "Step " + std::to_string(i) + " action " + std::to_string(a) +
                    " has an empty type";
                return false;
            }
        }
    }
    return true;
}

bool EducationalTour::saveToFile(const std::string& filepath) const {
    std::ofstream out(filepath, std::ios::binary);
    if (!out) {
        std::cerr << "[EducationalTour] Cannot open for writing: " << filepath << std::endl;
        return false;
    }
    json j = *this;
    out << j.dump(2);
    return out.good();
}

bool EducationalTour::loadFromFile(const std::string& filepath) {
    std::ifstream in(filepath, std::ios::binary);
    if (!in) {
        std::cerr << "[EducationalTour] Cannot open for reading: " << filepath << std::endl;
        return false;
    }
    try {
        json j;
        in >> j;
        *this = j.get<EducationalTour>();
    } catch (const json::exception& e) {
        std::cerr << "[EducationalTour] JSON parse error in " << filepath << ": " << e.what()
                  << std::endl;
        return false;
    }
    std::string error;
    if (!validate(&error)) {
        std::cerr << "[EducationalTour] Validation failed for " << filepath << ": " << error
                  << std::endl;
        return false;
    }
    return true;
}

} // namespace quantumverse