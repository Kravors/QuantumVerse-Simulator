/**
 * @file EducationalTour.h
 * @brief Pure data model for scripted educational tours.
 *
 * A tour is a JSON document describing a sequence of steps.  Each step has
 * explanatory text, an optional camera target, and optional actions that
 * mutate the simulation state (set black hole mass, toggle lensing, load a
 * scenario, ...).  This file is data only -- it knows nothing about cameras,
 * Qt, or QML.  That is deliberate: TourController owns the runtime behaviour
 * and can be unit-tested with injected providers.
 *
 * Schema (see data/tours/black_hole_basics.json):
 *   {
 *     "id":            "black_hole_basics",
 *     "title":         "Black Hole Basics",
 *     "description":   "...",
 *     "steps": [
 *       {
 *         "title":            "The Event Horizon",
 *         "body":             "...",
 *         "duration_sec":     6.0,        // 0 = wait for user "Next"
 *         "camera":           {"azimuth": 45, "elevation": 20,
 *                              "distance": 200, "target": [0,0,0]},
 *         "actions": [
 *           {"type": "enable_lensing", "value": true},
 *           {"type": "set_bh_mass",    "value": 10.0}
 *         ]
 *       }
 *     ]
 *   }
 *
 * Camera angles are in degrees; azimuth is measured about the vertical axis,
 * elevation above/below the equatorial plane.  The controller interpolates
 * azimuth along the shortest path (350 -> 10 passes through 0, not 180).
 */

#ifndef QUANTUMVERSE_EDUCATIONAL_TOUR_H
#define QUANTUMVERSE_EDUCATIONAL_TOUR_H

#include <string>
#include <vector>
#include <array>
#include <cstdint>

#include <nlohmann/json.hpp>

namespace quantumverse {

using json = nlohmann::json;

/**
 * @brief One scripted action applied when a step becomes active.
 *
 * `value` is a JSON value so it can be a bool, a double, or a string
 * depending on `type`.  The controller dispatches on `type` and coerces
 * `value` as appropriate.
 */
struct TourAction {
    std::string type;   ///< Action name, e.g. "enable_lensing", "set_bh_mass"
    json value;         ///< Action payload (bool / number / string)
};

/**
 * @brief Camera target for a single tour step.
 *
 * All angles are in degrees.  `target` is the world-space point the camera
 * looks at; `distance` is the camera radius from that point.
 */
struct TourCamera {
    double azimuth = 45.0;      ///< Azimuth about vertical axis (degrees)
    double elevation = 20.0;    ///< Elevation above equator (degrees)
    double distance = 200.0;    ///< Distance from target (geometric units)
    std::array<double, 3> target = {0.0, 0.0, 0.0};  ///< Look-at point
};

/**
 * @brief One step of an educational tour.
 */
struct TourStep {
    std::string title;              ///< Short heading shown in the panel
    std::string body;               ///< Explanatory text (plain text or Markdown)
    double duration_sec = 6.0;      ///< Auto-advance after this many seconds;
                                    ///  0 means "wait for the user to click Next"
    TourCamera camera;              ///< Camera to move to for this step
    std::vector<TourAction> actions; ///< State mutations applied on entry
};

/**
 * @brief A complete educational tour: metadata plus an ordered step list.
 */
struct EducationalTour {
    std::string id;                 ///< Stable identifier, e.g. "black_hole_basics"
    std::string title;              ///< Human-readable title
    std::string description;        ///< Short blurb for the selection panel
    std::vector<TourStep> steps;    ///< Ordered step list (must be non-empty)

    // ----------------------------------------------------------------------
    // Validation
    // ----------------------------------------------------------------------

    /**
     * @brief Validate the tour structure.
     *
     * @param[out] error_msg Populated with the first problem found, if any.
     * @return true if the tour is usable.
     */
    bool validate(std::string* error_msg = nullptr) const;

    // ----------------------------------------------------------------------
    // JSON (de)serialisation
    // ----------------------------------------------------------------------

    bool saveToFile(const std::string& filepath) const;
    bool loadFromFile(const std::string& filepath);
};

// ============================================================================
// JSON (de)serialisation
// ============================================================================

inline void to_json(json& j, const TourAction& a) {
    j = json{{"type", a.type}, {"value", a.value}};
}

inline void from_json(const json& j, TourAction& a) {
    a.type = j.value("type", "");
    a.value = j.value("value", json(nullptr));
}

inline void to_json(json& j, const TourCamera& c) {
    j = json{
        {"azimuth", c.azimuth},
        {"elevation", c.elevation},
        {"distance", c.distance},
        {"target", {c.target[0], c.target[1], c.target[2]}}
    };
}

inline void from_json(const json& j, TourCamera& c) {
    c.azimuth   = j.value("azimuth", 45.0);
    c.elevation = j.value("elevation", 20.0);
    c.distance  = j.value("distance", 200.0);
    if (j.contains("target") && j["target"].is_array() && j["target"].size() == 3) {
        for (int i = 0; i < 3; i++) c.target[i] = j["target"][i].get<double>();
    }
}

inline void to_json(json& j, const TourStep& s) {
    j = json{
        {"title", s.title},
        {"body", s.body},
        {"duration_sec", s.duration_sec},
        {"camera", s.camera},
        {"actions", s.actions}
    };
}

inline void from_json(const json& j, TourStep& s) {
    s.title         = j.value("title", "");
    s.body          = j.value("body", "");
    s.duration_sec  = j.value("duration_sec", 6.0);
    if (j.contains("camera") && j["camera"].is_object()) {
        s.camera = j["camera"].get<TourCamera>();
    }
    if (j.contains("actions") && j["actions"].is_array()) {
        s.actions = j["actions"].get<std::vector<TourAction>>();
    }
}

inline void to_json(json& j, const EducationalTour& t) {
    j = json{
        {"id", t.id},
        {"title", t.title},
        {"description", t.description},
        {"steps", t.steps}
    };
}

inline void from_json(const json& j, EducationalTour& t) {
    t.id          = j.value("id", "");
    t.title       = j.value("title", "");
    t.description = j.value("description", "");
    if (j.contains("steps") && j["steps"].is_array()) {
        t.steps = j["steps"].get<std::vector<TourStep>>();
    }
}

} // namespace quantumverse

#endif // QUANTUMVERSE_EDUCATIONAL_TOUR_H