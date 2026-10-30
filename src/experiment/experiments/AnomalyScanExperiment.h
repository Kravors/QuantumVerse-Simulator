#pragma once

/**
 * @file AnomalyScanExperiment.h
 * @brief Curvature anomaly scan over a (r, theta) grid.
 */

#ifndef QUANTUMVERSE_ANOMALY_SCAN_EXPERIMENT_H
#define QUANTUMVERSE_ANOMALY_SCAN_EXPERIMENT_H

#include "../Experiment.h"

namespace quantumverse {

class AnomalyScanExperiment : public Experiment {
public:
    QString id() const override { return QStringLiteral("anomaly_scan"); }
    QString name() const override { return QStringLiteral("Curvature Anomaly Scan"); }
    QString category() const override { return QStringLiteral("Discovery"); }
    QString description() const override {
        return QStringLiteral("Grid scan of Kretschmann scalar; flags points above threshold (stand-in for DiscoveryEngine::detectAnomaly).");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params,
                         std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_ANOMALY_SCAN_EXPERIMENT_H
