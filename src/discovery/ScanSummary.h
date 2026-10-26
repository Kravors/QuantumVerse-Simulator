/**
 * @file ScanSummary.h
 * @brief Aggregated discovery scan result handed from the GUI to PaperGenerator.
 *
 * ScanSummary is the single source of truth for a discovery paper: physics
 * values (mass, spin, charge, kretschmann, redshift), run provenance, and
 * JSON serialization for the LLM prompt. It is a plain C++ struct — no QObject,
 * no signals — safe to copy, stack-allocate, and unit test.
 */
#pragma once

#include <QString>
#include <QJsonObject>
#include <cmath>

namespace quantumverse {

/// Solar mass in kilograms (IAU 2015 nominal value).
constexpr double M_SUN_KG = 1.98892e30;

struct ScanSummary {
    // Physics values (populated from DiscoveryResult::parameters)
    // mass is stored in solar masses (M_sun), NOT kilograms.
    double mass        = 0.0;
    double spin        = 0.0;
    double charge      = 0.0;
    double kretschmann = 0.0;
    double redshift    = 0.0;

    // Run provenance (L2 will use these; L3 tolerates zeros)
    int    candidatesExplored = 0;
    int    candidatesRejected = 0;
    double extremalityMargin  = 0.0;

    // Provenance
    QString discoveryId;
    QString discoveryType;
    double  confidence = 0.0;
    QString bhType;        // "Kerr" or "Schwarzschild" — drives narrative

    QJsonObject toJson() const {
        QJsonObject o;
        o["mass"]               = mass;
        o["spin"]               = spin;
        o["charge"]             = charge;
        o["kretschmann"]        = kretschmann;
        o["redshift"]           = redshift;
        o["candidatesExplored"] = candidatesExplored;
        o["candidatesRejected"] = candidatesRejected;
        o["extremalityMargin"]  = extremalityMargin;
        o["discoveryId"]        = discoveryId;
        o["discoveryType"]      = discoveryType;
        o["confidence"]         = confidence;
        o["bhType"]             = bhType;
        return o;
    }
};

} // namespace quantumverse
