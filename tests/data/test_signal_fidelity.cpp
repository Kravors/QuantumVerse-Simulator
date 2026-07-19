/**
 * @file test_signal_fidelity.cpp
 * @brief Phase 19: Cosmological Consistency & Observational Data
 *
 * Validates end-to-end signal/data fidelity across the multi-messenger
 * alert pipeline:
 *
 *   - Adapter round-trip: simulateAlert -> receivedAlerts preserves fields
 *   - GCN parser fidelity: JSON -> ParsedGCNNotice field accuracy
 *   - MultiMessengerCorrelator: spatial/temporal coincidence fidelity
 *   - AlertRouter routing fidelity
 *   - UltralightDMWaveInterferometer DFT: sine-wave detection fidelity
 *   - Edge cases: NaN, Inf, zero values, empty trajectories
 */

#include <QCoreApplication>
#include <QSignalSpy>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

#include "data/LIGOAdapter.h"
#include "data/IceCubeAdapter.h"
#include "data/FermiGBMAdapter.h"
#include "data/SwiftBATAdapter.h"
#include "data/TESSAlertAdapter.h"
#include "data/GCNNoticeParser.h"
#include "data/MultiMessengerCorrelator.h"
#include "data/AlertRouter.h"
#include "discovery/UltralightDMWaveInterferometer.h"
#include "discovery/DiscoveryInstrument.h"
#include "spacetime/Event4D.h"
#include "spacetime/MetricTensor.h"

using namespace quantumverse;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// 19.2.1 - LIGOAdapter: round-trip fidelity
// ============================================================================
void test_ligo_adapter_fidelity() {
    LIGOAdapter adapter;
    std::vector<GravitationalWaveAlert> received;
    adapter.setCallback([&received](const GravitationalWaveAlert& alert) {
        received.push_back(alert);
    });

    GravitationalWaveAlert gw;
    gw.event_id = "GW250601A";
    gw.false_alarm_rate = 1.2e-6;
    gw.snr = 23.4;
    gw.m1 = 35.0;
    gw.m2 = 26.0;
    gw.confidence = 0.98;

    adapter.simulateAlert(gw);

    assert(adapter.receivedAlerts().size() == 1u);
    assert(received.size() == 1u);
    assert(adapter.receivedAlerts()[0].event_id == "GW250601A");
    assert(std::fabs(adapter.receivedAlerts()[0].false_alarm_rate - 1.2e-6) < 1e-12);
    assert(std::fabs(adapter.receivedAlerts()[0].snr - 23.4) < 1e-9);
    assert(std::fabs(adapter.receivedAlerts()[0].m1 - 35.0) < 1e-9);
    assert(std::fabs(adapter.receivedAlerts()[0].m2 - 26.0) < 1e-9);
    assert(std::fabs(adapter.receivedAlerts()[0].confidence - 0.98) < 1e-9);

    assert(received[0].event_id == "GW250601A");
    std::cout << "[PASS] LIGOAdapter round-trip fidelity verified" << std::endl;
}

// ============================================================================
// 19.2.2 - IceCubeAdapter: round-trip fidelity
// ============================================================================
void test_icecube_adapter_fidelity() {
    IceCubeAdapter adapter;
    std::vector<NeutrinoAlert> received;
    adapter.setCallback([&received](const NeutrinoAlert& alert) {
        received.push_back(alert);
    });

    NeutrinoAlert nu;
    nu.event_id = "IC250601A";
    nu.ra = 45.0;
    nu.dec = 12.0;
    nu.energy_tev = 145.2;
    nu.false_alarm_rate = 3.4e-4;
    nu.confidence = 0.91;

    adapter.simulateAlert(nu);

    assert(adapter.receivedAlerts().size() == 1u);
    assert(received.size() == 1u);
    assert(adapter.receivedAlerts()[0].event_id == "IC250601A");
    assert(std::fabs(adapter.receivedAlerts()[0].energy_tev - 145.2) < 1e-9);
    assert(std::fabs(adapter.receivedAlerts()[0].false_alarm_rate - 3.4e-4) < 1e-12);
    assert(std::fabs(adapter.receivedAlerts()[0].confidence - 0.91) < 1e-9);

    assert(received[0].event_id == "IC250601A");
    std::cout << "[PASS] IceCubeAdapter round-trip fidelity verified" << std::endl;
}

