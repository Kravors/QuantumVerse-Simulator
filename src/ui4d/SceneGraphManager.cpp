/**
 * @file SceneGraphManager.cpp
 * @brief Implementation of the SceneGraphManager for object management
 */

#include "SceneGraphManager.h"
#include "spacetime/Event4D.h"
#include "spacetime/MetricTensor.h"
#include "physics/GeodesicIntegrator.h"
#include <random>
#include <sstream>
#include <string>
#include <limits>
#include <fstream>
#include <iostream>
#include <sstream>

namespace quantumverse {

// ============================================================================
// SceneGraphManager Implementation
// ============================================================================

SceneGraphManager::SceneGraphManager(std::shared_ptr<MetricTensor> metric)
    : metric(std::move(metric)) {
    // Initialize with a default solar system if desired
    // createSolarSystem();
}

std::string SceneGraphManager::generateId(const std::string& prefix) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(1000, 9999);
    return prefix + "_" + std::to_string(dis(gen));
}

std::string SceneGraphManager::addObject(ObjectType type, const std::string& name, const Event4D& pos) {
    auto obj = std::make_shared<ObjectProperties>();
    obj->id = generateId(name.substr(0, 4));
    obj->name = name + " (" + obj->id + ")";
    obj->type = type;
    obj->position = pos;
    
    // Set default color based on type
    switch (type) {
        case ObjectType::CelestialBody:
            obj->color[0] = 0.8f; obj->color[1] = 0.8f; obj->color[2] = 0.2f;  // Yellowish
            break;
        case ObjectType::BlackHole:
            obj->color[0] = 0.2f; obj->color[1] = 0.2f; obj->color[2] = 0.2f;  // Dark
            obj->emissive[0] = 0.5f; obj->emissive[1] = 0.0f; obj->emissive[2] = 0.0f;  // Red glow
            break;
        case ObjectType::Particle:
            obj->color[0] = 0.2f; obj->color[1] = 0.8f; obj->color[2] = 0.2f;  // Green
            break;
        case ObjectType::QuantumObject:
            obj->color[0] = 0.8f; obj->color[1] = 0.2f; obj->color[2] = 0.8f;  // Magenta
            break;
        default:
            obj->color[0] = 1.0f; obj->color[1] = 1.0f; obj->color[2] = 1.0f;  // White
            break;
    }
    
    scene.objects.push_back(obj);
    scene.objectIndex[obj->id] = scene.objects.size() - 1;
    
    return obj->id;
}

void SceneGraphManager::removeObject(const std::string& id) {
    auto it = scene.objectIndex.find(id);
    if (it != scene.objectIndex.end()) {
        size_t idx = it->second;
        if (idx < scene.objects.size()) {
            scene.objects.erase(scene.objects.begin() + idx);
            scene.objectIndex.erase(it);
            // Rebuild index
            scene.objectIndex.clear();
            for (size_t i = 0; i < scene.objects.size(); ++i) {
                scene.objectIndex[scene.objects[i]->id] = i;
            }
        }
        if (scene.selectedObjectId == id) {
            scene.selectedObjectId.clear();
        }
    }
}

std::shared_ptr<ObjectProperties> SceneGraphManager::getObject(const std::string& id) {
    auto it = scene.objectIndex.find(id);
    if (it != scene.objectIndex.end() && it->second < scene.objects.size()) {
        return scene.objects[it->second];
    }
    return nullptr;
}

std::shared_ptr<const ObjectProperties> SceneGraphManager::getObject(const std::string& id) const {
    auto it = scene.objectIndex.find(id);
    if (it != scene.objectIndex.end() && it->second < scene.objects.size()) {
        return scene.objects[it->second];
    }
    return nullptr;
}

std::shared_ptr<ObjectProperties> SceneGraphManager::getSelectedObject() {
    if (scene.selectedObjectId.empty()) return nullptr;
    return getObject(scene.selectedObjectId);
}

