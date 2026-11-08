#pragma once

/**
 * @file QuantumGeometryExperiment.h
 * @brief Quantum geometry: spectral dimension vs diffusion scale.
 */

#ifndef QUANTUMVERSE_QUANTUM_GEOMETRY_EXPERIMENT_H
#define QUANTUMVERSE_QUANTUM_GEOMETRY_EXPERIMENT_H

#include "../Experiment.h"

namespace quantumverse {

class QuantumGeometryExperiment : public Experiment {
   public:
    QString id() const override {
        return QStringLiteral("quantum_geometry");
    }
    QString name() const override {
        return QStringLiteral("Quantum Geometry (CDT)");
    }
    QString category() const override {
        return QStringLiteral("Quantum Gravity");
    }
    QString description() const override {
        return QStringLiteral(
            "Spectral dimension d_s(mu) vs diffusion scale mu; CDT runs 4→2 near Planck scale.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params, std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

}  // namespace quantumverse

#endif  // QUANTUMVERSE_QUANTUM_GEOMETRY_EXPERIMENT_H
