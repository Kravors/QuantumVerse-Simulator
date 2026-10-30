#pragma once

/**
 * @file TheoryComparisonExperiment.h
 * @brief Compare curvature invariants across metric theories at one probe.
 */

#ifndef QUANTUMVERSE_THEORY_COMPARISON_EXPERIMENT_H
#define QUANTUMVERSE_THEORY_COMPARISON_EXPERIMENT_H

#include "../Experiment.h"

namespace quantumverse {

class TheoryComparisonExperiment : public Experiment {
public:
    QString id() const override { return QStringLiteral("theory_comparison"); }
    QString name() const override { return QStringLiteral("Theory Comparison"); }
    QString category() const override { return QStringLiteral("Discovery"); }
    QString description() const override {
        return QStringLiteral("Kretschmann scalar at a probe for Schwarzschild vs Kerr vs Minkowski.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params,
                         std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_THEORY_COMPARISON_EXPERIMENT_H
