#pragma once

#ifndef QUANTUMVERSE_GW_RINGDOWN_EXPERIMENT_H
#define QUANTUMVERSE_GW_RINGDOWN_EXPERIMENT_H

#include "../Experiment.h"

namespace quantumverse {

/**
 * @brief Black hole ringdown via Kerr quasinormal modes (l=m=2, n=0).
 *
 * h(t) = A * exp(-t/tau) * cos(2 * pi * f_R * t)
 *
 * QNM frequencies from Berti, Cardoso, Will 2006, Table VIII
 * (linear interpolation over table points; exact Teukolsky is
 * deferred to session 6+).
 *
 * Reference values:
 *   M = 10 M_sun, a = 0.9:  f_R ≈ 1700 Hz,  tau ≈ 0.63 ms,  Q ≈ 3.4
 *
 * First instrument producing a damped-sinusoid curve shape
 * (previous instruments: peaked/monotonic/power-law curves).
 */

class GWRingdownExperiment : public Experiment {
public:
    QString id()          const override { return QStringLiteral("gw_ringdown"); }
    QString name()        const override { return QStringLiteral("GW Ringdown QNMs (l=m=2, n=0)"); }
    QString category()    const override { return QStringLiteral("Gravitational Waves"); }
    QString description() const override {
        return QStringLiteral("Berti table VIII QNM ringdown waveform: damped sinusoid "
                              "h(t) = A * exp(-t/tau) * cos(2*pi*f_R*t) with table-based "
                              "f_R and tau interpolation over spin a*.");
    }
    QVector<ExperimentParameter> parameters() const override;
    ExperimentResult run(const QVariantMap& params,
                         std::function<void(double)> progress,
                         std::function<bool()> shouldAbort) override;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_GW_RINGDOWN_EXPERIMENT_H
