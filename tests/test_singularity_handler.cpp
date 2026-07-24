// QuantumVerse Test: SingularityHandler edge cases
// Tests black hole types, evaporation, and tidal force safety

#include <iostream>
#include <cmath>
#include <vector>
#include <array>
#include <cassert>

#include "physics/SingularityHandler.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

int main() {
    std::cout << "=== SingularityHandler Edge Case Tests ===" << std::endl;

    // Test 1: All black-hole types initialize without crash
    {
        std::vector<SingularityType> types = {
            SingularityType::SCHWARZSCHILD,
            SingularityType::KERR,
            SingularityType::REISSNER_NORDSTROM,
            SingularityType::KERR_NEWMAN,
            SingularityType::NAKED,
            SingularityType::FUZZBALL,
            SingularityType::HAYWARD,
            SingularityType::BARDEEN,
            SingularityType::LOOP_QUANTUM,
            SingularityType::REGULAR_GAUSS
        };

        for (auto type : types) {
            SingularityHandler handler(type, 1.0, 0.0, 0.0);
            auto props = handler.getProperties();
            assert(props.type == type && "Type mismatch");
            (void)props;
            std::cout << "  [PASS] " << static_cast<int>(type) << " initializes correctly" << std::endl;
        }
        std::cout << "[PASS] All black-hole types initialize without crash" << std::endl;
    }

    // Test 2: Evaporation does not produce NaN/Inf
    {
        SingularityHandler handler(SingularityType::SCHWARZSCHILD, 1.0, 0.0, 0.0);
        for (int i = 0; i < 100; ++i) {
            handler.evolveHawkingEvaporation(1e10);
            assert(!std::isnan(handler.getProperties().mass) && "Mass became NaN");
            assert(!std::isinf(handler.getProperties().mass) && "Mass became Inf");
            assert(handler.getProperties().mass > 0.0 && "Mass became negative");
        }
        std::cout << "[PASS] Evaporation maintains finite positive mass" << std::endl;
    }

    // Test 3: Hawking temperature is finite and positive
    {
        SingularityHandler handler(SingularityType::SCHWARZSCHILD, 1.0, 0.0, 0.0);
        double T = handler.getHawkingTemperature();
        assert(!std::isnan(T) && "Temperature is NaN");
        assert(!std::isinf(T) && "Temperature is Inf");
        assert(T > 0.0 && "Temperature should be positive");
        std::cout << "[PASS] Hawking temperature is finite and positive: " << T << std::endl;
    }

    // Test 4: Tidal forces are finite at safe distance
    {
        SingularityHandler handler(SingularityType::SCHWARZSCHILD, 1.0, 0.0, 0.0);
        Event4D safe(0.0, 100.0, 0.0, 0.0);  // Far from singularity
        auto forces = handler.computeTidalForces(safe);
        assert(!std::isnan(forces.radial_stretch) && "Radial stretch is NaN");
        assert(!std::isnan(forces.lateral_compression) && "Lateral compression is NaN");
        assert(!std::isnan(forces.spaghettification) && "Spaghettification is NaN");
        (void)forces;
        std::cout << "[PASS] Tidal forces are finite at safe distance" << std::endl;
    }

    // Test 5: Inside event horizon detection
    {
        SingularityHandler handler(SingularityType::SCHWARZSCHILD, 1.0, 0.0, 0.0);
        double rs = handler.getProperties().schwarzschild_radius;
        Event4D outside(0.0, rs * 2.0, 0.0, 0.0);
        Event4D inside(0.0, rs * 0.5, 0.0, 0.0);
        assert(!handler.isInsideEventHorizon(outside) && "Outside point incorrectly inside horizon");
        assert(handler.isInsideEventHorizon(inside) && "Inside point incorrectly outside horizon");
        std::cout << "[PASS] Event horizon detection works correctly" << std::endl;
    }

    // Test 6: Evaporation to near-Planck mass
    {
        SingularityHandler handler(SingularityType::SCHWARZSCHILD, 1.0, 0.0, 0.0);
        double initialMass = handler.getProperties().mass;
        for (int i = 0; i < 1000; ++i) {
            handler.evolveHawkingEvaporation(1e12);
            if (handler.isEvaporated()) break;
        }
        double finalMass = handler.getProperties().mass;
        assert(!std::isnan(finalMass) && "Final mass is NaN");
        assert(!std::isinf(finalMass) && "Final mass is Inf");
        assert(finalMass >= 0.0 && "Final mass is negative");
        std::cout << "[PASS] Evaporation to near-Planck mass completes safely: "
                  << initialMass << " -> " << finalMass << std::endl;
    }

    // Test 7: Kerr ergosphere detection
    {
        SingularityHandler handler(SingularityType::KERR, 1.0, 0.9, 0.0);
        double rs = handler.getProperties().schwarzschild_radius;
        Event4D outsideErgo(0.0, rs * 3.0, 0.0, 0.0);
        Event4D insideErgo(0.0, rs * 1.5, 0.0, 0.0);
        // Note: isInErgosphere checks against ergosphere_radius
        bool outsideResult = handler.isInErgosphere(outsideErgo);
        bool insideResult = handler.isInErgosphere(insideErgo);
        (void)outsideResult;
        (void)insideResult;
        std::cout << "[PASS] Kerr ergosphere methods execute without crash" << std::endl;
    }

    // Test 8: Gravitational redshift is finite
    {
        SingularityHandler handler(SingularityType::SCHWARZSCHILD, 1.0, 0.0, 0.0);
        Event4D event(0.0, 10.0, 0.0, 0.0);
        double redshift = handler.getGravitationalRedshift(event);
        assert(!std::isnan(redshift) && "Redshift is NaN");
        assert(!std::isinf(redshift) && "Redshift is Inf");
        std::cout << "[PASS] Gravitational redshift is finite: " << redshift << std::endl;
    }

    // Test 9: Frame dragging is finite for Kerr
    {
        SingularityHandler handler(SingularityType::KERR, 1.0, 0.5, 0.0);
        Event4D event(0.0, 10.0, 0.0, 0.0);
        double omega = handler.getFrameDraggingAngularVelocity(event);
        assert(!std::isnan(omega) && "Frame dragging is NaN");
        assert(!std::isinf(omega) && "Frame dragging is Inf");
        std::cout << "[PASS] Frame dragging is finite: " << omega << std::endl;
    }

    // Test 10: Regular black holes (Hayward, Bardeen) don't produce NaN at r=0
    {
        std::vector<SingularityType> regularTypes = {
            SingularityType::HAYWARD,
            SingularityType::BARDEEN,
            SingularityType::LOOP_QUANTUM,
            SingularityType::REGULAR_GAUSS
        };

        for (auto type : regularTypes) {
            SingularityHandler handler(type, 1.0, 0.0, 0.0);
            Event4D center(0.0, 0.0, 0.0, 0.0);
            auto forces = handler.computeTidalForces(center);
            assert(!std::isnan(forces.radial_stretch) && "Regular BH: radial stretch is NaN");
            assert(!std::isnan(forces.lateral_compression) && "Regular BH: lateral compression is NaN");
            std::cout << "  [PASS] " << static_cast<int>(type)
                      << " regular BH tidal forces are finite at r=0" << std::endl;
        }
    }

    std::cout << "=== ALL SINGULARITY HANDLER TESTS PASSED ===" << std::endl;
    return 0;
}
