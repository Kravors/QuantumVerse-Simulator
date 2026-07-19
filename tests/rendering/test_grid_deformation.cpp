#include <QTest>
#include <QProcess>
#include <QImage>
#include <QFile>
#include <QDir>
#include <QCoreApplication>

class GridDeformationTest : public QObject
{
    Q_OBJECT

private slots:
    void test_headless_screenshot_produces_valid_image()
    {
        QString exePath = QCoreApplication::applicationDirPath() + "/quantumverse_qml";
#ifdef Q_OS_WIN
        exePath += ".exe";
#endif

        QString tempOutput = QDir::tempPath() + "/qv_grid_deform_test.png";
        QFile::remove(tempOutput);

        QProcess process;
        QStringList args;
        args << "--frames" << "1" << "--screenshot" << tempOutput;

        process.start(exePath, args);
        bool finished = process.waitForFinished(30000);
        QVERIFY2(finished, qPrintable("Process timed out or failed to start: " + process.errorString()));
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), 0);

        QVERIFY(QFile::exists(tempOutput));

        QImage img(tempOutput);
        QVERIFY(!img.isNull());
        QVERIFY(img.width() > 0);
        QVERIFY(img.height() > 0);

        QFile::remove(tempOutput);
    }

    void test_geodesic_mode_produces_valid_image()
    {
        QString exePath = QCoreApplication::applicationDirPath() + "/quantumverse_qml";
#ifdef Q_OS_WIN
        exePath += ".exe";
#endif

        QString tempOutput = QDir::tempPath() + "/qv_grid_geodesic_test.png";
        QFile::remove(tempOutput);

        QProcess process;
        QStringList args;
        args << "--frames" << "1" << "--enable-geodesics" << "--screenshot" << tempOutput;

        process.start(exePath, args);
        bool finished = process.waitForFinished(30000);
        QVERIFY2(finished, qPrintable("Process timed out or failed to start: " + process.errorString()));
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), 0);

        QVERIFY(QFile::exists(tempOutput));

        QImage img(tempOutput);
        QVERIFY(!img.isNull());
        QVERIFY(img.width() > 0);
        QVERIFY(img.height() > 0);

        QFile::remove(tempOutput);
    }

    void test_screenshot_dimensions_are_consistent()
    {
        QString exePath = QCoreApplication::applicationDirPath() + "/quantumverse_qml";
#ifdef Q_OS_WIN
        exePath += ".exe";
#endif

        QString tempOutput1 = QDir::tempPath() + "/qv_grid_dims_1.png";
        QString tempOutput2 = QDir::tempPath() + "/qv_grid_dims_2.png";
        QFile::remove(tempOutput1);
        QFile::remove(tempOutput2);

        QProcess process;
        QStringList args;
        args << "--frames" << "1" << "--screenshot" << tempOutput1;

        process.start(exePath, args);
        QVERIFY(process.waitForFinished(30000));
        QCOMPARE(process.exitCode(), 0);

        args[3] = tempOutput2;
        process.start(exePath, args);
        QVERIFY(process.waitForFinished(30000));
        QCOMPARE(process.exitCode(), 0);

        QImage img1(tempOutput1);
        QImage img2(tempOutput2);
        QVERIFY(!img1.isNull());
        QVERIFY(!img2.isNull());
        QCOMPARE(img1.size(), img2.size());

        QFile::remove(tempOutput1);
        QFile::remove(tempOutput2);
    }
};

QTEST_MAIN(GridDeformationTest)
#include "test_grid_deformation.moc"