std::shared_ptr<const ObjectProperties> SceneGraphManager::getSelectedObject() const {
    if (scene.selectedObjectId.empty()) return nullptr;
    return getObject(scene.selectedObjectId);
}

void SceneGraphManager::selectObject(const std::string& id) {
    if (scene.objectIndex.find(id) != scene.objectIndex.end()) {
        scene.selectedObjectId = id;
    }
}

void SceneGraphManager::clearSelection() {
    scene.selectedObjectId.clear();
}

void SceneGraphManager::addWorldline(const std::string& objectId, const std::vector<Event4D>& points) {
    Worldline wl;
    wl.objectId = objectId;
    wl.points = points;
    if (!points.empty()) {
        wl.affineParameterStart = 0.0;
        wl.affineParameterEnd = static_cast<double>(points.size() - 1);
    }
    scene.worldlines.push_back(wl);
}

std::vector<Event4D> SceneGraphManager::getWorldline(const std::string& objectId) const {
    for (const auto& wl : scene.worldlines) {
        if (wl.objectId == objectId) {
            return wl.points;
        }
    }
    return {};
}

std::vector<std::shared_ptr<ObjectProperties>> SceneGraphManager::getObjectsInRadius(
    const Event4D& center, double radius, bool includeWorldlines) const {
    
    std::vector<std::shared_ptr<ObjectProperties>> result;
    
    for (const auto& obj : scene.objects) {
        // Simple Euclidean distance in spatial coordinates (for now)
        // In full GR, we'd use proper distance along a spacelike geodesic
        double dx = obj->position.x - center.x;
        double dy = obj->position.y - center.y;
        double dz = obj->position.z - center.z;
        double dist = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        if (dist <= radius) {
            result.push_back(obj);
        }
    }
    
    return result;
}

void SceneGraphManager::setSimulationTime(double time) {
    scene.simulationTime = time;
    updateWorldlines();
}

void SceneGraphManager::focusCameraOn(const std::string& objectId) {
    selectObject(objectId);
    // Camera focus will be handled by the UI layer
}

void SceneGraphManager::updateWorldlines() {
    // In a full implementation, we'd interpolate worldlines to current time
    // For now, worldlines are static point clouds
}

std::vector<std::shared_ptr<ObjectProperties>> SceneGraphManager::getObjectsByType(ObjectType type) const {
    std::vector<std::shared_ptr<ObjectProperties>> result;
    for (const auto& obj : scene.objects) {
        if (obj->type == type) {
            result.push_back(obj);
        }
    }
    return result;
}

std::vector<std::shared_ptr<ObjectProperties>> SceneGraphManager::getObjectsInTimeRange(
    double tMin, double tMax) const {
    std::vector<std::shared_ptr<ObjectProperties>> result;
    for (const auto& obj : scene.objects) {
        if (obj->position.t >= tMin && obj->position.t <= tMax) {
            result.push_back(obj);
        }
    }
    return result;
}

std::shared_ptr<ObjectProperties> SceneGraphManager::getNearestObject(
    const Event4D& event, double maxDist) const {
    std::shared_ptr<ObjectProperties> nearest;
    double minDist = maxDist;

    for (const auto& obj : scene.objects) {
        double dx = obj->position.x - event.x;
        double dy = obj->position.y - event.y;
        double dz = obj->position.z - event.z;
        double dt = obj->position.t - event.t;
        double dist = std::sqrt(dx*dx + dy*dy + dz*dz + dt*dt);

        if (dist < minDist) {
            minDist = dist;
            nearest = obj;
        }
    }
    return nearest;
}

