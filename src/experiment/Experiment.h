#pragma once

/**
 * @file Experiment.h
 * @brief Base class for advanced experiments runnable from the UI.
 *
 * An experiment is a parameterised, cancellable procedure that produces
 * scalar results, a curve, and a log. Subclasses implement run() purely
 * in C++ (no Qt GUI, no viewport state) so it can execute on a worker
 * thread without freezing the UI.
 */

#ifndef QUANTUMVERSE_EXPERIMENT_H
#define QUANTUMVERSE_EXPERIMENT_H

#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QVector>
#include <QPointF>
#include <QStringList>
#include <functional>

namespace quantumverse {

/**
 * @brief One declared input parameter of an experiment, used to build the
 * QML parameter form dynamically.
 */
struct ExperimentParameter {
    QString name;          // parameter key, e.g. "mass_min"
    QString label;         // human-readable label, e.g. "Mass min (M\u2299)"
    QString type;          // "double" | "int" | "bool" | "enum" | "string"
    QVariant defaultValue; // default / current value
    QVariant min;          // inclusive lower bound (numeric)
    QVariant max;          // inclusive upper bound (numeric)
    QStringList options;   // enum options
    QString unit;          // "M\u2299", "arcsec", "m", ...
    QString help;          // tooltip text
};

/**
 * @brief Output of an experiment run.
 */
struct ExperimentResult {
    QVariantMap scalars;        // named scalar results, e.g. {"kretschmann": 1.2e-5}
    QVector<QPointF> curve;     // (x, y) samples for plotting
    QStringList logLines;       // human-readable progress log
    QString outputDir;          // directory where exports were written
    bool success = true;
};

/**
 * @brief Base class for all experiments.
 *
 * @param params       parameter name -> value map (from the QML form)
 * @param progress     called with 0..1 as the run advances
 * @param shouldAbort  called periodically; return true to cancel
 * @return ExperimentResult
 */
class Experiment {
public:
    virtual ~Experiment() = default;

    /** Stable machine id, e.g. "mass_sweep". */
    virtual QString id() const = 0;
    /** Human-readable name, e.g. "Black Hole Mass Sweep". */
    virtual QString name() const = 0;
    /** Category: "Sweep" | "GR Validation" | "Discovery" | "Quantum Gravity". */
    virtual QString category() const = 0;
    /** One-line description shown in the experiment list. */
    virtual QString description() const = 0;
    /** Declared input parameters, used to build the QML form. */
    virtual QVector<ExperimentParameter> parameters() const = 0;

    /**
     * Run the experiment. Must not touch QML or the viewport.
     * Long-running work should check shouldAbort() and report progress.
     */
    virtual ExperimentResult run(const QVariantMap& params,
                                 std::function<void(double)> progress,
                                 std::function<bool()> shouldAbort) = 0;
};

} // namespace quantumverse

#endif // QUANTUMVERSE_EXPERIMENT_H
