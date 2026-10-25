/**
 * @file AbstractVerifier.h
 * @brief Verifies an LLM-generated paper abstract against known scan values.
 *
 * Extracts all decimal numbers from the abstract and checks each against the
 * ScanSummary's known values (mass, spin, charge, confidence). Numbers within
 * tolerance pass; numbers not matching any known value (and not on the
 * structural allowlist) flag a mismatch. The goal is to catch the LLM
 * inventing a different mass/spin/charge — not to reject every legitimate
 * derived number.
 */
#pragma once

#include <QString>
#include <QStringList>
#include "discovery/ScanSummary.h"

namespace quantumverse {

struct VerificationResult {
    bool passed = false;
    QStringList mismatches;
};

class AbstractVerifier {
public:
    /// Verify that all numeric claims in the abstract are consistent with summary.
    static VerificationResult verifyAbstract(const QString& abstract,
                                             const ScanSummary& summary,
                                             double relativeTolerance = 0.01);

private:
    /// True for values that are legitimately structural (percentages, unity, etc.).
    static bool isStructuralNumber(double v);
};

} // namespace quantumverse
