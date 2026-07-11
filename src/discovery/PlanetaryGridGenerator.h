/**
 * @file PlanetaryGridGenerator.h
 * @brief Minimal planetary grid result stub
 */
#ifndef QUANTUMVERSE_PLANETARY_GRID_GENERATOR_H
#define QUANTUMVERSE_PLANETARY_GRID_GENERATOR_H

#include <vector>
#include "spacetime/Event4D.h"

namespace quantumverse {

struct PlanetaryGridResult {
    bool anomalyDetected = false;
    double fifthForceStrength = 0.0;
    std::vector<Event4D> trajectory;
};

}

#endif