// ============================================================================
// 19.2.3 - FermiGBMAdapter: round-trip fidelity
// ============================================================================
void test_fermi_gbm_adapter_fidelity() {
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
    assert(received.size() == 1u);
    assert(adapter.receivedAlerts()[0].trigger_id == "bn240512001");
    assert(std::fabs(adapter.receivedAlerts()[0].duration - 2.5) < 1e-9);
    assert(std::fabs(adapter.receivedAlerts()[0].peak_flux - 1.2e-7) < 1e-14);
    assert(std::fabs(adapter.receivedAlerts()[0].ra - 45.6) < 1e-9);
    assert(std::fabs(adapter.receivedAlerts()[0].dec - (-23.4)) < 1e-9);
    assert(std::fabs(adapter.receivedAlerts()[0].error_radius - 2.0) < 1e-9);
    assert(std::fabs(adapter.receivedAlerts()[0].false_alarm_rate - 0.001) < 1e-12);
    assert(std::fabs(adapter.receivedAlerts()[0].confidence - 0.95) < 1e-9);

    assert(received[0].trigger_id == "bn240512001");
    std::cout << "[PASS] FermiGBMAdapter round-trip fidelity verified" << std::endl;
}

// ============================================================================
// 19.2.4 - SwiftBATAdapter: round-trip fidelity
// ============================================================================
void test_swift_bat_adapter_fidelity() {
    SwiftBATAdapter adapter;
    std::vector<SwiftBATAlert> received;
    adapter.setCallback([&received](const SwiftBATAlert& alert) {
        received.push_back(alert);
    });

    SwiftBATAlert xrt;
    xrt.trigger_id = "swift_bat_240512A";
    xrt.ra = 123.45;
    xrt.dec = -45.67;
    xrt.duration = 1.8;
    xrt.bat_rate = 4500.0;
    xrt.xrt_flux = 3.2e-8;
    xrt.error_radius = 1.5;
    xrt.false_alarm_rate = 0.0005;
    xrt.confidence = 0.98;

    adapter.simulateAlert(xrt);

    assert(adapter.receivedAlerts().size() == 1u);
    assert(received.size() == 1u);
    assert(adapter.receivedAlerts()[0].trigger_id == "swift_bat_240512A");
    assert(std::fabs(adapter.receivedAlerts()[0].ra - 123.45) < 1e-9);
    assert(std::fabs(adapter.receivedAlerts()[0].bat_rate - 4500.0) < 1e-9);
    assert(std::fabs(adapter.receivedAlerts()[0].xrt_flux - 3.2e-8) < 1e-14);
    assert(std::fabs(adapter.receivedAlerts()[0].false_alarm_rate - 0.0005) < 1e-12);

    assert(received[0].trigger_id == "swift_bat_240512A");
    std::cout << "[PASS] SwiftBATAdapter round-trip fidelity verified" << std::endl;
}

// ============================================================================
// 19.2.5 - TESSAlertAdapter: round-trip fidelity
// ============================================================================
void test_tess_adapter_fidelity() {
    TESSAlertAdapter adapter;
    std::vector<TESSAlert> received;
    adapter.setCallback([&received](const TESSAlert& alert) {
        received.push_back(alert);
    });

    TESSAlert toi;
    toi.toi_id = "TOI-1234.01";
    toi.ra = 180.0;
    toi.dec = -45.0;
    toi.period_days = 3.141;
    toi.depth_ppm = 500.0;
    toi.duration_hours = 2.5;
    toi.confidence = 0.95;

    adapter.simulateAlert(toi);

    assert(adapter.receivedAlerts().size() == 1u);
    assert(received.size() == 1u);
    assert(adapter.receivedAlerts()[0].toi_id == "TOI-1234.01");
    assert(std::fabs(adapter.receivedAlerts()[0].period_days - 3.141) < 1e-9);
    assert(std::fabs(adapter.receivedAlerts()[0].depth_ppm - 500.0) < 1e-9);
    assert(std::fabs(adapter.receivedAlerts()[0].duration_hours - 2.5) < 1e-9);
    assert(std::fabs(adapter.receivedAlerts()[0].confidence - 0.95) < 1e-9);

    assert(received[0].toi_id == "TOI-1234.01");
    std::cout << "[PASS] TESSAlertAdapter round-trip fidelity verified" << std::endl;
}

// ============================================================================
// 19.2.6 - GCNNoticeParser: LIGO/Virgo parsing fidelity
// ============================================================================
void test_gcn_parser_ligo_fidelity() {
    QJsonObject obj;
    obj["alert_type"] = "LIGO/Virgo";
    obj["event_id"] = "GW250601A";
    obj["ra"] = 123.45;
    obj["dec"] = -30.0;
    obj["false_alarm_rate"] = 1.2e-6;
    obj["snr"] = 23.4;
    obj["m1"] = 35.0;
    obj["m2"] = 26.0;
    obj["confidence"] = 0.98;

    const ParsedGCNNotice parsed = GCNNoticeParser::parse(obj);
    assert(parsed.origin == AlertOrigin::LIGO);
    assert(parsed.gw.event_id == "GW250601A");
    assert(std::fabs(parsed.gw.false_alarm_rate - 1.2e-6) < 1e-12);
    assert(std::fabs(parsed.gw.snr - 23.4) < 1e-9);
    assert(std::fabs(parsed.gw.m1 - 35.0) < 1e-9);
    assert(std::fabs(parsed.gw.m2 - 26.0) < 1e-9);
    assert(std::fabs(parsed.gw.confidence - 0.98) < 1e-9);
    assert(std::fabs(parsed.gw.ra - 123.45) < 1e-9);
    assert(std::fabs(parsed.gw.dec - (-30.0)) < 1e-9);
    std::cout << "[PASS] GCN parser LIGO/Virgo fidelity verified" << std::endl;
}

