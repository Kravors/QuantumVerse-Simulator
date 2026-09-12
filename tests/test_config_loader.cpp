#include <iostream>
#include "config/ConfigLoader.h"
#include "physics/PhysicsConstants.h"
#include "test_assert.h"

int main() {
    auto& config = quantumverse::ConfigLoader::instance();

    // Test loading from file
    bool loaded = config.loadFromFile("config/simulator.json");
    if (!loaded) {
        std::cerr << "Failed to load config file, using defaults" << std::endl;
        config.loadDefaults();
    }

    // Verify black hole config
    QV_CHECK(config.config().black_hole.mass_solar_masses == 10.0);
    QV_CHECK(config.config().black_hole.type == "schwarzschild");

    // Verify solar system config
    QV_CHECK(!config.config().bodies.empty());
    QV_CHECK(config.config().bodies.size() == 8);

    // Verify grid config
    QV_CHECK(config.config().grid.resolution == 30);
    QV_CHECK(config.config().grid.size == 100.0f);

    // Verify camera config
    QV_CHECK(config.config().camera.distance == 500.0);
    QV_CHECK(config.config().camera.azimuth == 45.0);
    QV_CHECK(config.config().camera.elevation == 30.0);

    // Verify physics constants
    QV_CHECK(std::abs(config.config().constants.G - 6.67430e-11) < 1e-20);
    QV_CHECK(std::abs(config.config().constants.c - 299792458.0) < 1.0);
    QV_CHECK(std::abs(config.config().constants.h - 6.62607015e-34) < 1e-40);

    // Test PhysicsConstants singleton
    quantumverse::PhysicsConstants::instance().set_G(1.0);
    QV_CHECK(quantumverse::PhysicsConstants::instance().get_G() == 1.0);
    quantumverse::PhysicsConstants::instance().set_G_log(-11.18);
    QV_CHECK(std::abs(quantumverse::PhysicsConstants::instance().get_G() - 6.6e-12) < 1e-12);
    quantumverse::PhysicsConstants::instance().reset();
    QV_CHECK(std::abs(quantumverse::PhysicsConstants::instance().get_G() - 6.67430e-11) < 1e-20);

    std::cout << "ConfigLoader tests passed!" << std::endl;
    std::cout << "  Black hole: " << config.config().black_hole.mass_solar_masses << " solar masses" << std::endl;
    std::cout << "  Bodies: " << config.config().bodies.size() << std::endl;
    std::cout << "  Grid: " << config.config().grid.resolution << "x" << config.config().grid.resolution << std::endl;

    return 0;
}