bool SceneGraphManager::loadFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "SceneGraphManager: Cannot open file: " << filepath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string typeStr, name;
        double t, x, y, z, mass, radius;

        if (!(iss >> typeStr >> name >> t >> x >> y >> z >> mass >> radius)) {
            continue;  // Skip malformed lines
        }

        ObjectType type = ObjectType::CelestialBody;
        if (typeStr == "BlackHole") type = ObjectType::BlackHole;
        else if (typeStr == "Particle") type = ObjectType::Particle;
        else if (typeStr == "QuantumObject") type = ObjectType::QuantumObject;
        else if (typeStr == "Singularity") type = ObjectType::Singularity;

        Event4D pos(t, x, y, z);
        std::string id = addObject(type, name, pos);
        auto obj = getObject(id);
        if (obj) {
            obj->mass = mass;
            obj->radius = radius;
        }
    }

    file.close();
    std::cout << "SceneGraphManager: Loaded scene from " << filepath << std::endl;
    return true;
}

bool SceneGraphManager::saveToFile(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "SceneGraphManager: Cannot create file: " << filepath << std::endl;
        return false;
    }

    for (const auto& obj : scene.objects) {
        std::string typeStr = "CelestialBody";
        switch (obj->type) {
            case ObjectType::BlackHole: typeStr = "BlackHole"; break;
            case ObjectType::Particle: typeStr = "Particle"; break;
            case ObjectType::QuantumObject: typeStr = "QuantumObject"; break;
            case ObjectType::Singularity: typeStr = "Singularity"; break;
            default: break;
        }

        file << typeStr << " "
             << obj->name << " "
             << obj->position.t << " "
             << obj->position.x << " "
             << obj->position.y << " "
             << obj->position.z << " "
             << obj->mass << " "
             << obj->radius << "\n";
    }

    file.close();
    std::cout << "SceneGraphManager: Saved scene to " << filepath << std::endl;
    return true;
}

void SceneGraphManager::importFromMetricTensor(const std::shared_ptr<MetricTensor>& metric,
                                               const std::vector<Event4D>& samplePoints,
                                               double properTimeStep) {
    if (!metric) return;

    GeodesicIntegrator integrator;
    integrator.setMetric(metric);

    for (const auto& event : samplePoints) {
        // Generate a short geodesic worldline from each sample point
        std::vector<Event4D> worldline;
        Event4D current = event;

        // Initial 4-velocity (stationary in spatial coordinates)
        std::array<double, 4> velocity = {1.0, 0.0, 0.0, 0.0};

        for (int i = 0; i < 20; ++i) {
            worldline.push_back(current);

            // Simple Euler integration along geodesic
            integrator.computeChristoffelSymbols(current);
            auto christoffel = integrator.getChristoffelSymbols();
            for (int mu = 0; mu < 4; ++mu) {
                double accel = 0.0;
                for (int alpha = 0; alpha < 4; ++alpha) {
                    for (int beta = 0; beta < 4; ++beta) {
                        accel -= christoffel[mu][alpha][beta] * velocity[alpha] * velocity[beta];
                    }
                }
                velocity[mu] += accel * properTimeStep;
                current[mu] += velocity[mu] * properTimeStep;
            }
        }

        // Create object for this worldline
        std::string name = "Geodesic_" + std::to_string(scene.objectIndex.size());
        std::string id = addObject(ObjectType::Particle, name, event);
        addWorldline(id, worldline);
    }
}

// ============================================================================
// Utility: Create standard scenarios
// ============================================================================

