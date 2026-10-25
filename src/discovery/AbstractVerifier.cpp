/**
 * @file AbstractVerifier.cpp
 * @brief Implementation of AbstractVerifier.
 */
#include "AbstractVerifier.h"
#include <QRegularExpression>
#include <cmath>

namespace quantumverse {

bool AbstractVerifier::isStructuralNumber(double v) {
    static const double kAllowlist[] = {0.0, 1.0, 50.0, 100.0};
    for (double allowed : kAllowlist) {
        if (std::abs(v - allowed) < 1e-9) return true;
    }
    return false;
}

VerificationResult AbstractVerifier::verifyAbstract(const QString& abstract,
                                                    const ScanSummary& summary,
                                                    double relativeTolerance) {
    VerificationResult result;

    QRegularExpression re(R"((\d+\.\d+))");
    QRegularExpressionMatchIterator it = re.globalMatch(abstract);

    const double known[] = {summary.mass, summary.spin, summary.charge,
                            summary.confidence};

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        double v = match.captured(1).toDouble();

        bool matched = false;
        for (double k : known) {
            if (k != 0.0 && std::abs(v - k) / std::abs(k) < relativeTolerance) {
                matched = true;
                break;
            }
        }

        if (!matched && !isStructuralNumber(v)) {
            result.mismatches << QStringLiteral("abstract cites %1, not in scan")
                                  .arg(v);
        }
    }

    result.passed = result.mismatches.isEmpty();
    return result;
}

} // namespace quantumverse
