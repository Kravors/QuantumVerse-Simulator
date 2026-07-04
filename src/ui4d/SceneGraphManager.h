#pragma once

#include <memory>
#include <string>
#include <vector>
#include <array>
#include <variant>
#include <map>

#include "../spacetime/Event4D.h"
#include "../spacetime/MetricTensor.h"

namespace quantumverse {

// ============================================================================
// Scene Graph Data Structures
// ============================================================================

enum class ObjectType {
    CelestialBody,    // Star, planet, moon
    BlackHole,        // Schwarzschild, Kerr, etc.
    Particle,         // Test particle/worldline
    QuantumObject,    // Spin network node, foam region
    Singularity,      // Coordinate/curvature singularity
    Custom            // User-defined
};

struct ObjectProperties {
    std::string name;
    std::string id;
    ObjectType type;
    
    // Physical properties
    double mass = 0.0;           // kg
    double charge = 0.0;         // Coulombs
    double spin = 0.0;           // Angular momentum (J⋅s)
    double radius = 0.0;         // m (for rendering)
    
    // Position & motion
    Event4D position;            // Current 4-position
    Event4D velocity;            // 4-velocity (if timelike)
    bool isStatic = false;
    
    // Visual properties
    float color[3] = {1.0f, 1.0f, 1.0f};
    float emissive[3] = {0.0f, 0.0f, 0.0f};  // For glowing objects
    bool showOrbit = true;
    bool showLabel = true;
    
    // Metadata
    std::map<std::string, std::string> metadata;
};

struct Worldline {
    std::string objectId;
    std::vector<Event4D> points;  // Sampled worldline points
    bool isClosed = false;        // Timelike closed curve?
    double affineParameterStart = 0.0;
    double affineParameterEnd = 0.0;
};

struct SceneGraph {
    std::vector<std::shared_ptr<ObjectProperties>> objects;
    std::vector<Worldline> worldlines;
    std::map<std::string, size_t> objectIndex;  // name -> index in objects
    
    // Selection state
    std::string selectedObjectId;
    size_t hoveredObjectIndex = SIZE_MAX;
    
    // Time state
    double simulationTime = 0.0;
    double timeScale = 1.0;
    bool isPaused = false;
};

// ============================================================================
// SceneGraphManager - Central registry for all simulation objects
// ============================================================================

class SceneGraphManager {
public:
    explicit SceneGraphManager(std::shared_ptr<MetricTensor> metric);
    ~SceneGraphManager() = default;
    
    // Object management
    std::string addObject(ObjectType type, const std::string& name, const Event4D& pos);
    void removeObject(const std::string& id);
    std::shared_ptr<ObjectProperties> getObject(const std::string& id);
    std::shared_ptr<const ObjectProperties> getObject(const std::string& id) const;
    std::shared_ptr<ObjectProperties> getSelectedObject();
    std::shared_ptr<const ObjectProperties> getSelectedObject() const;
    void selectObject(const std::string& id);
    void clearSelection();
    
    // Worldline management
    void addWorldline(const std::string& objectId, const std::vector<Event4D>& points);
    std::vector<Event4D> getWorldline(const std::string& objectId) const;
    
    // Query by region
    std::vector<std::shared_ptr<ObjectProperties>> getObjectsInRadius(
        const Event4D& center, double radius, bool includeWorldlines = false
    ) const;

    // Query by type
    std::vector<std::shared_ptr<ObjectProperties>> getObjectsByType(ObjectType type) const;

    // Query by time range (objects whose worldlines intersect [tMin, tMax])
    std::vector<std::shared_ptr<ObjectProperties>> getObjectsInTimeRange(
        double tMin, double tMax
    ) const;

    // Query nearest object to a given event
    std::shared_ptr<ObjectProperties> getNearestObject(
        const Event4D& event, double maxDist = std::numeric_limits<double>::infinity()
    ) const;

    // Load/save scene from file
    virtual bool loadFromFile(const std::string& filepath);
    virtual bool saveToFile(const std::string& filepath) const;

    // Import from MetricTensor database (extract worldlines from metric geodesics)
    void importFromMetricTensor(const std::shared_ptr<MetricTensor>& metric,
                                const std::vector<Event4D>& samplePoints,
                                double properTimeStep = 0.1);
    
    // Update simulation time
    void setSimulationTime(double time);
    double getSimulationTime() const { return scene.simulationTime; }
    void setTimeScale(double scale) { scene.timeScale = scale; }
    void togglePause() { scene.isPaused = !scene.isPaused; }
    
    // Access to full scene
    const SceneGraph& getScene() const { return scene; }
    SceneGraph& getScene() { return scene; }
    
    // Utility: create standard solar system bodies
    void createSolarSystem();
    void createSchwarzschildBlackHole(double mass, const Event4D& pos);
    void createKerrBlackHole(double mass, double spin, const Event4D& pos);
    
    // Camera helpers
    void focusCameraOn(const std::string& objectId);
    Event4D getFocusPosition() const {
        if (scene.selectedObjectId.empty()) return Event4D(0,0,0,0);
        auto obj = getObject(scene.selectedObjectId);
        return obj ? obj->position : Event4D(0,0,0,0);
    }
    
private:
    std::shared_ptr<MetricTensor> metric;
    SceneGraph scene;
    
    // Helper to generate unique IDs
    std::string generateId(const std::string& prefix = "obj");
    
    // Internal: update worldlines based on current time
    void updateWorldlines();
};

} // namespace quantumverse