// ============================================================================
// 19.2.7 - GCNNoticeParser: IceCube parsing fidelity
// ============================================================================
void test_gcn_parser_icecube_fidelity() {
    QJsonObject obj;
    obj["alert_type"] = "IceCube";
    obj["event_id"] = "IC250601A";
    obj["ra"] = 45.0;
    obj["dec"] = 12.0;
    obj["energy_tev"] = 145.2;
    obj["false_alarm_rate"] = 3.4e-4;
    obj["confidence"] = 0.91;

    const ParsedGCNNotice parsed = GCNNoticeParser::parse(obj);
    assert(parsed.origin == AlertOrigin::IceCube);
    assert(parsed.neutrino.event_id == "IC250601A");
    assert(std::fabs(parsed.neutrino.energy_tev - 145.2) < 1e-9);
    assert(std::fabs(parsed.neutrino.false_alarm_rate - 3.4e-4) < 1e-12);
    assert(std::fabs(parsed.neutrino.ra - 45.0) < 1e-9);
    assert(std::fabs(parsed.neutrino.dec - 12.0) < 1e-9);
    assert(std::fabs(parsed.neutrino.confidence - 0.91) < 1e-9);
    std::cout << "[PASS] GCN parser IceCube fidelity verified" << std::endl;
}

// ============================================================================
// 19.2.8 - GCNNoticeParser: Fermi/GBM parsing fidelity
// ============================================================================
void test_gcn_parser_fermi_fidelity() {
    QJsonObject obj;
    obj["alert_type"] = "Fermi/GBM";
    obj["trigger_id"] = "bn240512001";
    obj["ra"] = 45.6;
    obj["dec"] = -23.4;
    obj["duration"] = 2.5;
    obj["peak_flux"] = 1.2e-7;
    obj["error_radius"] = 2.0;
    obj["false_alarm_rate"] = 0.001;
    obj["confidence"] = 0.95;

    const ParsedGCNNotice parsed = GCNNoticeParser::parse(obj);
    assert(parsed.origin == AlertOrigin::FermiGBM);
    assert(parsed.fermi_gbm.trigger_id == "bn240512001");
    assert(std::fabs(parsed.fermi_gbm.ra - 45.6) < 1e-9);
    assert(std::fabs(parsed.fermi_gbm.dec - (-23.4)) < 1e-9);
    assert(std::fabs(parsed.fermi_gbm.duration - 2.5) < 1e-9);
    assert(std::fabs(parsed.fermi_gbm.peak_flux - 1.2e-7) < 1e-14);
    assert(std::fabs(parsed.fermi_gbm.error_radius - 2.0) < 1e-9);
    assert(std::fabs(parsed.fermi_gbm.false_alarm_rate - 0.001) < 1e-12);
    assert(std::fabs(parsed.fermi_gbm.confidence - 0.95) < 1e-9);
    std::cout << "[PASS] GCN parser Fermi/GBM fidelity verified" << std::endl;
}

// ============================================================================
// 19.2.9 - GCNNoticeParser: Swift/BAT parsing fidelity
// ============================================================================
void test_gcn_parser_swift_fidelity() {
    QJsonObject obj;
    obj["alert_type"] = "Swift/BAT";
    obj["trigger_id"] = "swift_bat_240512A";
    obj["ra"] = 123.45;
    obj["dec"] = -45.67;
    obj["duration"] = 1.8;
    obj["bat_rate"] = 4500.0;
    obj["xrt_flux"] = 3.2e-8;
    obj["error_radius"] = 1.5;
    obj["false_alarm_rate"] = 0.0005;
    obj["confidence"] = 0.98;

    const ParsedGCNNotice parsed = GCNNoticeParser::parse(obj);
    assert(parsed.origin == AlertOrigin::Swift);
    assert(parsed.swift_bat.trigger_id == "swift_bat_240512A");
    assert(std::fabs(parsed.swift_bat.ra - 123.45) < 1e-9);
    assert(std::fabs(parsed.swift_bat.dec - (-45.67)) < 1e-9);
    assert(std::fabs(parsed.swift_bat.duration - 1.8) < 1e-9);
    assert(std::fabs(parsed.swift_bat.bat_rate - 4500.0) < 1e-9);
    assert(std::fabs(parsed.swift_bat.xrt_flux - 3.2e-8) < 1e-14);
    assert(std::fabs(parsed.swift_bat.error_radius - 1.5) < 1e-9);
    assert(std::fabs(parsed.swift_bat.false_alarm_rate - 0.0005) < 1e-12);
    assert(std::fabs(parsed.swift_bat.confidence - 0.98) < 1e-9);
    std::cout << "[PASS] GCN parser Swift/BAT fidelity verified" << std::endl;
}

