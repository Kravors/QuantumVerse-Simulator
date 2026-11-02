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
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QElapsedTimer>
#include <QEventLoop>
#include <iostream>

#include "experiment/ExperimentRunner.h"
#include "experiment/experiments/MassSweepExperiment.h"
#include "experiment/experiments/LensingTimeDelayExperiment.h"
#include "experiment/experiments/PhotonSphereExperiment.h"
#include "experiment/experiments/RedshiftProfileExperiment.h"
#include "experiment/experiments/FrameDraggingExperiment.h"
#include "experiment/experiments/GWRingdownExperiment.h"
#include "experiment/experiments/CosmologyValidationSuite.h"
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
    for (int i = 0; i < 4800 && !pdfDone1; ++i) { // up to 120s: first MiKTeX run can exceed 10s
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
        QString pdftotextPath = QStringLiteral("pdftotext");
#ifdef Q_OS_WIN
        const QString miktexPdftotext = QStringLiteral("C:/Program Files/MiKTeX/miktex/bin/x64/pdftotext.exe");
        if (QFileInfo::exists(miktexPdftotext)) {
            pdftotextPath = miktexPdftotext;
        }
#endif
        QProcess pdftotext;
        pdftotext.setProcessChannelMode(QProcess::MergedChannels);
        pdftotext.start(pdftotextPath, QStringList() << actualPdfPath1 << "-");
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

    // ---- Discovery paper test: all supported experiments ----
    runner.registerExperiment(std::make_unique<PhotonSphereExperiment>());
    runner.registerExperiment(std::make_unique<RedshiftProfileExperiment>());
    runner.registerExperiment(std::make_unique<FrameDraggingExperiment>());
    runner.registerExperiment(std::make_unique<GWRingdownExperiment>());
    QV_CHECK(runner.count() == 6);

    struct DiscoveryCase {
        QString id;
        QVariantMap params;
        QStringList contentMustContain;
        QString contentMustNotContain;
        QString bhTypeExpected;
    };

    QVector<DiscoveryCase> cases = {
        { QStringLiteral("photon_sphere"),
          {{QStringLiteral("mass_Msun"), 10.0},
           {QStringLiteral("spin_a_max"), 0.99},
           {QStringLiteral("n_steps"), 50}},
          {QStringLiteral("Kerr"), QStringLiteral("10"), QStringLiteral("0.99"), QStringLiteral("e-")},
          QStringLiteral("NaN"), QStringLiteral("Kerr") },
        { QStringLiteral("redshift_profile"),
          {{QStringLiteral("mass"), 42.0},
           {QStringLiteral("r_min"), 3000.0},
           {QStringLiteral("r_max"), 1.0e6},
           {QStringLiteral("steps"), 50}},
          {QStringLiteral("Schwarzschild"), QStringLiteral("42"), QStringLiteral("e-")},
          QStringLiteral("NaN"), QStringLiteral("Schwarzschild") },
         { QStringLiteral("frame_dragging"),
           {{QStringLiteral("mass_Msun"), 42.0},
            {QStringLiteral("spin_a_star"), 0.5},
            {QStringLiteral("orbit_radius_m"), 7.0e6},
            {QStringLiteral("steps"), 50}},
           {QStringLiteral("Kerr"), QStringLiteral("42"), QStringLiteral("0.5"), QStringLiteral("e-")},
           QStringLiteral("NaN"), QStringLiteral("Kerr") },
        { QStringLiteral("gw_ringdown"),
          {{QStringLiteral("mass_Msun"), 42.0},
           {QStringLiteral("spin_a"), 0.5},
           {QStringLiteral("amplitude"), 1.0},
           {QStringLiteral("duration_ms"), 5.0},
           {QStringLiteral("n_steps"), 50}},
          {QStringLiteral("Kerr"), QStringLiteral("42"), QStringLiteral("0.5"), QStringLiteral("e-"),
           QStringLiteral("f R"), QStringLiteral("quasinormal")},
          QStringLiteral("NaN"), QStringLiteral("Kerr") },
    };

    const QString outputDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                            + "/QuantumVerse/output";

    for (const auto& c : cases) {
        // Clean up old discovery PDFs for this experiment id.
        {
            QDir cleanupDir(outputDir);
            const QStringList pattern = { QStringLiteral("%1_discovery_*.pdf").arg(c.id) };
            const auto oldEntries = cleanupDir.entryList(pattern, QDir::Files);
            for (const auto& e : oldEntries) {
                QFile::remove(cleanupDir.absoluteFilePath(e));
            }
        }

        QSignalSpy finishedSpy(&runner, &ExperimentRunner::experimentFinished);
        runner.start(c.id, c.params);

        bool finished = false;
        for (int i = 0; i < 200 && !finished; ++i) {
            QTest::qWait(25);
            if (finishedSpy.count() >= 1) finished = true;
            QCoreApplication::processEvents();
        }
        QV_CHECK(finished);
        QV_CHECK(!runner.running());

        QString discoveryPath = runner.generateDiscoveryPaper(c.id);
        QV_CHECK(!discoveryPath.isEmpty());

        // Poll for the compiled PDF.
        QString resolvedPath;
        QElapsedTimer timer; timer.start();
        while (timer.elapsed() < 120000) {
            QDir dir(outputDir);
            const QStringList pattern = { QStringLiteral("%1_discovery_*.pdf").arg(c.id) };
            const auto entries = dir.entryList(pattern, QDir::Files, QDir::Time);
            if (!entries.isEmpty()) {
                const QString candidate = dir.absoluteFilePath(entries.first());
                if (QFileInfo(candidate).size() > 1000) {
                    resolvedPath = candidate;
                    break;
                }
            }
            QCoreApplication::processEvents(QEventLoop::AllEvents, 200);
        }
        QV_CHECK(!resolvedPath.isEmpty());
        QV_CHECK(resolvedPath.endsWith(QStringLiteral(".pdf")));

        std::cout << "Discovery paper (" << c.id.toStdString()
                  << ") exported to: " << resolvedPath.toStdString() << std::endl;

        // Content verification via pdftotext.
        {
            QString pdftotextPath = QStringLiteral("pdftotext");
#ifdef Q_OS_WIN
            const QString miktexPdftotext = QStringLiteral("C:/Program Files/MiKTeX/miktex/bin/x64/pdftotext.exe");
            if (QFileInfo::exists(miktexPdftotext)) {
                pdftotextPath = miktexPdftotext;
            }
#endif
            QProcess pdftotext;
            pdftotext.setProcessChannelMode(QProcess::MergedChannels);
            pdftotext.start(pdftotextPath, QStringList() << resolvedPath << "-");
            pdftotext.waitForFinished(30000);
            const QString pdfText = QString::fromLocal8Bit(pdftotext.readAllStandardOutput());
            QV_CHECK(!pdfText.isEmpty());
            for (const auto& token : c.contentMustContain) {
                QV_CHECK(pdfText.contains(token));
            }
            QV_CHECK(!pdfText.contains(c.contentMustNotContain));
            QV_CHECK(!pdfText.contains(QStringLiteral("{")));
        }
    }

    std::cout << "=== ExperimentRunner Test: ALL PASS ===" << std::endl;

    // ---- CosmologyValidationSuite with f(R) theory (b=0.3) ----
    // Verifies the first non-ΛCDM theory in the validation harness:
    // captures σ-tensions between f(R) gravity and ΛCDM across all 6
    // cosmology experiments.
    runner.registerExperiment(std::make_unique<CosmologyValidationSuite>());
    QV_CHECK(runner.count() == 7);

    QVariantMap cvsParams;
    cvsParams[QStringLiteral("theory_choice")] = QStringLiteral("fR");
    cvsParams[QStringLiteral("passSigma")] = 3.0;

    QSignalSpy cvsFinishedSpy(&runner, &ExperimentRunner::experimentFinished);
    runner.start(QStringLiteral("cosmology_validation_suite"), cvsParams);

    bool cvsFinished = false;
    for (int i = 0; i < 800 && !cvsFinished; ++i) {
        QTest::qWait(25);
        if (cvsFinishedSpy.count() >= 1) cvsFinished = true;
        QCoreApplication::processEvents();
    }
    QV_CHECK(cvsFinished);
    QV_CHECK(!runner.running());

    // Print the validation suite result scalars (σ-tensions) for inspection.
    runner.showHistoryEntry(QStringLiteral("cosmology_validation_suite"));
    const QVariantMap cvsScalars = runner.currentScalars();
    std::cout << "  suite_theory=" << cvsScalars.value(QStringLiteral("suite_theory")).toString().toStdString()
              << " suite_fR_b=" << cvsScalars.value(QStringLiteral("suite_fR_b")).toDouble()
              << std::endl;
    for (auto it = cvsScalars.cbegin(); it != cvsScalars.cend(); ++it) {
        const QString key = it.key();
        if (key.contains(QStringLiteral("tension")) || key.contains(QStringLiteral("pass"))) {
            std::cout << "    " << key.toStdString() << " = " << it.value().toDouble() << std::endl;
        }
    }
    std::cout << "CosmologyValidationSuite with f(R) b=0.3 completed successfully." << std::endl;

    return 0;
}