void SceneGraphManager::createSolarSystem() {
    // Complete solar system (positions in AU, masses in kg)
    const double AU = 1.496e11;  // meters
    
    // Sun at origin
    std::string sunId = addObject(ObjectType::CelestialBody, "Sun", Event4D(0.0, 0.0, 0.0, 0.0));
    auto sun = getObject(sunId);
    if (sun) {
        sun->mass = 1.989e30;
        sun->radius = 6.9634e8;
        sun->color[0] = 1.0f; sun->color[1] = 0.9f; sun->color[2] = 0.0f;  // Yellow-orange
        sun->emissive[0] = 1.0f; sun->emissive[1] = 0.8f; sun->emissive[2] = 0.0f;
    }
    
    // Terrestrial planets
    // Mercury (0.39 AU)
    std::string mercuryId = addObject(ObjectType::CelestialBody, "Mercury", Event4D(0.0, 0.39*AU, 0.0, 0.0));
    auto mercury = getObject(mercuryId);
    if (mercury) {
        mercury->mass = 3.301e23;
        mercury->radius = 2.4397e6;
        mercury->color[0] = 0.8f; mercury->color[1] = 0.8f; mercury->color[2] = 0.7f;  // Gray
    }
    
    // Venus (0.72 AU)
    std::string venusId = addObject(ObjectType::CelestialBody, "Venus", Event4D(0.0, 0.72*AU, 0.0, 0.0));
    auto venus = getObject(venusId);
    if (venus) {
        venus->mass = 4.867e24;
        venus->radius = 6.0518e6;
        venus->color[0] = 0.9f; venus->color[1] = 0.8f; venus->color[2] = 0.4f;  // Yellow-white
    }
    
    // Earth (1.0 AU)
    std::string earthId = addObject(ObjectType::CelestialBody, "Earth", Event4D(0.0, 1.0*AU, 0.0, 0.0));
    auto earth = getObject(earthId);
    if (earth) {
        earth->mass = 5.972e24;
        earth->radius = 6.371e6;
        earth->color[0] = 0.2f; earth->color[1] = 0.4f; earth->color[2] = 1.0f;  // Blue
    }
    
    // Mars (1.52 AU)
    std::string marsId = addObject(ObjectType::CelestialBody, "Mars", Event4D(0.0, 1.52*AU, 0.0, 0.0));
    auto mars = getObject(marsId);
    if (mars) {
        mars->mass = 6.417e23;
        mars->radius = 3.3895e6;
        mars->color[0] = 0.9f; mars->color[1] = 0.3f; mars->color[2] = 0.1f;  // Red
    }
    
    // Gas giants
    // Jupiter (5.2 AU)
    std::string jupiterId = addObject(ObjectType::CelestialBody, "Jupiter", Event4D(0.0, 5.2*AU, 0.0, 0.0));
    auto jupiter = getObject(jupiterId);
    if (jupiter) {
        jupiter->mass = 1.898e27;
        jupiter->radius = 6.9911e7;
        jupiter->color[0] = 0.9f; jupiter->color[1] = 0.6f; jupiter->color[2] = 0.4f;  // Orange-brown
    }
    
    // Saturn (9.5 AU)
    std::string saturnId = addObject(ObjectType::CelestialBody, "Saturn", Event4D(0.0, 9.5*AU, 0.0, 0.0));
    auto saturn = getObject(saturnId);
    if (saturn) {
        saturn->mass = 5.683e26;
        saturn->radius = 5.8232e7;
        saturn->color[0] = 0.95f; saturn->color[1] = 0.9f; saturn->color[2] = 0.6f;  // Pale gold
    }
    
    // Uranus (19.2 AU)
    std::string uranusId = addObject(ObjectType::CelestialBody, "Uranus", Event4D(0.0, 19.2*AU, 0.0, 0.0));
    auto uranus = getObject(uranusId);
    if (uranus) {
        uranus->mass = 8.681e25;
        uranus->radius = 2.5362e7;
        uranus->color[0] = 0.4f; uranus->color[1] = 0.8f; uranus->color[2] = 0.9f;  // Cyan
    }
    
    // Neptune (30.1 AU)
    std::string neptuneId = addObject(ObjectType::CelestialBody, "Neptune", Event4D(0.0, 30.1*AU, 0.0, 0.0));
    auto neptune = getObject(neptuneId);
    if (neptune) {
        neptune->mass = 1.024e26;
        neptune->radius = 2.4622e7;
        neptune->color[0] = 0.2f; neptune->color[1] = 0.4f; neptune->color[2] = 0.9f;  // Deep blue
    }
    
    // Major moons
    // Earth's Moon
    std::string moonId = addObject(ObjectType::CelestialBody, "Moon", Event4D(0.0, 1.0*AU + 3.844e8, 0.0, 0.0));
    auto moon = getObject(moonId);
    if (moon) {
        moon->mass = 7.342e22;
        moon->radius = 1.7374e6;
        moon->color[0] = 0.7f; moon->color[1] = 0.7f; moon->color[2] = 0.7f;  // Gray
    }
    
    // Jupiter's major moons
    std::string ioId = addObject(ObjectType::CelestialBody, "Io", Event4D(0.0, 5.2*AU + 4.217e8, 0.0, 0.0));
    auto io = getObject(ioId);
    if (io) {
        io->mass = 8.932e22;
        io->radius = 1.8216e6;
        io->color[0] = 0.95f; io->color[1] = 0.7f; io->color[2] = 0.3f;  // Yellow-orange (volcanic)
    }
    
    std::string europaId = addObject(ObjectType::CelestialBody, "Europa", Event4D(0.0, 5.2*AU + 6.710e8, 0.0, 0.0));
    auto europa = getObject(europaId);
    if (europa) {
        europa->mass = 4.799e22;
        europa->radius = 1.5608e6;
        europa->color[0] = 0.7f; europa->color[1] = 0.8f; europa->color[2] = 0.95f;  // Ice white
    }
    
    std::string ganymedeId = addObject(ObjectType::CelestialBody, "Ganymede", Event4D(0.0, 5.2*AU + 1.070e9, 0.0, 0.0));
    auto ganymede = getObject(ganymedeId);
    if (ganymede) {
        ganymede->mass = 1.480e23;
        ganymede->radius = 2.6341e6;
        ganymede->color[0] = 0.6f; ganymede->color[1] = 0.6f; ganymede->color[2] = 0.55f;  // Brown-gray
    }
    
    std::string callistoId = addObject(ObjectType::CelestialBody, "Callisto", Event4D(0.0, 5.2*AU + 1.883e9, 0.0, 0.0));
    auto callisto = getObject(callistoId);
    if (callisto) {
        callisto->mass = 1.076e23;
        callisto->radius = 2.4102e6;
        callisto->color[0] = 0.5f; callisto->color[1] = 0.5f; callisto->color[2] = 0.45f;  // Dark gray
    }
    
    // Add Sagittarius A* (Milky Way center black hole) for reference
    createSchwarzschildBlackHole(4.0e6 * 1.989e30, Event4D(0.0, 0.0, 0.0, 0.0));
}