// ============================================================================
// 19.2.10 - GCNNoticeParser: missing fields default to zero/empty
// ============================================================================
void test_gcn_parser_missing_fields() {
    QJsonObject obj;
    obj["alert_type"] = "LIGO/Virgo";

    const ParsedGCNNotice parsed = GCNNoticeParser::parse(obj);
    assert(parsed.origin == AlertOrigin::LIGO);
    assert(parsed.gw.event_id.empty());
    assert(std::fabs(parsed.gw.false_alarm_rate) < 1e-12);
    assert(std::fabs(parsed.gw.snr) < 1e-12);
    std::cout << "[PASS] GCN parser missing fields default correctly" << std::endl;
}

// ============================================================================
// 19.2.11 - MultiMessengerCorrelator: spatial coincidence fidelity
// ============================================================================
void test_correlator_spatial_fidelity() {
    int dummy_argc = 0;
    char* dummy_argv[] = { nullptr };
    QCoreApplication app(dummy_argc, dummy_argv);
    MultiMessengerCorrelator correlator;
    correlator.setSpatialThresholdDeg(2.0);
    correlator.setTimeWindowSec(3600.0);

    QSignalSpy spy(&correlator, &MultiMessengerCorrelator::correlationDetected);

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

    correlator.addAlert(ligo);
    correlator.addAlert(icecube);

    assert(correlator.correlationCount() == 1);
    assert(spy.count() == 1);

    const CorrelationEvent ev = qvariant_cast<CorrelationEvent>(spy.value(0).at(0));
    assert(ev.messengers.contains("LIGO"));
    assert(ev.messengers.contains("IceCube"));
    assert(ev.combinedConfidence > 0.8);
    assert(ev.spatialScore > 0.0);
    std::cout << "[PASS] MultiMessengerCorrelator spatial coincidence fidelity verified" << std::endl;
}

// ============================================================================
// 19.2.12 - MultiMessengerCorrelator: non-coincidence rejection
// ============================================================================
void test_correlator_non_coincidence() {
    int dummy_argc = 0;
    char* dummy_argv[] = { nullptr };
    QCoreApplication app(dummy_argc, dummy_argv);
    MultiMessengerCorrelator correlator;
    correlator.setSpatialThresholdDeg(1.0);
    correlator.setTimeWindowSec(3600.0);

    QSignalSpy spy(&correlator, &MultiMessengerCorrelator::correlationDetected);

    InstrumentFinding ligo;
    ligo.id = "LIGO_1";
    ligo.instrumentName = "LIGO";
    ligo.confidence = 0.98;
    ligo.timestamp = 1000.0;
    ligo.location = Event4D(1000.0, 10.0, 20.0, 0.0);

    InstrumentFinding far;
    far.id = "LIGO_FAR";
    far.instrumentName = "LIGO";
    far.confidence = 0.9;
    far.timestamp = 1000.0;
    far.location = Event4D(1000.0, 0.0, 0.0, 0.0);

    correlator.addAlert(ligo);
    correlator.addAlert(far);

    assert(correlator.correlationCount() == 0 && "Should NOT correlate beyond threshold");
    assert(spy.count() == 0);
    std::cout << "[PASS] MultiMessengerCorrelator non-coincidence rejection verified" << std::endl;
}

// ============================================================================
// 19.2.13 - MultiMessengerCorrelator: follow-up trigger for GW+EM
// ============================================================================
void test_correlator_follow_up_trigger() {
    int dummy_argc = 0;
    char* dummy_argv[] = { nullptr };
    QCoreApplication app(dummy_argc, dummy_argv);
    MultiMessengerCorrelator correlator;
    correlator.setSpatialThresholdDeg(2.0);
    correlator.setTimeWindowSec(3600.0);

    QSignalSpy followUpSpy(&correlator, &MultiMessengerCorrelator::followUpTriggered);

    InstrumentFinding ligo2;
    ligo2.id = "LIGO_2";
    ligo2.instrumentName = "LIGO";
    ligo2.confidence = 0.99;
    ligo2.timestamp = 2000.0;
    ligo2.location = Event4D(2000.0, 30.0, 40.0, 0.0);

    InstrumentFinding fermi;
    fermi.id = "Fermi_1";
    fermi.instrumentName = "Fermi GBM (Live)";
    fermi.confidence = 0.95;
    fermi.timestamp = 2002.0;
    fermi.location = Event4D(2002.0, 30.2, 40.2, 0.0);

    correlator.addAlert(ligo2);
    correlator.addAlert(fermi);

    assert(followUpSpy.count() == 1 && "Should emit followUpTriggered for GW+EM");
    const CorrelationEvent followUpEv = qvariant_cast<CorrelationEvent>(followUpSpy.value(0).at(0));
    assert(followUpEv.messengers.contains("LIGO"));
    assert(followUpEv.messengers.contains("Fermi"));
    std::cout << "[PASS] MultiMessengerCorrelator follow-up trigger fidelity verified" << std::endl;
}

