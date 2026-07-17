/**
 * @file test_fermi_gbm_adapter.cpp
 * @brief Unit tests for FermiGBMAdapter
 *
 * Validates that Fermi GBM GRB alerts are correctly stored and
 * delivered through the callback interface.
 */

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "data/FermiGBMAdapter.h"

using namespace quantumverse;

int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    // --- simulateAlert stores alert and invokes callback ------------------------
    {
        FermiGBMAdapter adapter;
        std::vector<FermiGBMAlert> received;
        adapter.setCallback([&received](const FermiGBMAlert& alert) {
            received.push_back(alert);
        });

        FermiGBMAlert grb;
        grb.trigger_id = "bn240512001";
        grb.duration = 2.5;
        grb.peak_flux = 1.2e-7;
        grb.false_alarm_rate = 0.001;
        grb.ra = 45.6;
        grb.dec = -23.4;
        grb.error_radius = 2.0;
        grb.confidence = 0.95;

        adapter.simulateAlert(grb);

        assert(adapter.receivedAlerts().size() == 1u);
        assert(adapter.receivedAlerts()[0].trigger_id == "bn240512001");
        assert(std::fabs(adapter.receivedAlerts()[0].duration - 2.5) < 1e-9);
        assert(std::fabs(adapter.receivedAlerts()[0].peak_flux - 1.2e-7) < 1e-14);
        assert(std::fabs(adapter.receivedAlerts()[0].ra - 45.6) < 1e-9);
        assert(std::fabs(adapter.receivedAlerts()[0].dec - (-23.4)) < 1e-9);
        assert(std::fabs(adapter.receivedAlerts()[0].error_radius - 2.0) < 1e-9);
        assert(std::fabs(adapter.receivedAlerts()[0].false_alarm_rate - 0.001) < 1e-12);
        assert(std::fabs(adapter.receivedAlerts()[0].confidence - 0.95) < 1e-9);

        assert(received.size() == 1u);
        assert(received[0].trigger_id == "bn240512001");
    }

    // --- start() is a no-op ----------------------------------------------------
    {
        FermiGBMAdapter adapter;
        adapter.start();
        assert(adapter.receivedAlerts().empty());
    }

    // --- Multiple alerts accumulate ---------------------------------------------
    {
        FermiGBMAdapter adapter;
        int count = 0;
        adapter.setCallback([&count](const FermiGBMAlert&) { ++count; });

        FermiGBMAlert g1; g1.trigger_id = "evt1"; adapter.simulateAlert(g1);
        FermiGBMAlert g2; g2.trigger_id = "evt2"; adapter.simulateAlert(g2);

        assert(adapter.receivedAlerts().size() == 2u);
        assert(count == 2);
        assert(adapter.receivedAlerts()[0].trigger_id == "evt1");
        assert(adapter.receivedAlerts()[1].trigger_id == "evt2");
    }

    std::cout << "All FermiGBMAdapterTest checks passed." << std::endl;
    return 0;
}
