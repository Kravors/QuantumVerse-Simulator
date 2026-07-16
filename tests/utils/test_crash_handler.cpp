// Phase 5.6: Crash Handler Verification Test (Windows-only)
// Launches crash_helper.exe and asserts a .dmp file is created.

#include <QTest>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QString>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QTimer>

class CrashHandlerTest : public QObject
{
    Q_OBJECT

private slots:
    void test_crash_dump_generated()
    {
        QString helperPath = QCoreApplication::applicationDirPath() + QLatin1String("/crash_helper.exe");
        QVERIFY2(QFile::exists(helperPath), qPrintable("crash_helper.exe not found at " + helperPath));

        // Dumps are written next to the helper executable.
        QString expectedDumpDir = QCoreApplication::applicationDirPath();

        // Remove any stale dump from a previous run.
        QStringList oldDumps = QDir(expectedDumpDir).entryList(QStringList() << "quantumverse_crash_*.dmp", QDir::Files);
        for (const QString& f : oldDumps) {
            QFile::remove(expectedDumpDir + QLatin1String("/") + f);
        }

        QProcess proc;
        proc.start(helperPath, {});
        bool finished = proc.waitForFinished(5000);
        QVERIFY2(finished, "crash_helper.exe did not terminate within timeout");
        QVERIFY2(proc.exitStatus() == QProcess::CrashExit || proc.exitCode() != 0,
                  "crash_helper.exe should have crashed");

        // Look for a newly created dump file.
        QStringList dumps = QDir(expectedDumpDir).entryList(QStringList() << "quantumverse_crash_*.dmp", QDir::Files);
        QVERIFY2(!dumps.isEmpty(), qPrintable("No crash dump found in " + expectedDumpDir));

        // Validate minidump header (MDMP signature = 0x4D444D50, little-endian bytes "PMDM")
        QString dumpPath = expectedDumpDir + QLatin1String("/") + dumps.first();
        QFile dumpFile(dumpPath);
        QVERIFY2(dumpFile.open(QIODevice::ReadOnly), qPrintable("Cannot open dump file: " + dumpPath));
        QByteArray header = dumpFile.read(4);
        dumpFile.close();
        QVERIFY2(header.size() == 4, "Dump file too small to contain minidump header");
        QVERIFY2(header[0] == char(0x4D) && header[1] == char(0x44) &&
                  header[2] == char(0x4D) && header[3] == char(0x50),
                  qPrintable("Invalid minidump header in " + dumpPath + ": expected MDMP"));

        // Cleanup
        for (const QString& f : dumps) {
            QFile::remove(expectedDumpDir + QLatin1String("/") + f);
        }
    }
};

QTEST_MAIN(CrashHandlerTest)
#include "test_crash_handler.moc"