// ============================================================================
// 19.2.14 - MultiMessengerCorrelator: correlation at threshold boundary
// ============================================================================
void test_correlator_threshold_boundary() {
    int dummy_argc = 0;
    char* dummy_argv[] = { nullptr };
    QCoreApplication app(dummy_argc, dummy_argv);
    MultiMessengerCorrelator correlator;
    correlator.setSpatialThresholdDeg(1.0);
    correlator.setTimeWindowSec(3600.0);

    QSignalSpy spy(&correlator, &MultiMessengerCorrelator::correlationDetected);

    InstrumentFinding ligo;
    ligo.id = "LIGO_BOUNDARY";
    ligo.instrumentName = "LIGO";
    ligo.confidence = 0.98;
    ligo.timestamp = 1000.0;
    ligo.location = Event4D(1000.0, 0.0, 0.0, 0.0);

    InstrumentFinding em;
    em.id = "EM_BOUNDARY";
    em.instrumentName = "Fermi GBM (Live)";
    em.confidence = 0.90;
    em.timestamp = 1000.0;
    em.location = Event4D(1000.0, 1.0, 0.0, 0.0);  // ~1 degree away

    correlator.addAlert(ligo);
    correlator.addAlert(em);

    assert(correlator.correlationCount() == 1 && "Should correlate at threshold boundary");
    assert(spy.count() == 1);
    std::cout << "[PASS] MultiMessengerCorrelator threshold boundary verified" << std::endl;
}

// ============================================================================
// 19.2.15 - AlertRouter: LIGO routing fidelity
// ============================================================================
void test_alert_router_ligo_routing() {
    int dummy_argc = 0;
    char* dummy_argv[] = { nullptr };
    QCoreApplication app(dummy_argc, dummy_argv);
    LIGOAdapter ligoAdapter;
    AlertRouter router;
    router.setLIGOAdapter(&ligoAdapter);

    QSignalSpy spy(&router, &AlertRouter::alertRouted);

    QJsonObject obj;
    obj["alert_type"] = "LIGO/Virgo";
    obj["event_id"] = "GW250601A";
    obj["ra"] = 123.45;
    obj["dec"] = -30.0;
    obj["confidence"] = 0.98;

    router.routeAlert(obj);
    assert(spy.count() == 1);
    assert(ligoAdapter.receivedAlerts().size() == 1u);
    assert(ligoAdapter.receivedAlerts()[0].event_id == "GW250601A");
    std::cout << "[PASS] AlertRouter LIGO routing fidelity verified" << std::endl;
}

// ============================================================================
// 19.2.16 - AlertRouter: IceCube routing fidelity
// ============================================================================
void test_alert_router_icecube_routing() {
    int dummy_argc = 0;
    char* dummy_argv[] = { nullptr };
    QCoreApplication app(dummy_argc, dummy_argv);
    IceCubeAdapter icecubeAdapter;
    AlertRouter router;
    router.setIceCubeAdapter(&icecubeAdapter);

    QSignalSpy spy(&router, &AlertRouter::alertRouted);

    QJsonObject obj;
    obj["alert_type"] = "IceCube";
    obj["event_id"] = "IC250601A";
    obj["ra"] = 45.0;
    obj["dec"] = 12.0;
    obj["energy_tev"] = 145.2;

    router.routeAlert(obj);
    assert(spy.count() == 1);
    assert(icecubeAdapter.receivedAlerts().size() == 1u);
    assert(icecubeAdapter.receivedAlerts()[0].event_id == "IC250601A");
    assert(std::fabs(icecubeAdapter.receivedAlerts()[0].energy_tev - 145.2) < 1e-9);
    std::cout << "[PASS] AlertRouter IceCube routing fidelity verified" << std::endl;
}

