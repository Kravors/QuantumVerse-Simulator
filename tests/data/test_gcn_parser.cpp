/**
 * @file test_gcn_parser.cpp
 * @brief Unit tests for GCNNoticeParser
 *
 * Validates that GCN Kafka JSON payloads are correctly classified and
 * normalised into the internal ParsedGCNNotice structure.
 */

#include <QJsonObject>
#include <QJsonDocument>
#include <cassert>
#include <iostream>

#include "data/GCNNoticeParser.h"

using namespace quantumverse;

static QJsonObject makeJson(const char* json)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromRawData(json, int(strlen(json))), &err);
    assert(err.error == QJsonParseError::NoError);
    assert(doc.isObject());
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
        assert(parsed.origin == AlertOrigin::LIGO);
        assert(parsed.gw.event_id == "GW250601A");
        assert(std::fabs(parsed.gw.false_alarm_rate - 1.2e-6) < 1e-12);
        assert(std::fabs(parsed.gw.snr - 23.4) < 1e-9);
        assert(std::fabs(parsed.gw.m1 - 35.0) < 1e-9);
        assert(std::fabs(parsed.gw.m2 - 26.0) < 1e-9);
        assert(std::fabs(parsed.gw.confidence - 0.98) < 1e-9);
        assert(std::fabs(parsed.gw.ra - 123.45) < 1e-9);
        assert(std::fabs(parsed.gw.dec - (-30.0)) < 1e-9);
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
        assert(parsed.origin == AlertOrigin::IceCube);
        assert(parsed.neutrino.event_id == "IC250601A");
        assert(std::fabs(parsed.neutrino.energy_tev - 145.2) < 1e-9);
        assert(std::fabs(parsed.neutrino.false_alarm_rate - 3.4e-4) < 1e-12);
        assert(std::fabs(parsed.neutrino.ra - 45.0) < 1e-9);
        assert(std::fabs(parsed.neutrino.dec - 12.0) < 1e-9);
        assert(std::fabs(parsed.neutrino.confidence - 0.91) < 1e-9);
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
        assert(parsed.origin == AlertOrigin::TESS);
        assert(parsed.tess.toi_id == "TOI-1234.01");
        assert(std::fabs(parsed.tess.period_days - 3.141) < 1e-9);
        assert(std::fabs(parsed.tess.depth_ppm - 500.0) < 1e-9);
        assert(std::fabs(parsed.tess.duration_hours - 2.5) < 1e-9);
        assert(std::fabs(parsed.tess.confidence - 0.95) < 1e-9);
        assert(std::fabs(parsed.tess.ra - 180.0) < 1e-9);
        assert(std::fabs(parsed.tess.dec - (-45.0)) < 1e-9);
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
        assert(parsed.origin == AlertOrigin::Unknown);
        assert(parsed.raw_type == "Mysterious/ALIEN");
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
        assert(parsed.origin == AlertOrigin::FermiGBM);
        assert(parsed.fermi_gbm.trigger_id == "bn240512001");
        assert(std::fabs(parsed.fermi_gbm.ra - 45.6) < 1e-9);
        assert(std::fabs(parsed.fermi_gbm.dec - (-23.4)) < 1e-9);
        assert(std::fabs(parsed.fermi_gbm.duration - 2.5) < 1e-9);
        assert(std::fabs(parsed.fermi_gbm.peak_flux - 1.2e-7) < 1e-14);
        assert(std::fabs(parsed.fermi_gbm.error_radius - 2.0) < 1e-9);
        assert(std::fabs(parsed.fermi_gbm.false_alarm_rate - 0.001) < 1e-12);
        assert(std::fabs(parsed.fermi_gbm.confidence - 0.95) < 1e-9);
    }

    // --- Missing fields default to zero -----------------------------------------
    {
        const QJsonObject obj = makeJson(
            "{"
            "  \"alert_type\": \"LIGO/Virgo\""
            "}"
        );

        const ParsedGCNNotice parsed = GCNNoticeParser::parse(obj);
        assert(parsed.origin == AlertOrigin::LIGO);
        assert(parsed.gw.event_id.empty());
        assert(std::fabs(parsed.gw.false_alarm_rate) < 1e-12);
    }

    // --- supportedTypes contains expected entries --------------------------------
    {
        const QStringList types = GCNNoticeParser::supportedTypes();
        assert(types.contains("LIGO/Virgo"));
        assert(types.contains("IceCube"));
        assert(types.contains("TESS"));
        assert(types.contains("Fermi/GBM"));
        assert(types.contains("Swift"));
    }

    std::cout << "All GCNParserTest checks passed." << std::endl;
    return 0;
}
