/**
 * @file test_experiment_runner.cpp
 * @brief Unit tests for ExperimentRunner model + worker-thread executor.
 */

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>
#include <QVariantMap>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <iostream>

#include "experiment/ExperimentRunner.h"
#include "experiment/experiments/MassSweepExperiment.h"
#include "experiment/experiments/LensingTimeDelayExperiment.h"
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

    // ---- PDF export test ----
    runner.registerExperiment(std::make_unique<LensingTimeDelayExperiment>());
    QV_CHECK(runner.count() == 2);

    QVariantMap lensParams;
    lensParams[QStringLiteral("lens_mass_msun")] = 1.0e11;
    lensParams[QStringLiteral("lens_redshift")] = 0.3;
    lensParams[QStringLiteral("D_L_Gpc")] = 1.0;
    lensParams[QStringLiteral("D_S_Gpc")] = 2.0;
    lensParams[QStringLiteral("D_LS_Gpc")] = 1.5;
    lensParams[QStringLiteral("source_positions")] = 10;
    lensParams[QStringLiteral("cosmology_source")] = QStringLiteral("fixed");

    QSignalSpy pdfSpy(&runner, &ExperimentRunner::pdfFinished);
    runner.start(QStringLiteral("lensing_time_delay"), lensParams);

    bool lensFinished = false;
    for (int i = 0; i < 200 && !lensFinished; ++i) {
        QTest::qWait(25);
        if (finishedSpy.count() >= 1) lensFinished = true;
        QCoreApplication::processEvents();
    }
    QV_CHECK(lensFinished);
    QV_CHECK(!runner.running());

    // Export PDF for the lensing experiment (first export)
    QSignalSpy pdfDoneSpy(&runner, &ExperimentRunner::pdfFinished);
    QString pdfPath1 = runner.exportPDF(QStringLiteral("lensing_time_delay"), QString());

    bool pdfDone1 = false;
    QString actualPdfPath1;
    for (int i = 0; i < 400 && !pdfDone1; ++i) { // pdflatex can be slow
        QTest::qWait(25);
        if (pdfDoneSpy.count() >= 1) {
            pdfDone1 = true;
            actualPdfPath1 = pdfDoneSpy.takeFirst().at(0).toString();
        }
        QCoreApplication::processEvents();
    }
    QV_CHECK(pdfDone1);
    QV_CHECK(QFileInfo::exists(actualPdfPath1));
    QV_CHECK(actualPdfPath1.size() > 0);
    QV_CHECK(actualPdfPath1.contains(QStringLiteral("lensing_time_delay_")));
    QV_CHECK(actualPdfPath1.endsWith(QStringLiteral(".pdf")));

    std::cout << "PDF #1 exported to: " << actualPdfPath1.toStdString() << std::endl;

    // ---- Verify PDF content via pdftotext ----
    QString pdfText;
    {
        QProcess pdftotext;
        pdftotext.setProcessChannelMode(QProcess::MergedChannels);
        pdftotext.start("pdftotext", QStringList() << actualPdfPath1 << "-");
        pdftotext.waitForFinished(30000);
        pdfText = QString::fromLocal8Bit(pdftotext.readAllStandardOutput());
    }
    QV_CHECK(!pdfText.isEmpty());
    QV_CHECK(pdfText.contains(QStringLiteral("lensing")));
    QV_CHECK(pdfText.contains(QStringLiteral("29.96")) || pdfText.contains(QStringLiteral("29.9")));
    QV_CHECK(pdfText.contains(QStringLiteral("0.5")));

    // ---- Second export must produce a DIFFERENT (unique) file ----
    QSignalSpy pdfDoneSpy2(&runner, &ExperimentRunner::pdfFinished);
    QString pdfPath2 = runner.exportPDF(QStringLiteral("lensing_time_delay"), QString());

    bool pdfDone2 = false;
    QString actualPdfPath2;
    for (int i = 0; i < 400 && !pdfDone2; ++i) {
        QTest::qWait(25);
        if (pdfDoneSpy2.count() >= 1) {
            pdfDone2 = true;
            actualPdfPath2 = pdfDoneSpy2.takeFirst().at(0).toString();
        }
        QCoreApplication::processEvents();
    }
    QV_CHECK(pdfDone2);
    QV_CHECK(QFileInfo::exists(actualPdfPath2));
    QV_CHECK(actualPdfPath2 != actualPdfPath1);

    std::cout << "PDF #2 exported to: " << actualPdfPath2.toStdString() << std::endl;
    std::cout << "=== ExperimentRunner Test: ALL PASS ===" << std::endl;
    return 0;
}