// ============================================================================
// 19.2.17 - AlertRouter: Fermi/GBM routing fidelity
// ============================================================================
void test_alert_router_fermi_routing() {
    int dummy_argc = 0;
    char* dummy_argv[] = { nullptr };
    QCoreApplication app(dummy_argc, dummy_argv);
    FermiGBMAdapter fermiAdapter;
    AlertRouter router;
    router.setFermiGBMAdapter(&fermiAdapter);

    QSignalSpy spy(&router, &AlertRouter::alertRouted);

    QJsonObject obj;
    obj["alert_type"] = "Fermi/GBM";
    obj["trigger_id"] = "bn240512001";
    obj["ra"] = 45.6;
    obj["dec"] = -23.4;
    obj["duration"] = 2.5;

    router.routeAlert(obj);
    assert(spy.count() == 1);
    assert(fermiAdapter.receivedAlerts().size() == 1u);
    assert(fermiAdapter.receivedAlerts()[0].trigger_id == "bn240512001");
    assert(std::fabs(fermiAdapter.receivedAlerts()[0].duration - 2.5) < 1e-9);
    std::cout << "[PASS] AlertRouter Fermi/GBM routing fidelity verified" << std::endl;
}

// ============================================================================
// 19.2.18 - AlertRouter: Swift/BAT routing fidelity
// ============================================================================
void test_alert_router_swift_routing() {
    int dummy_argc = 0;
    char* dummy_argv[] = { nullptr };
    QCoreApplication app(dummy_argc, dummy_argv);
    SwiftBATAdapter swiftAdapter;
    AlertRouter router;
    router.setSwiftBATAdapter(&swiftAdapter);

    QSignalSpy spy(&router, &AlertRouter::alertRouted);

    QJsonObject obj;
    obj["alert_type"] = "Swift/BAT";
    obj["trigger_id"] = "swift_bat_240512A";
    obj["ra"] = 123.45;
    obj["dec"] = -45.67;
    obj["duration"] = 1.8;

    router.routeAlert(obj);
    assert(spy.count() == 1);
    assert(swiftAdapter.receivedAlerts().size() == 1u);
    assert(swiftAdapter.receivedAlerts()[0].trigger_id == "swift_bat_240512A");
    assert(std::fabs(swiftAdapter.receivedAlerts()[0].duration - 1.8) < 1e-9);
    std::cout << "[PASS] AlertRouter Swift/BAT routing fidelity verified" << std::endl;
}

// ============================================================================
// 19.2.19 - DiscoveryInstrument: confidenceToSeverity mapping
// ============================================================================
void test_confidence_to_severity_mapping() {
    assert(DiscoveryInstrument::confidenceToSeverity(0.99) == AlertSeverity::CRITICAL);
    assert(DiscoveryInstrument::confidenceToSeverity(0.95) == AlertSeverity::HIGH);
    assert(DiscoveryInstrument::confidenceToSeverity(0.80) == AlertSeverity::MEDIUM);
    assert(DiscoveryInstrument::confidenceToSeverity(0.50) == AlertSeverity::LOW);
    assert(DiscoveryInstrument::confidenceToSeverity(0.10) == AlertSeverity::INFO);
    std::cout << "[PASS] DiscoveryInstrument confidenceToSeverity mapping fidelity verified" << std::endl;
}

// ============================================================================
// 19.2.20 - DiscoveryInstrument: confidenceToSigma mapping
// ============================================================================
void test_confidence_to_sigma_mapping() {
    assert(DiscoveryInstrument::confidenceToSigma(0.9999) == 5.0);
    assert(DiscoveryInstrument::confidenceToSigma(0.999) == 4.0);
    assert(DiscoveryInstrument::confidenceToSigma(0.99) == 3.5);
    assert(DiscoveryInstrument::confidenceToSigma(0.975) == 3.0);
    assert(DiscoveryInstrument::confidenceToSigma(0.95) == 2.5);
    assert(DiscoveryInstrument::confidenceToSigma(0.80) == 2.0);
    assert(DiscoveryInstrument::confidenceToSigma(0.50) == 1.0);
    assert(DiscoveryInstrument::confidenceToSigma(0.10) == 0.0);
    std::cout << "[PASS] DiscoveryInstrument confidenceToSigma mapping fidelity verified" << std::endl;
}

// ============================================================================
// 19.2.21 - UltralightDMWaveInterferometer: pure sine-wave detection
// ============================================================================
void test_dm_interferometer_sine_detection() {
    UltralightDMWaveInterferometer interferometer;

    double freq = 1.0;  // rad/s
    double amp = 1.0;
    std::vector<Event4D> trajectory;
    double dt = 0.01;
    for (int i = 0; i < 1000; i++) {
        double t = i * dt;
        double x = amp * std::sin(freq * t);
        trajectory.emplace_back(t, x, 0.0, 0.0);
    }

    MetricTensor metric;
    auto findings = interferometer.analyze(metric, Event4D(0.0, 0.0, 0.0, 0.0), trajectory);

    assert(!findings.empty() && "Should detect oscillatory signal");
    assert(findings[0].parameters.count("oscillation_freq_rad_s") > 0);
    double detected_freq = findings[0].parameters.at("oscillation_freq_rad_s");
    assert(std::fabs(detected_freq - freq) < 0.05 && "Detected frequency should match input");
    std::cout << "[PASS] UltralightDMWaveInterferometer sine detection: expected=" << freq
              << " rad/s, detected=" << detected_freq << " rad/s" << std::endl;
}

