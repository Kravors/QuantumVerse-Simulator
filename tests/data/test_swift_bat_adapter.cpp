/**
 * @file test_swift_bat_adapter.cpp
 * @brief Unit tests for SwiftBATAdapter
 *
 * Validates that Swift BAT X-ray transient alerts are correctly stored and
 * delivered through the callback interface.
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "data/SwiftBATAdapter.h"

using namespace quantumverse;

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    // --- simulateAlert stores alert and invokes callback ------------------------
    {
        SwiftBATAdapter adapter;
        std::vector<SwiftBATAlert> received;
        adapter.setCallback([&received](const SwiftBATAlert& alert) {
            received.push_back(alert);
        });

        SwiftBATAlert bat;
        bat.trigger_id = "swift_bat_240512A";
        bat.bat_rate = 4500.0;
        bat.xrt_flux = 3.2e-8;
        bat.duration = 1.8;
        bat.false_alarm_rate = 0.0005;
        bat.ra = 123.45;
        bat.dec = -45.67;
        bat.error_radius = 1.5;
        bat.confidence = 0.98;

        adapter.simulateAlert(bat);

        assert(adapter.receivedAlerts().size() == 1u);
        assert(adapter.receivedAlerts()[0].trigger_id == "swift_bat_240512A");
        assert(std::fabs(adapter.receivedAlerts()[0].bat_rate - 4500.0) < 1e-9);
        assert(std::fabs(adapter.receivedAlerts()[0].xrt_flux - 3.2e-8) < 1e-14);
        assert(std::fabs(adapter.receivedAlerts()[0].duration - 1.8) < 1e-9);
        assert(std::fabs(adapter.receivedAlerts()[0].false_alarm_rate - 0.0005) < 1e-12);
        assert(std::fabs(adapter.receivedAlerts()[0].ra - 123.45) < 1e-9);
        assert(std::fabs(adapter.receivedAlerts()[0].dec - (-45.67)) < 1e-9);
        assert(std::fabs(adapter.receivedAlerts()[0].error_radius - 1.5) < 1e-9);
        assert(std::fabs(adapter.receivedAlerts()[0].confidence - 0.98) < 1e-9);

        assert(received.size() == 1u);
        assert(received[0].trigger_id == "swift_bat_240512A");
    }

    // --- start() is a no-op ----------------------------------------------------
    {
        SwiftBATAdapter adapter;
        adapter.start();
        assert(adapter.receivedAlerts().empty());
    }

    // --- Multiple alerts accumulate ---------------------------------------------
    {
        SwiftBATAdapter adapter;
        int count = 0;
        adapter.setCallback([&count](const SwiftBATAlert&) { ++count; });

        SwiftBATAlert a1; a1.trigger_id = "evt1"; adapter.simulateAlert(a1);
        SwiftBATAlert a2; a2.trigger_id = "evt2"; adapter.simulateAlert(a2);

        assert(adapter.receivedAlerts().size() == 2u);
        assert(count == 2);
        assert(adapter.receivedAlerts()[0].trigger_id == "evt1");
        assert(adapter.receivedAlerts()[1].trigger_id == "evt2");
    }

    std::cout << "All SwiftBATAdapterTest checks passed." << std::endl;
    return 0;
}