void SceneGraphManager::createSchwarzschildBlackHole(double mass, const Event4D& pos) {
    std::string bhId = addObject(ObjectType::BlackHole, "BlackHole", pos);
    auto bh = getObject(bhId);
    if (bh) {
        bh->mass = mass;
        bh->radius = 2.0 * mass;  // In geometric units (G=c=1), r_s = 2M
        bh->color[0] = 0.0f; bh->color[1] = 0.0f; bh->color[2] = 0.0f;
        bh->emissive[0] = 0.3f; bh->emissive[1] = 0.0f; bh->emissive[2] = 0.0f;  // Red glow for accretion
    }
}

void SceneGraphManager::createKerrBlackHole(double mass, double spin, const Event4D& pos) {
    std::string bhId = addObject(ObjectType::BlackHole, "KerrBlackHole", pos);
    auto bh = getObject(bhId);
    if (bh) {
        bh->mass = mass;
        bh->spin = spin;
        bh->radius = mass + std::sqrt(mass*mass - spin*spin);  // Outer horizon radius
        bh->color[0] = 0.0f; bh->color[1] = 0.0f; bh->color[2] = 0.2f;
        bh->emissive[0] = 0.0f; bh->emissive[1] = 0.3f; bh->emissive[2] = 0.8f;  // Blue ergosphere glow
    }
}

} // namespace quantumverse
