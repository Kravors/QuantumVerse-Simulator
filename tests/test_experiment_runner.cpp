/**
 * @file test_experiment_runner.cpp
 * @brief Unit tests for ExperimentRunner model + worker-thread executor.
 */

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>
#include <QVariantMap>
#include <iostream>

#include "experiment/ExperimentRunner.h"
#include "experiment/experiments/MassSweepExperiment.h"
#include "test_assert.h"

using namespace quantumverse;

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    std::cout << "=== ExperimentRunner Test ===" << std::endl;

    ExperimentRunner runner;
    QV_CHECK(runner.count() == 0);
    QV_CHECK(!runner.running());
    QV_CHECK(runner.historyCount() == 0);

    runner.registerExperiment(std::make_unique<MassSweepExperiment>());
    QV_CHECK(runner.count() == 1);

    QVariantList params = runner.parametersFor(QStringLiteral("mass_sweep"));
    QV_CHECK(params.size() == 5);
    QVariantMap first = params[0].toMap();
    QV_CHECK(first.value(QStringLiteral("name")).toString() == QStringLiteral("mass_min"));

    QSignalSpy finishedSpy(&runner, &ExperimentRunner::experimentFinished);
    QSignalSpy runningSpy(&runner, &ExperimentRunner::runningChanged);

    QVariantMap runParams;
    runParams[QStringLiteral("mass_min")] = 0.1;
    runParams[QStringLiteral("mass_max")] = 2.0;
    runParams[QStringLiteral("steps")] = 5;
    runParams[QStringLiteral("spin")] = 0.0;
    runParams[QStringLiteral("probe_r")] = 1.0e5;
    runner.start(QStringLiteral("mass_sweep"), runParams);

    QV_CHECK(runner.running());
    QV_CHECK(runner.currentId() == QStringLiteral("mass_sweep"));
    QV_CHECK(runningSpy.count() >= 1);

    // Wait for the worker-thread run to finish (results marshalled via queued
    // invokes, which need the event loop).
    bool finished = false;
    for (int i = 0; i < 200 && !finished; ++i) {
        QTest::qWait(25); // processes pending events
        if (finishedSpy.count() >= 1) finished = true;
        QCoreApplication::processEvents();
    }
    QV_CHECK(finished);

    QV_CHECK(!runner.running());
    QV_CHECK(qAbs(runner.progress() - 1.0) < 1e-9);
    QV_CHECK(runner.historyCount() == 1);

    QVariantList args = finishedSpy.takeFirst();
    QV_CHECK(args.at(0).toString() == QStringLiteral("mass_sweep"));
    QV_CHECK(args.at(1).toBool() == true);

    QVariantMap scalars = runner.currentScalars();
    QV_CHECK(scalars.contains(QStringLiteral("max_kretschmann")));
    QV_CHECK(scalars.value(QStringLiteral("steps")).toInt() == 5);

    QVariantList curve = runner.currentCurve();
    QV_CHECK(curve.size() == 5);

    QString log = runner.currentLog();
    QV_CHECK(!log.isEmpty());
    QV_CHECK(log.contains(QStringLiteral("K=")));

    std::cout << "=== ExperimentRunner Test: ALL PASS ===" << std::endl;
    return 0;
}
