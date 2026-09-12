/**
 * @file test_gcn_parser.cpp
 * @brief Unit tests for GCNNoticeParser
 *
 * Validates that GCN Kafka JSON payloads are correctly classified and
 * normalised into the internal ParsedGCNNotice structure.
 */

#include <QJsonObject>
#include <QJsonDocument>
#include <iostream>

#include "data/GCNNoticeParser.h"
#include "test_assert.h"

using namespace quantumverse;

static QJsonObject makeJson(const char* json)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromRawData(json, int(strlen(json))), &err);
    QV_CHECK(err.error == QJsonParseError::NoError);
    QV_CHECK(doc.isObject());
    return doc.object();
}

int main(int argc, char** argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    // --- LIGO/Virgo gravitational-wave alert ----------------------------------
    {
        const QJsonObject obj = makeJson(
            "{"
            "  \"alert_type\": \"LIGO/Virgo\","
            "  \"event_id\": \"GW250601A\","
            "  \"ra\": 123.45,"
            "  \"dec\": -30.0,"
            "  \"false_alarm_rate\": 1.2e-6,"
            "  \"snr\": 23.4,"
            "  \"m1\": 35.0,"
            "  \"m2\": 26.0,"
            "  \"confidence\": 0.98"
            "}"
        );

        const ParsedGCNNotice parsed = GCNNoticeParser::parse(obj);
        QV_CHECK(parsed.origin == AlertOrigin::LIGO);
        QV_CHECK(parsed.gw.event_id == "GW250601A");
        QV_CHECK_NEAR(parsed.gw.false_alarm_rate - 1.2e, 6, 1e-12);
        QV_CHECK_NEAR(parsed.gw.snr - 23.4, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.gw.m1 - 35.0, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.gw.m2 - 26.0, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.gw.confidence - 0.98, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.gw.ra - 123.45, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.gw.dec - (-30.0), 0.0, 1e-9);
    }

    // --- IceCube neutrino alert -------------------------------------------------
    {
        const QJsonObject obj = makeJson(
            "{"
            "  \"alert_type\": \"IceCube\","
            "  \"event_id\": \"IC250601A\","
            "  \"ra\": 45.0,"
            "  \"dec\": 12.0,"
            "  \"energy_tev\": 145.2,"
            "  \"false_alarm_rate\": 3.4e-4,"
            "  \"confidence\": 0.91"
            "}"
        );

        const ParsedGCNNotice parsed = GCNNoticeParser::parse(obj);
        QV_CHECK(parsed.origin == AlertOrigin::IceCube);
        QV_CHECK(parsed.neutrino.event_id == "IC250601A");
        QV_CHECK_NEAR(parsed.neutrino.energy_tev - 145.2, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.neutrino.false_alarm_rate - 3.4e, 4, 1e-12);
        QV_CHECK_NEAR(parsed.neutrino.ra - 45.0, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.neutrino.dec - 12.0, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.neutrino.confidence - 0.91, 0.0, 1e-9);
    }

    // --- TESS TOI alert ---------------------------------------------------------
    {
        const QJsonObject obj = makeJson(
            "{"
            "  \"alert_type\": \"TESS\","
            "  \"toi_id\": \"TOI-1234.01\","
            "  \"ra\": 180.0,"
            "  \"dec\": -45.0,"
            "  \"period_days\": 3.141,"
            "  \"depth_ppm\": 500.0,"
            "  \"duration_hours\": 2.5,"
            "  \"confidence\": 0.95"
            "}"
        );

        const ParsedGCNNotice parsed = GCNNoticeParser::parse(obj);
        QV_CHECK(parsed.origin == AlertOrigin::TESS);
        QV_CHECK(parsed.tess.toi_id == "TOI-1234.01");
        QV_CHECK_NEAR(parsed.tess.period_days - 3.141, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.tess.depth_ppm - 500.0, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.tess.duration_hours - 2.5, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.tess.confidence - 0.95, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.tess.ra - 180.0, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.tess.dec - (-45.0), 0.0, 1e-9);
    }

    // --- Unknown alert type -----------------------------------------------------
    {
        const QJsonObject obj = makeJson(
            "{"
            "  \"alert_type\": \"Mysterious/ALIEN\","
            "  \"event_id\": \"X250601\""
            "}"
        );

        const ParsedGCNNotice parsed = GCNNoticeParser::parse(obj);
        QV_CHECK(parsed.origin == AlertOrigin::Unknown);
        QV_CHECK(parsed.raw_type == "Mysterious/ALIEN");
    }

    // --- Fermi GBM gamma-ray burst alert -----------------------------------------
    {
        const QJsonObject obj = makeJson(
            "{"
            "  \"alert_type\": \"Fermi/GBM\","
            "  \"trigger_id\": \"bn240512001\","
            "  \"ra\": 45.6,"
            "  \"dec\": -23.4,"
            "  \"duration\": 2.5,"
            "  \"peak_flux\": 1.2e-7,"
            "  \"error_radius\": 2.0,"
            "  \"false_alarm_rate\": 0.001,"
            "  \"confidence\": 0.95"
            "}"
        );

        const ParsedGCNNotice parsed = GCNNoticeParser::parse(obj);
        QV_CHECK(parsed.origin == AlertOrigin::FermiGBM);
        QV_CHECK(parsed.fermi_gbm.trigger_id == "bn240512001");
        QV_CHECK_NEAR(parsed.fermi_gbm.ra - 45.6, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.fermi_gbm.dec - (-23.4), 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.fermi_gbm.duration - 2.5, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.fermi_gbm.peak_flux - 1.2e, 7, 1e-14);
        QV_CHECK_NEAR(parsed.fermi_gbm.error_radius - 2.0, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.fermi_gbm.false_alarm_rate - 0.001, 0.0, 1e-12);
        QV_CHECK_NEAR(parsed.fermi_gbm.confidence - 0.95, 0.0, 1e-9);
    }

    // --- Swift BAT X-ray transient alert -----------------------------------------
    {
        const QJsonObject obj = makeJson(
            "{"
            "  \"alert_type\": \"Swift/BAT\","
            "  \"trigger_id\": \"swift_bat_240512A\","
            "  \"ra\": 123.45,"
            "  \"dec\": -45.67,"
            "  \"duration\": 1.8,"
            "  \"bat_rate\": 4500.0,"
            "  \"xrt_flux\": 3.2e-8,"
            "  \"error_radius\": 1.5,"
            "  \"false_alarm_rate\": 0.0005,"
            "  \"confidence\": 0.98"
            "}"
        );

        const ParsedGCNNotice parsed = GCNNoticeParser::parse(obj);
        QV_CHECK(parsed.origin == AlertOrigin::Swift);
        QV_CHECK(parsed.swift_bat.trigger_id == "swift_bat_240512A");
        QV_CHECK_NEAR(parsed.swift_bat.ra - 123.45, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.swift_bat.dec - (-45.67), 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.swift_bat.duration - 1.8, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.swift_bat.bat_rate - 4500.0, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.swift_bat.xrt_flux - 3.2e, 8, 1e-14);
        QV_CHECK_NEAR(parsed.swift_bat.error_radius - 1.5, 0.0, 1e-9);
        QV_CHECK_NEAR(parsed.swift_bat.false_alarm_rate - 0.0005, 0.0, 1e-12);
        QV_CHECK_NEAR(parsed.swift_bat.confidence - 0.98, 0.0, 1e-9);
    }

    // --- Missing fields default to zero -----------------------------------------
    {
        const QJsonObject obj = makeJson(
            "{"
            "  \"alert_type\": \"LIGO/Virgo\""
            "}"
        );

        const ParsedGCNNotice parsed = GCNNoticeParser::parse(obj);
        QV_CHECK(parsed.origin == AlertOrigin::LIGO);
        QV_CHECK(parsed.gw.event_id.empty());
        QV_CHECK_NEAR(parsed.gw.false_alarm_rate, 0.0, 1e-12);
    }

    // --- supportedTypes contains expected entries --------------------------------
    {
        const QStringList types = GCNNoticeParser::supportedTypes();
        QV_CHECK(types.contains("LIGO/Virgo"));
        QV_CHECK(types.contains("IceCube"));
        QV_CHECK(types.contains("TESS"));
        QV_CHECK(types.contains("Fermi/GBM"));
        QV_CHECK(types.contains("Swift"));
    }

    std::cout << "All GCNParserTest checks passed." << std::endl;
    return 0;
}
