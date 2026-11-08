#pragma once

/**
 * @file FrameDraggingExperiment.h
 * @brief GR validation: Lense–Thirring frame dragging (GP-B ≈ 39 mas/yr).
 */

#ifndef QUANTUMVERSE_FRAME_DRAGGING_EXPERIMENT_H
#define QUANTUMVERSE_FRAME_DRAGGING_EXPERIMENT_H

#include "../Experiment.h"

namespace quantumverse {

class FrameDraggingExperiment : public Experiment {
   public:
    QString id() const override {
        return QStringLiteral("frame_dragging");
    }
    QString name() const override {
        return QStringLiteral("Frame Dragging (Lense–Thirring)");
    }
    QString category() const override {
        return QStringLiteral("GR Validation");
    }
    QString description() const override {
        return QStringLiteral(
            "Lense–Thirring angular velocity vs radius (weak-field, leading order). Ω_LT = 2 G J / "
            "(c² r³), J = a* G M² / c. Reference: M=1 M☉, a*=0.9, r=7e6 m gives ≈ 2.23e10 mas/yr — "
            "NOT GP-B (that assumes M=Earth and a different J). The 39 mas/yr scalar below is a "
            "historical reference label only.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params, std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

}  // namespace quantumverse

#endif  // QUANTUMVERSE_FRAME_DRAGGING_EXPERIMENT_H