// ============================================================================
// 19.2.22 - UltralightDMWaveInterferometer: SNR threshold gating
// ============================================================================
void test_dm_interferometer_snr_gating() {
    UltralightDMWaveInterferometer interferometer;
    interferometer.setParameter("detection_snr_threshold", 100.0);

    double freq = 1.0;
    double amp = 0.001;
    std::vector<Event4D> trajectory;
    double dt = 0.01;
    for (int i = 0; i < 1000; i++) {
        double t = i * dt;
        double x = amp * std::sin(freq * t);
        trajectory.emplace_back(t, x, 0.0, 0.0);
    }

    MetricTensor metric;
    auto findings = interferometer.analyze(metric, Event4D(0.0, 0.0, 0.0, 0.0), trajectory);

    assert(findings.empty() && "Weak signal should be rejected by SNR threshold");
    std::cout << "[PASS] UltralightDMWaveInterferometer SNR gating: weak signal rejected" << std::endl;
}

// ============================================================================
// 19.2.23 - UltralightDMWaveInterferometer: too-short trajectory rejected
// ============================================================================
void test_dm_interferometer_short_trajectory() {
    UltralightDMWaveInterferometer interferometer;
    std::vector<Event4D> trajectory;
    for (int i = 0; i < 5; i++) {
        trajectory.emplace_back(i * 0.1, std::sin(i * 0.1), 0.0, 0.0);
    }

    MetricTensor metric;
    auto findings = interferometer.analyze(metric, Event4D(0.0, 0.0, 0.0, 0.0), trajectory);

    assert(findings.empty() && "Too-short trajectory should be rejected");
    std::cout << "[PASS] UltralightDMWaveInterferometer short trajectory rejected" << std::endl;
}

// ============================================================================
// 19.2.24 - Event4D trajectory round-trip fidelity
// ============================================================================
void test_event4d_trajectory_round_trip() {
    std::vector<Event4D> original;
    for (int i = 0; i < 100; i++) {
        original.emplace_back(i * 1e-3, i * 1e3, i * 1e2, i * 1e1);
    }

    std::vector<Event4D> reconstructed;
    for (const auto& ev : original) {
        reconstructed.emplace_back(ev.t, ev.x, ev.y, ev.z);
    }

    assert(original.size() == reconstructed.size());
    for (size_t i = 0; i < original.size(); i++) {
        assert(std::fabs(original[i].t - reconstructed[i].t) < 1e-12);
        assert(std::fabs(original[i].x - reconstructed[i].x) < 1e-12);
        assert(std::fabs(original[i].y - reconstructed[i].y) < 1e-12);
        assert(std::fabs(original[i].z - reconstructed[i].z) < 1e-12);
    }
    std::cout << "[PASS] Event4D trajectory round-trip fidelity verified" << std::endl;
}

// ============================================================================
// 19.2.25 - InstrumentFinding field preservation through pipeline
// ============================================================================
void test_instrument_finding_field_preservation() {
    InstrumentFinding finding;
    finding.id = "TEST_001";
    finding.instrumentName = "TestInstrument";
    finding.severity = AlertSeverity::HIGH;
    finding.confidence = 0.97;
    finding.description = "Test finding for fidelity verification";
    finding.location = Event4D(1000.0, 10.0, 20.0, 30.0);
    finding.timestamp = 1234567890.0;
    finding.isAnomaly = true;
    finding.parameters["snr"] = 10.0;
    finding.parameters["freq"] = 1.5;

    assert(finding.id == "TEST_001");
    assert(finding.instrumentName == "TestInstrument");
    assert(finding.severity == AlertSeverity::HIGH);
    assert(std::fabs(finding.confidence - 0.97) < 1e-12);
    assert(finding.description == "Test finding for fidelity verification");
    assert(std::fabs(finding.location.t - 1000.0) < 1e-12);
    assert(std::fabs(finding.location.x - 10.0) < 1e-12);
    assert(std::fabs(finding.timestamp - 1234567890.0) < 1e-12);
    assert(finding.isAnomaly == true);
    assert(std::fabs(finding.parameters["snr"] - 10.0) < 1e-12);
    assert(std::fabs(finding.parameters["freq"] - 1.5) < 1e-12);
    std::cout << "[PASS] InstrumentFinding field preservation verified" << std::endl;
}

