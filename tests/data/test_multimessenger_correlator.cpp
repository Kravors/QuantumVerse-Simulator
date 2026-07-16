/**
 * @file test_multimessenger_correlator.cpp
 * @brief Unit tests for MultiMessengerCorrelator
 *
 * Validates spatial/temporal coincidence detection, correlation
 * emission, and threshold sensitivity for multi-messenger alerts.
 */

#include <QCoreApplication>
#include <QSignalSpy>
#include <cassert>
#include <iostream>

#include "data/MultiMessengerCorrelator.h"
#include "discovery/DiscoveryInstrument.h"

using namespace quantumverse;

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    std::cout << "=== MultiMessengerCorrelator Test ===" << std::endl;

    MultiMessengerCorrelator correlator;
    correlator.setSpatialThresholdDeg(2.0);
    correlator.setTimeWindowSec(3600.0);

    InstrumentFinding ligo;
    ligo.id = "LIGO_1";
    ligo.instrumentName = "LIGO";
    ligo.confidence = 0.98;
    ligo.timestamp = 1000.0;
    ligo.location = Event4D(1000.0, 10.0, 20.0, 0.0);

    InstrumentFinding icecube;
    icecube.id = "IceCube_1";
    icecube.instrumentName = "IceCube";
    icecube.confidence = 0.95;
    icecube.timestamp = 1005.0;
    icecube.location = Event4D(1005.0, 10.5, 20.5, 0.0);

    QSignalSpy spy(&correlator, &MultiMessengerCorrelator::correlationDetected);

    correlator.addAlert(ligo);
    correlator.addAlert(icecube);

    assert(correlator.correlationCount() == 1 && "Should detect one correlation");
    assert(spy.count() == 1);

    const CorrelationEvent ev = qvariant_cast<CorrelationEvent>(spy.value(0).at(0));
    assert(ev.messengers.contains("LIGO"));
    assert(ev.messengers.contains("IceCube"));
    assert(ev.combinedConfidence > 0.8);
    assert(ev.spatialScore > 0.0);

    correlator.clear();
    assert(correlator.correlationCount() == 0);

    InstrumentFinding far;
    far.id = "LIGO_FAR";
    far.instrumentName = "LIGO";
    far.confidence = 0.9;
    far.timestamp = 1000.0;
    far.location = Event4D(1000.0, 0.0, 0.0, 0.0);

    correlator.addAlert(ligo);
    correlator.addAlert(far);
    assert(correlator.correlationCount() == 0 && "Should NOT detect correlation beyond threshold");

    correlator.setSpatialThresholdDeg(0.001);
    correlator.clear();
    correlator.addAlert(ligo);
    correlator.addAlert(icecube);
    assert(correlator.correlationCount() == 0 && "Should not correlate beyond tiny spatial threshold");

    std::cout << "All MultiMessengerCorrelator tests passed." << std::endl;
    return 0;
}
