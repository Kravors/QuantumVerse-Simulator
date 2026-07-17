/**
 * @file GCNNoticeParser.h
 * @brief Parse raw GCN Kafka JSON notices into structured alert payloads
 *
 * GCN (Gamma-ray Coordinates Network) distributes time-domain astronomy
 * alerts via Kafka in a JSON envelope. This parser inspects the envelope
 * and normalises known payload shapes into a common internal structure
 * that the AlertRouter can dispatch.
 */

#ifndef QUANTUMVERSE_GCN_NOTICE_PARSER_H
#define QUANTUMVERSE_GCN_NOTICE_PARSER_H

#include <QJsonObject>
#include <QString>
#include <string>

namespace quantumverse {

/**
 * @brief Identifies the originating observatory / alert class.
 */
enum class AlertOrigin {
    LIGO,          ///< LIGO/Virgo/KAGRA gravitational-wave candidate
    IceCube,       ///< IceCube high-energy neutrino alert
    TESS,          ///< TESS Objects of Interest (exoplanet transit)
    FermiGBM,      ///< Fermi Gamma-ray Burst Monitor
    Swift,         ///< Swift BAT/XRT/UVOT alert
    Unknown        ///< Unrecognised / unsupported notice type
};

/**
 * @brief Normalised gravitational-wave alert fields.
 */
struct GWAlertFields {
    std::string event_id;
    double false_alarm_rate = 0.0;
    double snr = 0.0;
    double m1 = 0.0;
    double m2 = 0.0;
    double confidence = 0.0;
    double ra = 0.0;
    double dec = 0.0;
};

/**
 * @brief Normalised neutrino alert fields.
 */
struct NeutrinoAlertFields {
    std::string event_id;
    double energy_tev = 0.0;
    double false_alarm_rate = 0.0;
    double ra = 0.0;
    double dec = 0.0;
    double confidence = 0.0;
};

/**
 * @brief Normalised TESS transiting-exoplanet alert fields.
 */
struct TESSAlertFields {
    std::string toi_id;
    double period_days = 0.0;
    double depth_ppm = 0.0;
    double duration_hours = 0.0;
    double confidence = 0.0;
    double ra = 0.0;
    double dec = 0.0;
};

/**
 * @brief Normalised Fermi GBM gamma-ray burst alert fields.
 */
struct FermiGBMAlertFields {
    std::string trigger_id;
    double duration = 0.0;
    double peak_flux = 0.0;
    double false_alarm_rate = 0.0;
    double ra = 0.0;
    double dec = 0.0;
    double error_radius = 0.0;
    double confidence = 0.0;
};

/**
 * @brief Normalised Swift BAT gamma-ray burst / X-ray transient alert fields.
 */
struct SwiftBATAlertFields {
    std::string trigger_id;
    double bat_rate = 0.0;
    double xrt_flux = 0.0;
    double duration = 0.0;
    double false_alarm_rate = 0.0;
    double ra = 0.0;
    double dec = 0.0;
    double error_radius = 0.0;
    double confidence = 0.0;
};

/**
 * @brief Union-style parse result for any supported alert type.
 */
struct ParsedGCNNotice {
    AlertOrigin origin = AlertOrigin::Unknown;
    QString raw_type;                ///< Original "alert_type" string from GCN
    GWAlertFields gw;
    NeutrinoAlertFields neutrino;
    TESSAlertFields tess;
    FermiGBMAlertFields fermi_gbm;
    SwiftBATAlertFields swift_bat;
};

/**
 * @brief Static parser for GCN Kafka JSON notices.
 *
 * Accepts the parsed JSON object from a Kafka message and returns a
 * normalised ParsedGCNNotice.  Missing or unrecognised fields are left
 * at their default values; origin is set to Unknown when the notice
 * cannot be classified.
 */
class GCNNoticeParser
{
public:
    static ParsedGCNNotice parse(const QJsonObject& json);
    static QStringList supportedTypes();
    static AlertOrigin classify(const QJsonObject& json);
};

} // namespace quantumverse

#endif // QUANTUMVERSE_GCN_NOTICE_PARSER_H