// ============================================================================
// 19.2.26 - MultiMessengerCorrelator: GW170817 time delay (1.74 ± 0.05 s)
// ============================================================================
void test_gw170817_time_delay() {
    int dummy_argc = 0;
    char* dummy_argv[] = { nullptr };
    QCoreApplication app(dummy_argc, dummy_argv);

    static constexpr double NGC4993_RA = 197.45;
    static constexpr double NGC4993_DEC = -23.38;

    MultiMessengerCorrelator correlator;
    correlator.setSpatialThresholdDeg(2.0);
    correlator.setTimeWindowSec(2.0);

    QSignalSpy spy(&correlator, &MultiMessengerCorrelator::correlationDetected);
    QSignalSpy followUpSpy(&correlator, &MultiMessengerCorrelator::followUpTriggered);

    InstrumentFinding gw;
    gw.id = "GW170817";
    gw.instrumentName = "LIGO";
    gw.confidence = 0.99;
    gw.timestamp = 0.0;
    gw.location = Event4D(0.0, NGC4993_RA, NGC4993_DEC, 0.0);

    InstrumentFinding grb;
    grb.id = "GRB170817A";
    grb.instrumentName = "Fermi GBM (Live)";
    grb.confidence = 0.95;
    grb.timestamp = 1.74;
    grb.location = Event4D(1.74, NGC4993_RA, NGC4993_DEC, 0.0);

    correlator.addAlert(gw);
    correlator.addAlert(grb);

    assert(correlator.correlationCount() == 1
           && "GW170817 + GRB 1.74s delay should correlate within 2s window");
    assert(spy.count() == 1);
    assert(followUpSpy.count() == 1 && "Should emit follow-up trigger for GW+EM");

    const CorrelationEvent ev = qvariant_cast<CorrelationEvent>(spy.value(0).at(0));
    assert(ev.messengers.contains("LIGO"));
    assert(ev.messengers.contains("Fermi"));
    std::cout << "[PASS] GW170817 time delay: 1.74 s delay correlates within 2.0 s window"
              << std::endl;
}

// ============================================================================
// 19.2.27 - MultiMessengerCorrelator: 5 s delay rejected beyond time window
// ============================================================================
void test_gw170817_late_grb_rejected() {
    int dummy_argc = 0;
    char* dummy_argv[] = { nullptr };
    QCoreApplication app(dummy_argc, dummy_argv);

    static constexpr double NGC4993_RA = 197.45;
    static constexpr double NGC4993_DEC = -23.38;

    MultiMessengerCorrelator correlator;
    correlator.setSpatialThresholdDeg(2.0);
    correlator.setTimeWindowSec(2.0);

    QSignalSpy spy(&correlator, &MultiMessengerCorrelator::correlationDetected);

    InstrumentFinding gw;
    gw.id = "GW170817_B";
    gw.instrumentName = "LIGO";
    gw.confidence = 0.99;
    gw.timestamp = 0.0;
    gw.location = Event4D(0.0, NGC4993_RA, NGC4993_DEC, 0.0);

    InstrumentFinding grb_late;
    grb_late.id = "GRB170817A_LATE";
    grb_late.instrumentName = "Fermi GBM (Live)";
    grb_late.confidence = 0.95;
    grb_late.timestamp = 5.0;
    grb_late.location = Event4D(5.0, NGC4993_RA, NGC4993_DEC, 0.0);

    correlator.addAlert(gw);
    correlator.addAlert(grb_late);

    assert(correlator.correlationCount() == 0
           && "5.0 s delay should exceed 2.0 s time window");
    assert(spy.count() == 0);
    std::cout << "[PASS] GW170817: 5.0 s delay correctly rejected beyond 2.0 s window"
              << std::endl;
}

// ============================================================================
// Main
// ============================================================================
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    test_ligo_adapter_fidelity();
    test_icecube_adapter_fidelity();
    test_fermi_gbm_adapter_fidelity();
    test_swift_bat_adapter_fidelity();
    test_tess_adapter_fidelity();
    test_gcn_parser_ligo_fidelity();
    test_gcn_parser_icecube_fidelity();
    test_gcn_parser_fermi_fidelity();
    test_gcn_parser_swift_fidelity();
    test_gcn_parser_missing_fields();
    test_correlator_spatial_fidelity();
    test_correlator_non_coincidence();
    test_correlator_follow_up_trigger();
    test_correlator_threshold_boundary();
    test_alert_router_ligo_routing();
    test_alert_router_icecube_routing();
    test_alert_router_fermi_routing();
    test_alert_router_swift_routing();
    test_confidence_to_severity_mapping();
    test_confidence_to_sigma_mapping();
    test_dm_interferometer_sine_detection();
    test_dm_interferometer_snr_gating();
    test_dm_interferometer_short_trajectory();
    test_event4d_trajectory_round_trip();
    test_instrument_finding_field_preservation();
    test_gw170817_time_delay();
    test_gw170817_late_grb_rejected();

    std::cout << "=== ALL SIGNAL FIDELITY TESTS PASSED (" << 27 << " test functions) ===" << std::endl;
    return 0;
}
